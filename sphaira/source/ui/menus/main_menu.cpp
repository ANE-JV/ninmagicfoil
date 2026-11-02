#include "ui/menus/main_menu.hpp"

#include "ui/sidebar.hpp"
#include "ui/popup_list.hpp"
#include "ui/option_box.hpp"
#include "ui/progress_box.hpp"
#include "ui/install_queue.hpp"
#include "ui/error_box.hpp"
#include "ui/scrollable_text.hpp"
#include "ui/nvg_util.hpp"

#include "ui/menus/album_menu.hpp"
#include "ui/menus/homebrew.hpp"
#include "ui/menus/filebrowser.hpp"
#include "ui/menus/irs_menu.hpp"
#include "ui/menus/themezer.hpp"
#include "ui/menus/ghdl.hpp"
#include "ui/menus/usb_menu.hpp"
#include "ui/menus/ftp_menu.hpp"
#include "ui/menus/mtp_menu.hpp"
#include "ui/menus/gc_menu.hpp"
#include "ui/menus/game_menu.hpp"
#include "ui/menus/save_menu.hpp"
#include "ui/menus/storage_menu.hpp"
#include "ui/menus/appstore.hpp"
#include "ui/menus/ownfoil.hpp"

#include "app.hpp"
#include "log.hpp"
#include "download.hpp"
#include "defines.hpp"
#include "i18n.hpp"
#include "threaded_file_transfer.hpp"
#include "nacp_compat.hpp"
#include "ninmagic_updater.hpp"
#include "evman.hpp"

#include <cstring>
#include <yyjson.h>
#include <iomanip>
#include <algorithm>
#include <array>
#include <cctype>
#include <optional>
#include <string_view>

namespace sphaira::ui::menu::main {
namespace {

auto Trim(std::string_view value) -> std::string_view {
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front()))) {
        value.remove_prefix(1);
    }
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back()))) {
        value.remove_suffix(1);
    }
    return value;
}

auto Lower(std::string_view value) -> std::string {
    std::string result{value};
    std::ranges::transform(result, result.begin(), [](char c){
        return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    });
    return result;
}

auto ReadNotes(yyjson_val* value) -> std::string {
    if (!value) {
        return {};
    }
    if (yyjson_is_str(value)) {
        const auto text = yyjson_get_str(value);
        return text ? text : "";
    }
    if (!yyjson_is_arr(value)) {
        return {};
    }

    std::string notes;
    size_t index{}, count{};
    yyjson_val* item{};
    yyjson_arr_foreach(value, index, count, item) {
        if (const auto text = yyjson_get_str(item); text && *text) {
            if (!notes.empty()) {
                notes += '\n';
            }
            notes += "- ";
            notes += text;
        }
    }
    return notes;
}

auto ReadLocalizedNotes(yyjson_val* object, std::string_view language) -> std::string {
    if (!object || !yyjson_is_obj(object)) {
        return {};
    }

    auto value = yyjson_obj_getn(object, language.data(), language.size());
    if (!value && language.find('-') != std::string_view::npos) {
        const auto base = language.substr(0, language.find('-'));
        value = yyjson_obj_getn(object, base.data(), base.size());
    }
    if (!value) {
        value = yyjson_obj_get(object, "en");
    }
    return ReadNotes(value);
}

// Supports the language-keyed changelog format used by the supplied NRO. It
// also accepts an object containing language keys directly, which makes the
// same format convenient for a GitHub release body.
auto ParseLocalizedNotesJson(std::string_view input, std::string_view version, std::string_view language) -> std::string {
    const auto document = yyjson_read(input.data(), input.size(), YYJSON_READ_NOFLAG);
    if (!document) {
        return {};
    }
    ON_SCOPE_EXIT(yyjson_doc_free(document));

    const auto root = yyjson_doc_get_root(document);
    if (!root || !yyjson_is_obj(root)) {
        return {};
    }

    const auto versions = yyjson_obj_get(root, "versions");
    if (versions && yyjson_is_arr(versions)) {
        const auto wanted = App::GetVersionFromString(std::string{version}.c_str());
        size_t index{}, count{};
        yyjson_val* entry{};
        yyjson_arr_foreach(versions, index, count, entry) {
            const auto version_value = yyjson_obj_get(entry, "version");
            const auto entry_version = version_value ? yyjson_get_str(version_value) : nullptr;
            if (!entry_version) {
                continue;
            }
            const auto parsed = App::GetVersionFromString(entry_version);
            if ((wanted && parsed == wanted) || (!wanted && version == entry_version)) {
                return ReadLocalizedNotes(entry, language);
            }
        }
        return {};
    }

    return ReadLocalizedNotes(root, language);
}

auto ParseLanguageHeading(std::string_view line) -> std::optional<std::string> {
    line = Trim(line);
    if (line.starts_with("<!--") && line.ends_with("-->")) {
        line.remove_prefix(4);
        line.remove_suffix(3);
        line = Trim(line);
        if (const auto colon = line.find(':'); colon != std::string_view::npos
            && Lower(Trim(line.substr(0, colon))) == "lang") {
            line = Trim(line.substr(colon + 1));
        } else {
            return {};
        }
    } else {
        while (!line.empty() && line.front() == '#') {
            line.remove_prefix(1);
        }
        line = Trim(line);
        if (line.size() >= 2 && line.front() == '[' && line.back() == ']') {
            line.remove_prefix(1);
            line.remove_suffix(1);
        }
    }

    auto code = Lower(Trim(line));
    static constexpr std::array<std::string_view, 16> languages{
        "en", "ja", "fr", "de", "it", "es", "zh-cn", "ko", "nl",
        "pt", "ru", "zh-tw", "se", "vi", "uk", "tr",
    };
    if (std::ranges::find(languages, code) == languages.end()) {
        return {};
    }
    if (code == "zh-cn") return "zh-CN";
    if (code == "zh-tw") return "zh-TW";
    return code;
}

auto LocalizeReleaseNotes(std::string_view notes, std::string_view version) -> std::string {
    if (notes.empty()) {
        return "No changelog available."_i18n;
    }

    const auto language = i18n::GetLanguageCode();
    if (const auto json_notes = ParseLocalizedNotesJson(notes, version, language); !json_notes.empty()) {
        return json_notes;
    }

    // Optional Markdown format:
    //   ## [en]
    //   English notes
    //   ## [fr]
    //   French notes
    // Automatic language uses the resolved console language here.
    std::string selected;
    std::string english;
    std::string active_language;
    bool found_language_section{};
    for (std::size_t offset{}; offset <= notes.size();) {
        const auto end = notes.find('\n', offset);
        const auto line = notes.substr(offset, end == std::string_view::npos ? notes.size() - offset : end - offset);
        if (const auto heading = ParseLanguageHeading(line)) {
            active_language = *heading;
            found_language_section = true;
        } else if (!active_language.empty()) {
            auto& output = active_language == language ? selected : (active_language == "en" ? english : selected);
            if (active_language == language || active_language == "en") {
                output.append(line);
                output.push_back('\n');
            }
        }
        if (end == std::string_view::npos) {
            break;
        }
        offset = end + 1;
    }

    if (found_language_section) {
        auto result = !selected.empty() ? std::move(selected) : std::move(english);
        while (!result.empty() && std::isspace(static_cast<unsigned char>(result.back()))) {
            result.pop_back();
        }
        if (!result.empty()) {
            return result;
        }
    }
    return std::string{notes};
}

class UpdateInfoMenu final : public MenuBase {
public:
    UpdateInfoMenu(std::string version, std::string notes, bool update_available, std::function<void()> on_update)
    : MenuBase{"Update Checker"_i18n, MenuFlag_None}
    , m_version{std::move(version)}
    , m_update_available{update_available}
    , m_on_update{std::move(on_update)} {
        SetAction(Button::B, Action{"Back"_i18n, [this](){ SetPop(); }});
        if (m_update_available) {
            SetAction(Button::A, Action{"Update Now"_i18n, [this](){
                if (m_on_update) {
                    m_on_update();
                }
            }});
        }
        m_notes = std::make_unique<ScrollableText>(notes, 0.f, 205.f, 385.f, 1000.f, 20.f);
        SetTitleSubHeading(m_version.empty() ? "v" APP_DISPLAY_VERSION : m_version);
    }

    auto GetShortTitle() const -> const char* override { return "Update"; }

    void Update(Controller* controller, TouchInfo* touch) override {
        MenuBase::Update(controller, touch);
        m_notes->Update(controller, touch);
    }

    void Draw(NVGcontext* vg, Theme* theme) override {
        MenuBase::Draw(vg, theme);
        const auto status = m_update_available ? "New version available"_i18n : "You are already on the latest version."_i18n;
        gfx::drawText(vg, 110.f, 125.f, 25.f, theme->GetColour(ThemeEntryID_TEXT), status.c_str(), NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        gfx::drawText(vg, 110.f, 166.f, 20.f, theme->GetColour(ThemeEntryID_TEXT_INFO), "Changelog"_i18n.c_str(), NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        gfx::drawRect(vg, 110.f, 187.f, 1040.f, 1.f, theme->GetColour(ThemeEntryID_LINE_SEPARATOR));
        m_notes->Draw(vg, theme);
    }

private:
    std::string m_version;
    bool m_update_available{};
    std::function<void()> m_on_update;
    std::unique_ptr<ScrollableText> m_notes;
};

template<typename T>
auto MiscMenuFuncGenerator(u32 flags) {
    return std::make_unique<T>(flags);
}

const MiscMenuEntry MISC_MENU_ENTRIES[] = {
    { .name = "Homebrew", .title = "Homebrew", .func = MiscMenuFuncGenerator<ui::menu::homebrew::Menu>, .flag = MiscMenuFlag_Shortcut, .info =
        "The homebrew menu.\n\n"
        "Allows you to launch, delete and mount homebrew!"},

    { .name = "Appstore", .title = "Appstore", .func = MiscMenuFuncGenerator<ui::menu::appstore::Menu>, .flag = MiscMenuFlag_Shortcut, .info =
        "Download and update apps.\n\n"
        "Internet connection required." },

    { .name = "Games", .title = "Games", .func = MiscMenuFuncGenerator<ui::menu::game::Menu>, .flag = MiscMenuFlag_Shortcut, .info =
        "View all installed games. "
        "In this menu you can launch, backup, create savedata and much more." },

    { .name = "Storage", .title = "Data Management", .func = MiscMenuFuncGenerator<ui::menu::storage::Menu>, .flag = MiscMenuFlag_Shortcut, .info =
        "See how much space each game uses on the microSD card and in system memory. "
        "Games are listed largest first, so it's easy to find what to remove." },

    { .name = "FileBrowser", .title = "FileBrowser", .func = MiscMenuFuncGenerator<ui::menu::filebrowser::Menu>, .flag = MiscMenuFlag_Shortcut, .info =
        "Browse files on you SD Card. "
        "You can move, copy, delete, extract zip, create zip, upload and much more.\n\n"
        "A connected USB/HDD can be opened by mounting it in the advanced options." },

    { .name = "Saves", .title = "Saves", .func = MiscMenuFuncGenerator<ui::menu::save::Menu>, .flag = MiscMenuFlag_Shortcut, .info =
        "View save data for each user. "
        "You can backup and restore saves.\n\n"
        "Experimental support for backing up system saves is possible." },

    { .name = "Album", .title = "Album", .func = MiscMenuFuncGenerator<ui::menu::album::Menu>, .flag = MiscMenuFlag_Shortcut, .info =
        "Browse the screenshots and clips saved on the console. "
        "Captures can be viewed and deleted from here." },

    { .name = "Themezer", .title = "Themezer", .func = MiscMenuFuncGenerator<ui::menu::themezer::Menu>, .flag = MiscMenuFlag_Shortcut, .info =
        "Download themes from themezer.net. "
        "Themes are downloaded to /themes/sphaira\n"
        "To install the themes, NXThemesInstaller needs to be installed (can be downloaded via the AppStore)." },

    { .name = "GitHub", .title = "GitHub", .func = MiscMenuFuncGenerator<ui::menu::gh::Menu>, .flag = MiscMenuFlag_Shortcut, .info =
        "Download releases directly from GitHub. "
        "Custom entries can be added to /config/NinMagicFoil/github" },

    { .name = "Ownfoil", .title = "Ownfoil", .func = MiscMenuFuncGenerator<ui::menu::ownfoil::Menu>, .flag = MiscMenuFlag_Shortcut, .info =
        "Browse and install titles from a self-hosted Ownfoil server.\n\n"
        "Internet connection required." },

#ifdef ENABLE_FTPSRV
    { .name = "FTP", .title = "FTP Install", .func = MiscMenuFuncGenerator<ui::menu::ftp::Menu>, .flag = MiscMenuFlag_Install, .info =
        "Install apps via FTP." },
#endif // ENABLE_FTPSRV

#ifdef ENABLE_LIBHAZE
    { .name = "MTP", .title = "MTP Install", .func = MiscMenuFuncGenerator<ui::menu::mtp::Menu>, .flag = MiscMenuFlag_Install, .info =
        "Install apps via MTP." },
#endif // ENABLE_LIBHAZE

    { .name = "USB", .title = "USB Install", .func = MiscMenuFuncGenerator<ui::menu::usb::Menu>, .flag = MiscMenuFlag_Install, .info =
        "Install apps via USB.\n\n"
        "A USB client is required on PC." },

    { .name = "GameCard", .title = "GameCard", .func = MiscMenuFuncGenerator<ui::menu::gc::Menu>, .flag = MiscMenuFlag_Shortcut, .info =
        "View info on the inserted Game Card (GC). "
        "You can backup and install the inserted GC. "
        "To swap GC's, simply remove the old GC and insert the new one. "
        "You do not need to exit the menu." },

    { .name = "IRS", .title = "IRS (Infrared Joycon Camera)", .func = MiscMenuFuncGenerator<ui::menu::irs::Menu>, .flag = MiscMenuFlag_Shortcut, .info =
        "InfraRed Sensor (IRS) is the small camera found on right JoyCon." },
};

auto CreateCenterMenu(std::string& name_out) -> std::unique_ptr<MenuBase> {
    const auto name = App::GetApp()->m_center_menu.Get();

    for (auto& e : GetMenuMenuEntries()) {
        if (e.name == name) {
            name_out = name;
            return e.func(MenuFlag_Tab);
        }
    }

    name_out = "Homebrew";
    return std::make_unique<ui::menu::homebrew::Menu>(MenuFlag_Tab);
}

auto CreateLeftSideMenu(std::string_view center_name, std::string& name_out) -> std::unique_ptr<MenuBase> {
    const auto name = App::GetApp()->m_left_menu.Get();

    // handle if the user tries to mount the same menu twice.
    if (name == center_name) {
        // check if we can mount the default.
        if (center_name != "FileBrowser") {
            return std::make_unique<ui::menu::filebrowser::Menu>(MenuFlag_Tab);
        } else {
            // otherwise, fallback to center default.
            return std::make_unique<ui::menu::homebrew::Menu>(MenuFlag_Tab);
        }
    }

    for (auto& e : GetMenuMenuEntries()) {
        if (e.name == name) {
            name_out = name;
            return e.func(MenuFlag_Tab);
        }
    }

    name_out = "FileBrowser";
    return std::make_unique<ui::menu::filebrowser::Menu>(MenuFlag_Tab);
}

// todo: handle center / left menu being the same.
auto CreateRightSideMenu(std::string_view left_name) -> std::unique_ptr<MenuBase> {
    const auto name = App::GetApp()->m_right_menu.Get();

    // handle if the user tries to mount the same menu twice.
    if (name == left_name) {
        // check if we can mount the default.
        if (left_name != "AppStore") {
            return std::make_unique<ui::menu::appstore::Menu>(MenuFlag_Tab);
        } else {
            // otherwise, fallback to left side default.
            return std::make_unique<ui::menu::filebrowser::Menu>(MenuFlag_Tab);
        }
    }

    for (auto& e : GetMenuMenuEntries()) {
        if (e.name == name) {
            return e.func(MenuFlag_Tab);
        }
    }

    return std::make_unique<ui::menu::appstore::Menu>(MenuFlag_Tab);
}

} // namespace

auto GetMenuMenuEntries() -> std::span<const MiscMenuEntry> {
    return MISC_MENU_ENTRIES;
}

MainMenu::MainMenu(bool updater_only) {
    CheckForUpdates(false);
    if (updater_only) return;

    this->SetActions(
        std::make_pair(Button::START, Action{[](){
            // leaving mid install can leave half installed content behind.
            if (!InstallQueue::Get().IsBusy()) {
                App::Exit();
                return;
            }

            App::Push<OptionBox>("Installs are still running. Exit anyway?"_i18n, "No"_i18n, "Yes"_i18n, 0, [](auto op_index){
                if (op_index && *op_index) {
                    App::Exit();
                }
            });
        }}),
        std::make_pair(Button::SELECT, Action{App::DisplayMenuOptions}),
        std::make_pair(Button::Y, Action{"Menu"_i18n, [this](){
            auto options = std::make_unique<Sidebar>("Menu Options"_i18n, "v" APP_DISPLAY_VERSION, Sidebar::Side::LEFT);
            ON_SCOPE_EXIT(App::Push(std::move(options)));

            SidebarEntryArray::Items language_items;
            language_items.push_back("Auto"_i18n);
            language_items.push_back("English");
            language_items.push_back("日本語"); // Japanese
            language_items.push_back("Français"); // French
            language_items.push_back("Deutsch"); // German
            language_items.push_back("Italiano"); // Italian
            language_items.push_back("Español"); // Spanish
            language_items.push_back("简体中文"); // Chinese (Simplified)
            language_items.push_back("한국어"); // Korean
            language_items.push_back("Nederlands"); // Dutch
            language_items.push_back("Português"); // Portuguese
            language_items.push_back("Русский"); // Russian
            language_items.push_back("繁體中文"); // Chinese (Traditional)
            language_items.push_back("Svenska"); // Swedish
            language_items.push_back("Tiếng Việt"); // Vietnamese
            language_items.push_back("Українська"); // Ukrainian
            language_items.push_back("Türkçe"); // Turkish

            // build menus info.
            std::string menus_info = "Launch one of NinMagicFoil's menus:\n"_i18n;
            for (auto& e : GetMenuMenuEntries()) {
                if (e.name == App::GetApp()->m_left_menu.Get()) {
                    continue;
                } else if (e.name == App::GetApp()->m_right_menu.Get()) {
                    continue;
                }

                menus_info += "- " + i18n::get(e.title) + "\n";
            }
            menus_info += "\nYou can change the left/right menu in the Advanced Options."_i18n;

            options->Add<SidebarEntryCallback>("Menus"_i18n, [](){
                App::DisplayMenuOptions();
            },  menus_info);

            options->Add<SidebarEntryCallback>("Network"_i18n, [this](){
                auto options = std::make_unique<Sidebar>("Network Options"_i18n, Sidebar::Side::LEFT);
                ON_SCOPE_EXIT(App::Push(std::move(options)));

                options->Add<SidebarEntryCallback>("FTP"_i18n, [](){ App::DisplayFtpOptions(); },
                    i18n::get("ftp_settings_info",
                        "Enable / modify the FTP server settings such as port, user/pass and the folders that are shown.\n\n"
                        "NOTE: Changing any of the options will automatically restart the FTP server when exiting the options menu.")
                );

                options->Add<SidebarEntryCallback>("MTP"_i18n, [](){ App::DisplayMtpOptions(); },
                    i18n::get("mtp_settings_info",
                        "Enable / modify the MTP responder settings such as the folders that are shown.\n\n"
                        "NOTE: Changing any of the options will automatically restart the MTP server when exiting the options menu.")
                );

                options->Add<SidebarEntryCallback>("HDD"_i18n, [](){
                    App::DisplayHddOptions();
                },  "Enable / modify the HDD mount options."_i18n);

                options->Add<SidebarEntryBool>("NXlink"_i18n, App::GetNxlinkEnable(), [](bool& enable){
                    App::SetNxlinkEnable(enable);
                },  i18n::get("nxlink_enable_info",
                        "Enable NXlink server to run in the background. "
                        "NXlink is used to send .nro's from PC to the switch\n\n"
                        "If you are not a developer, you can disable this option."));

            },  i18n::get("network_options_info",
                    "Toggle FTP, MTP, HDD and NXlink"));

            options->Add<SidebarEntryCallback>("Theme"_i18n, [](){
                App::DisplayThemeOptions();
            }, "Customise the look of NinMagicFoil by changing the theme"_i18n);

            options->Add<SidebarEntryArray>("Language"_i18n, language_items, [](s64& index_out){
                App::SetLanguage(index_out);
            }, (s64)App::GetLanguage(),
                i18n::get("translation_info",
                    "Change the language.\n\n"
                    "If your language isn't found, or translations are missing, please consider opening a PR at "
                    "github.com/ITotalJustice/sphaira"));

            options->Add<SidebarEntryCallback>("Advanced Options"_i18n, [](){
                App::DisplayAdvancedOptions();
            },  i18n::get("advanced_options_info",
                    "Change the advanced options. "
                    "Please view the info boxes to better understand each option."));

            // Keep update controls in the main options list, directly below
            // Advanced Options (not inside its submenu).
            if (m_update_state == UpdateState::Update) {
                options->Add<SidebarEntryCallback>("Update available: "_i18n + m_update_version, [this](){
                    ShowUpdateInfo();
                }, true, "View the changelog and install the available update."_i18n);
            }

            options->Add<SidebarEntryCallback>("Check for Updates"_i18n, [this](){
                CheckForUpdates(true);
            }, true, "Check for NinMagicFoil updates and view the changelog."_i18n);
        }}
    ));

    std::string center_name;
    m_centre_menu = CreateCenterMenu(center_name);
    m_current_menu = m_centre_menu.get();

    std::string left_side_name;
    m_left_menu = CreateLeftSideMenu(center_name, left_side_name);

    m_right_menu = CreateRightSideMenu(left_side_name);

    AddOnLRPress();

    for (auto [button, action] : m_actions) {
        m_current_menu->SetAction(button, action);
    }
}

MainMenu::~MainMenu() {
    m_stop_source.request_stop();
    m_update_worker.request_stop();
}

void MainMenu::CheckForUpdates(bool show_result) {
    const auto generation = ++m_update_generation;
    m_update_state = UpdateState::Pending;
    if (show_result) App::Notify("Checking for updates"_i18n);
    m_update_worker.request_stop();
    const auto owner_token = GetToken();
    m_update_worker = std::jthread([this, generation, show_result, owner_token](std::stop_token token) {
        std::string body;
        long status{};
        const bool success = ninmagic::update::FetchReleaseJson(token, body, status);
        if (token.stop_requested() || owner_token.stop_requested()) return;
        evman::push(evman::CallbackEventData{
            .callback = [this, generation, show_result, success, status, body = std::move(body)]() {
                if (generation != m_update_generation) return;
                if (!success || !ParseUpdateMetadata(body)) m_update_state = UpdateState::Error;
                log_write("[NinMagic updater] status=%u http=%ld\n", static_cast<u8>(m_update_state), status);
                if (show_result) ShowUpdateInfo();
                else if (m_update_state == UpdateState::Update) App::Notify("Update available: "_i18n + m_update_version);
            },
            .stoken = owner_token,
        }, false);
    });
}

auto MainMenu::ParseUpdateMetadata(std::string_view json) -> bool {
    using namespace ninmagic::update;
    m_update_release = {};
    const auto document = yyjson_read(json.data(), json.size(), YYJSON_READ_NOFLAG);
    if (!document) return false;
    ON_SCOPE_EXIT(yyjson_doc_free(document));
    const auto root = yyjson_doc_get_root(document);
    if (!root || !yyjson_is_obj(root)) return false;
    const auto draft = yyjson_obj_get(root, "draft");
    const auto prerelease = yyjson_obj_get(root, "prerelease");
    if (!yyjson_is_bool(draft) || yyjson_get_bool(draft) || !yyjson_is_bool(prerelease) || yyjson_get_bool(prerelease)) return false;
    const auto tag_value = yyjson_obj_get(root, "tag_name");
    if (!yyjson_is_str(tag_value)) return false;
    const std::string tag{yyjson_get_str(tag_value), yyjson_get_len(tag_value)};
    const auto version = ParseVersion(tag);
    const auto current = ParseVersion(APP_VERSION, false);
    if (!version || !current) return false;
    m_update_version = tag;
    std::string body;
    if (const auto value = yyjson_obj_get(root, "body"); yyjson_is_str(value)) body.assign(yyjson_get_str(value), yyjson_get_len(value));
    m_update_description = LocalizeReleaseNotes(body, tag);
    if (*version <= *current) {
        m_update_state = UpdateState::None;
        return true;
    }
    const auto assets = yyjson_obj_get(root, "assets");
    if (!yyjson_is_arr(assets)) return false;
    std::size_t matches{};
    std::size_t index{}, count{};
    yyjson_val* asset{};
    yyjson_arr_foreach(assets, index, count, asset) {
        const auto name = yyjson_obj_get(asset, "name");
        if (!yyjson_is_str(name) || std::string_view{yyjson_get_str(name), yyjson_get_len(name)} != ninmagic::ReleaseAsset) continue;
        if (++matches != 1) return false;
        const auto state = yyjson_obj_get(asset, "state");
        const auto url = yyjson_obj_get(asset, "browser_download_url");
        const auto size = yyjson_obj_get(asset, "size");
        const auto digest = yyjson_obj_get(asset, "digest");
        if (!yyjson_is_str(state) || std::string_view{yyjson_get_str(state), yyjson_get_len(state)} != "uploaded"
            || !yyjson_is_str(url) || !yyjson_is_uint(size) || !yyjson_is_str(digest)) return false;
        const auto sha = ParseDigest({yyjson_get_str(digest), yyjson_get_len(digest)});
        if (!sha) return false;
        m_update_release = Release{tag, {yyjson_get_str(url), yyjson_get_len(url)}, *sha, yyjson_get_uint(size)};
        if (!IsValidRelease(m_update_release)) return false;
    }
    if (matches != 1) return false;
    m_update_state = UpdateState::Update;
    return true;
}

void MainMenu::ShowUpdateInfo() {
    if (m_update_state == UpdateState::Pending) {
        App::Notify("Checking for updates"_i18n);
        return;
    }
    if (m_update_state == UpdateState::Error) {
        App::Push<OptionBox>("Failed to check for updates"_i18n, "OK"_i18n);
        return;
    }

    App::Push<UpdateInfoMenu>(
        m_update_version,
        m_update_description.empty() ? "No changelog available."_i18n : m_update_description,
        m_update_state == UpdateState::Update,
        [this, token = GetToken()](){ if (!token.stop_requested()) StartUpdateInstall(); }
    );
}

void MainMenu::StartUpdateInstall() {
    if (m_update_state != UpdateState::Update || !ninmagic::update::IsValidRelease(m_update_release)) return;
    if (InstallQueue::Get().IsBusy()) {
        App::Notify("Espera a que termine la cola de instalaciones antes de actualizar NinMagicFoil.");
        return;
    }
    if (!ninmagic::update::IsInstallPath(App::GetExePath().toString())) {
        App::Push<OptionBox>("Para actualizar, abre NinMagicFoil desde /switch/NinMagicFoil/NinMagicFoil.nro", "OK"_i18n);
        return;
    }
    const auto token = GetToken();
    App::Push<ProgressBox>(0, "Downloading "_i18n, std::string{ninmagic::AppName} + " " + m_update_version,
        [release = m_update_release](auto pbox) -> Result { return ninmagic::update::Install(pbox, release); },
        [this, token](Result rc) {
            if (token.stop_requested()) return;
            App::PushErrorBox(rc, "No se pudo actualizar NinMagicFoil. Si fallo la sustitucion, la copia anterior se conserva en /config/NinMagicFoil/update/NinMagicFoil.previous.nro");
            if (R_SUCCEEDED(rc)) {
                m_update_state = UpdateState::None;
                App::Push<OptionBox>("NinMagicFoil se ha actualizado. Pulsa OK para reiniciar.", "OK"_i18n, [](auto){ App::ExitRestart(); });
            }
        });
}

void MainMenu::Update(Controller* controller, TouchInfo* touch) {
    if (m_current_menu) m_current_menu->Update(controller, touch);
}

void MainMenu::Draw(NVGcontext* vg, Theme* theme) {
    if (m_current_menu) m_current_menu->Draw(vg, theme);
}

void MainMenu::OnFocusGained() {
    Widget::OnFocusGained();
    if (m_current_menu) m_current_menu->OnFocusGained();
}

void MainMenu::OnFocusLost() {
    Widget::OnFocusLost();
    if (m_current_menu) m_current_menu->OnFocusLost();
}

void MainMenu::OnLRPress(MenuBase* menu, Button b) {
    if (m_current_menu) m_current_menu->OnFocusLost();
    if (m_current_menu == m_centre_menu.get()) {
        m_current_menu = menu;
        RemoveAction(b);
    } else {
        m_current_menu = m_centre_menu.get();
    }

    AddOnLRPress();
    if (m_current_menu) m_current_menu->OnFocusGained();

    for (auto [button, action] : m_actions) {
        m_current_menu->SetAction(button, action);
    }
}

void MainMenu::AddOnLRPress() {
    if (m_current_menu != m_left_menu.get()) {
        const auto label = m_current_menu == m_centre_menu.get() ? m_left_menu->GetShortTitle() : m_centre_menu->GetShortTitle();
        SetAction(Button::L, Action{i18n::get(label), [this]{
            OnLRPress(m_left_menu.get(), Button::L);
        }});
    }

    if (m_current_menu != m_right_menu.get()) {
        const auto label = m_current_menu == m_centre_menu.get() ? m_right_menu->GetShortTitle() : m_centre_menu->GetShortTitle();
        SetAction(Button::R, Action{i18n::get(label), [this]{
            OnLRPress(m_right_menu.get(), Button::R);
        }});
    }
}

} // namespace sphaira::ui::menu::main

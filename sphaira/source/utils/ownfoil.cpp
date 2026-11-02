#include "utils/ownfoil.hpp"

#include "ui/sidebar.hpp"

#include "app.hpp"
#include "defines.hpp"
#include "fs.hpp"
#include "log.hpp"
#include "i18n.hpp"

#include <algorithm>
#include <cstring>
#include <cstdio>
#include <minIni.h>
#include <curl/curl.h>
#include <cctype>
#include <switch.h>

namespace sphaira::ownfoil {
namespace {

using namespace sphaira::ui;

#define OWNFOIL_INI_PATH "/config/NinMagicFoil/ownfoil.ini"

constexpr const char* PLACEHOLDER_UID_PREFIX = "pending-";

// The local profile key remains stable even if the server identity changes.
auto GeneratePlaceholderUid() -> std::string {
    char buf[32]{};
    std::snprintf(buf, sizeof(buf), "%s%016lx", PLACEHOLDER_UID_PREFIX, randomGet64());
    return buf;
}

} // namespace

OwnfoilForm::OwnfoilForm(const Config& config, const OnSaved& on_saved)
: FormSidebar{"Ownfoil Server"_i18n}
, m_config{config}
, m_on_saved{on_saved} {
    SetupButtons();
}

void OwnfoilForm::SetupButtons() {
    m_name = this->Add<SidebarEntryTextInput>(
        "Name"_i18n, m_config.name, "", "", -1, 32,
        "Display name for this saved server profile."_i18n
    );

    m_local_address = this->Add<SidebarEntryTextInput>(
        "URL"_i18n, m_config.local_address, "", "", -1, PATH_MAX,
        "Server URL, including http:// or https:// and any path. Enter login details in the separate fields."_i18n
    );

    m_remote_address = this->Add<SidebarEntryTextInput>(
        "Alternative URL"_i18n, m_config.remote_address, "", "", -1, PATH_MAX,
        "Optional second URL for the same server, for example its remote address."_i18n
    );

    m_user = this->Add<SidebarEntryTextInput>(
        "Username"_i18n, m_config.user, "", "", -1, PATH_MAX,
        "Optional: username used to log in to the server."_i18n
    );

    m_pass = this->Add<SidebarEntryTextBase>(
        "Password"_i18n, m_config.pass.empty() ? "" : "********", [this]() {
            SwkbdConfig keyboard{};
            if (R_FAILED(swkbdCreate(&keyboard, 0))) return;
            ON_SCOPE_EXIT(swkbdClose(&keyboard));
            swkbdConfigMakePresetDefault(&keyboard);
            swkbdConfigSetType(&keyboard, SwkbdType_All);
            swkbdConfigSetInitialText(&keyboard, m_config.pass.c_str());
            swkbdConfigSetStringLenMax(&keyboard, 256);
            keyboard.arg.arg.arg.passwordFlag = 1;
            char password[1025]{};
            if (R_SUCCEEDED(swkbdShow(&keyboard, password, sizeof(password)))) {
                m_config.pass = password;
                m_pass->SetValue(m_config.pass.empty() ? "" : "********");
            }
            std::fill(std::begin(password), std::end(password), 0);
        }, "Saved locally on this SD card. The password is hidden on screen, not encrypted on disk."_i18n
    );

    this->Add<SidebarEntryArray>("Server type"_i18n,
        SidebarEntryArray::Items{"Automatic"_i18n, "Ownfoil API", "Tinfoil JSON", "Ownfoil legacy"},
        [this](s64& index) { m_config.backend = static_cast<Backend>(index); },
        static_cast<s64>(m_config.backend), "Automatic tries the Ownfoil API and then a plain JSON shop index."_i18n);

    const auto callback = this->Add<SidebarEntryCallback>("Save"_i18n, [this](){
        const auto& primary = m_local_address->GetValue();
        const auto& alternative = m_remote_address->GetValue();
        if ((!primary.empty() && !IsSafeAddress(primary)) || (!alternative.empty() && !IsSafeAddress(alternative))) {
            App::Notify("Use an HTTP(S) URL without embedded credentials or a fragment."_i18n);
            return;
        }
        if (primary != m_config.local_address || alternative != m_config.remote_address) {
            m_config.resolved_url.clear();
            m_config.server_uid.clear();
        }
        m_config.name = m_name->GetValue();
        m_config.local_address = m_local_address->GetValue();
        m_config.remote_address = m_remote_address->GetValue();
        m_config.user = m_user->GetValue();

        // a brand new entry gets a placeholder uid until its first connect.
        SaveConfig(m_config);
        App::Notify("Ownfoil server saved."_i18n);

        if (m_on_saved) {
            m_on_saved(m_config);
        }

        this->SetPop();
    },  "Saves the Ownfoil server entry."_i18n);

    callback->Depends([this](){
        return
            !m_name->GetValue().empty() &&
            (!m_local_address->GetValue().empty() || !m_remote_address->GetValue().empty());
    }, "Name and at least one URL must be set!"_i18n);
}

auto IsSafeAddress(const std::string& address) -> bool {
    if (address.empty() || address.find_first_of("\r\n\t #") != std::string::npos) return false;
    auto url = address.find("://") == std::string::npos ? "http://" + address : address;
    CURLU* parsed = curl_url();
    if (!parsed) return false;
    ON_SCOPE_EXIT(curl_url_cleanup(parsed));
    if (curl_url_set(parsed, CURLUPART_URL, url.c_str(), 0) != CURLUE_OK) return false;
    char* value{};
    if (curl_url_get(parsed, CURLUPART_USER, &value, 0) == CURLUE_OK) {
        curl_free(value);
        return false;
    }
    if (curl_url_get(parsed, CURLUPART_SCHEME, &value, 0) != CURLUE_OK) return false;
    const bool allowed = !std::strcmp(value, "http") || !std::strcmp(value, "https");
    curl_free(value);
    return allowed;
}

auto IsSameOrigin(const std::string& base, const std::string& target) -> bool {
    if (!IsSafeAddress(base) || !IsSafeAddress(target)) return false;
    CURLU* a = curl_url();
    CURLU* b = curl_url();
    ON_SCOPE_EXIT(if (a) curl_url_cleanup(a); if (b) curl_url_cleanup(b));
    if (!a || !b || curl_url_set(a, CURLUPART_URL, base.c_str(), 0) != CURLUE_OK
        || curl_url_set(b, CURLUPART_URL, target.c_str(), 0) != CURLUE_OK) return false;
    for (const auto part : {CURLUPART_SCHEME, CURLUPART_HOST, CURLUPART_PORT}) {
        char *left{}, *right{};
        const auto l = curl_url_get(a, part, &left, CURLU_DEFAULT_PORT);
        const auto r = curl_url_get(b, part, &right, CURLU_DEFAULT_PORT);
        const auto same = l == CURLUE_OK && r == CURLUE_OK && !strcasecmp(left, right);
        curl_free(left);
        curl_free(right);
        if (!same) return false;
    }
    return true;
}

void LoadConfigs(Configs& out_configs) {
    static const auto cb = [](const mTCHAR *Section, const mTCHAR *Key, const mTCHAR *Value, void *UserData) -> int {
        auto e = static_cast<Configs*>(UserData);
        if (!Section || !Key || !Value) {
            return 1;
        }

        // add new entry if the section (uid) changed.
        if (e->empty() || std::strcmp(Section, e->back().uid.c_str())) {
            e->emplace_back();
            e->back().uid = Section;
        }

        if (!std::strcmp(Key, "server_uid")) {
            e->back().server_uid = Value;
        } else if (!std::strcmp(Key, "backend")) {
            e->back().backend = !std::strcmp(Value, "json") ? Backend::JsonIndex :
                !std::strcmp(Value, "native") ? Backend::Native : !std::strcmp(Value, "legacy") ? Backend::LegacyHtml : Backend::Auto;
        } else if (!std::strcmp(Key, "name")) {
            e->back().name = Value;
        } else if (!std::strcmp(Key, "local_address")) {
            e->back().local_address = Value;
        } else if (!std::strcmp(Key, "remote_address")) {
            e->back().remote_address = Value;
        } else if (!std::strcmp(Key, "user")) {
            e->back().user = Value;
        } else if (!std::strcmp(Key, "pass")) {
            e->back().pass = Value;
        } else if (!std::strcmp(Key, "resolved_url")) {
            e->back().resolved_url = Value;
        } else {
            log_write("[OWNFOIL] Ignoring unknown configuration key\n");
        }

        return 1;
    };

    out_configs.resize(0);
    ini_browse(cb, &out_configs, OWNFOIL_INI_PATH);
    log_write("[OWNFOIL] Found %zu server configs\n", out_configs.size());
}

auto IsPlaceholderUid(const std::string& uid) -> bool {
    return uid.starts_with(PLACEHOLDER_UID_PREFIX);
}

void SaveConfig(Config& config, const std::string& previous_uid) {
    if (config.uid.empty()) {
        config.uid = GeneratePlaceholderUid();
    }

    // the uid is the ini section, so replacing it is a delete plus a re-write.
    if (!previous_uid.empty() && previous_uid != config.uid) {
        log_write("[OWNFOIL] Updating local profile key\n");
        DeleteConfig(previous_uid);
    }

    // Profiles are kept separate even when they use the same server UID.
    fs::FsNativeSd().CreateDirectoryRecursively("/config/NinMagicFoil/");
    ini_puts(config.uid.c_str(), "server_uid", config.server_uid.c_str(), OWNFOIL_INI_PATH);
    ini_puts(config.uid.c_str(), "backend", config.backend == Backend::JsonIndex ? "json" : config.backend == Backend::Native ? "native" : config.backend == Backend::LegacyHtml ? "legacy" : "auto", OWNFOIL_INI_PATH);
    ini_puts(config.uid.c_str(), "name", config.name.c_str(), OWNFOIL_INI_PATH);
    ini_puts(config.uid.c_str(), "local_address", config.local_address.c_str(), OWNFOIL_INI_PATH);
    ini_puts(config.uid.c_str(), "remote_address", config.remote_address.c_str(), OWNFOIL_INI_PATH);
    ini_puts(config.uid.c_str(), "user", config.user.c_str(), OWNFOIL_INI_PATH);
    ini_puts(config.uid.c_str(), "pass", config.pass.c_str(), OWNFOIL_INI_PATH);
    ini_puts(config.uid.c_str(), "resolved_url", config.resolved_url.c_str(), OWNFOIL_INI_PATH);
}

void DeleteConfig(const std::string& uid) {
    ini_puts(uid.c_str(), nullptr, nullptr, OWNFOIL_INI_PATH);
}

auto FindByUid(Configs& configs, const std::string& uid) -> Config* {
    if (uid.empty()) {
        return nullptr;
    }

    const auto it = std::find_if(configs.begin(), configs.end(), [&uid](const auto& e) {
        return e.uid == uid || e.server_uid == uid;
    });

    return it == configs.end() ? nullptr : &*it;
}

} // namespace sphaira::ownfoil

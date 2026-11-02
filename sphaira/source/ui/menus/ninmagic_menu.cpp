#include "ui/menus/ninmagic_menu.hpp"
#include "ui/menus/game_menu.hpp"
#include "ui/menus/filebrowser.hpp"
#include "ui/menus/file_picker.hpp"
#include "ui/menus/homebrew.hpp"
#include "ui/install_queue.hpp"
#include "ui/option_box.hpp"
#include "ui/sidebar.hpp"
#include "ui/nvg_util.hpp"
#include "app.hpp"
#include "ninmagic_branding.hpp"
#include "i18n.hpp"
#include "nro.hpp"
#include <algorithm>
#include <cstdio>
#include <fstream>
namespace sphaira::ui::menu::ninmagic {
using Cat = sphaira::ownfoil::api::Category;
namespace {
void ConfirmDbi(const std::string& path) {
    if (InstallQueue::Get().IsBusy()) {
        App::Notify("Espera a que termine la cola antes de abrir DBI.");
        return;
    }
    const auto message = "Abrir " + path + "?\n\nEn DBI, entra en Browse SD Card y abre /switch/NinMagicFoil/downloads.\n\nSe abrirá la aplicación seleccionada; elige allí el paquete que quieras instalar.";
    App::Push<OptionBox>(message, "Cancelar", "Abrir DBI", 0, [path](auto choice) {
        if (!choice || *choice != 1) return;
        if (InstallQueue::Get().IsBusy()) {
            App::Notify("La cola está ocupada. DBI no se ha abierto.");
            return;
        }
        App::PushErrorBox(nro_launch(path), "No se pudo abrir DBI.");
    });
}
void SelectDbi() {
    App::Push<filebrowser::picker::Menu>([](const fs::FsPath& path) {
        if (!fs::FsNativeSd().FileExists(path)) {
            App::Notify("Selecciona el archivo .nro de DBI guardado en la SD.");
            return false;
        }
        ConfirmDbi(path.toString());
        return true;
    }, std::vector<std::string>{"nro"}, fs::FsPath{"/switch"});
}
}
const std::vector<Menu::Section> Menu::Sections = {
    {"Instalados", "\u25c6", Kind::Installed}, {"Explorador", "\u25a4", Kind::Files},
    {"Juegos nuevos", "\u2731", Kind::NewGames}, {"Actualizaciones", "\u2731", Kind::Updates},
    {"DLC", "+", Kind::Dlc}, {"Servidores Ownfoil", "\u25a4", Kind::Servers},
    {"XCI / XCZ", "\u25c7", Kind::Xci}, {"Homebrew", "\u25c8", Kind::Homebrew},
    {"Partidas guardadas", "\u25a3", Kind::Saves}, {"Cartucho", "\u25a3", Kind::GameCard},
    {"USB y red", "\u21c4", Kind::Transfers}, {"Cola", "\u2637", Kind::Queue},
    {"Consola", "\u25a4", Kind::Console}, {"Opciones", "\u2699", Kind::Options}, {"Ayuda", "?", Kind::Help},
};
Menu::Menu() {
    m_updater = std::make_unique<main::MainMenu>(true);
    if (R_SUCCEEDED(romfsInit())) {
        m_logo = nvgCreateImage(App::GetVg(), "romfs:/ninmagic-logo.jpg", 0);
        romfsExit();
    }
    SetAction(Button::A, Action{"Aceptar", [this]{ Activate(); }});
    SetAction(Button::B, Action{"Salir", [this]{ Exit(); }});
    SetAction(Button::Y, Action{"Opciones", [this]{ Select(13); FocusPanel(true); }});
    SetAction(Button::START, Action{"Buscar", [this]{ Search(); }});
    SetAction(Button::UP, Action{[this]{ if (m_section) Select(m_section-1); }});
    SetAction(Button::DOWN, Action{[this]{ if (m_section+1<Sections.size()) Select(m_section+1); }});
    SetAction(Button::RIGHT, Action{[this]{ Activate(); }});
    Select(0);
}
Menu::~Menu() {
    m_panel=nullptr; m_content.reset(); m_shop.reset(); m_updater.reset();
    if (m_logo > 0) nvgDeleteImage(App::GetVg(), m_logo);
}
void Menu::Exit() {
    if (!InstallQueue::Get().IsBusy()) { App::Exit(); return; }
    App::Push<OptionBox>("Hay instalaciones en curso. ¿Salir y cancelarlas?", "Continuar", "Salir", 0, [](auto i){ if (i && *i==1) App::Exit(); });
}
void Menu::FocusPanel(bool focus) {
    m_panel_focus=focus;
    if (m_panel) { if (focus) m_panel->OnFocusGained(); else m_panel->OnFocusLost(); }
}
void Menu::OnFocusGained() { Widget::OnFocusGained(); if (m_panel && m_panel_focus) m_panel->OnFocusGained(); }
void Menu::OnFocusLost() { Widget::OnFocusLost(); if (m_panel) m_panel->OnFocusLost(); }
void Menu::Select(size_t index) {
    if (index >= Sections.size()) return;
    if (m_panel) m_panel->OnFocusLost();
    m_panel=nullptr; m_content.reset(); m_panel_focus=false; m_section=index; m_row=0; m_row_first=0; m_scroll=0;
    if (index<m_first) m_first=index;
    if (index>=m_first+8) m_first=index-7;
    const auto flags=MenuFlag_Tab | MenuFlag_Embedded;
    switch (Sections[index].kind) {
        case Kind::Installed: m_content=std::make_unique<game::Menu>(flags); break;
        case Kind::Files: m_content=std::make_unique<filebrowser::Menu>(flags, filebrowser::FsOption_All & ~filebrowser::FsOption_CanSplit); break;
        case Kind::Xci: {
            auto menu=std::make_unique<filebrowser::Menu>(flags, filebrowser::FsOption_All & ~filebrowser::FsOption_CanSplit);
            menu->SetFilter({"xci", "xcz"}); m_content=std::move(menu);
        } break;
        case Kind::Homebrew: m_content=std::make_unique<homebrew::Menu>(flags); break;
        case Kind::NewGames: case Kind::Updates: case Kind::Dlc: case Kind::Servers:
            if (!m_shop) m_shop=std::make_unique<ownfoil::Menu>(flags);
            if (Sections[index].kind==Kind::Servers) m_shop->ShowServers();
            else m_shop->SelectCategory(Sections[index].kind==Kind::Updates ? Cat::Updates : Sections[index].kind==Kind::Dlc ? Cat::Dlc : Cat::NewGames);
            m_panel=m_shop.get(); break;
        default: break;
    }
    if (m_content) m_panel=m_content.get();
    if (m_panel) { m_panel->SetPop(false); m_panel->OnFocusGained(); m_panel->OnFocusLost(); }
    BuildRows(); if (Sections[index].kind==Kind::Console) RefreshLog();
}
void Menu::OpenMenu(const char* name) {
    for (const auto& e : main::GetMenuMenuEntries()) if (std::string_view(e.name)==name) { App::Push(e.func(MenuFlag_None)); return; }
    App::Notify("Esta función no está incluida en esta compilación.");
}
void Menu::BuildRows() {
    m_rows.clear();
    const auto add=[this](const char* name,const char* info,std::function<void()> fn){m_rows.push_back({name,info,std::move(fn)});};
    switch (Sections[m_section].kind) {
        case Kind::Saves: add("Abrir partidas guardadas", "Elige usuario y juego para crear o restaurar una copia de la partida.", [this]{OpenMenu("Saves");}); break;
        case Kind::GameCard: add("Abrir cartucho", "Información, copia e instalación del cartucho insertado.", [this]{OpenMenu("GameCard");}); break;
        case Kind::Transfers:
            add("Instalar por USB", "Recepción de paquetes desde un cliente compatible en el ordenador.", [this]{OpenMenu("USB");});
            add("Instalar por MTP", "Accede desde el ordenador conectado por USB.", [this]{OpenMenu("MTP");});
            add("Instalar por FTP", "Recibe archivos por red con el servidor FTP.", [this]{OpenMenu("FTP");});
            add("Configurar MTP", "Carpetas visibles y servicio USB MTP.", []{App::DisplayMtpOptions(false);});
            add("Configurar FTP", "Puerto, usuario, contraseña y carpetas.", []{App::DisplayFtpOptions(false);});
            add("Unidades USB / HDD", "Montar almacenamiento externo.", []{App::DisplayHddOptions(false);});
            add("Conexiones de red", "HTTP, FTP, SFTP, SMB y NFS en las opciones avanzadas.", []{App::DisplayAdvancedOptions(false);});
            add("Abrir DBI", "Instala los paquetes descargados en la SD con DBI.", [this]{OpenDbi();}); break;
        case Kind::Options:
            add("Tema y apariencia", "Personaliza colores, fondo y música.", []{App::DisplayThemeOptions(false);});
            add("Idioma", "Elige el idioma de las pantallas y mensajes.", []{
                auto list=std::make_unique<Sidebar>("Idioma",Sidebar::Side::RIGHT);
                list->Add<SidebarEntryArray>("Idioma", SidebarEntryArray::Items{"Auto","English","日本語","Français","Deutsch","Italiano","Español","简体中文","한국어","Nederlands","Português","Русский","繁體中文","Svenska","Tiếng Việt","Українська","Türkçe"}, [](s64& n){App::SetLanguage(n);},App::GetLanguage());
                App::Push(std::move(list));
            });
            add("Instalación", "Destino, verificación y opciones del instalador.", []{App::DisplayInstallOptions(false);});
            add("Servidores Ownfoil", "URL, usuario y contraseña de cada servidor.", [this]{Select(5);FocusPanel(true);});
            add("Actualizar NinMagicFoil", "Buscar una versión publicada en ANE-JV/ninmagicfoil.", [this]{m_updater->CheckForUpdates(true);});
            add("Copias y exportación", "Opciones de las copias de contenido.", []{App::DisplayDumpOptions(false);});
            add("Opciones avanzadas", "Registro, montajes de red y rendimiento.", []{App::DisplayAdvancedOptions(false);});
            add("Más herramientas", "Álbum, AppStore, GitHub y otras herramientas.", []{App::DisplayMenuOptions(false);}); break;
        default: break;
    }
}
void Menu::Search() {
    Select(2);
    FocusPanel(true);
    m_shop->SelectCategory(Cat::Search, true);
}
void Menu::OpenDbi() {
    if (InstallQueue::Get().IsBusy()) {
        App::Notify("Espera a que termine la cola antes de abrir DBI.");
        return;
    }
    fs::FsNativeSd filesystem;
    std::string path;
    for (const auto candidate : {"/switch/DBI/dbi.nro", "/switch/DBI/DBI.nro", "/switch/DBI.nro", "/switch/dbi.nro", "/switch/DBI/DBIru.nro", "/switch/DBI/dbiru.nro", "/switch/DBIru/DBIru.nro", "/switch/DBIru.nro"}) {
        if (filesystem.FileExists(candidate)) { path = candidate; break; }
    }
    if (path.empty()) {
        App::Push<OptionBox>("No se encontró DBI en las rutas habituales. Selecciona su archivo .nro en la SD.", "Cancelar", "Buscar DBI", 0, [](auto choice) {
            if (choice && *choice == 1) SelectDbi();
        });
        return;
    }
    ConfirmDbi(path);
}
auto Menu::FooterActions() -> Actions {
    if (!m_panel_focus) return GetActions();
    Actions actions;
    if (m_panel) {
        actions = m_panel->GetActions();
        if (Sections[m_section].kind == Kind::Files || Sections[m_section].kind == Kind::Xci) {
            auto active = static_cast<filebrowser::Menu*>(m_panel)->GetActiveActions();
            actions.merge(active);
        }
        actions.erase(Button::START);
        actions.erase(Button::L2);
        if (m_panel == m_shop.get()) actions.erase(Button::R2);
    } else {
        if (!m_rows.empty() || Sections[m_section].kind == Kind::Queue || Sections[m_section].kind == Kind::Console)
            actions.emplace(Button::A, Action{Sections[m_section].kind == Kind::Queue ? "Gestionar" : "Aceptar", [this]{Activate();}});
        actions.emplace(Button::B, Action{"Volver", [this]{FocusPanel(false);}});
        if (Sections[m_section].kind == Kind::Console) actions.emplace(Button::X, Action{"Refrescar", [this]{RefreshLog();}});
    }
    actions.insert_or_assign(Button::L2, Action{"Menú", [this]{FocusPanel(false);}});
    return actions;
}

void Menu::Activate() {
    if (!m_panel_focus) { FocusPanel(true); return; }
    if (m_panel) return;
    if (!m_rows.empty()) { auto action=m_rows[m_row].action; action(); }
    else if (Sections[m_section].kind==Kind::Queue) InstallQueue::Get().ShowMenu();
    else if (Sections[m_section].kind==Kind::Console) RefreshLog();
}
void Menu::RefreshLog() {
    m_log.clear(); std::ifstream file("/config/NinMagicFoil/log.txt",std::ios::binary);
    if (!file) {m_log.emplace_back("Activa el registro en Opciones > Opciones avanzadas.");return;}
    file.seekg(0,std::ios::end);const auto size=file.tellg();
    if (size>65536) {file.seekg(size-std::streamoff(65536));std::string discard;std::getline(file,discard);} else file.seekg(0);
    for(std::string line;std::getline(file,line);) m_log.push_back(std::move(line));
    if(m_log.empty()) m_log.emplace_back("El registro está vacío.");
    m_scroll=m_log.size()>24?m_log.size()-24:0;
}
void Menu::Update(Controller* controller, TouchInfo* touch) {
    // Keep visible content current while sidebar inputs are handled separately.
    if (m_panel && !m_panel_focus) {
        Controller idle_controller{};
        TouchInfo idle_touch{};
        m_panel->Update(&idle_controller, &idle_touch);
    }
    if (controller->GotDown(Button::START)) { Search(); return; }
    if (controller->GotDown(Button::L2)) { FocusPanel(false); return; }
    if (touch->is_clicked && touch->in_range(28,96,346,536)) {
        const auto index = m_first + (touch->cur.y - 96) / 67;
        if (index < Sections.size()) {
            if (index == m_section && !m_panel_focus) FocusPanel(true);
            else Select(index);
        }
        return;
    }
    if (touch->is_clicked && touch->cur.y >= 646) {
        const auto actions = FooterActions();
        for (const auto& button : Widget::GetUiButtons(actions)) {
            if (touch->in_range(button.GetPos())) {
                if (const auto action = actions.find(button.m_button); action != actions.end()) action->second.Invoke(true);
                return;
            }
        }
        return;
    }
    if (!m_panel_focus && touch->is_clicked && touch->in_range(390,96,860,538)) FocusPanel(true);
    if (!m_panel_focus) { Widget::Update(controller,touch); return; }
    if (m_panel) {
        // ZL returns to the shell; the shop's page controls move to L/R.
        if (m_panel == m_shop.get()) {
            if (controller->GotDown(Button::B) && m_shop->CanGoUp()) { m_shop->GoUp(); return; }
            if (controller->GotDown(Button::L)) { m_panel->FireAction(Button::L2); return; }
            if (controller->GotDown(Button::R)) { m_panel->FireAction(Button::R2); return; }
        }
        m_panel->Update(controller,touch);
        if (m_panel->ShouldPop()) { m_panel->SetPop(false); FocusPanel(false); }
        return;
    }
    if (controller->GotDown(Button::B) || controller->GotDown(Button::LEFT)) { FocusPanel(false); return; }
    if (controller->GotDown(Button::A)) { Activate(); return; }
    if (controller->GotDown(Button::X) && Sections[m_section].kind == Kind::Console) RefreshLog();
    if (controller->GotDown(Button::DOWN)) { if (!m_rows.empty()) m_row=std::min(m_row+1,m_rows.size()-1); else ++m_scroll; }
    if (controller->GotDown(Button::UP)) { if (!m_rows.empty()) { if (m_row) --m_row; } else if (m_scroll) --m_scroll; }
    if (m_row < m_row_first) m_row_first=m_row;
    if (m_row >= m_row_first+8) m_row_first=m_row-7;
    if (!m_rows.empty() && touch->is_clicked && touch->in_range(396,100,844,496)) {
        const auto row=m_row_first+(touch->cur.y-100)/62;
        if (row<m_rows.size()) { m_row=row; Activate(); }
    }
}

void Menu::DrawChrome(NVGcontext* vg, Theme* theme) {
    const auto text=theme->GetColour(ThemeEntryID_TEXT), muted=theme->GetColour(ThemeEntryID_TEXT_INFO), accent=theme->GetColour(ThemeEntryID_TEXT_SELECTED);
    DrawElement(0,0,1280,720,ThemeEntryID_BACKGROUND);
    if(m_logo > 0) gfx::drawImage(vg,30,18,52,52,m_logo,4);
    gfx::drawText(vg,96,26,25,text,"NINMAGICFOIL");gfx::drawText(vg,96,57,17,muted,Sections[m_section].name);
    if (m_shop && m_panel == m_shop.get() && m_panel_focus && m_shop->HasAction(Button::R2)) gfx::drawText(vg,440,58,16,muted,"L / R  Página");
    const auto data=MenuBase::GetPolledData();
    const auto storage=[&](float x,const char* label,s64 free,s64 total){
        gfx::drawText(vg,x,23,16,text,label);gfx::drawRect(vg,x,46,165,5,muted);
        const auto used=std::clamp(1.0-double(free)/double(std::max<s64>(total,1)),0.0,1.0);
        gfx::drawRect(vg,x,46,float(used)*165,5,accent);
        gfx::drawTextArgs(vg,x,58,15,NVG_ALIGN_LEFT|NVG_ALIGN_TOP,muted,"Libre %.1f GB",double(free)/(1024.*1024.*1024.));
    };
    storage(740,"Memoria del sistema",data.emmc_free,data.emmc_total);storage(930,"Tarjeta microSD",data.sd_free,data.sd_total);
    char clock[24];std::snprintf(clock,sizeof(clock),"%02d:%02d",data.tm.tm_hour,data.tm.tm_min);
    gfx::drawText(vg,1238,23,27,text,clock,NVG_ALIGN_RIGHT|NVG_ALIGN_TOP);
    gfx::drawText(vg,1238,58,15,muted,data.ip?"En línea":"Sin red",NVG_ALIGN_RIGHT|NVG_ALIGN_TOP);
    gfx::drawRect(vg,28,86,1222,1,muted);gfx::drawRect(vg,378,96,1,538,muted);gfx::drawRect(vg,28,646,1222,1,muted);
    for(size_t i=m_first;i<Sections.size()&&i<m_first+8;++i) {
        const float y=98+(i-m_first)*67;const auto selected=i==m_section;
        if(selected) {gfx::drawRect(vg,31,y,337,61,theme->GetColour(ThemeEntryID_SELECTED_BACKGROUND));gfx::drawRectOutline(vg,theme,m_panel_focus?1:2,31,y,337,61);}
        gfx::drawText(vg,48,y+18,22,selected?accent:text,Sections[i].icon);gfx::drawText(vg,85,y+20,23,selected?accent:text,Sections[i].name);
    }
    if(m_first) gfx::drawText(vg,355,93,14,accent,"▲");
    if(m_first+8<Sections.size()) gfx::drawText(vg,355,622,14,accent,"▼");
    const auto actions = FooterActions();
    auto buttons=Widget::GetUiButtons(actions);
    for (auto& button:buttons) button.Draw(vg,theme);
}
void Menu::DrawRows(NVGcontext* vg, Theme* theme) {
    const auto text=theme->GetColour(ThemeEntryID_TEXT),muted=theme->GetColour(ThemeEntryID_TEXT_INFO),accent=theme->GetColour(ThemeEntryID_TEXT_SELECTED);
    for(size_t i=m_row_first;i<m_rows.size() && i<m_row_first+8;++i) {
        const float y=100+(i-m_row_first)*62;if(m_panel_focus&&i==m_row) gfx::drawRectOutline(vg,theme,2,398,y,842,58);
        nvgSave(vg); nvgIntersectScissor(vg,412,y+2,814,54);
        gfx::drawText(vg,412,y+7,22,i==m_row?accent:text,m_rows[i].name.c_str());gfx::drawText(vg,412,y+35,15,muted,m_rows[i].info.c_str());
        nvgRestore(vg);
        gfx::drawRect(vg,408,y+60,820,1,theme->GetColour(ThemeEntryID_LINE_SEPARATOR));
    }
    if (m_row_first) gfx::drawText(vg,1226,99,14,accent,"▲");
    if (m_row_first+8<m_rows.size()) gfx::drawText(vg,1226,603,14,accent,"▼");
}
void Menu::DrawPanel(NVGcontext* vg, Theme* theme) {
    if(m_panel) {m_panel->Draw(vg,theme);return;}
    const auto text=theme->GetColour(ThemeEntryID_TEXT),muted=theme->GetColour(ThemeEntryID_TEXT_INFO),accent=theme->GetColour(ThemeEntryID_TEXT_SELECTED);
    if(!m_rows.empty()) {DrawRows(vg,theme);return;}
    switch(Sections[m_section].kind) {
        case Kind::Queue: {
            gfx::drawText(vg,410,103,18,muted,"Juego");gfx::drawText(vg,1110,103,18,muted,"Estado");
            const auto jobs=InstallQueue::Get().GetSnapshot();
            if(jobs.empty()) {gfx::drawText(vg,410,150,22,text,"No hay instalaciones en la cola.");break;}
            m_scroll=std::min(m_scroll,jobs.size()>15?jobs.size()-15:0);
            static const char* states[]={"En espera","Instalando","Completado","Falló","Cancelado"};
            for(size_t i=m_scroll;i<jobs.size()&&i<m_scroll+15;++i) {
                float y=139+(i-m_scroll)*31;nvgSave(vg);nvgIntersectScissor(vg,410,y,680,30);
                gfx::drawText(vg,410,y,18,text,jobs[i].name.c_str());nvgRestore(vg);
                gfx::drawText(vg,1110,y,17,accent,states[static_cast<int>(jobs[i].state)]);
            }
        } break;
        case Kind::Console: {
            m_scroll=std::min(m_scroll,m_log.size()>24?m_log.size()-24:0);
            for(size_t i=m_scroll;i<m_log.size()&&i<m_scroll+24;++i) gfx::drawText(vg,401,103+(i-m_scroll)*21,15,text,m_log[i].c_str());
        } break;
        case Kind::Help:
            gfx::drawText(vg,410,108,25,accent,"NinMagicFoil 0.1.0");
            gfx::drawTextBox(vg,410,153,18,800,text,
                "Ownfoil\nAñade tus servidores en Servidores Ownfoil. Cada uno conserva su URL y sus datos de acceso.\n\n"
                "Instalación\nAbre un juego y selecciona el contenido. Cola muestra el progreso y permite cancelar. Opciones > Instalación elige el destino.\n\n"
                "DBI\nDescarga el paquete a la SD desde Ownfoil. USB y red > Abrir DBI abre la aplicación; selecciona allí /switch/NinMagicFoil/downloads.\n\n"
                "Navegación\nA entra, B retrocede, ZL vuelve al menú. + busca en Ownfoil. L/R cambian la página del catálogo.\n\n"
                "Basado en Sphaira 1.0.9 · GPL-3.0\ngithub.com/ANE-JV/ninmagicfoil"); break;
        default: break;
    }
}
void Menu::Draw(NVGcontext* vg, Theme* theme) {
    DrawChrome(vg,theme);nvgSave(vg);nvgIntersectScissor(vg,390,96,860,538);DrawPanel(vg,theme);nvgRestore(vg);
}
}

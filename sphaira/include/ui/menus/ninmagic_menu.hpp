#pragma once
#include "ui/menus/menu_base.hpp"
#include "ui/menus/main_menu.hpp"
#include "ui/menus/ownfoil.hpp"
#include <vector>
#include <functional>
namespace sphaira::ui::menu::ninmagic {
struct Menu final : Widget {
    Menu(); ~Menu();
    void Update(Controller*, TouchInfo*) override;
    void Draw(NVGcontext*, Theme*) override;
    void OnFocusGained() override; void OnFocusLost() override;
    auto IsMenu() const -> bool override { return true; }
private:
    enum class Kind { Installed, Files, NewGames, Updates, Dlc, Servers, Xci, Homebrew, Saves, GameCard, Transfers, Queue, Console, Options, Help };
    struct Section { const char* name; const char* icon; Kind kind; };
    struct Row { std::string name; std::string info; std::function<void()> action; };
    void Select(size_t); void Activate(); void FocusPanel(bool); void Exit();
    void DrawChrome(NVGcontext*, Theme*); void DrawPanel(NVGcontext*, Theme*); void DrawRows(NVGcontext*, Theme*);
    void BuildRows(); void OpenMenu(const char*); void RefreshLog();
    void Search(); void OpenDbi();
    auto FooterActions() -> Actions;
    static const std::vector<Section> Sections;
    std::unique_ptr<MenuBase> m_content;
    std::unique_ptr<ownfoil::Menu> m_shop;
    std::unique_ptr<main::MainMenu> m_updater;
    MenuBase* m_panel{};
    std::vector<Row> m_rows;
    std::vector<std::string> m_log;
    size_t m_section{}, m_first{}, m_row{}, m_row_first{}, m_scroll{};
    bool m_panel_focus{};
    int m_logo{};
};
}

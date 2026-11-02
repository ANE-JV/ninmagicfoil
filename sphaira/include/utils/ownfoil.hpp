#pragma once

#include "ui/sidebar.hpp"
#include <string>
#include <vector>
#include <functional>

namespace sphaira::ownfoil {

enum class Backend { Auto, Native, JsonIndex, LegacyHtml };

// A locally saved server profile. Credentials never belong in the URL.
struct Config {
    // Stable local key: separate profiles can address the same server.
    std::string uid{};
    std::string server_uid{};
    std::string name{};

    std::string local_address{};   // host[:port], optional.
    std::string remote_address{};  // host[:port] or hostname, optional.
    std::string user{};
    std::string pass{};
    Backend backend{Backend::Auto};
    bool legacy_html{}; // older Ownfoil directory protocol, negotiated at connect
    bool json_index{}; // negotiated at connect time; never persisted

    // the root url that last answered, so a connect goes straight there rather
    // than paying a connect timeout on a dead local address every launch.
    std::string resolved_url{};
};
using Configs = std::vector<Config>;

void LoadConfigs(Configs& out_configs);
auto IsSafeAddress(const std::string& address) -> bool;
auto IsSameOrigin(const std::string& base, const std::string& target) -> bool;

// true for a locally generated stand-in uid. the `pending-` namespace is
// reserved by the API spec, so it can never collide with a real one.
auto IsPlaceholderUid(const std::string& uid) -> bool;

// creates or updates a saved server, keyed by config.uid (the ini section), and
// assigns a placeholder uid when it is empty. pass the entry's former uid as
// `previous_uid` so the old section is removed - the only way a key ever moves.
// Locally generated keys preserve separate URL/account profiles.
void SaveConfig(Config& config, const std::string& previous_uid = {});

void DeleteConfig(const std::string& uid);

// an empty uid matches nothing, and a placeholder never matches a real uid.
auto FindByUid(Configs& configs, const std::string& uid) -> Config*;

// the one way a saved entry is ever created or changed.
struct OwnfoilForm final : public ui::FormSidebar {
    using OnSaved = std::function<void(const Config&)>;

    explicit OwnfoilForm(const Config& config = {}, const OnSaved& on_saved = {});

private:
    void SetupButtons();

private:
    Config m_config{};
    const OnSaved m_on_saved{};

    ui::SidebarEntryTextInput* m_name{};
    ui::SidebarEntryTextInput* m_local_address{};
    ui::SidebarEntryTextInput* m_remote_address{};
    ui::SidebarEntryTextInput* m_user{};
    ui::SidebarEntryTextBase* m_pass{};
};

} // namespace sphaira::ownfoil

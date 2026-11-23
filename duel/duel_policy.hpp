#pragma once
// w0uFF Together - protocol-independent 1v1 rules.
// GPL-3.0, see upstream UnchainedTogether attribution in NOTICE.md.
// Not a network transport and not a client tamper-proof anticheat.

#include <cstdint>

namespace wouff::duel
{
    enum class stage : std::uint8_t { vanilla, lobby, racing, finished };
    constexpr std::uint32_t protocol_version = 1;

    struct host_settings
    {
        std::uint32_t version = protocol_version;
        std::uint64_t session_id = 0;
        std::uint32_t revision = 0;
        bool cheats_enabled = false; // always OFF by default
    };

    // Callers MUST authenticate the host and match the session ID through
    // an authoritative transport before invoking accept_host_settings.
    class policy
    {
    public:
        stage current_stage() const noexcept { return stage_; }
        bool is_host() const noexcept { return local_is_host_; }
        bool has_host_confirmation() const noexcept { return host_confirmed_; }
        bool cheats_enabled_in_lobby() const noexcept { return settings_.cheats_enabled; }

        void reset() noexcept
        {
            stage_ = stage::vanilla;
            local_is_host_ = false;
            host_confirmed_ = false;
            settings_ = {};
        }

        bool open_host_lobby(std::uint64_t session_id) noexcept
        {
            if (stage_ != stage::vanilla || session_id == 0) return false;
            reset();
            local_is_host_ = true;
            host_confirmed_ = true;
            settings_.session_id = session_id;
            stage_ = stage::lobby;
            return true;
        }

        // Guest mode is locked down until settings arrive from an authenticated
        // host; this prevents a local checkbox from changing match rules.
        bool open_guest_lobby(std::uint64_t session_id) noexcept
        {
            if (stage_ != stage::vanilla || session_id == 0) return false;
            reset();
            settings_.session_id = session_id;
            stage_ = stage::lobby;
            return true;
        }

        bool host_set_cheats(bool enable) noexcept
        {
            if (!local_is_host_ || stage_ != stage::lobby) return false;
            settings_.cheats_enabled = enable;
            ++settings_.revision;
            return true;
        }

        // Verified message transport must reject non-host senders before
        // calling this method; never trust a sender-supplied is_host flag.
        bool accept_host_settings(const host_settings& settings) noexcept
        {
            if (local_is_host_ || stage_ != stage::lobby ||
                settings.version != protocol_version ||
                settings.session_id == 0 ||
                settings.session_id != settings_.session_id ||
                (host_confirmed_ &&
                 (settings.revision < settings_.revision ||
                  (settings.revision == settings_.revision &&
                   settings.cheats_enabled != settings_.cheats_enabled))))
                return false;
            settings_ = settings;
            host_confirmed_ = true;
            return true;
        }

        // The host's network layer must verify exactly two mod-compatible
        // peers and that server-side physical chain detachment was completed.
        // A UI toggle alone is NOT sufficient to begin a 1v1.
        bool start_race(bool two_players_confirmed, bool chain_detached_on_server,
                        bool peers_synchronized) noexcept
        {
            if (stage_ != stage::lobby || !host_confirmed_ ||
                !two_players_confirmed || !chain_detached_on_server ||
                !peers_synchronized)
                return false;
            stage_ = stage::racing;
            return true;
        }

        void finish() noexcept
        {
            if (stage_ == stage::racing) stage_ = stage::finished;
        }

        // The caller uses this when deciding whether to enable Fly, Teleport,
        // Freecam or other mod-provided cheats. Vanilla is not affected.
        bool allow_mod_cheats() const noexcept
        {
            if (stage_ == stage::vanilla) return true;
            return host_confirmed_ && settings_.cheats_enabled;
        }

        host_settings settings_for_host_broadcast() const noexcept
        {
            return local_is_host_ ? settings_ : host_settings{};
        }

    private:
        stage stage_ = stage::vanilla;
        bool local_is_host_ = false;
        bool host_confirmed_ = false;
        host_settings settings_{};
    };
}

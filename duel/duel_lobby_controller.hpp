#pragma once
// w0uFF Together - experimental Steam 1v1 *lobby handshake* UI.
// Does NOT detach chains or start a race until server authority is verified.
// All Steam calls stay on the renderer thread. Unreal data must arrive as POD.
// GPL-3.0, original author credited in NOTICE.md.

#include "steam_p2p_transport.hpp"
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <random>
#include <string>

namespace wouff::duel
{
    class lobby_controller
    {
    public:
        bool staged() const noexcept { return staged_; }
        bool mod_cheats_allowed() const noexcept
        {
            if (!staged_) return true; // vanilla: no changes
            if (!gate_ || !gate_->initialized()) return false; // fail closed
            if (!hosting_ && ImGui::GetTime() - last_incoming_ > 12.0) return false;
            return gate_->rules().allow_mod_cheats();
        }

        // Only a read-only renderer-thread operation; doesn't access UE.
        void tick()
        {
            const double now = ImGui::GetTime();
            if (!net_.available() && now - last_probe_ >= 6.0)
            {
                last_probe_ = now;
                net_.attach(); // existing Steam runtime; never initializes Steam
            }

            if (!staged_ || !gate_ || !net_.available()) return;
            for (int i=0; i<8; ++i)
            {
                const auto msg=net_.poll();
                if (!msg) break;
                if (!gate_->accept(msg->data,msg->authenticated_sender_id)) continue;
                last_incoming_ = now;
                if (!hosting_ && msg->data.type==message_type::hello)
                    remote_nonce_=msg->data.lobby_nonce;
                if (!hosting_ && msg->data.type==message_type::settings)
                    send_ack();
            }

            if (now - last_outgoing_ < 1.0) return;
            last_outgoing_ = now;
            if (hosting_)
            {
                packet hello{message_type::hello,0,session_id_,local_nonce_,
                    net_.local_steam_id(),peer_id_,0,1};
                net_.send(peer_id_,hello);
                const auto config=gate_->rules().settings_for_host_broadcast();
                packet settings{message_type::settings,
                    static_cast<std::uint8_t>(config.cheats_enabled ? cheat_permission : 0),
                    session_id_,local_nonce_,net_.local_steam_id(),peer_id_,
                    config.revision,static_cast<std::uint32_t>(2u+config.revision)};
                net_.send(peer_id_,settings);
            }
            else if (!gate_->rules().has_host_confirmation())
            {
                // Wake a pending Steam P2P session in both directions. The
                // host rejects this packet as a duel command, as intended.
                packet wake{message_type::hello,0,session_id_,session_id_,
                    peer_id_,net_.local_steam_id(),0,1};
                net_.send(peer_id_,wake);
            }
            else
                send_ack();
        }

        void draw(bool game_instance_known, bool local_is_host, int player_count)
        {
            ImGui::TextColored(ImVec4(.78f,.55f,1.f,1.f), "1v1 RACE / PRIVATE LOBBY - NETWORK ALPHA");
            ImGui::TextWrapped("Host and guest rule sync only. The actual race and chain-detach controls are not enabled until their authoritative game-server implementation has been validated.");
            ImGui::Separator();
            ImGui::Text("Steam P2P API: %s",net_.available() ? "DETECTED" : "NOT AVAILABLE");
            ImGui::Text("Your Steam ID64: %llu",static_cast<unsigned long long>(net_.local_steam_id()));
            ImGui::Text("Native game lobby: %s",game_instance_known?"DETECTED":"UNAVAILABLE");
            ImGui::Text("Game reports host: %s",local_is_host?"YES":"NO");
            ImGui::Text("Players in session: %d (requires exactly 2)",player_count);
            if (ImGui::Button("Retry Steam API"))
            {
                net_.detach();
                last_probe_ = -100.0;
            }
            if (!staged_)
            {
                ImGui::InputText("Opponent Steam ID64",&peer_text_);
                if (!local_is_host)
                    ImGui::InputText("Host session code",&session_text_);
                const bool eligible = net_.available() && game_instance_known &&
                    player_count==2 && parse_id(peer_text_)!=0 &&
                    parse_id(peer_text_)!=net_.local_steam_id();
                if (!eligible) ImGui::BeginDisabled();
                if (local_is_host)
                {
                    if (ImGui::Button("Open 1v1 host settings (alpha)"))
                        begin_host();
                }
                else if (ImGui::Button("Connect to 1v1 host (alpha)") &&
                    parse_id(session_text_)!=0)
                    begin_guest();
                if (!eligible) ImGui::EndDisabled();
                ImGui::TextDisabled("First create/join the game's normal private two-player lobby.");
            }
            else
            {
                ImGui::SeparatorText("Host-controlled rules");
                ImGui::Text("Role: %s",hosting_?"HOST":"GUEST");
                ImGui::Text("Session code: %llu",static_cast<unsigned long long>(session_id_));
                if (ImGui::Button("Copy session code"))
                    ImGui::SetClipboardText(std::to_string(session_id_).c_str());
                ImGui::Text("Partner: %llu",static_cast<unsigned long long>(peer_id_));
                ImGui::Text("Steam message status: %s",
                    hosting_ ? (gate_ && gate_->has_peer_ack() ?
                        "GUEST ACKNOWLEDGED HOST RULES" : "AWAITING GUEST ACK") :
                        (gate_ && gate_->rules().has_host_confirmation() ?
                        "HOST RULES RECEIVED" : "AWAITING HOST RULES"));
                if (hosting_ && gate_)
                {
                    bool enabled=gate_->rules().cheats_enabled_in_lobby();
                    if (ImGui::Checkbox("Cheats Enabled (host only)",&enabled))
                        gate_->host_set_cheats(enabled);
                }
                else
                    ImGui::Text("Cheats Enabled: %s",
                        gate_ && gate_->rules().cheats_enabled_in_lobby() ? "YES (host setting)" : "NO");
                if (!mod_cheats_allowed())
                    ImGui::TextColored(ImVec4(1.0f,.55f,.35f,1.0f),
                        "Mod cheats locked: Fly F7, Teleport and Free Camera.");
                ImGui::SeparatorText("1v1 race");
                ImGui::TextWrapped("Physical chains are still attached. Server-side detachment and winner validation are not implemented yet.");
                ImGui::BeginDisabled();
                ImGui::Button("Start 1v1 - unavailable until server/chain verification");
                ImGui::EndDisabled();
                if (ImGui::Button("Leave 1v1 staging"))
                    leave();
            }
        }

        void leave() noexcept
        {
            if (net_.available() && peer_id_) net_.close(peer_id_);
            gate_.reset();
            staged_ = false; hosting_ = false;
            session_id_ = 0; local_nonce_ = 0; remote_nonce_ = 0;
            peer_id_ = 0; last_incoming_ = 0;
        }

    private:
        static std::uint64_t parse_id(const std::string& s) noexcept
        {
            if (s.empty() || s.size()>20) return 0;
            for (char c:s) if (c<'0' || c>'9') return 0;
            try
            {
                return static_cast<std::uint64_t>(std::stoull(s));
            }
            catch (...) { return 0; }
        }

        static std::uint64_t random_nonzero() noexcept
        {
            try
            {
                std::random_device entropy;
                const std::uint64_t value =
                    (static_cast<std::uint64_t>(entropy())<<32) |
                    static_cast<std::uint64_t>(entropy());
                return value ? value : 1;
            }
            catch (...) { return 0; }
        }

        void begin_host()
        {
            if (!net_.available() || !net_.local_steam_id()) return;
            const auto peer=parse_id(peer_text_);
            if (!peer || peer==net_.local_steam_id()) return;
            const auto session=random_nonzero();
            const auto nonce=random_nonzero();
            if (!session || !nonce || session==nonce) return;
            auto next=std::make_unique<peer_gate>(session,net_.local_steam_id(),peer,true);
            if (!next->initialized() || !next->host_bind_nonce(nonce)) return;
            leave();
            gate_=std::move(next);
            staged_=true; hosting_=true;
            session_id_=session; local_nonce_=nonce;
            peer_id_=peer;
            last_outgoing_=-100.0;
        }

        void begin_guest()
        {
            if (!net_.available() || !net_.local_steam_id()) return;
            const auto peer=parse_id(peer_text_);
            const auto session=parse_id(session_text_);
            if (!peer || peer==net_.local_steam_id() || !session) return;
            auto next=std::make_unique<peer_gate>(session,peer,net_.local_steam_id(),false);
            if (!next->initialized()) return;
            leave();
            gate_=std::move(next);
            staged_=true; hosting_=false;
            session_id_=session; peer_id_=peer;
            last_outgoing_=-100.0;
        }

        void send_ack()
        {
            if (!gate_ || !remote_nonce_ || !net_.available()) return;
            const std::uint32_t revision = last_settings_revision_;
            packet ack{message_type::settings_ack,0,session_id_,remote_nonce_,
                peer_id_,net_.local_steam_id(),revision,
                static_cast<std::uint32_t>(revision+1)};
            net_.send(peer_id_,ack);
        }

        steam_p2p_transport net_{};
        std::unique_ptr<peer_gate> gate_{};
        std::string peer_text_{};
        std::string session_text_{};
        std::uint64_t session_id_=0,local_nonce_=0,remote_nonce_=0,peer_id_=0;
        std::uint32_t last_settings_revision_=0;
        double last_probe_=-100.0, last_outgoing_=-100.0,last_incoming_=0;
        bool staged_=false,hosting_=false;
    };
}

#pragma once
// w0uFF Together 1v1: optional Steamworks P2P adapter.
// This reads the existing game's Steam API; it never calls SteamAPI_Init,
// SteamAPI_Shutdown or SteamAPI_RunCallbacks, which belong to the game.
// GPL-3.0; see NOTICE.md. The game's Steam runtime and private lobby must
// authenticate both Steam IDs before this transport is used.
//
// NOTE: ISteamNetworking (P2P packets) is deprecated by Valve. For this
// game's legacy Steamworks runtime it is a compatibility adapter, not a
// guarantee that the export or P2P session will be available. Probe first.

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

#include "duel_wire.hpp"

namespace wouff::duel
{
    // Isolated P2P channel; never reuse the game's own network channel.
    inline constexpr int steam_p2p_channel = 0x7107;
    inline constexpr int steam_reliable = 2;

    class steam_p2p_transport
    {
    public:
        struct incoming
        {
            packet data{};
            std::uint64_t authenticated_sender_id = 0;
        };

        bool attach() noexcept
        {
            detach();
            const auto module = ::GetModuleHandleW(L"steam_api64.dll");
            if (!module) return false;
            auto ptr = [&](const char* symbol) noexcept -> FARPROC
            {
                return ::GetProcAddress(module,symbol);
            };
            const auto get_network = reinterpret_cast<get_network_fn>(ptr("SteamAPI_SteamNetworking_v006"));
            get_user_fn get_user = nullptr;
            // The title's Steam runtime may expose a different SteamUser
            // interface revision. Only acquire a version that actually exists.
            for (const char* name : {"SteamAPI_SteamUser_v023",
                                     "SteamAPI_SteamUser_v022",
                                     "SteamAPI_SteamUser_v021",
                                     "SteamAPI_SteamUser_v020",
                                     "SteamAPI_SteamUser_v019"})
            {
                if ((get_user = reinterpret_cast<get_user_fn>(ptr(name))))
                    break;
            }
            send_packet_ = reinterpret_cast<send_fn>(ptr("SteamAPI_ISteamNetworking_SendP2PPacket"));
            can_read_ = reinterpret_cast<can_read_fn>(ptr("SteamAPI_ISteamNetworking_IsP2PPacketAvailable"));
            read_packet_ = reinterpret_cast<read_fn>(ptr("SteamAPI_ISteamNetworking_ReadP2PPacket"));
            close_peer_ = reinterpret_cast<close_fn>(ptr("SteamAPI_ISteamNetworking_CloseP2PSessionWithUser"));
            steam_id_ = reinterpret_cast<steam_id_fn>(ptr("SteamAPI_ISteamUser_GetSteamID"));
            if (!get_network || !get_user || !send_packet_ || !can_read_ || !read_packet_ ||
                !close_peer_ || !steam_id_)
            {
                detach();
                return false;
            }
            networking_ = get_network();
            user_ = get_user();
            if (!networking_ || !user_)
            {
                detach();
                return false;
            }
            const auto local = steam_id_(user_);
            if (!is_plausible_steam_id(local))
            {
                detach();
                return false;
            }
            local_id_ = local;
            attached_ = true;
            return true;
        }

        bool available() const noexcept { return attached_; }
        std::uint64_t local_steam_id() const noexcept { return local_id_; }

        // Send a reliable 48-byte versioned message to an ID that the host/
        // guest role code has *already* checked belongs to the game lobby.
        bool send(std::uint64_t peer_id, const packet& msg) const noexcept
        {
            if (!attached_ || !is_plausible_steam_id(peer_id) ||
                peer_id==local_id_ || !send_packet_) return false;
            if (!decode(encode(msg).data(),packet_size)) return false;
            const auto bytes=encode(msg);
            return send_packet_(networking_,peer_id,bytes.data(),
                                static_cast<std::uint32_t>(bytes.size()),
                                steam_reliable,steam_p2p_channel);
        }

        // Bounded receive: read only our reserved channel; preserve the
        // transport-authenticated Steam sender ID instead of trusting a
        // sender field asserted by the packet payload.
        std::optional<incoming> poll() const noexcept
        {
            if (!attached_ || !can_read_ || !read_packet_) return std::nullopt;
            std::uint32_t pending_size=0;
            if (!can_read_(networking_,&pending_size,steam_p2p_channel))
                return std::nullopt;
            if (pending_size>packet_size) return std::nullopt;
            std::array<std::uint8_t,packet_size> data{};
            std::uint32_t bytes_read=0;
            std::uint64_t source=0;
            if (!read_packet_(networking_,data.data(),
                              static_cast<std::uint32_t>(data.size()),
                              &bytes_read,&source,steam_p2p_channel) ||
                bytes_read!=packet_size ||
                !is_plausible_steam_id(source))
                return std::nullopt;
            const auto decoded=decode(data.data(),bytes_read);
            if (!decoded) return std::nullopt;
            return incoming{*decoded,source};
        }

        bool close(std::uint64_t peer_id) const noexcept
        {
            if (!attached_ || !is_plausible_steam_id(peer_id) ||
                !close_peer_) return false;
            return close_peer_(networking_,peer_id);
        }

        void detach() noexcept
        {
            // Never shut down Steamworks; game owns its lifecycle.
            attached_ = false;
            networking_ = nullptr; user_ = nullptr; local_id_ = 0;
            send_packet_ = nullptr; can_read_ = nullptr;
            read_packet_ = nullptr; close_peer_ = nullptr; steam_id_ = nullptr;
        }

    private:
        static bool is_plausible_steam_id(std::uint64_t id) noexcept
        {
            // Only a sanity check. The game lobby must establish true member/
            // host identity separately. This does not authenticate a player.
            return id >= 76561197960265728ULL &&
                   id <= 76561202255233023ULL;
        }

        using get_network_fn = void* (__cdecl*)();
        using get_user_fn = void* (__cdecl*)();
        using steam_id_fn = std::uint64_t (__cdecl*)(void*);
        using send_fn = bool (__cdecl*)(void*,std::uint64_t,const void*,std::uint32_t,int,int);
        using can_read_fn = bool (__cdecl*)(void*,std::uint32_t*,int);
        using read_fn = bool (__cdecl*)(void*,void*,std::uint32_t,std::uint32_t*,std::uint64_t*,int);
        using close_fn = bool (__cdecl*)(void*,std::uint64_t);

        void* networking_=nullptr;
        void* user_=nullptr;
        std::uint64_t local_id_=0;
        send_fn send_packet_=nullptr;
        can_read_fn can_read_=nullptr;
        read_fn read_packet_=nullptr;
        close_fn close_peer_=nullptr;
        steam_id_fn steam_id_=nullptr;
        bool attached_=false;
    };
}
#endif

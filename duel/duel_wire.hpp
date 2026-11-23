#pragma once
// w0uFF Together 1v1 network protocol: bounded, endian-stable messages.
// GPL-3.0; original project attribution in NOTICE.md.
// This is a message codec and verified-peer gate, NOT a Steamworks transport.
// The caller must supply authenticated Steam identities from a real session.

#include "duel_policy.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace wouff::duel
{
    inline constexpr std::size_t packet_size = 48;
    inline constexpr std::uint16_t wire_version = 1;

    enum class message_type : std::uint8_t
    {
        hello = 1,
        settings = 2,
        settings_ack = 3,
        start = 4,
        stop = 5,
    };

    inline constexpr std::uint8_t cheat_permission = 0x01;
    inline constexpr std::uint8_t two_players_ready = 0x02;
    inline constexpr std::uint8_t chain_detached = 0x04;
    inline constexpr std::uint8_t peers_synchronized = 0x08;
    inline constexpr std::uint8_t all_flags = 0x0f;

    struct packet
    {
        message_type type = message_type::hello;
        std::uint8_t flags = 0;
        std::uint64_t session_id = 0;
        std::uint64_t lobby_nonce = 0;
        std::uint64_t host_id = 0;
        std::uint64_t guest_id = 0;
        std::uint32_t revision = 0;
        std::uint32_t sequence = 0;
    };

    namespace detail
    {
        template<class Number>
        void put(std::array<std::uint8_t, packet_size>& out, std::size_t& pos, Number number) noexcept
        {
            for (std::size_t i=0; i<sizeof(Number); ++i)
                out[pos++] = static_cast<std::uint8_t>((number >> (i*8)) & 0xff);
        }
        template<class Number>
        Number get(const std::uint8_t* in, std::size_t& pos) noexcept
        {
            Number result=0;
            for (std::size_t i=0; i<sizeof(Number); ++i)
                result |= static_cast<Number>(static_cast<Number>(in[pos++]) << (i*8));
            return result;
        }
    }

    inline std::array<std::uint8_t, packet_size> encode(const packet& p) noexcept
    {
        std::array<std::uint8_t, packet_size> result{};
        std::size_t pos=0;
        result[pos++]='W'; result[pos++]='0'; result[pos++]='V'; result[pos++]='1';
        detail::put<std::uint16_t>(result,pos,wire_version);
        result[pos++]=static_cast<std::uint8_t>(p.type);
        result[pos++]=p.flags;
        detail::put<std::uint64_t>(result,pos,p.session_id);
        detail::put<std::uint64_t>(result,pos,p.lobby_nonce);
        detail::put<std::uint64_t>(result,pos,p.host_id);
        detail::put<std::uint64_t>(result,pos,p.guest_id);
        detail::put<std::uint32_t>(result,pos,p.revision);
        detail::put<std::uint32_t>(result,pos,p.sequence);
        return result;
    }

    inline std::optional<packet> decode(const std::uint8_t* data, std::size_t length) noexcept
    {
        if (!data || length != packet_size ||
            data[0]!='W' || data[1]!='0' || data[2]!='V' || data[3]!='1')
            return std::nullopt;
        std::size_t pos=4;
        if (detail::get<std::uint16_t>(data,pos) != wire_version) return std::nullopt;
        packet result;
        const auto kind = data[pos++];
        if (kind < static_cast<std::uint8_t>(message_type::hello) ||
            kind > static_cast<std::uint8_t>(message_type::stop))
            return std::nullopt;
        result.type = static_cast<message_type>(kind);
        result.flags=data[pos++];
        if (result.flags & static_cast<std::uint8_t>(~all_flags)) return std::nullopt;
        result.session_id=detail::get<std::uint64_t>(data,pos);
        result.lobby_nonce=detail::get<std::uint64_t>(data,pos);
        result.host_id=detail::get<std::uint64_t>(data,pos);
        result.guest_id=detail::get<std::uint64_t>(data,pos);
        result.revision=detail::get<std::uint32_t>(data,pos);
        result.sequence=detail::get<std::uint32_t>(data,pos);
        if (!result.session_id || !result.lobby_nonce ||
            !result.host_id || !result.guest_id ||
            result.host_id == result.guest_id || !result.sequence)
            return std::nullopt;
        if (result.type == message_type::hello && result.flags) return std::nullopt;
        if (result.type == message_type::settings &&
            (result.flags & ~cheat_permission)) return std::nullopt;
        if ((result.type==message_type::settings_ack ||
             result.type==message_type::stop) && result.flags)
            return std::nullopt;
        if (result.type==message_type::start &&
            (result.flags & ~(cheat_permission | two_players_ready |
                              chain_detached | peers_synchronized))) return std::nullopt;
        return result;
    }

    // Constructor takes server/lobby-confirmed peer identities, not sender
    // declarations inside packets. A Steamworks adapter must authenticate
    // the actual remote sender and compare it to its lobby's owner/members.
    class peer_gate
    {
    public:
        peer_gate(std::uint64_t session_id,
                  std::uint64_t host_id,
                  std::uint64_t guest_id,
                  bool local_is_host) noexcept :
            session_id_(session_id),
            host_id_(host_id),
            guest_id_(guest_id),
            local_is_host_(local_is_host)
        {
            if (!session_id || !host_id || !guest_id || host_id==guest_id)
                return;
            ready_ = local_is_host_ ? rules_.open_host_lobby(session_id)
                                   : rules_.open_guest_lobby(session_id);
        }

        bool initialized() const noexcept { return ready_; }
        bool accepted_hello() const noexcept { return nonce_!=0; }
        bool has_peer_ack() const noexcept { return ack_revision_!=0; }
        policy const& rules() const noexcept { return rules_; }

        // The host generates a unique, unpredictable nonzero nonce using a
        // real secure random source. Never use a constant shared nonce.
        bool host_bind_nonce(std::uint64_t nonce) noexcept
        {
            if (!ready_ || !local_is_host_ || nonce_ || !nonce) return false;
            nonce_=nonce;
            return true;
        }

        // Returns false unless authenticated_sender is the expected remote
        // peer identity supplied by Steamworks. Payload-supplied IDs alone
        // do not grant authority.
        bool accept(const packet& p, std::uint64_t authenticated_sender) noexcept
        {
            if (!ready_ || !p.session_id || p.session_id!=session_id_ ||
                p.host_id!=host_id_ || p.guest_id!=guest_id_ ||
                !authenticated_sender || p.sequence<=last_sequence_ ||
                (p.flags & static_cast<std::uint8_t>(~all_flags)))
                return false;
            if (authenticated_sender != (local_is_host_ ? guest_id_ : host_id_))
                return false;
            if (!local_is_host_ && p.type==message_type::hello)
            {
                if (nonce_ || p.lobby_nonce==0 || p.flags) return false;
                nonce_=p.lobby_nonce;
                last_sequence_=p.sequence;
                return true;
            }
            if (!nonce_ || p.lobby_nonce!=nonce_) return false;
            if (local_is_host_)
            {
                if (p.type != message_type::settings_ack ||
                    p.flags || p.revision != rules_.settings_for_host_broadcast().revision)
                    return false;
                ack_revision_ = p.revision;
                last_sequence_ = p.sequence;
                return true;
            }
            if (p.type==message_type::settings)
            {
                if (p.flags & ~cheat_permission) return false;
                const host_settings config{protocol_version, session_id_, p.revision,
                    static_cast<bool>(p.flags & cheat_permission)};
                if (!rules_.accept_host_settings(config)) return false;
            }
            else if (p.type==message_type::start)
            {
                if (!rules_.has_host_confirmation() ||
                    p.revision!=latest_settings_revision_ ||
                    (p.flags & cheat_permission) !=
                    (rules_.cheats_enabled_in_lobby() ? cheat_permission : 0) ||
                    !rules_.start_race(
                        static_cast<bool>(p.flags & two_players_ready),
                        static_cast<bool>(p.flags & chain_detached),
                        static_cast<bool>(p.flags & peers_synchronized)))
                    return false;
            }
            else if (p.type==message_type::stop)
            {
                if (p.flags) return false;
                rules_.reset();
                ready_=false;
            }
            else
                return false;
            if (p.type==message_type::settings)
                latest_settings_revision_=p.revision;
            last_sequence_=p.sequence;
            return true;
        }

        void disconnect() noexcept
        {
            // Disconnect is not evidence a race was safely unwound.
            // The host must independently restore authoritative chain state.
            ready_=false;
            nonce_=0;
            rules_.reset();
        }

    private:
        policy rules_{};
        std::uint64_t session_id_=0;
        std::uint64_t host_id_=0;
        std::uint64_t guest_id_=0;
        std::uint64_t nonce_=0;
        std::uint32_t last_sequence_=0;
        std::uint32_t latest_settings_revision_=0;
        std::uint32_t ack_revision_=0;
        bool local_is_host_=false;
        bool ready_=false;
    };
}

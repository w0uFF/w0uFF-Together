#pragma once
// w0uFF Together: fail-closed coordinator for future authoritative chain operations.
// Deliberately NO Unreal/SDK calls here. This must not be used as proof of actual
// replication or chain detachment; server receipts must come from verified hooks.
// GPL-3.0; retain original UnchainedTogether attribution in NOTICE.md.
#include <cstdint>

namespace wouff::duel
{
enum class chain_phase : std::uint8_t
{
    idle, detaching, detached, restoring, faulted
};

struct chain_receipt
{
    std::uint64_t session_id = 0;
    std::uint64_t server_operation_id = 0;
    bool server_authoritative = false;
    bool physical_constraints_verified = false;
    bool replicated_to_both_players = false;
};

class chain_coordinator
{
public:
    chain_phase phase() const noexcept { return phase_; }
    bool ready_for_race() const noexcept { return phase_ == chain_phase::detached; }
    bool requires_recovery() const noexcept
    {
        return phase_ != chain_phase::idle;
    }
    std::uint64_t session() const noexcept { return session_; }

    // A real game-thread server adapter must establish ALL these facts, not
    // a renderer checkbox or a self-declared Steam packet.
    bool begin_detach(std::uint64_t session, bool game_server_authority,
                      bool native_host_verified, bool exactly_two_members,
                      bool peer_identity_verified, bool rules_acknowledged) noexcept
    {
        if (phase_ != chain_phase::idle || !session ||
            !game_server_authority || !native_host_verified ||
            !exactly_two_members || !peer_identity_verified ||
            !rules_acknowledged)
            return false;
        session_ = session;
        operation_id_ = 0;
        phase_ = chain_phase::detaching;
        return true;
    }

    // Receipt must represent evidence from the authoritative server that
    // constraints were physically removed and replicated to both clients.
    bool confirm_detach(const chain_receipt& receipt) noexcept
    {
        if (phase_ != chain_phase::detaching ||
            !valid_receipt(receipt)) return false;
        operation_id_ = receipt.server_operation_id;
        phase_ = chain_phase::detached;
        return true;
    }

    // On disconnect/migration, this must be called by a verified server-side
    // teardown path. It does NOT silently restore anything by itself.
    bool begin_restore(bool game_server_authority) noexcept
    {
        if (!game_server_authority || phase_ == chain_phase::idle ||
            phase_ == chain_phase::restoring) return false;
        phase_ = chain_phase::restoring;
        return true;
    }

    bool confirm_restore(const chain_receipt& receipt) noexcept
    {
        if (phase_ != chain_phase::restoring ||
            !valid_receipt(receipt)) return false;
        reset_after_verified_restore();
        return true;
    }

    // Failure must remain visible: never mark an interrupted detach as idle
    // until the server has confirmed restoration of constraints.
    void mark_fault() noexcept
    {
        if (phase_ != chain_phase::idle) phase_ = chain_phase::faulted;
    }

private:
    bool valid_receipt(const chain_receipt& r) const noexcept
    {
        return session_ && r.session_id == session_ &&
               r.server_operation_id != 0 &&
               r.server_operation_id != operation_id_ &&
               r.server_authoritative &&
               r.physical_constraints_verified &&
               r.replicated_to_both_players;
    }
    void reset_after_verified_restore() noexcept
    {
        phase_ = chain_phase::idle;
        session_ = 0;
        operation_id_ = 0;
    }
    chain_phase phase_ = chain_phase::idle;
    std::uint64_t session_ = 0;
    std::uint64_t operation_id_ = 0;
};
} // namespace wouff::duel

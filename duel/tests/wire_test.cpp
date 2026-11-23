#include "../duel_wire.hpp"
#include <array>
#include <cassert>
#include <cstdint>

using namespace wouff::duel;
static constexpr std::uint64_t lobby=0xfedcba0012345678ULL;
static constexpr std::uint64_t host=76561198000000001ULL;
static constexpr std::uint64_t guest=76561198000000002ULL;
static constexpr std::uint64_t nonce=0x1001119992223334ULL;

int main()
{
    const packet hello{message_type::hello,0,lobby,nonce,host,guest,0,1};
    const auto bytes=encode(hello);
    assert(bytes.size()==packet_size);
    assert(bytes[0]=='W' && bytes[1]=='0' && bytes[2]=='V' && bytes[3]=='1');
    const auto read=decode(bytes.data(),bytes.size());
    assert(read && read->type==message_type::hello &&
           read->lobby_nonce==nonce && read->host_id==host &&
           read->guest_id==guest && read->session_id==lobby);
    assert(!decode(bytes.data(),bytes.size()-1));
    assert(!decode(nullptr,bytes.size()));
    auto corrupt=bytes;
    corrupt[0]='x'; assert(!decode(corrupt.data(),corrupt.size()));
    corrupt=bytes;
    corrupt[4]=0xff; assert(!decode(corrupt.data(),corrupt.size())); // Bad version
    corrupt=bytes;
    corrupt[6]=99; assert(!decode(corrupt.data(),corrupt.size())); // Bad kind
    corrupt=bytes;
    corrupt[7]=0x10; assert(!decode(corrupt.data(),corrupt.size())); // Reserved flags
    corrupt=bytes;
    corrupt[8]=0; corrupt[9]=0; corrupt[10]=0; corrupt[11]=0;
    corrupt[12]=0; corrupt[13]=0; corrupt[14]=0; corrupt[15]=0;
    assert(!decode(corrupt.data(),corrupt.size())); // Empty session
    corrupt=bytes;
    corrupt[44]=0; corrupt[45]=0; corrupt[46]=0; corrupt[47]=0;
    assert(!decode(corrupt.data(),corrupt.size())); // Sequence zero
    assert(!peer_gate(0,host,guest,false).initialized());
    assert(!peer_gate(lobby,host,host,true).initialized());

    peer_gate guest_rule(lobby,host,guest,false);
    peer_gate host_rule(lobby,host,guest,true);
    assert(guest_rule.initialized() && host_rule.initialized());
    assert(!guest_rule.rules().allow_mod_cheats());
    assert(!guest_rule.accept(hello,guest)); // Sender must be real host
    assert(!guest_rule.accept(hello,0)); // Unauthenticated sender
    assert(!guest_rule.accept(packet{message_type::hello,0,lobby+1,nonce,host,guest,0,1},host));
    assert(guest_rule.accept(hello,host));
    assert(guest_rule.accepted_hello());
    assert(!guest_rule.accept(hello,host)); // Replay forbidden
    assert(host_rule.host_bind_nonce(nonce));
    assert(!host_rule.host_bind_nonce(nonce)); // Host nonce cannot change
    assert(!guest_rule.rules().allow_mod_cheats());
    const packet settings{message_type::settings,0,lobby,nonce,host,guest,0,2};
    assert(!guest_rule.accept(settings,guest)); // Fake host must be rejected
    assert(guest_rule.accept(settings,host));
    assert(guest_rule.rules().has_host_confirmation());
    assert(!guest_rule.rules().allow_mod_cheats());
    assert(!guest_rule.accept(settings,host)); // Duplicate seq
    assert(!guest_rule.accept(packet{message_type::settings,cheat_permission,lobby,nonce,host,guest,0,3},host)); // Equal revision cannot change policy
    const packet missing_peer{message_type::start,chain_detached|peers_synchronized,lobby,nonce,host,guest,0,4};
    assert(!guest_rule.accept(missing_peer,host));
    const packet missing_chain{message_type::start,two_players_ready|peers_synchronized,lobby,nonce,host,guest,0,4};
    assert(!guest_rule.accept(missing_chain,host));
    const packet valid_start{message_type::start,two_players_ready|chain_detached|peers_synchronized,lobby,nonce,host,guest,0,4};
    assert(guest_rule.accept(valid_start,host));
    assert(guest_rule.rules().current_stage()==stage::racing);
    assert(!guest_rule.rules().allow_mod_cheats());
    assert(!guest_rule.accept(valid_start,host));
    const packet stop{message_type::stop,0,lobby,nonce,host,guest,0,5};
    assert(!guest_rule.accept(stop,guest));
    assert(guest_rule.accept(stop,host));
    assert(!guest_rule.initialized());

    // Host confirms guest ACK only if Steam authenticated sender matches
    // the pre-verified lobby member and revision.
    assert(!host_rule.has_peer_ack());
    const packet bad_ack{message_type::settings_ack,0,lobby,nonce,host,guest,5,1};
    assert(!host_rule.accept(bad_ack,guest));
    const packet ack{message_type::settings_ack,0,lobby,nonce,host,guest,0,1};
    assert(!host_rule.accept(ack,host));
    assert(host_rule.accept(ack,guest));
    assert(host_rule.has_peer_ack());
    host_rule.disconnect();
    assert(!host_rule.initialized());

    // Versioned permission test: only trusted host config can enable cheats.
    peer_gate allowed(lobby,host,guest,false);
    assert(allowed.accept(hello,host));
    const packet permit{message_type::settings,cheat_permission,lobby,nonce,host,guest,1,2};
    assert(allowed.accept(permit,host));
    assert(allowed.rules().allow_mod_cheats());
    assert(!allowed.accept(packet{message_type::settings,0,lobby,nonce,host,guest,0,3},host));
    assert(allowed.rules().allow_mod_cheats());
    const packet forbidden_replay{message_type::settings,0,lobby,nonce,host,guest,1,3};
    assert(!allowed.accept(forbidden_replay,host));
    assert(allowed.rules().allow_mod_cheats());
    const packet revoke{message_type::settings,0,lobby,nonce,host,guest,2,4};
    assert(allowed.accept(revoke,host));
    assert(!allowed.rules().allow_mod_cheats());
    assert(!allowed.accept(packet{message_type::start,cheat_permission|two_players_ready|chain_detached|peers_synchronized,lobby,nonce,host,guest,2,5},host));
    assert(allowed.accept(packet{message_type::start,two_players_ready|chain_detached|peers_synchronized,lobby,nonce,host,guest,2,5},host));
    assert(!allowed.rules().allow_mod_cheats());
}

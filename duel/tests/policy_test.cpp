#include "../duel_policy.hpp"
#include <cassert>
using wouff::duel::policy;
using wouff::duel::stage;
using wouff::duel::host_settings;
int main()
{
    policy guest;
    assert(guest.allow_mod_cheats()); // Vanilla behavior preserved
    assert(guest.open_guest_lobby(101));
    assert(!guest.allow_mod_cheats()); // Unknown host rules => deny
    assert(!guest.host_set_cheats(true)); // Guest must not mutate host rules
    assert(!guest.start_race(true,true,true)); // Needs authenticated host config
    assert(!guest.accept_host_settings({999,101,1,true})); // Version mismatch
    assert(!guest.accept_host_settings({1,102,1,true})); // Session mismatch
    assert(!guest.allow_mod_cheats());
    assert(guest.accept_host_settings({1,101,3,false}));
    assert(!guest.allow_mod_cheats());
    assert(!guest.accept_host_settings({1,101,2,true})); // No revision downgrade
    assert(!guest.start_race(false,true,true));
    assert(!guest.start_race(true,false,true));
    assert(!guest.start_race(true,true,false));
    assert(guest.start_race(true,true,true));
    assert(guest.current_stage()==stage::racing);
    assert(!guest.allow_mod_cheats());
    guest.finish();
    assert(guest.current_stage()==stage::finished);
    guest.reset();
    assert(guest.allow_mod_cheats());

    policy host;
    assert(!host.open_host_lobby(0));
    assert(host.open_host_lobby(202));
    assert(!host.allow_mod_cheats());
    assert(host.host_set_cheats(true));
    const auto config = host.settings_for_host_broadcast();
    assert(config.version==1 && config.session_id==202 && config.cheats_enabled && config.revision==1);
    assert(!host.start_race(true,false,true)); // Cannot fake detached chain
    assert(host.start_race(true,true,true));
    assert(host.allow_mod_cheats());
    assert(!host.host_set_cheats(false)); // Settings fixed once race starts
    host.reset();
    assert(host.current_stage()==stage::vanilla && host.allow_mod_cheats());

    policy peer;
    assert(peer.open_guest_lobby(202));
    assert(peer.accept_host_settings(config));
    assert(peer.allow_mod_cheats()); // Only with host-authenticated data
    return 0;
}

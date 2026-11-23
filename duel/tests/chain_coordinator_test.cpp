#include "../chain_coordinator.hpp"
#include <cassert>
using namespace wouff::duel;
int main()
{
    chain_coordinator c;
    assert(c.phase()==chain_phase::idle && !c.ready_for_race());
    assert(!c.begin_detach(0,true,true,true,true,true));
    assert(!c.begin_detach(11,false,true,true,true,true));
    assert(!c.begin_detach(11,true,false,true,true,true));
    assert(!c.begin_detach(11,true,true,false,true,true));
    assert(!c.begin_detach(11,true,true,true,false,true));
    assert(!c.begin_detach(11,true,true,true,true,false));
    assert(c.begin_detach(11,true,true,true,true,true));
    assert(!c.ready_for_race() && c.requires_recovery());
    assert(!c.begin_detach(12,true,true,true,true,true));
    assert(!c.confirm_detach({11,1,true,true,false}));
    assert(!c.confirm_detach({12,1,true,true,true}));
    assert(!c.confirm_detach({11,0,true,true,true}));
    assert(!c.confirm_detach({11,1,false,true,true}));
    assert(c.confirm_detach({11,1,true,true,true}));
    assert(c.ready_for_race());
    assert(!c.confirm_restore({11,2,true,true,true}));
    assert(!c.begin_restore(false));
    assert(c.begin_restore(true));
    assert(!c.ready_for_race() && c.requires_recovery());
    assert(!c.confirm_restore({11,1,true,true,true}));
    assert(!c.confirm_restore({11,2,true,false,true}));
    c.mark_fault();
    assert(c.phase()==chain_phase::faulted && c.requires_recovery());
    assert(c.begin_restore(true));
    assert(c.confirm_restore({11,2,true,true,true}));
    assert(!c.requires_recovery());
    assert(c.begin_detach(12,true,true,true,true,true));
    assert(!c.ready_for_race());
    return 0;
}

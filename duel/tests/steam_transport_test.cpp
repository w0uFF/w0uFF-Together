#include "../steam_p2p_transport.hpp"
#include <cassert>
#include <cstdint>
int main()
{
#ifdef _WIN32
    wouff::duel::steam_p2p_transport net;
    assert(!net.available());
    assert(!net.local_steam_id());
    const wouff::duel::packet p{
        wouff::duel::message_type::hello,0,
        4444,5555,76561198000000001ULL,76561198000000002ULL,0,1};
    assert(!net.send(76561198000000002ULL,p));
    assert(!net.poll());
    assert(!net.close(76561198000000002ULL));
    // DLL availability is a game-runtime test, not a prerequisite to compiling.
    net.detach();
    assert(!net.available());
#endif
    return 0;
}

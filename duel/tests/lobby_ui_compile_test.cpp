#include "../duel_lobby_controller.hpp"
void compile_only_ui_probe()
{
    wouff::duel::lobby_controller panel;
    panel.tick(false,false,0);
    panel.draw(false,false,0);
    panel.leave();
}

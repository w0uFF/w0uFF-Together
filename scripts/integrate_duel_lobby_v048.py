#!/usr/bin/env python3
"""Integrate the experimental 1v1 Steam P2P LOBBY handshake into v0.4.7.

This intentionally does not enable racing, alter chains, modify native lobby
UI assets, or install a gameplay RPC hook. The host-only settings and guest
handshake are ready for private multiplayer verification first.
"""
from pathlib import Path
from shutil import copyfile

root=Path("w0uFF-Together")
gui=root/"src/gui/gui.cpp"
content=gui.read_text(encoding="utf-8")
feature_dir=root/"src/features"
feature_dir.mkdir(parents=True,exist_ok=True)
for name in ("duel_policy.hpp","duel_wire.hpp","steam_p2p_transport.hpp","duel_lobby_controller.hpp","solo_race_panel.hpp"):
    copyfile(Path("duel")/name,feature_dir/name)

def patch_one(old,new,label):
    global content
    if content.count(old)!=1:
        raise AssertionError("%s: expected one anchor, got %d"%(label,content.count(old)))
    content=content.replace(old,new,1)

patch_one('#include "features/teleport_mailbox.hpp"',
          '#include "features/teleport_mailbox.hpp"\n#include "features/duel_lobby_controller.hpp"',
          "DUEL_HEADER")

patch_one('        bool level_valid             = false;',
          '        bool level_valid             = false;\n'
          '        bool duel_game_instance_valid = false;\n'
          '        bool duel_hosting_online_session = false;\n'
          '        int duel_players_in_session = 0;\n'
          '        int duel_gamestate_players = 0;\n'
          '        int duel_gi_players_in_session = 0;',
          "POD_HOST_SNAPSHOT")

patch_one('    static std::atomic_uint64_t g_resolver_faults{0};',
          '    static std::atomic_uint64_t g_resolver_faults{0};\n'
          '    static std::atomic_uint64_t g_duel_gi_read_faults{0};\n'
          '    static std::atomic_bool g_duel_staged{false};\n'
          '    static std::atomic_bool g_duel_cheats_allowed{true};\n'
          '    static bool mod_cheats_allowed() noexcept\n'
          '    {\n'
          '        return !g_duel_staged.load(std::memory_order_acquire) ||\n'
          '               g_duel_cheats_allowed.load(std::memory_order_acquire);\n'
          '    }',
          "ATOMIC_CHEAT_RULES")

patch_one('static void snapshot_body()',
          """// Isolate SEH into a POD-only helper; snapshot_body itself constructs
// C++ objects and MSVC refuses __try in functions requiring unwinding.
static void snapshot_duel_game_instance(SDK::UWorld* world) noexcept
{
    g_snap.duel_game_instance_valid = false;
    g_snap.duel_hosting_online_session = false;
    g_snap.duel_players_in_session = 0;
    g_snap.duel_gamestate_players = 0;
    g_snap.duel_gi_players_in_session = 0;
    __try
    {
        auto* gi = world ? world->OwningGameInstance : nullptr;
        if (gi && gi->IsA(SDK::UCustomGI_C::StaticClass()))
        {
            auto* game_gi = reinterpret_cast<SDK::UCustomGI_C*>(gi);
            const int players = game_gi->PlayersInSession;
            if (players >= 0 && players <= 16)
            {
                g_snap.duel_game_instance_valid = true;
                g_snap.duel_hosting_online_session = game_gi->HostingOnlineSession;
                g_snap.duel_players_in_session = players;
                g_snap.duel_gi_players_in_session = players;
            }
        }
        auto* game_state = world ? world->GameState : nullptr;
        if (game_state && game_state->IsA(SDK::AGameStateBase::StaticClass()))
        {
            const int replicated_players = game_state->PlayerArray.Num();
            if (replicated_players >= 0 && replicated_players <= 16)
            {
                g_snap.duel_gamestate_players = replicated_players;
                // GameInstance can lag behind the replicated lobby roster.
                // This is only for alpha staging; authoritative gameplay
                // still requires verified Steam members and server state.
                if (replicated_players == 2 &&
                    g_snap.duel_players_in_session == 1)
                    g_snap.duel_players_in_session = 2;
            }
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        g_duel_gi_read_faults.fetch_add(1, std::memory_order_relaxed);
    }
}

static void snapshot_body()""",
          "ISOLATE_SEH_HELPER")

patch_one('        g_snap.pawn_valid = false;\n\n        if (Level)',
          '        g_snap.pawn_valid = false;\n'
          '        snapshot_duel_game_instance(World);\n\n'
          '        if (Level)',
          "GAME_THREAD_LOBBY_GI")

patch_one('if (const auto pending = g_teleports.consume();\n                pending && g_pawn && wouff::feature_registry::enabled(wouff::feature_id::teleport))',
          'if (const auto pending = g_teleports.consume();\n'
          '                pending && g_pawn && mod_cheats_allowed() &&\n'
          '                wouff::feature_registry::enabled(wouff::feature_id::teleport))',
          "TELEPORT_CONSUMER_GATE")


patch_one('// ---- 写请求：自由视角 ----\n\t\t\tif (g_freecam_request.exchange(false))',
          '// ---- 写请求：自由视角 ----\n'
          '\t\t\tstatic bool s_duel_freecam_on = false;\n'
          '\t\t\tif (!mod_cheats_allowed() && s_duel_freecam_on &&\n'
          '\t\t\t    g_player_controller && g_player_controller->CheatManager)\n'
          '\t\t\t{\n'
          '\t\t\t    g_player_controller->CheatManager->ToggleDebugCamera();\n'
          '\t\t\t    s_duel_freecam_on = false;\n'
          '\t\t\t}\n'
          '\t\t\tif (g_freecam_request.exchange(false))',
          "DISABLE_PREEXISTING_FREECAM")
patch_one('g_player_controller->CheatManager->ToggleDebugCamera();\n\t\t\t\t}',
          'g_player_controller->CheatManager->ToggleDebugCamera();\n'
          '\t\t\t\t    s_duel_freecam_on = !s_duel_freecam_on;\n'
          '\t\t\t\t}',
          "TRACK_MOD_FREECAM")
patch_one('if (g_player_controller && wouff::feature_registry::enabled(wouff::feature_id::free_camera))',
          'if (g_player_controller && mod_cheats_allowed() &&\n'
          '                    wouff::feature_registry::enabled(wouff::feature_id::free_camera))',
          "FREECAM_GATE")

patch_one('const bool effective_fly = wouff::feature_registry::enabled(wouff::feature_id::fly_mode) && g_wouff_fly_mode_state;',
          'if (!mod_cheats_allowed()) g_wouff_fly_mode_state.store(false, std::memory_order_release);\n'
          '            const bool effective_fly = mod_cheats_allowed() &&\n'
          '                wouff::feature_registry::enabled(wouff::feature_id::fly_mode) &&\n'
          '                g_wouff_fly_mode_state.load(std::memory_order_acquire);',
          "FLY_GAME_THREAD_GATE")

patch_one('static bool s_snap_valid = false;',
          '    static bool s_snap_valid = false;\n'
          '    static wouff::duel::lobby_controller s_duel_lobby;\n'
          '    static bool s_duel_was_staged = false;',
          "RENDER_DUEL_STATE")

patch_one('        // Named cut presets: reliable only after the player captures coordinates.',
          """        // Host/guest networking runs on renderer only; Unreal session info
        // comes from the game-thread POD snapshot.
        s_duel_lobby.tick(s_snap.duel_game_instance_valid,\n            s_snap.duel_hosting_online_session, s_snap.duel_players_in_session);
        const bool staged_now = s_duel_lobby.staged();
        if (staged_now && !s_duel_was_staged)
        {
            // Force all pre-existing Fly/requested cheat actions OFF on entry.
            g_wouff_fly_mode_state.store(false, std::memory_order_release);
            g_freecam_request.store(false, std::memory_order_release);
            g_repeat_checkpoint_request.store(false, std::memory_order_release);
        }
        s_duel_was_staged = staged_now;
        g_duel_cheats_allowed.store(s_duel_lobby.mod_cheats_allowed(), std::memory_order_release);
        g_duel_staged.store(staged_now, std::memory_order_release);

        // Named cut presets: reliable only after the player captures coordinates.""",
          "RENDER_NETWORK_TICK")

patch_one('if (ImGui::Selectable("04  Settings", selected_tab==3)) selected_tab=3;',
          'if (ImGui::Selectable("04  Settings", selected_tab==3)) selected_tab=3;\n'
          '                    if (ImGui::Selectable("05  1v1 Lobby", selected_tab==4)) selected_tab=4;',
          "DUEL_NAV")

patch_one('                    if (selected_tab==3)\n                    {',
          """                    if (selected_tab==4)
                    {
                        s_duel_lobby.draw(s_snap.duel_game_instance_valid,
                            s_snap.duel_hosting_online_session, s_snap.duel_players_in_session);
                        // Update the cross-thread cheat permission immediately
                        // following any host checkbox or open/leave transition.
                        g_duel_cheats_allowed.store(s_duel_lobby.mod_cheats_allowed(), std::memory_order_release);
                        g_duel_staged.store(s_duel_lobby.staged(), std::memory_order_release);
                    }
                    else if (selected_tab==3)
                    {""",
          "DUEL_UI_TAB")

patch_one('                        ImGui::Text("GWorld read exceptions: %llu",',
          '                        ImGui::Text("1v1 lobby GI: %s, hosting: %s, players: %d",\n'
          '                            s_snap.duel_game_instance_valid ? "YES" : "NO",\n'
          '                            s_snap.duel_hosting_online_session ? "YES" : "NO",\n'
          '                            s_snap.duel_players_in_session);\n'
          '                        ImGui::Text("1v1 raw GI count: %d, GameState PlayerStates: %d",\n'
          '                            s_snap.duel_gi_players_in_session, s_snap.duel_gamestate_players);\n'
          '                        ImGui::Text("1v1 lobby GI read exceptions: %llu",\n'
          '                            static_cast<unsigned long long>(g_duel_gi_read_faults.load(std::memory_order_relaxed)));\n'
          '                        ImGui::Text("GWorld read exceptions: %llu",',
          "LOBBY_DIAG")

patch_one('        if (g_repeat_checkpoint_request.exchange(false) &&\n            wouff::feature_registry::enabled(wouff::feature_id::teleport)',
          '        if (g_repeat_checkpoint_request.exchange(false) &&\n'
          '            mod_cheats_allowed() &&\n'
          '            wouff::feature_registry::enabled(wouff::feature_id::teleport)',
          "REPEAT_CHECKPOINT_GATE")

patch_one('        if (msg == WM_KEYUP && wparam != 0 &&\n            wparam == g_repeat_checkpoint_hotkey.get_vk_value() &&',
          '        if (mod_cheats_allowed() && msg == WM_KEYUP && wparam != 0 &&\n'
          '            wparam == g_repeat_checkpoint_hotkey.get_vk_value() &&',
          "REPEAT_HOTKEY_GATE")

patch_one('if (wouff::feature_registry::enabled(wouff::feature_id::teleport) && msg == WM_KEYUP && wparam == g_wouff_tp_to_latest_saved_position.get_vk_value()',
          'if (mod_cheats_allowed() && wouff::feature_registry::enabled(wouff::feature_id::teleport) && msg == WM_KEYUP && wparam == g_wouff_tp_to_latest_saved_position.get_vk_value()',
          "TELEPORT_HOTKEY_GATE")

patch_one('if (wouff::feature_registry::enabled(wouff::feature_id::fly_mode) && msg == WM_KEYUP && wparam == g_wouff_fly_mode.get_vk_value())',
          'if (mod_cheats_allowed() && wouff::feature_registry::enabled(wouff::feature_id::fly_mode) && msg == WM_KEYUP && wparam == g_wouff_fly_mode.get_vk_value())',
          "FLY_HOTKEY_GATE")

patch_one('if (wouff::feature_registry::enabled(wouff::feature_id::free_camera) && msg == WM_KEYUP && wparam == g_wouff_freecam_hotkey.get_vk_value())',
          'if (mod_cheats_allowed() && wouff::feature_registry::enabled(wouff::feature_id::free_camera) && msg == WM_KEYUP && wparam == g_wouff_freecam_hotkey.get_vk_value())',
          "FREECAM_HOTKEY_GATE")

# Avoid pretending a controller-only handshake removes actual chains.
assert "RemoveCharacterConstraints" not in content
assert "Start 1v1 - unavailable until server/chain verification" in (feature_dir/"duel_lobby_controller.hpp").read_text()
assert "g_duel_staged.store" in content
assert "mod_cheats_allowed() &&" in content
gui.write_text(content,encoding="utf-8")
print("v0.4.8: 1v1 Steam host/guest staging, GInstance-only metadata and game-thread cheat path gates; chain/race NOT activated")

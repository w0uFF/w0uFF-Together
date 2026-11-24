#!/usr/bin/env python3
"""v0.4.9 Fly controls + stable teleport + separate custom cut CRUD."""
from pathlib import Path
p=Path("w0uFF-Together/src/gui/gui.cpp")
s=p.read_text(encoding="utf-8")
def patch(old,new,name):
    global s
    n=s.count(old)
    if n!=1:raise AssertionError(f"{name}: expected 1 anchor, got {n}")
    s=s.replace(old,new,1)

patch('static hotkey g_wouff_fly_mode("wouff_fly_mode", VK_F7);',
'''static hotkey g_wouff_fly_mode("wouff_fly_mode", VK_F7);
    // UI/input values are published atomically; only the game thread writes UE movement.
    static std::atomic<float> g_fly_speed{600.f};
    static std::atomic<bool> g_fly_scroll_adjust{true};''',"FLY_SETTINGS")
patch('                SetPlayerPosition(g_pawn, target);',
'''                // Do not retain pre-teleport momentum across repeated cut practice.
                // Only the local resolved character is touched, on the game thread.
                if (g_pawn->CharacterMovement)
                {
                    g_pawn->CharacterMovement->StopMovementImmediately();
                    g_pawn->CharacterMovement->PendingLaunchVelocity = SDK::FVector{0,0,0};
                }
                SetPlayerPosition(g_pawn, target);
                if (g_pawn->CharacterMovement)
                    g_pawn->CharacterMovement->StopMovementImmediately();''',"TELEPORT_ZERO_VELOCITY")
patch('''				g_pawn->SetFlyMode(effective_fly);
			}''',
'''				g_pawn->SetFlyMode(effective_fly);
			}
            // Restore movement tuning when Fly is disabled or pawn changes.
            static SDK::ABP_Character_C* s_fly_speed_pawn = nullptr;
            static float s_original_fly_speed = 0.f;
            static bool s_fly_speed_changed = false;
            if (s_fly_speed_pawn && s_fly_speed_changed &&
                (s_fly_speed_pawn != g_pawn || !effective_fly))
            {
                // Never dereference an old pawn after level transition.
                if (s_fly_speed_pawn == g_pawn && g_pawn && g_pawn->CharacterMovement)
                    g_pawn->CharacterMovement->MaxFlySpeed = s_original_fly_speed;
                s_fly_speed_pawn = nullptr;
                s_fly_speed_changed = false;
            }
            if (effective_fly && g_pawn && g_pawn->CharacterMovement)
            {
                auto* movement = g_pawn->CharacterMovement;
                if (!s_fly_speed_changed || s_fly_speed_pawn != g_pawn)
                {
                    s_original_fly_speed = movement->MaxFlySpeed;
                    s_fly_speed_pawn = g_pawn;
                    s_fly_speed_changed = true;
                }
                movement->MaxFlySpeed = g_fly_speed.load(std::memory_order_relaxed);
                // World Z, never camera-relative. Only while Fly is enabled.
                const bool up = (::GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;
                const bool down = (::GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
                if (up != down)
                    g_pawn->AddMovementInput(SDK::FVector{0,0,1}, up ? 1.f : -1.f, true);
            }''',"FLY_GAME_THREAD")
patch('''                            ImGui::Hotkey("Fly hotkey", g_wouff_fly_mode);''',
'''                            ImGui::Hotkey("Fly hotkey", g_wouff_fly_mode);
                            float speed = g_fly_speed.load(std::memory_order_relaxed);
                            if (ImGui::SliderFloat("Fly speed (cm/s)", &speed, 100.f, 3000.f, "%.0f"))
                                g_fly_speed.store(speed, std::memory_order_relaxed);
                            bool scroll = g_fly_scroll_adjust.load(std::memory_order_relaxed);
                            if (ImGui::Checkbox("Mouse wheel changes Fly speed", &scroll))
                                g_fly_scroll_adjust.store(scroll, std::memory_order_relaxed);
                            ImGui::TextDisabled("Space = world up | Ctrl = world down | F7 = toggle Fly");
                            ImGui::TextDisabled("Mouse wheel adjusts speed during Fly with the menu closed.");''',"FLY_UI")
patch('''		if (mod_cheats_allowed() && wouff::feature_registry::enabled(wouff::feature_id::fly_mode) && msg == WM_KEYUP && wparam == g_wouff_fly_mode.get_vk_value())''',
'''        if (msg == WM_MOUSEWHEEL && !m_is_open &&
            mod_cheats_allowed() &&
            wouff::feature_registry::enabled(wouff::feature_id::fly_mode) &&
            g_wouff_fly_mode_state.load(std::memory_order_acquire) &&
            g_fly_scroll_adjust.load(std::memory_order_relaxed))
        {
            const float delta = static_cast<float>(GET_WHEEL_DELTA_WPARAM(wparam)) / WHEEL_DELTA;
            const float speed = g_fly_speed.load(std::memory_order_relaxed);
            g_fly_speed.store(std::clamp(speed + delta * 100.f, 100.f, 3000.f),
                std::memory_order_relaxed);
            return;
        }
		if (mod_cheats_allowed() && wouff::feature_registry::enabled(wouff::feature_id::fly_mode) && msg == WM_KEYUP && wparam == g_wouff_fly_mode.get_vk_value())''',"FLY_WHEEL")
# Keep fixed known cuts untouched; custom checkpoints persist separately.
patch('#include "features/duel_lobby_controller.hpp"',
      '#include "features/duel_lobby_controller.hpp"\n#include "features/custom_cut_panel.hpp"',
      "CUSTOM_CUT_HEADER")
patch('''                            if (ImGui::CollapsingHeader("w0uFF Practice — Known Cuts", ImGuiTreeNodeFlags_DefaultOpen))''',
'''                            static wouff::practice::custom_cut_panel s_custom_cuts;
                            static auto s_custom_cut_path =
                                g_file_manager.get_project_file("./practice_custom_cuts.json").get_path();
                            s_custom_cuts.draw(s_custom_cut_path, current_pos.X, current_pos.Y, current_pos.Z,
                                s_snap_valid && s_snap.pawn_valid,
                                [](double x, double y, double z)
                                {
                                    if (mod_cheats_allowed())
                                        request_teleport(SDK::FVector{x,y,z});
                                });
                            if (ImGui::CollapsingHeader("w0uFF Practice — Known Cuts", ImGuiTreeNodeFlags_DefaultOpen))''',
"CUSTOM_CUT_UI")
p.write_text(s,encoding="utf-8")
print("Fly: adjustable speed, wheel, Space/Ctrl world Z; teleport clears local velocity")

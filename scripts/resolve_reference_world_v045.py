#!/usr/bin/env python3
"""Use the exact working reference's game-world pointer; preserve safe fallback.

This is intentionally narrow: no changes to Fly mechanics, player ownership,
movement, jump, checkpoints, other features or multiplayer behavior.
"""
from pathlib import Path

path = Path("w0uFF-Together/src/gui/gui.cpp")
source = path.read_text(encoding="utf-8")
marker = "static void snapshot_body()"
assert source.count(marker) == 1, "snapshot body moved"

impl = r"""
    // Game version 1.11.2: verified in the user's working 2026-11-09
    // x64 d3d12.dll. It reads GWorld from GetModuleHandleA(0)+0x6FD1448.
    // Its subsequent world->OwningGameInstance and local-player offsets
    // match our generated SDK. Use this reference only while the expected
    // module layout is present; never select an arbitrary network pawn.
    static std::atomic_int g_world_acquisition_path{0}; // 1=reference 2=SDK
    static std::atomic_uint64_t g_reference_world_faults{0};

    static SDK::UWorld* resolve_reference_world()
    {
        SDK::UWorld* candidate = nullptr;
        __try
        {
            auto* image_base = reinterpret_cast<unsigned char*>(::GetModuleHandleW(nullptr));
            if (image_base)
            {
                candidate = *reinterpret_cast<SDK::UWorld**>(image_base + 0x6FD1448);
                // The reference checks World+0x30, World+0x1B8. These fields
                // are PersistentLevel and OwningGameInstance in our SDK.
                if (candidate && candidate->PersistentLevel && candidate->OwningGameInstance)
                {
                    g_world_acquisition_path.store(1, std::memory_order_relaxed);
                    return candidate;
                }
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            g_reference_world_faults.fetch_add(1, std::memory_order_relaxed);
        }

        g_world_acquisition_path.store(2, std::memory_order_relaxed);
        return SDK::UWorld::GetWorld();
    }

"""
source = source.replace(marker, impl + marker)
old = "SDK::UWorld* World = SDK::UWorld::GetWorld();"
assert source.count(old) == 1, "world lookup changed unexpectedly"
source = source.replace(old, "SDK::UWorld* World = resolve_reference_world();", 1)

ui_anchor = 'ImGui::Text("World / persistent level: %s / %s", s_snap.world_valid ? "FOUND" : "NOT FOUND", s_snap.level_valid ? "FOUND" : "NOT FOUND");'
assert source.count(ui_anchor) == 1, "runtime diagnostic screen changed"
ui_extra = r"""
                        ImGui::Text("World source: %s", g_world_acquisition_path.load(std::memory_order_relaxed) == 1
                            ? "Working DLL GWorld+0x6FD1448" : "SDK fallback");
                        ImGui::Text("GWorld read exceptions: %llu",
                            static_cast<unsigned long long>(g_reference_world_faults.load(std::memory_order_relaxed)));
"""
source = source.replace(ui_anchor, ui_anchor + ui_extra, 1)

assert source.count("g_world_acquisition_path.store(1") == 1
assert source.count("resolve_reference_world();") == 1
assert "GetModuleHandleW(nullptr)" in source
assert "0x6FD1448" in source
path.write_text(source, encoding="utf-8")
print("v0.4.5: reference-GWorld preferred, local-only pawn chain preserved, fallback retained.")

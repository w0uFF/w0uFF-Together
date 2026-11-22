#!/usr/bin/env python3
"""Instrument only the v0.4.3 local-player resolver, fail closed on source drift."""
from pathlib import Path

path = Path("w0uFF-Together/src/gui/gui.cpp")
text = path.read_text(encoding="utf-8")
anchor = "    static std::atomic_uint64_t g_resolver_faults{0};"
assert text.count(anchor) == 1, "resolver fault counter anchor changed"
extra = """
    // v0.4.4 diagnostics: separate protected resolver calls. No actor selection changes.
    static std::atomic_uint64_t g_controller_route_faults[3]{};
    static std::atomic_uint64_t g_pawn_route_faults[5]{};
    static std::atomic_ulong g_controller_route_codes[3]{};
    static std::atomic_ulong g_pawn_route_codes[5]{};

    static void record_controller_exception(int route, unsigned long code) noexcept
    {
        g_resolver_faults.fetch_add(1, std::memory_order_relaxed);
        g_controller_route_faults[route].fetch_add(1, std::memory_order_relaxed);
        g_controller_route_codes[route].store(code, std::memory_order_relaxed);
    }

    static void record_pawn_exception(int route, unsigned long code) noexcept
    {
        g_resolver_faults.fetch_add(1, std::memory_order_relaxed);
        g_pawn_route_faults[route].fetch_add(1, std::memory_order_relaxed);
        g_pawn_route_codes[route].store(code, std::memory_order_relaxed);
    }
"""
text = text.replace(anchor, anchor + extra)

begin = text.index("static SDK::APlayerController* resolve_local_controller(")
middle = text.index("static SDK::ABP_Character_C* resolve_local_pawn(", begin)
end = text.index("static void snapshot_body()", middle)
controller = text[begin:middle]
pawn = text[middle:end]
old = "g_resolver_faults.fetch_add(1, std::memory_order_relaxed);"
assert controller.count(old) == 3, "unexpected number of protected controller routes"
assert pawn.count(old) == 5, "unexpected number of protected pawn routes"
for route in range(3):
    controller = controller.replace(old,
        f"record_controller_exception({route}, static_cast<unsigned long>(GetExceptionCode()));", 1)
for route in range(5):
    pawn = pawn.replace(old,
        f"record_pawn_exception({route}, static_cast<unsigned long>(GetExceptionCode()));", 1)
text = text[:begin] + controller + pawn + text[end:]

anchor_ui = '                        ImGui::Text("Local resolver exceptions: %llu", static_cast<unsigned long long>(g_resolver_faults.load(std::memory_order_relaxed)));'
assert text.count(anchor_ui) == 1, "diagnostics UI anchor changed"
extra_ui = """
                        // All counters are read on the render thread through atomics.
                        ImGui::TextDisabled("Controller route failures (total / last SEH code)");
                        static const char* controller_names[] = {"LocalPlayers", "GameplayStatics", "LocalActor"};
                        for (int route = 0; route < 3; ++route)
                            ImGui::Text("C%d %s: %llu / 0x%08lX",
                                route + 1, controller_names[route],
                                static_cast<unsigned long long>(g_controller_route_faults[route].load(std::memory_order_relaxed)),
                                g_controller_route_codes[route].load(std::memory_order_relaxed));
                        ImGui::TextDisabled("Pawn route failures (total / last SEH code)");
                        static const char* pawn_names[] = {"Pawn", "K2_GetPawn", "GetPlayerPawn", "GetPlayerCharacter", "LocalActor"};
                        for (int route = 0; route < 5; ++route)
                            ImGui::Text("P%d %s: %llu / 0x%08lX",
                                route + 1, pawn_names[route],
                                static_cast<unsigned long long>(g_pawn_route_faults[route].load(std::memory_order_relaxed)),
                                g_pawn_route_codes[route].load(std::memory_order_relaxed));
"""
text = text.replace(anchor_ui, anchor_ui + extra_ui)
assert text.count("record_controller_exception(") == 4
assert text.count("record_pawn_exception(") == 6
assert text.count("ImGui::Text(\"C%d %s:") == 1
assert text.count("ImGui::Text(\"P%d %s:") == 1
path.write_text(text, encoding="utf-8")
print("Player Resolver routes instrumented: controller 3, pawn 5; no gameplay behavior modified.")

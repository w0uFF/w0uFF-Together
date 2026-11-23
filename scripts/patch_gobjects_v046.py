#!/usr/bin/env python3
"""Fix only versioned generated-SDK globals confirmed in the working DLL."""
from pathlib import Path

root = Path("w0uFF-Together")
p = root / "src/Dumper-7_5.3.2-29314046+++UE5+Release-5.3-ChainedTogether/CppSDK/SDK/Basic.hpp"
s = p.read_text(encoding="utf-8")
for old, new in (
    ("GObjects          = 0x06E63470;", "GObjects          = 0x06E644F0;"),
    ("GWorld            = 0x06FD03C8;", "GWorld            = 0x06FD1448;"),
    ("AppendString      = 0x00C80DE0;", "AppendString      = 0x00C80E00;"),
):
    assert s.count(old) == 1, "SDK offset drift: " + old
    s = s.replace(old, new)
p.write_text(s, encoding="utf-8")

p = root / "src/gui/gui.cpp"
s = p.read_text(encoding="utf-8")
anchor = "    static std::atomic_uint64_t g_reference_world_faults{0};"
assert s.count(anchor) == 1, "Missing v0.4.5 GWorld diagnostic"
s = s.replace(anchor, anchor + """
    // Read-only: GUObjectArray address extracted from exact working DLL.
    static std::atomic_int g_reference_object_count{-1};
    static std::atomic_uint64_t g_reference_object_probe_faults{0};
    static void probe_reference_objects() noexcept
    {
        __try
        {
            auto* base = reinterpret_cast<unsigned char*>(::GetModuleHandleW(nullptr));
            if (!base) return;
            const int num = *reinterpret_cast<const int*>(base + 0x06E644F0 + 0x14);
            g_reference_object_count.store(num > 0 && num < 4000000 ? num : -2, std::memory_order_relaxed);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            g_reference_object_probe_faults.fetch_add(1, std::memory_order_relaxed);
            g_reference_object_count.store(-3, std::memory_order_relaxed);
        }
    }
""")
anchor = "        SDK::UWorld* World = resolve_reference_world();"
assert s.count(anchor) == 1, "Missing v0.4.5 world selection"
s = s.replace(anchor,
    "        if ((g_tick_calls.load(std::memory_order_relaxed) % 180) == 1) probe_reference_objects();\n"
    + anchor)
anchor = '                        ImGui::Text("GWorld read exceptions: %llu",'
assert s.count(anchor) == 1, "Missing v0.4.5 world diagnostic UI"
s = s.replace(anchor, """
                        ImGui::Text("Reference GObjects +0x6E644F0 count: %d", g_reference_object_count.load(std::memory_order_relaxed));
                        ImGui::Text("Reference GObjects probe exceptions: %llu",
                            static_cast<unsigned long long>(g_reference_object_probe_faults.load(std::memory_order_relaxed)));
""" + anchor)
p.write_text(s, encoding="utf-8")
print("v0.4.6: confirmed GObjects/GWorld/FName AppendString offsets, read-only object diagnostics")

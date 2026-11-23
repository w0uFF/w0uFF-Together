#!/usr/bin/env python3
"""Polish startup attribution and reusable draggable HUD behavior.

Requires v0.4.6 source. No changes to player resolver or gameplay features.
"""
from pathlib import Path

root = Path("w0uFF-Together")
main = root / "src/main.cpp"
src = main.read_text(encoding="utf-8")
lines = src.splitlines(keepends=True)
matching = [i for i, line in enumerate(lines) if 'LOG(INFO) << "Lua manager initialized.";' in line]
assert len(matching) == 1, "Lua initialization log marker changed"
at = matching[0]
indent = lines[at].split("LOG(INFO)")[0]
assert "w0uFF Together initialized successfully." not in src
credit = [
    indent + '// Log success after renderer, hooks and Lua are initialized.\n',
    indent + 'if (!g_abort)\n',
    indent + '{\n',
    indent + '\tLOG(INFO) << "w0uFF Together initialized successfully.";\n',
    indent + '\tLOG(INFO) << "Original mod: UnchainedTogether by xiaoxiao921 (Quentin).";\n',
    indent + '\tLOG(INFO) << "Upstream: https://github.com/xiaoxiao921/UnchainedTogether";\n',
    indent + '\tLOG(INFO) << "w0uFF Together reuses substantial parts of the upstream GPL-3.0 project as its foundation.";\n',
    indent + '\tLOG(INFO) << "We adapted that foundation to newer Chained Together releases and added our own features.";\n',
    indent + '\tLOG(INFO) << "Credits and GPL-3.0 obligations: see NOTICE.md and LICENSE.";\n',
    indent + '}\n',
]
lines[at + 1 : at + 1] = credit
main.write_text("".join(lines), encoding="utf-8")

gui = root / "src/gui/gui.cpp"
src = gui.read_text(encoding="utf-8")
needle = """\t\t\tconst auto flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
\t\t\t    ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoNav;"""
assert src.count(needle)==1,"Coordinates HUD ImGui flags marker changed"
new = """\t\t\t// Shared HUD policy: normal gameplay is non-interactive; opening INSERT
\t\t\t// exposes the title bar so widgets can be dragged. Saved ImGui window
\t\t\t// settings retain their positions when the backend supports persistence.
\t\t\t// Future HUD widgets should use this same policy.
\t\t\tconst auto flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoNav |
\t\t\t    (m_is_open ? ImGuiWindowFlags_NoCollapse :
\t\t\t     ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs);"""
src=src.replace(needle,new)
intro='ImGui::TextDisabled("Optional overlays; nothing enabled automatically.");'
assert src.count(intro)==1, "Visuals menu marker changed"
src=src.replace(intro,intro+'\n                        ImGui::TextDisabled("With INSERT open, drag a HUD widget by its title bar to reposition it.");')
assert src.count("ImGuiWindowFlags_NoSavedSettings")==0,"Unexpected duplicate coordinates HUD"
assert src.count("ImGuiWindowFlags_NoInputs")==1
gui.write_text(src,encoding="utf-8")
print("v0.4.7: Startup success + proper upstream credit; draggable HUD in menu, click-through during play.")

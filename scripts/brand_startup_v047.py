#!/usr/bin/env python3
"""Add truthful startup success and prominent upstream attribution after init.

Patches reconstructed GPL source; intentionally preserves framework logging.
Fail closed if the expected v0.4.6 source location has changed.
"""
from pathlib import Path
src = Path("w0uFF-Together/src/main.cpp")
content = src.read_text(encoding="utf-8")
needle = '            LOG(INFO) << "Lua manager initialized.";'
# Old source uses tabs (not spaces); match the line without changing other code.
lines = content.splitlines(keepends=True)
matches = [i for i,l in enumerate(lines) if 'LOG(INFO) << "Lua manager initialized.";'
           in l]
assert len(matches)==1,"Expected exactly one Lua initialization log"
pos=matches[0]
indent=lines[pos].split("LOG(INFO)")[0]
block=[
    indent + '// Emit success only after all core systems have initialized.\n',
    indent + 'if (!g_abort)\n',
    indent + '{\n',
    indent + '\tLOG(INFO) << "w0uFF Together initialized successfully.";\n',
    indent + '\tLOG(INFO) << "Original project: UnchainedTogether by xiaoxiao921 (Quentin)";\n',
    indent + '\tLOG(INFO) << "Original source: https://github.com/xiaoxiao921/UnchainedTogether";\n',
    indent + '\tLOG(INFO) << "w0uFF Together reuses substantial portions of UnchainedTogether as its foundation.";\n',
    indent + '\tLOG(INFO) << "We adapted this foundation for recent Chained Together versions and added our own features.";\n',
    indent + '\tLOG(INFO) << "Original code and derivative are credited under GPL-3.0; see LICENSE and NOTICE.md.";\n',
    indent + '}\n',
]
assert "w0uFF Together initialized successfully." not in content
lines[pos+1:pos+1]=block
src.write_text("".join(lines),encoding="utf-8")
print("Added success and explicit upstream attribution after Lua init; unchanged ROM setup.")

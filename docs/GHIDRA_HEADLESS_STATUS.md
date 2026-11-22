# Ghidra headless — verified installation and Player Resolver analysis

## Verified (2026-11-22)
Ghidra **11.4.3 PUBLIC** from the official NSA GitHub release, with SHA-256:
`b22a8baebb14339f19903c54363ba7e01781930cc43ab430539aabce0d58547a`

The CI workflow [`.github/workflows/ghidra-headless.yml`](../.github/workflows/ghidra-headless.yml) sets up Temurin Java 21, downloads/validates Ghidra, and runs `support/analyzeHeadless` on **our own v0.4.4 Windows DLL** via [`scripts/wouff_ghidra_headless.py`](../scripts/wouff_ghidra_headless.py).

Run **45065182333**: `success`, with `Analysis succeeded`, **16,879 identified functions**, `ANALYSIS_DONE` in the report. The report is the artifact `w0uFF-Ghidra-11.4.3-smoke-report`.

The script has `smoke` and `reference` modes. `reference` currently targets validated `Fly Mode` xrefs at `0x180062BA0` and `0x180063FC4` **only for the exact 2026-11-09 user reference binary**. Do not use these addresses on a different binary.

## Critical limitation
The **working reference DLL** was attached to the ChatGPT conversation, not stored in the GitHub repository. This CI workflow **does not** import that file, and it would be inappropriate to publish it to the public repository automatically. It must be made available to an authorized private analysis environment or passed via a secure transfer path before running `reference` mode. The local ChatGPT execution environment is currently unavailable. Do not label the smoke report as decompilation of the working reference DLL.

## Priority
Only **Player Resolver / Fly** is in scope. In v0.4.3 the user reported 5,397 ticks, 21,588 SEH resolver exceptions, world/level FOUND, controller/pawn NOT FOUND. v0.4.4 adds per-route fault counters; it has compiled, but in-game resolution remains unverified.

Reverse-engineered pseudocode is **not original source code**. Preserve original licenses and use only the minimal technical findings needed for a reliable local player lookup.

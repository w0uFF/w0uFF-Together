# Player Resolver v0.4.5 — reconstructed from working DLL evidence

## Scope and provenance
User-reported working reference: `d3d12.dll` from 2026-11-09, 15,788,032 bytes, SHA-256 `b9055133ce5c28397e3fcd285cc4ba30268da0bfc0c1e82d5555ecf0aff0ac4f`. This exact binary is in the authorized GitHub repository; its source code and matching PDB are unavailable.

Ghidra 11.4.3 successful analysis of **this exact binary**: run [45076617212](https://github.com/w0uFF/w0uFF-Together/actions/runs/45076617212), 44,798 functions. Both Fly UI strings resolve to the function starting `0x180061AA0`. Ghidra pseudocode is reconstructed, not original C++.

Read-only binary disassembly: runs [45077334687](https://github.com/w0uFF/w0uFF-Together/actions/runs/45077334687) and [45077501235](https://github.com/w0uFF/w0uFF-Together/actions/runs/45077501235). At `0x180061E93` the reference calls `GetModuleHandleA(0)`, then at `0x180061E99` loads `[imageBase+0x6FD1448]`. The subsequent dereferences, checked against generated SDK class offsets in run [45077587636](https://github.com/w0uFF/w0uFF-Together/actions/runs/45077587636), are:

| Object | Offset | SDK member / evidence |
|---|---:|---|
| Executable module | `0x6FD1448` | Current `GWorld` pointer (inferred from World structure reads) |
| World | `0x30` | `PersistentLevel` |
| World | `0x1B8` | `OwningGameInstance` |
| GameInstance | `0x38` | `LocalPlayers` array |
| LocalPlayer | `0x30` | `PlayerController` inherited from `UPlayer` |
| PlayerController / Controller | `0x2D8` | `Pawn` |
| DLL-local cached pawn | `0x180E81910` | Store at instruction `0x180061F91` after runtime type check |

The generated SDK `Basic.hpp` advertises `Offsets::GWorld=0x06FD03C8` (also explicitly cautions that the SDK's `UWorld::GetWorld()` does not use it). The working binary accesses `0x06FD1448`, a **`0x1080` difference**. The World returned by the old SDK lookup may therefore not be the active playable World in game 1.11.2. This is a hypothesis supported by the reference pointer chain and the user's v0.4.3/4 diagnostics, not a gameplay-confirmed root cause.

## v0.4.5 isolated change
`scripts/resolve_reference_world_v045.py` installs a guarded `resolve_reference_world()` helper that reads the working reference's GWorld pointer from the executable module, validates that PersistentLevel and OwningGameInstance are available, and falls back to `SDK::UWorld::GetWorld()` if not. It does not modify the existing local-only controller/pawn selection nor any Fly logic. Runtime Settings now report `World source` and `GWorld read exceptions`.

This offset is **build-specific** and must not silently be considered valid for arbitrary Chained Together versions. In particular, no arbitrary actor or teammate may be chosen if the local player chain fails.

Build source reconstruction uses v0.4.2 source, checked v0.4.3 patch, v0.4.3 MSVC fix, v0.4.4 per-route diagnostics, and this isolated v0.4.5 World resolver. Workflow `.github/workflows/wouff-resolver-v045.yml` builds Windows x64 with GPL source artifact.

## Human validation gate (still open)
1. In a private **solo** game of Chained Together 1.11.2, install the v0.4.5 Windows DLL (only after successful Actions build) and open Settings diagnostics.
2. Confirm `World source: Working DLL GWorld+0x6FD1448`, controller and pawn both FOUND. If not, capture all C1–C3 and P1–P5 faults/codes.
3. Enable the Fly module **manually**, then test F7. Modules remain OFF at startup, vanilla movement and jumping retained.
4. Only after solo succeeds, validate co-op in a private lobby and verify that Fly affects the **local player**, not the partner.

Do not claim resolved or ship stable until these tests pass.

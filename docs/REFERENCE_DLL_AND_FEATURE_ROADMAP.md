> **CURRENT PRIORITY — P0 PLAYER RESOLVER ONLY (2026-11-22):** Suspend Hitbox Debug changes, Chain ON/OFF and Partner Glow development. First fix detection of the local PlayerController and local Pawn, then verify Fly F7 in solo and co-op. No additional gameplay feature is to be enabled by default.
>
> **Latest in-game result (v0.4.3):** 5,397 Unreal tick callbacks, 0 snapshot read failures, 21,588 local resolver exceptions, World/level FOUND/FOUND, controller path 0, pawn path 0, controller/pawn NOT FOUND. The exactly 4:1 exception/tick ratio narrows the regression to candidate resolver calls but **does not identify which of the separate guarded routes fault**.
>
> **Code inspected:** v0.4.3 `src/gui/gui.cpp` uses `UWorld->OwningGameInstance->LocalPlayers`, `UGameplayStatics::GetPlayerController` and a local-controller actor fallback, followed by `pc->Pawn`, `K2_GetPawn`, `UGameplayStatics::GetPlayerPawn/GetPlayerCharacter` and matching local-controller actor fallback. Each route catches SEH but increments the same `g_resolver_faults` counter, erasing the failing route identity. The next correction should attribute exceptions per route and capture their exception codes, then validate a safe lookup against the user-provided working DLL; do not guess offsets or use a remote co-op pawn.
>
> **Exit criterion:** Controller FOUND, local pawn FOUND, Fly F7 acts on that pawn after manual activation, all mod features OFF at startup. Follow with private co-op regression testing.

# Reference DLL research and requested features

## Scope and evidence

The user-supplied **working reference** is the 2026-11-09 `d3d12.dll`, SHA-256 `b9055133ce5c28397e3fcd285cc4ba30268da0bfc0c1e82d5555ecf0aff0ac4f` (15,788,032 bytes; PE32+ x64).

Static examination of the user's original ZIP found the `Unchained Together` / `Fly Mode` strings, `chained_together_fly_mode`, Unreal player-resolution function names, and a CodeView reference to `build-vs2026/Release/d3d12.pdb`. The PDB was **not included**. These are fingerprints and leads, **not** proof of specific runtime call paths or a full decompilation.

**Source-code availability:** We do **not** have the source code or matching build tree for this particular working reference DLL. The available UnchainedTogether C++ repository is an **older/separate codebase**, not a verified source match for the supplied binary. It illustrates earlier local-player lookup through `UWorld -> OwningGameInstance -> LocalPlayers[0] -> PlayerController` and `ABP_Character_C::SetFlyMode(bool)`, but these functions must **not** be attributed to the reference DLL without binary evidence. The working reference can be studied via PE metadata, static analysis, disassembly and decompiler pseudocode (where feasible); generated pseudocode is not original source. The absence of its matching PDB and source limits exact reconstruction. Treat reported Fly/co-op functionality as a behavioral baseline, not proof that a specific C++ path exists in the DLL.

Current regression: v0.4.2 rendered the menu, recorded 46,366 Unreal tick callbacks but 773 snapshot failures, no local controller/pawn and inactive Fly. v0.4.3 contains a proposed multi-path resolver; its gameplay behavior is not yet verified.

## Next requested features

### 1. Detach / reattach chains

- Add **Chains enabled** toggle, default **ON** (the mod itself starts with no chain override). OFF should mean characters are *actually freed from chain physics*, not just the mesh hidden.
- Restore normal chain linking with the same toggle, including co-op. Require valid players and appropriate host/server authority. Avoid modifying network arrays or destroying chain actors without a recovery/re-creation path.
- Candidate SDK APIs: `ABP_Character_C::ConnectedChains`, `Chained`, `Add_Chain`, `Spawn_Chains`, `SpawnChainsServer`, `GetConnectedChains`; `ABP_Chain_C::DissolveChain`, `AttachChainsToCharacters`, `RefreshChain`, `InitChain`; and `AGS_Game_C::AttachPlayersAndBeginGame`, `AttachToTeamMember`.
- **Not yet implemented or tested.** Do not assume `DissolveChain` alone performs a safe synchronized detach.

### 2. Glow-through-walls for duo

- Independent **Partner glow** toggle, default **OFF**, unaffected by whether chains are enabled.
- Only outline the actual paired teammate; do not highlight every pawn. Disable and restore render state on disconnect, map change, and toggle OFF.
- Candidate SDK APIs: `ABP_Character_C::MakeOutline(bool)`, `SetRenderCustomDepth(bool)`, `Update_CustomDepth()`; `UPrimitiveComponent::SetRenderCustomDepth`, `SetCustomDepthStencilValue`.
- Test actual occluded visibility in game. Unreal CustomDepth availability alone does not prove an outline will show through walls.
- **Not yet implemented or tested.**

## Delivery order

1. Repair local player acquisition and verify Fly in solo with all opt-in features OFF on launch.
2. Recheck co-op behavior with the reference DLL as a behavioral baseline.
3. Implement partner glow and in-game occlusion tests.
4. Implement actual reversible chain mechanics and test multiplayer authority/replication before a stable release.

Maintain GPL-3.0 attribution for reused sources; keep the w0uFF-branded UI.


## Binary disassembly notes (22 Nov 2026)

Read-only analysis of the actual supplied working `d3d12.dll` confirms the following concrete PE addresses (image base `0x180000000`):

- `Fly Mode` text: `0x180CD5908`; referenced by instruction at `0x180062BA0`.
- `Fly Mode##2` text: `0x180CD5B40`; referenced by instruction at `0x180063FC4`.
- Both instructions lie in one large UI function with exception-directory extent `0x180061AA0..0x180064670`.
- The `GetPlayerController` and `GetPlayerPawn` names exist as strings, but could be reflected SDK metadata. **These strings do not prove that the reference DLL uses those APIs.**
- The matching `d3d12.pdb` is absent; the binary cannot be reconstructed as exact original C++ source.

For deeper call-graph and pseudocode analysis, the tested compatible Ghidra package is the official `ghidra_11.4.3_PUBLIC_20251203.zip` (456,551,841 bytes, SHA-256 `b22a8baebb14339f19903c54363ba7e01781930cc43ab430539aabce0d58547a`). This is a decompiler; its output remains reconstructed pseudocode. Java 21 is already available for local analysis; no Java download is requested.

### Windows build hotfix

The initial v0.4.3 Windows build failed at `gui.cpp:682` with MSVC C2440 converting SDK `ACharacter*` to `ABP_Character_C*`; patch `patches/v043-msvc-build-fix.patch` changes the conversion following an explicit runtime `IsA` check. Three portable tests pass. Git checkout on Windows also changed patch line endings; the workflow now normalizes to LF before verifying SHA-256. The Windows DLL must still successfully compile and be tested in game before claiming Fly is repaired.

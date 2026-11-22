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

# Player Resolver v0.4.4 — diagnostic build

## Priority
Only the local player resolver and its Fly regression are in scope. Hitbox, Chains and Partner Glow are paused.

## Evidence from game v0.4.3
- Unreal ticks: **5,397**
- Snapshot read failures: **0**
- Resolver exceptions: **21,588** (four per tick)
- World / persistent level: **FOUND / FOUND**
- Local PlayerController and pawn: **NOT FOUND**
- All controller/pawn resolution paths: **0**

The 4:1 ratio does not establish which routes throw.

## v0.4.4 code changes
The existing three controller and five pawn lookup routes now have separate **SEH exception counters and last exception codes**, displayed under `INSERT → Settings → Runtime diagnostics`.

- C1 LocalPlayers, C2 GameplayStatics, C3 LocalActor
- P1 Pawn, P2 K2_GetPawn, P3 GetPlayerPawn, P4 GetPlayerCharacter, P5 LocalActor

A value `0x00000000` and zero counter means the route has not thrown. `0xC0000005` indicates an access violation; other codes must be interpreted individually. Nonzero faults are not automatically proof that an API itself is broken: wrong offsets or obsolete generated SDK definitions can cause it.

**No gameplay behavior or player-selection fallback is changed** in this instrumentation build. A successful build is not proof that Fly works.

## User validation (after successful Windows artifact only)
1. Back up the prior d3d12.dll, then install only the new DLL.
2. Launch Chained Together 1.11.2 in a private solo game.
3. Open `INSERT → Settings → Runtime diagnostics`.
4. Stay in-game for several seconds, then capture the full **C1–C3**, **P1–P5** counters, hex codes, controller path/pawn path and FOUND/NOT FOUND statuses.
5. Do not enable Fly while controller or pawn is NOT FOUND. Report errors or crashes.
6. If controller and pawn are both FOUND, enable the Fly feature manually then test F7. Keep all features OFF at startup.
7. Only after solo Fly succeeds, re-test co-op in a private lobby, ensuring the local pawn is selected, not the remote teammate.

## Follow-up
Correlate failing routes with the generated SDK definitions and, if possible, the supplied working reference DLL via Ghidra; repair only the failing resolution path. Preserve GPL-3.0 source attribution.

## Subsequent UI request — deferred until Player Resolver is fixed

When the practice menu is OPEN, constrain the mouse cursor to the actual **Chained Together client area** on the active monitor to avoid accidental movement onto a second display and loss of mouse-wheel jump input. On menu close, focus loss, alt-tab, window resize/move, game shutdown, or UI error, restore the original cursor clipping state immediately. Respect DPI and borderless/windowed modes; never force a permanent global lock or steal focus. Implement and test only **after** local PlayerController/Pawn detection and Fly have been verified. All existing feature defaults remain unchanged.
## SDK review — newly confirmed generated wrapper behavior

A dedicated GitHub Actions inspection (run `45066719449`) read the actual C++ SDK bundled with the project. This SDK identifies itself as `Dumper-7_5.3.2-29314046+++UE5+Release-5.3-ChainedTogether` and uses a generated wrapper for `UGameplayStatics::GetPlayerController(world, 0)`. The wrapper does:

```cpp
if (Func == nullptr)
    Func = StaticClass()->GetFunction("GameplayStatics", "GetPlayerController");
auto Flags = Func->FunctionFlags; // no null check here
GetDefaultObj()->ProcessEvent(Func, &Parms);
```

**Evidence:** in-game v0.4.4 C2/P3/P4 all throw `0xC0000005` (access violation), while C1 LocalPlayers has no exception but never returns a controller, and C3 LocalActor throws `0xE06D7363` (MSVC C++ exception). This wrapper **could** fault if `Func` or its class/instance is invalid; it is not proof of which pointer is invalid. Never treat `World FOUND` as proof that the generated SDK offsets and reflection wrappers remain valid for game 1.11.2. There is a concrete risk that older SDK metadata has drifted. Do not retry broken wrappers repeatedly without new evidence. Preserve local-only checks for co-op.

The working reference DLL was identified on the Nexus Mods `UnchainedTogether (Updated)` page, version 2, dated 2026-11-09; the original source and PDB are still unavailable. **Do not reupload that third-party DLL to a public repo.** GitHub Actions Ghidra currently only confirmed analysis of our own DLL, not that reference.

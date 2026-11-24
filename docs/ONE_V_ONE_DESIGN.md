# w0uFF Together — planned 1v1 Race (no fake enforcement)

## Verified working baseline
The user confirmed Chained Together **1.11.2** with our v0.4.6 runtime: Controller and Pawn FOUND, Fly works. Do not disturb this resolver, F7, game-thread/renderer separation, or vanilla movement. All mod features are OFF by default.

## Product request
- Exactly **two online players** in a private 1v1 race, independently climbing instead of being tethered as a coop duo.
- Host configures a custom **1v1 lobby panel**, including a **Cheats Enabled** checkbox, **OFF by default**.
- If Cheats Enabled is OFF, this mod refuses Fly, Teleport, Freecam and any other cheat-providing feature for both participants **during the 1v1**, even if those modules were enabled in the practice configuration.
- When entering the 1v1, forcibly apply `SetFlyMode(false)` on the game thread and clear pending teleport actions if host cheats are disabled; blocking only new hotkeys is insufficient if Fly was already active before the match.
- During an active 1v1, gate **every invocation path** (menu toggles, F7, checkpoint/teleport hotkeys, mailbox processing, scripted actions) against the synchronized host policy; do not rely on disabled UI controls alone.
- Host's configuration must propagate to the guest. User's own unauthorized checkbox or local file must not grant permission.
- Reversibly detach the **actual physical chain constraints** on the authoritative host, not merely hide the chain mesh. Restore original state / return to normal on match exit.
- Both players must have compatible versions of the mod. Solo practice and ordinary co-op must remain unchanged unless the host explicitly creates a 1v1 lobby.
- Default race metric: reach the summit first; support a server-confirmed finishing event, not client-asserted positional victory.
- Future HUD widgets (coordinates, race timer, leaderboard) share drag-to-position behavior while the menu is open; mouse/input passthrough while closed.

## What the game already exposes (read-only SDK inspection)
GitHub Actions runs 45124348439 and 45124492202 inspected SDK generated for the actual game build:

| System | Evidence |
|---|---|
| Existing native competition UI | `UW_Lobby_C::VersusBox`, `UW_Lobby_C::ForceGameMode`, `UW_Lobby_C::OnTeamSelectionChanged` |
| Existing lobby form | `UW_CreateSession_C` owns `W_ComboBox_GameMode`, `W_ComboBox_NumPlayerSelect`, `W_ComboBox_ChainType` |
| Race state | `AGS_Game_C::RaceModeWinnerTeamID`, `Selected_Game_Mode`, `TeamHeights`, `IsRaceScoreFinalised` |
| Chain instance | `ABP_Chain_C::Attached01`, `Attached02`, `Constraints`, `Socket01ConstraintToChain`, `Socket02ConstraintToChain` |
| Chain methods | `ABP_Chain_C::RemoveCharacterConstraints`, `DissolveChain`, `AttachChainsToCharacters`, `RefreshChain` |
| Host/session state | `UCustomGI_C::HostingOnlineSession`, `OpenedChain`, `SelectedGameMode`; `UAdvancedSessionsLibrary` session settings APIs |

The game's **September 2026 2v2 Race mode** may be a useful native scoring/team baseline, but **it is NOT evidence that native 2v2 supports only two players**. Never label it a working 1v1 without testing.

## Current implementation, limits and verification
- `duel/duel_policy.hpp`: protocol-neutral state machine, cheats disabled at lobby creation, host-only configuration, fail-closed guest rules, two-peer/chain-detached/synchronized start requirements, vanilla unaffected until 1v1 is active.
- `duel/tests/policy_test.cpp`: unit tests executed in CI. This module is **not yet connected to the gameplay binary, game lobby, network transport or physical chain objects**. It is **NOT** a playable 1v1.
- Authoritative transport needed: authenticated host+guest handshake (protocol/build, peer identities, nonce, monotonic settings revision), server-controlled mode start/stop, rules propagation, confirmations and disconnect recovery. Do not use unverifiable local C++ bools as host proof.
- Network cheat rules can control only **w0uFF Together features on compatible, cooperating clients**. They cannot prevent an adversary from changing external trainers or tampering with the mod. Avoid marketing as full anticheat.
- Physics detach must be **host/server only**. Require an explicit reversible teardown path, ownership of chain actors, and replication verification. Testing `DissolveChain` alone does not establish constraints were removed. Do not mutate chain objects during ordinary co-op.
- UI recommendation for initial rollout: styled ImGui overlay (shares our active renderer) in the existing game's create-session/lobby screens. The SDK exposes native UMG widgets, but editing cooked game widget layouts at runtime may be brittle; integrate native UMG only after proving safe attachment points.

## Verification gates (not met)
1. Two users with the exact same mod version create/join a private host-owned lobby.
2. 1v1 option appears for host and applies to both. Guest may not change rules.
3. Host confirms both participants, disables the true physics constraints on the game/server thread; each avatar moves independently. Both can return to standard coop safely.
4. Cheats Enabled OFF: F7 Fly and all other mod cheats unavailable to **both** during duel. Cheats Enabled ON: host choice is synchronised; no client-local bypass via the mod UI. Reset to OFF each new lobby.
5. Show native score/timer and winner (host authoritative), disconnect cleanup and recovery, then test across Windows windowed/borderless settings.
6. Preserve GPL-3.0 and original UnchainedTogether author xiaoxiao921 (Quentin) credit.

**Do not say "1v1 implemented" until at least the first four runtime gates pass.**


## Expanded race product direction (2026-11-23)
The original exactly-two-player duel is now one configuration of a broader Race mode.

- **Solo time trial** must be launchable without another participant and visible in game. The first alpha is a local configurable timer in the existing ImGui lobby tab, not yet a native UMG button.
- **Finish-first**: first participant confirmed at the map finish wins. Solo records elapsed completion time only when a real finish signal is wired; until then a manual stop is an *unranked timer*, not a completed map.
- **Height-at-deadline**: configurable duration (including arbitrary minutes and multi-day races), host/server determines each participant's highest verified vertical progress and winner at deadline. Timers must remain correct across frame drops and overlay closure, and duration must be bounded to avoid overflow.
- **HUD**: remaining time, local height in metres, map-completion percentage only after proving exact level origin and summit height, and per-participant progress bars. Drag/edit when INSERT is open; pass mouse/game input through when closed.
- **Multiplayer expansion**: allow solo and N-player races up to an *experimentally verified* game/session cap. Never assume the earlier hard-coded 16-player SDK sanity bound is a supported capacity.
- **Records**: local durable per-run history (mode, version, map, verified height/finish, duration, cheats, participants, timestamps), best attempts and local leaderboards. Current alpha stores only unranked timer entries in memory; restarting the game loses them.
- **Global leaderboard**: a future opt-in backend must validate run provenance and resist replay/forged submissions. Do not ship GitHub write credentials or a repository-scoped personal access token in the DLL. GitHub Actions by itself does not authenticate arbitrary game clients or prove runs are legitimate. Explicit consent/privacy and a service-side ingestion endpoint are prerequisites.
- **Native integration**: first probe a safe UMG entry point near the game's existing Play/session buttons. Continue using the tested ImGui panel until a native widget hook is proven stable. Solo mode should not modify physical chain actors.
- **Cheat policy**: differentiate explicitly enabled unranked practice from ranked competitive sessions; avoid presenting local records as authenticated competitive results.

### Shipped incremental alpha
`duel/solo_race_panel.hpp` is a renderer-thread local timer, accessed through `duel/duel_lobby_controller.hpp` and packaged by `scripts/integrate_duel_lobby_v048.py`. It supports manually stopped finish timers or 1-second-to-30-day countdowns, with a non-persistent in-memory history (20 entries max). It does **not** detect map finish, record height, show an always-on HUD, persist results, provide native buttons or support a playable competitive multiplayer race. All gameplay-chain manipulation remains disabled.

### CI caveat
After commit `227b74306cc9cb46c5db6144b7cf360c68da7365`, GitHub Actions began failing immediately without job step logs. Treat this as an **unresolved build/CI blocker**, not proof of compiler failure or success. The original v0.4.8 tested by the user remains the last verified playable baseline.

### Solo timer HUD follow-up
The solo alpha now calls `solo_race_panel::tick()` on every renderer frame through `lobby_controller::tick()`, independently of multiplayer membership or tab selection. While active, a read-only, non-interactive remaining/elapsed-time HUD is rendered at the upper-right. Expiry stops the countdown even when the 1v1 panel is closed. This is **not** yet a movable HUD and does not expose verified height, finish signals, a persistent record or ranked results. UI compile and game testing remain required.

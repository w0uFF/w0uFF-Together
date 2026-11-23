# v0.4.8 — Steam P2P 1v1 lobby staging (NOT a playable race)

## Scope / confirmed baseline
The user verified in-game **v0.4.6** PlayerController FOUND, Pawn FOUND and Fly F7 actually functioning in Chained Together 1.11.2. v0.4.7 compiled successfully and contains movable coordinates HUD plus upstream GPL credits; no manual v0.4.7 test yet.

v0.4.8 preserves those features and introduces an *opt-in* **05 1v1 Lobby** tab to test Steam-to-Steam rules exchange in a real native two-person private game lobby. **It does not detach chains, choose a native race mode, start a competitive match, track scores or declare winners.** Its Start button is disabled and explicitly says why.

## Implemented in source
- `duel/duel_policy.hpp`: host-only settings, default Cheats Enabled OFF, fail-closed client acceptance, revision monotonicity.
- `duel/duel_wire.hpp`: fixed 48-byte endian-stable v1 messages; session/nonce, host and guest SteamID64, sequence number, anti-replay checks, sender ID checked against authenticated Steam transport identity.
- `duel/steam_p2p_transport.hpp`: optional dynamic adapter for *the game's already initialized* `steam_api64.dll`, on a reserved P2P channel; reliable legacy Steam P2P messaging; no Steam init/shutdown and no game session changes. This legacy interface is deprecated and **runtime availability in this particular game is unverified**. Reference: https://partner.steamgames.com/doc/api/ISteamNetworking
- `duel/duel_lobby_controller.hpp`: host/guest experimental ImGui tab, host sets Cheats Enabled, guest receives host settings and acknowledges revisions. Both users must have identical mod versions.
- `scripts/integrate_duel_lobby_v048.py`: instruments read-only `UCustomGI_C` lobby flags and count on Unreal game thread, then uses only snapshot POD on renderer; gates Fly, Teleport and Free Camera when duel staging is active. Fly that was enabled before staging is forced OFF on the game thread; previously mod-activated Free Camera is toggled off when cheats are denied. Reverts to ordinary behavior when staging is exited.
- Guard against host migration, lost session and a participant count other than two: deny mod cheats, even when a stale host permission was previously received.
- No runtime chain mutation or gameplay mode selection; network protocol unit tests and standalone Windows Steam transport compile test pass.

## Human test gate — two separate Windows Steam clients, private lobby only
1. **Back up the working v0.4.6 DLL.** Download the successful v0.4.8 Windows artifact from its GitHub Actions workflow after build completion. Do not install on a public lobby.
2. Both players install the **same v0.4.8 DLL**, launch Chained Together 1.11.2, and join the game's existing **private 2-player online lobby**. No game modification should occur before using the new tab.
3. On each client, open `INSERT → 05 1v1 Lobby`. Confirm **Steam P2P API: DETECTED**, a plausible **Your Steam ID64**, **Native game lobby: DETECTED**, **Players in session: 2**, and **Game reports host YES on only the actual host**. Report any mismatch; do not attempt a race.
4. Exchange the displayed SteamID64 numbers. Host enters the guest SteamID64 and clicks **Open 1v1 host settings (alpha)**. The host copies the generated **Session code** and sends it privately to the guest.
5. Guest enters the host SteamID64 and session code; click **Connect to 1v1 host (alpha)**. Host should see **GUEST ACKNOWLEDGED HOST RULES**; guest should see **HOST RULES RECEIVED**.
6. With **Cheats Enabled OFF** (default), neither should be able to activate Fly F7, Teleport F8, or mod Free Camera while staging is active. Normal vanilla walking/jumping must continue. The actual physical co-op chain remains unchanged.
7. Host checks **Cheats Enabled**. The guest should receive **YES (host setting)**, and F7 Fly should again be available, if its original module is enabled. Guest must not be able to modify the setting.
8. Host unchecks it again. Both should lose mod Fly access. On each client use **Leave 1v1 staging** and verify normal co-op and previously functioning Fly return.
9. Repeat a quick Alt+Tab, menu close and disconnect/rejoin test, checking that no abnormal crashes occur.

Report in-game screenshots of the **05 1v1 Lobby** pane on BOTH clients and whether host/guest messages synchronized. Capture `Settings → Runtime diagnostics` if `Native game lobby`, host flag or player count are wrong. Do not perform any experiment on the chain physics or manually set a competitive mode until the host-authoritative detach flow is implemented.

## Next implementation gate (NOT yet met)
- Verify full two-way P2P handshake and authoritative host identity against actual Steam lobby membership, not just manually typed peer IDs.
- Verify game-server chain actor belongs to exactly those two pawns, then **detach real constraint components server-side** and confirm client replication, with reversible restoration on abort/host disconnect.
- Only after the real chain detach is verified, enable Start 1v1 and a native/host-confirmed scoring and winner mechanism.
- Enforce mod rules across all paths, including Lua extensions; this is cooperative mod policy, not protection from external cheating software.

## Attribution
UnchainedTogether originally by xiaoxiao921 (Quentin), GPL-3.0; see NOTICE.md and LICENSE. Keep default modules OFF, normal speed/jump vanilla, and never select a remote pawn as the local player.

# w0uFF Together

**Practice Studio for Chained Together** — Windows x64 (DirectX 12).

A custom dark-violet practice interface with opt-in Fly, repeatable cut checkpoints and solo/duo training notes. Gameplay speed and jumping remain vanilla.

## Current status

- **v0.4.1:** Windows DLL compiled, menu opens in game, but gameplay features were reported inoperative. Keep the previous DLL as a backup.
- **v0.4.2:** interface redesigned, Unlock Wings removed, all modules start disabled, and Unreal runtime diagnostics added. Windows CI compile and source tests pass. **In-game test: menu works, but Fly remains inactive and the interface reports no local pawn detected.**

## Building v0.4.2

1. The source archive `wouff-source-v042.zip` is already committed to this repository. Leave the existing `wouff-vendor-*.zip` archives unchanged.
2. The GitHub Actions workflow `wouff-windows-build.yml` produced the v0.4.2 Windows DLL in run #3.
3. If the build succeeds, download the artifact `w0uFF-Together-v0.4.2-Windows-DLL-unverified` from the Actions run.
4. Back up your existing working DLL before testing in a private game. Open `INSERT → Settings`; check Unreal tick callbacks and local pawn status before enabling Fly / Teleport. Send these diagnostic results if gameplay actions remain inactive.

**Do not treat CI compilation as proof of gameplay compatibility.** The user's earlier third-party DLL worked in co-op (including Fly) according to their testing. This is useful regression evidence, not proof that the current w0uFF Together fork is working in co-op. The current checkpoint implementation targets the local pawn; its resulting chain/network behavior has not been tested or established. Do not describe co-op gameplay support as unimplemented: investigate and preserve the original working behavior while fixing local pawn detection. No claim is made that our current fork synchronizes checkpoint teleports.

## Source and licensing

The source archive contains GPL-3.0 license and `NOTICE.md`. w0uFF Together includes new w0uFF-specific work and reuses GPL-licensed UnchainedTogether code originally by xiaoxiao921. Its source and attribution are retained according to the original license, together with notices for bundled third-party dependencies.

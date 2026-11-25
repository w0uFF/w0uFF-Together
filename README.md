# w0uFF Together updated version of unchained together (by xiaoxiao921) but with new feature and improvement

**Practice Studio for Chained Together** — Windows x64 (DirectX 12).

1. The source archive `wouff-source-v042.zip` is already committed to this repository. Leave the existing `wouff-vendor-*.zip` archives unchanged.
2. The GitHub Actions workflow `wouff-windows-build.yml` produced the v0.4.2 Windows DLL in run #3.
3. After the automated Windows build and checks succeed, download `d3d12.dll` from the latest **development prerelease** in [GitHub Releases](https://github.com/w0uFF/w0uFF-Together/releases). Each successful build creates a unique `dev-<run>-<attempt>` prerelease with the rebuilt source ZIP and `SHA256SUMS.txt`. Unverified gameplay builds remain labeled experimental; a failed build does not create a release.
4. Back up your existing working DLL before testing in a private game. Open `INSERT → Settings`; check Unreal tick callbacks and local pawn status before enabling Fly / Teleport. Send these diagnostic results if gameplay actions remain inactive.

## Source and licensing

The source archive contains GPL-3.0 license and `NOTICE.md`. w0uFF Together includes new w0uFF-specific work and reuses GPL-licensed UnchainedTogether code originally by xiaoxiao921. Its source and attribution are retained according to the original license, together with notices for bundled third-party dependencies.

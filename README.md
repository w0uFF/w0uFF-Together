# w0uFF Together

**An updated and expanded version of [UnchainedTogether](https://github.com/xiaoxiao921/UnchainedTogether) by [xiaoxiao921](https://github.com/xiaoxiao921), with new features and improvements for Chained Together.**

w0uFF Together includes a **Practice Studio** for practicing cuts, moving around the map and exploring experimental Race features.

**Platform:** Windows x64 · DirectX 12 · Chained Together 1.11.2

> [!IMPORTANT]
> This mod is in active development. Some features, especially multiplayer Race mode, are experimental and are not yet fully playable.

## Installation

1. **Download `d3d12.dll`** from the [latest GitHub Release](https://github.com/w0uFF/w0uFF-Together/releases).  
   **Direct download (dev-32-1):** [d3d12.dll](https://github.com/w0uFF/w0uFF-Together/releases/download/dev-32-1/d3d12.dll)
2. Open your Chained Together game folder:
   ```text
   Steam\steamapps\common\ChainedTogether\ChainedTogether\Binaries\Win64
   ```
   In Steam, you can find the game directory via **Chained Together → Properties → Installed Files → Browse**.
3. **Copy `d3d12.dll` into the `Win64` folder.** If the file already exists, back it up before replacing it.
4. Launch the game normally through Steam.
5. Press **`INSERT`** in-game to open the w0uFF Together menu.

> [!NOTE]
> This direct download is automatically updated after each successful development Release. Previous builds remain available from the [Releases page](https://github.com/w0uFF/w0uFF-Together/releases).

## Features

- **Practice / checkpoints:** Save positions and teleport back to practice difficult cuts. Development builds also include work on custom checkpoint management.
- **Fly mode:** Toggle Fly with **`F7`**. Development builds introduce adjustable speed, mouse-wheel control, and camera-independent vertical movement (**`Space`** up / **`Ctrl`** down).
- **Free camera and visuals:** Additional exploration and diagnostic tools, configurable from the mod menu.
- **Race mode (experimental):** Early lobby and Steam networking work, plus a solo timer prototype. Chain detachment, authoritative competitive starts and multiplayer race results are **not yet fully implemented**.

Features may vary by development release. Check the release notes and verify new controls in-game before relying on them.

## Controls

| Key | Action |
| --- | --- |
| `INSERT` | Open or close the mod menu |
| `F7` | Toggle Fly (when the module is enabled) |
| `F8` | Repeat the selected calibrated cut (when Teleport is enabled) |
| `Space` / `Ctrl` | Fly up / down in builds that include the updated Fly controls |

Modules can be enabled or disabled under **`INSERT → Settings`**.

## Builds and downloads

Windows development builds are published through **[GitHub Releases](https://github.com/w0uFF/w0uFF-Together/releases)** after automated checks succeed. A prerelease may include:

- `d3d12.dll` — Windows x64 mod binary
- `rebuilt-source.zip` — reconstructed GPL source
- `SHA256SUMS.txt` — hashes for file integrity verification

**A successful automated build does not mean every feature has been verified in-game.** Development releases are marked experimental.

## Credits and licensing

w0uFF Together is based on **[UnchainedTogether](https://github.com/xiaoxiao921/UnchainedTogether)**, originally created by **xiaoxiao921**. Thanks to the original author for making this project possible.

w0uFF Together retains and extends GPL-licensed code from the original project and includes additional work by w0uFF. See the repository's `LICENSE` and `NOTICE.md` for the GPL-3.0 terms, attribution and notices for included third-party components.

## Feedback and issues

Found a bug or a broken feature? [Open a GitHub issue](https://github.com/w0uFF/w0uFF-Together/issues) and include the release tag, game version, steps to reproduce and any relevant screenshots or error messages.

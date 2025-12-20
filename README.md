# MultiCAD

**MultiCAD** is a universal graphics DLL replacement for **Sudden Strike**, **Sudden Strike Forever**, and related games.
It supports **any custom screen resolution** from 640x480 up to 3440x1600  (note: higher resolutions may be a bit laggy) and includes various bug fixes across different game versions.

## Supported Games

| Game                           | Status | Versions / Languages      | Fixes       |
|--------------------------------|--------|---------------------------|-------------|
| **Sudden Strike**              |   ✔   | 1.0: de, ru, 1.2: en      | 2 bug fixes |
| **Sudden Strike HD v1.1**      |   ✔   | en, ru                    | 2 bug fixes |
| **Sudden Strike Forever**      |   ✔   | en, de, fr, ru, ch        | 7 bug fixes |
| **Sudden Strike Gold**         |   ✔   | en, de, fr, ru            | 7 bug fixes |
| **Sudden Strike Gold HD v1.2** |   ✔   | en, de, fr, ru            | 7 bug fixes |
| **APRM**                       |   ✔   | 3.0, 3.1, 4.0             | 7 bug fixes |
| **TWO**                        |   ✔   | en                        | 7 bug fixes |

> 💡 Note: `Audio Mixer Zero-Volume Fix` restores the game volume in the audio mixer to full if it was set to zero. Applies to **all versions**.

## Installation

1. Download the latest precompiled `cadMulti.dll` file from the [Releases](../../releases/latest) page.
2. Place `cadMulti.dll` into the folder containing the original `cad*.dll` files (typically the game directory).
3. Open `sudtest.ini` and set **at least one** `SSDraw` entry to `cadMulti.dll`. Example:
> ```ini
> [Game]
> SSDraw1=cad640.dll
> SSDraw2=cad1024.dll
> SSDraw3=cadMulti.dll
> ```
4. Launch the game.

> 💡 Note: If the game fails to launch or closes immediately with no error, it usually means the required Microsoft Visual C++ runtime is missing. You have two options to fix this:
> 1. Install the Microsoft Visual C++ 2015-2022 Redistributable: [vc_redist.x86.exe](https://aka.ms/vc14/vc_redist.x86.exe).
> 2. Alternatively, use [cadMulti_mt.dll](../../releases/latest), which works without installing any additional runtime (slightly larger than cadMulti.dll)

## Configuration

### Resolution Setup

By default, the game will launch in 1920x1080.
To use a custom resolution, add a `Resolution` line **anywhere after** the `[Game]` header in `sudtest.ini`:
> ```ini
> [Game]
> Resolution=1600x900
> SSDraw1=cad640.dll
> SSDraw2=cad1024.dll
> SSDraw3=cadMulti.dll
> ```
Restart the game to apply the change.

## Compilation

To build the project from source:

### Requirements

- Windows 10 or 11
- Visual Studio 2022 or later
- Visual C++ build tools

### Steps

1. Clone or download this repository.
2. Open `MultiCAD.sln` in **Visual Studio 2022**.
3. Choose the `Release`, `Release_MT` or `Debug` configuration.
4. Build the solution.

## License

This project is licensed under the MIT License — see the [LICENSE](LICENSE) file for details.

## Contact

For author information and ways to get in touch, see the [Contact Information](CONTACT.md) file.
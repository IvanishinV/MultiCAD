# MultiCAD

**MultiCAD** is a universal graphics DLL replacement for **Sudden Strike**, **Sudden Strike Forever**, and related games.
It supports **any custom screen resolution** from 640x480 up to 2560x1440  (note: higher resolutions may be a bit laggy) and includes various bug fixes across different game versions.

## Supported Games

| Game                     | Status | Versions / Languages      | Fixes       |
|--------------------------|--------|---------------------------|-------------|
| **Sudden Strike**        |   🛠️   |                           |             |
| **Sudden Strike Forever**|   ✔   | en, de, fr, ru, ch        | 6 bug fixes |
| **Sudden Strike Gold**   |   ✔   | en, de, fr, ru            | 6 bug fixes |
| **Sudden Strike HD v1.2**|   ✔   | en, de, fr, ru            | 6 bug fixes |
| **APRM**                 |   ✔   | 3.0, 3.1, 4.0             | 6 bug fixes |
| **TWO**                  |   ✔   | en                        | 6 bug fixes |

> 💡 Note: `Audio Mixer Zero-Volume Fix` restores the game volume in the audio mixer to full if it was set to zero. Applies to **all versions**.

## Installation

1. Build the project or download the latest precompiled `cadMulti.dll` file from the [Releases](../../releases) page.
2. Place `cadMulti.dll` into the folder containing the original `cad*.dll` files (typically the game directory).
3. Open `sudtest.ini` and set **at least one** `SSDraw` entry to `cadMulti.dll`. Example:
> ```ini
> [Game]
> SSDraw1=cad640.dll
> SSDraw2=cad1024.dll
> SSDraw3=cadMulti.dll
> ```
4. Launch the game 

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
3. Choose the `Release` or `Debug` configuration.
4. Build the solution.

## License

This project is licensed under the MIT License — see the [LICENSE](LICENSE) file for details.

## Contact

For author information and ways to get in touch, see the [Contact Information](CONTACT.md) file.
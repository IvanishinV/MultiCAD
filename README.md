# MultiCAD

**MultiCAD** is a universal graphics DLL replacement for **Sudden Strike**, **Sudden Strike Forever**, and related games.
It supports **any custom screen resolution** from 640x480 up to 3840x2160 (4K) — note that higher resolutions may be a bit laggy — and includes various bug fixes across different game versions.

## Supported Games

### Original Games

| Game                             | Status | Versions / Languages  | Fixes       |
|----------------------------------|--------|-----------------------|-------------|
| **Sudden Strike**                |   ✔   | 1.0: de, ru, 1.2: en  | 2 bug fixes |
| **Sudden Strike Forever**        |   ✔   | en, de, fr, ru, ch    | 7 bug fixes |
| **Sudden Strike Gold**           |   ✔   | en, de, fr, ru        | 7 bug fixes |
| **Sudden Strike 2**              |   ✔   | 2.2                   | 6 bug fixes |
| **Sudden Strike: Resource War**  |   ✔   | 2.3, 2.4              | 6 bug fixes |

### Red Ice Team Games

| Game                             | Status | Fixes       |
|----------------------------------|--------|-------------|
| **Black Gold**                   |   ✔   | 6 bug fixes |
| **Black Sea**                    |   ✔   | 6 bug fixes |
| **Cold War Conflicts**           |   ✔   | 6 bug fixes |
| **Europe 2015**                  |   ✔   | 6 bug fixes |
| **Gulf War**                     |   ✔   | 6 bug fixes |

### Mods

| Game                             | Status | Versions / Languages  | Fixes       |
|----------------------------------|--------|-----------------------|-------------|
| **Sudden Strike HD v1.1**        |   ✔   | en, ru                | 2 bug fixes |
| **Sudden Strike Gold HD v1.2**   |   ✔   | en, de, fr, ru        | 7 bug fixes |
| **APRM**                         |   ✔   | 3.0, 3.1, 4.0         | 7 bug fixes |
| **AXPRM**                        |   ✔   | 2.0                   | 7 bug fixes |
| **TWO**                          |   ✔   | en                    | 7 bug fixes |
| **Eastern Front Mod**            |   ✔   |                       | 6 bug fixes |
| **Hidden Stroke 2 APRM**         |   ✔   |                       | 6 bug fixes |
| **Hidden Stroke 2 Fusion**       |   ✔   |                       | 6 bug fixes |
| **Hidden Stroke 2 Resource War** |   ✔   |                       | 6 bug fixes |
| **Hidden Stroke 3**              |   ✔   |                       | 6 bug fixes |
| **Hidden Stroke 4**              |   ✔   |                       | 6 bug fixes |
| **Liberation Mod**               |   ✔   | 2.75, 5.1, 5.3        | 6 bug fixes |
| **LRM**                          |   ✔   | 5.1                   | 6 bug fixes |
| **MWM 3**                        |   ✔   |                       | 6 bug fixes |
| **Neddus Stroke**                |   ✔   |                       | 6 bug fixes |
| **PWM**                          |   ✔   | 2.0, 3.0              | 6 bug fixes |
| **RCM**                          |   ✔   | 2.7                   | 6 bug fixes |
| **RWM 6.x**                      |   ✔   | 6.5, 6.6, 6.71, 6.8   | 6 bug fixes |
| **RWM 8.x**                      |   ✔   | 8.0, 8.5              | 6 bug fixes |
| **RWG Truth of War**             |   ✔   | en, de, fr, ru        | 6 bug fixes |
| **Vietnam Project**              |   ✔   | 1.0, 1.1, 1.2         | 6 bug fixes |
| **Warzone 2**                    |   ✔   |                       | 6 bug fixes |
| **World at War**                 |   ✔   | 0.5                   | 6 bug fixes |

> 💡 Note: `Audio Mixer Zero-Volume Fix` restores the game volume in the audio mixer to full if it was set to zero. Applies to **all versions**.

## Installation

1. Download the latest precompiled `cadMulti.dll` file from the [Releases](../../releases/latest) page.
2. Place `cadMulti.dll` into the folder containing the original `cad*.dll` files (typically the game directory).
3. Open the game's ini in the game folder — `sudtest.ini` for Sudden Strike, or `gulfwar.ini`, `blackgold.ini`, `blacksea.ini`, `euro2015.ini` for the Confrontation titles — and set **at least one** `SSDraw` entry to `cadMulti.dll`. Example:
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

The game launches at your desktop resolution by default — no configuration needed.

To set a specific resolution, add a `Resolution` line **anywhere after** the `[Game]` header in the game's ini:
> ```ini
> [Game]
> Resolution=1600x900
> SSDraw1=cad640.dll
> SSDraw2=cad1024.dll
> SSDraw3=cadMulti.dll
> ```
Restart the game to apply the change.

> 💡 Note: `Resolution` must be between 640x480 and 3840x2160, with a height divisible by 8 — a renderer requirement. Out-of-range values are ignored with a message.
> A height that isn't divisible by 8, or any mode your display doesn't report, brings up a picker listing the supported modes and saves your choice back to the ini. With no `Resolution` line set, the game uses your desktop resolution with the height rounded down to a multiple of 8.

### UI Toggle Hotkey

You can temporarily disable or enable the in-game UI overlay by pressing:

**Alt + Y**

This can be useful when taking screenshots or when the UI interferes with gameplay.

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
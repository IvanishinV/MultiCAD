# MultiCAD

**MultiCAD** is a universal graphics DLL replacement for **Sudden Strike**, **Sudden Strike Forever**, and other derivative games. It adds support for **any custom screen resolution**.

## Features

This DLL supports arbitrary screen resolutions for the following games:

- Sudden Strike *(WIP)*
- Sudden Strike Forever *(WIP)*
- Sudden Strike Gold:
  - en (Steam) — ✔
  - de (Steam) — ⏳ *(WIP)*
  - fr (Steam) — ⏳ *(WIP)*
  - ru (Steam) — ⏳ *(WIP)*
- Sudden Strike HD v1.2:
  - en ⏳ — *(WIP)*
  - de ⏳ — *(WIP)*
  - fr ⏳ — *(WIP)*
  - ru ⏳ — *(WIP)*
- *(More titles and mods to be added)*

The DLL supports native screen resolutions from 640x480 to 2560x1440 (a bit laggy).

## Usage

1. Build or download the `cadMulti.dll` file.
2. Place the file in the game folder (same directory as the game executable).
3. Open `sudtest.ini` and set the value of any existing *SSDraw* variable to `cadMulti.dll`.
4. After launching the game, in the main menu, select the required resolution option depending on the *SSDraw* number.
5. By default, the game will launch in 1920x1080 resolution. If this is not supported, the game will offer to change it.

> ⚠️ If you want to specify a different resolution, add a new line before *SSDraw* specifying the desired resolution, e.g.: `Resolution=1600x900` and rerun the game.

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
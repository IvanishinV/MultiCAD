# MultiCAD

**MultiCAD** is a universal graphics DLL replacement for **Sudden Strike**, **Sudden Strike Forever**, and related games. It supports **any custom screen resolution** from 640x480 up to 2560x1440  (note: higher resolutions may be a bit laggy) and includes various bug fixes across different game versions.

## Supported Games

- Sudden Strike *(WIP)*
- Sudden Strike Forever *(WIP)*
- Sudden Strike Gold (5 bug fixes):
  - en (Steam) ✔
  - de (Steam) ⏳ *(WIP)*
  - fr (Steam) ⏳ *(WIP)*
  - ru (Steam) ⏳ *(WIP)*
- Sudden Strike HD v1.2 (5 bug fixes):
  - en ✔
  - de ✔
  - fr ✔
  - ru ✔
- *(More titles and mods to be added)*

## Usage

1. Build the project or download the latest precompiled `cadMulti.dll` file from the [Releases](../../releases) page.
2. Place the file in the game folder (same directory as the game executable).
3. Open `sudtest.ini` and set any existing *SSDraw* variable to `cadMulti.dll`.
4. After launching the game, in the main menu, select the required resolution option depending on the changed *SSDraw* number.
5. By default, the game will launch in 1920x1080. If this resolution is not supported, the game will offer to change it.

> ⚠️ To use a custom resolution, add a `Resolution` line **anywhere after the [Game] header** in `sudtest.ini` and make sure **at least one `SSDraw` entry** is set to `cadMulti.dll`:
> 
> ```ini
> [Game]
> Resolution=1600x900   ; optional, change to a supported resolution
> SSDraw1=cad640.dll
> SSDraw2=cad1024.dll
> SSDraw3=cadMulti.dll  ; required
> ```
> Restart the game to apply the change.

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
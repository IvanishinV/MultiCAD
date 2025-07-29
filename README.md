# MultiCAD

**MultiCAD** is a universal graphics DLL replacement for **Sudden Strike**, **Sudden Strike Forever**, and other derivative games. It adds support for **any custom screen resolution**.

## Features

This DLL supports arbitrary screen resolutions for the following games:

- Sudden Strike *(WIP)*
- Sudden Strike Forever *(WIP)*
- Sudden Strike Gold *(WIP)*
- (More titles and mods to be added)

## Usage

1. Build or download the `cadMulti.dll` file.
2. Place the file in the game folder (same directory as the game executable).
3. Open `sudtest.ini` and set the value of any *SSDraw* variable to `cadMulti.dll`.
4. Select the appropriate resolution option from the main menu.
5. By default, the game will launch using your current desktop screen resolution.

> ⚠️ If you want to override the resolution, support for custom settings will be added in future updates.

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
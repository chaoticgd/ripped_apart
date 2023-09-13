# Changelog

## v2.0.3

- The texture metadata stored in the table of contents is now regenerated for modified textures.
- Fixed a bug where too many mods loaded at once would cause mod loading to fail.
- The mod list is now sorted case insensitively.

## v2.0.2

- Fixed installation of certain headerless assets (e.g. high-resolution textures).
- Create cache files at mod installation time instead of while loading the mod list.
- Automatically delete old cache files.
- Fixed file handle leak when loading the mod list.

## v2.0.1

- Fixed a crash when the mods folder is deleted while the mod manager is running.
- Fixed message boxes sometimes not being displayed correctly on Linux.

## v2.0

- Added a mod manager for the PC version.
- Added a command line tool to extract files from the PC version.
- Lots more new development tools and other improvements.

## v1.4

- Added support for BC6 HDR textures (that are exported to .exr files).
- The texture decompression library bc7enc has been replaced with bcdec.

## v1.3

- Added support for BC3 and BC5 textures.
- Fixed a bug where certain BC7 blocks were left uninitialized.

## v1.2

- Added support for R8G8 format textures.
- Non-power-of-two textures should actually work now.
- The executable files are now stripped (which has reduced the file size).

## v1.1

- Support for R8G8B8A8, R8 and BC4 texture formats.
- Support for texture which have a width or height that is not a multiple of 256x256.
- Support for non-power-of-two textures.
- Print out metadata (format, width and height) for each texture in a directory and all its sub-directories with the new igfile tool.

## v1.0

- Initial release.

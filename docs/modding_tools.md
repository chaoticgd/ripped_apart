# Modding Tools

These are all command line tools.

## extractall

Extract all the games files from the archives.

Command line usage:

```
./bin/extractall <game directory> <output directory>
```

## rebuildtoc

Installs mods from the command line. Currently supports the .stage and .rcmod formats.

Command line usage:

```
./bin/rebuildtoc <game directory>
```

Mods should be placed in `<game directory>/mods` e.g. `/ra/steamapps/common/Ratchet & Clank - Rift Apart/mods`.

## superswizzle

**Not for use with the PC version!**

Converts .texture files to .png (or .exr in the case of HDR textures), automatically handling decompression and unswizzling. Can be used by dragging a texture onto the .exe file from within Windows explorer.

Supported texture formats:

- BC1
- BC3 (since v1.3)
- BC4 (since v1.1)
- BC5 (since v1.3)
- BC6 (since v1.4)
- BC7
- R8 (since v1.1)
- R8G8 (since v1.2)
- R8G8B8A8 (since v1.1)

Command line usage:

```
./bin/superswizzle <.texture input path>
./bin/superswizzle <.texture input path> <.png output path>
./bin/superswizzle <.texture input path> <.stream input path> <.png output path>
```

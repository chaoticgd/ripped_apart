# Documentation

## Modding Tools

### extractall

Extract all the games files from the archive.

Command line usage:

```
./bin/extractall <game directory> <output directory>
```

### modeleditor

Very unfinished tool to view model files.

### rebuildtoc

Installs mods. Currently supports the .rcmod format.

Command line usage:

```
./bin/rebuildtoc <game directory>
```

Mods should be placed in `<game directory>/mods` e.g. `/ra/steamapps/common/Ratchet & Clank - Rift Apart/mods`.

### superswizzle

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

## Development Tools

### brutecrc

Brute force the CRC32 hashes these games use for various things.

### dag

A tool for working with dependency DAG files.

### dsar

A tool for working with DSAR archives.

### genlumptypes

Generate the lump_types.h file from the lump_types.txt file (in the libra/ directory).

### igfile

Prints out metadata about the specified asset or all the assets in a directory.

Command line usage:

```
./bin/igfile <input paths>
```

### libra

Library containing some common code such as a DAT1 file parser and texture stuff.

# Ripped Apart

Ratchet & Clank: Rift Apart modding stuff.

## Downloads

See the [Releases](https://github.com/chaoticgd/ripped_apart/releases) page.

## superswizzle

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

	./superswizzle <.texture input path>
	./superswizzle <.texture input path> <.png output path>
	./superswizzle <.texture input path> <.stream input path> <.png output path>

## igfile

Prints out metadata about the specified asset or all the assets in a directory.

Command line usage:

	./igfile <input paths>

## libra

Library containing some common code such as a DAT1 file parser and texture stuff.

## Building from source
	
	git clone https://github.com/chaoticgd/ripped_apart
	cd ripped_apart
	cmake -S . -B bin/
	cmake --build bin/

## Credit

Some of the swizzle tables were originally reverse engineered by Slipsy.

The [bcdec](https://github.com/iOrange/bcdec) library is used for texture decompression and the [lodepng](https://github.com/lvandeve/lodepng) library is used for PNG encoding.

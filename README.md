# Ripped Apart

Ratchet & Clank: Rift Apart modding stuff.

## Downloads

See the [Releases](https://github.com/chaoticgd/ripped_apart/releases) page.

## superswizzle

Converts .texture files to .png, automatically handling decompression and unswizzling. Can be used by dragging a texture onto the .exe file from within Windows explorer.

Command line usage:

	./superswizzle <.texture input path>
	./superswizzle <.texture input path> <.png output path>
	./superswizzle <.texture input path> <.stream input path> <.png output path>

## igfile

Prints out metadata about the specified asset or all the assets in a directory.

Commane line usage:

	./igfile <input paths>

## libra

Library containing some common code such as a DAT1 file parser and texture stuff.

## Building from source
	
	git clone --recursive https://github.com/chaoticgd/ripped_apart
	cd ripped_apart
	cmake -S . -B bin/
	cmake --build bin/

## Credit

Some of the swizzle tables were originally reverse engineered by Slipsy.

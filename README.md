# Ripped Apart

## superswizzle

Converts .texture files to .png, automatically handling BC1/BC7 decompression and unswizzling. Only works for textures where the width and height are multiples of 256. Can be used by dragging a texture on the .exe file from within Windows explorer.

Command line options:

	./superswizzle <.texture input path>
	./superswizzle <.texture input path> <.stream input path> <.png output path>

## Building from source
	
	git clone --recursive https://github.com/chaoticgd/ripped_apart
	cd ripped_apart
	cmake -S . -B bin/
	cmake --build bin/

## Credit

The swizzle tables were reverse engineered by Slipsy.

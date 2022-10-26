add_executable(superswizzle
	superswizzle.c
	decode.cpp
	../thirdparty/bc7enc/bc7decomp.cpp
	../thirdparty/bc7enc/lodepng.cpp
)
include_directories(superswizzle ../thirdparty/zlib ../thirdparty/libpng)

add_executable(superswizzle
	superswizzle.c
	decode.cpp
	../thirdparty/bc7enc/bc7decomp.cpp
	../thirdparty/bc7enc/lodepng.cpp
)
include_directories(superswizzle ../thirdparty/zlib ../thirdparty/libpng)

if(UNIX)
	set(GCC_FLAGS -Wall -Wno-sign-compare -g)
	target_compile_options(superswizzle PRIVATE ${GCC_FLAGS} -O3)
endif()
if(MSVC)
	target_compile_options(superswizzle PRIVATE /W4 /EHsc /MP)
endif()

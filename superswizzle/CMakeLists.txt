add_executable(superswizzle
	superswizzle.c
	decode.cpp
	../thirdparty/bc7enc/bc7decomp.cpp
)
include_directories(superswizzle ../thirdparty/zlib ../thirdparty/libpng)
target_link_libraries(superswizzle png_static zlibstatic)

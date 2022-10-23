add_executable(superswizzle
	superswizzle.c
	decode.cpp
)
include_directories(superswizzle ../thirdparty/zlib ../thirdparty/libpng)
target_link_libraries(superswizzle png_static zlibstatic)

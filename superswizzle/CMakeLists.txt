add_executable(superswizzle
	superswizzle.c
	../thirdparty/lodepng/lodepng.c
)
target_link_libraries(superswizzle libra)

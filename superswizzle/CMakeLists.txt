add_executable(superswizzle
	superswizzle.c
	../thirdparty/lodepng/lodepng.c
)
target_link_libraries(superswizzle libra)

if(UNIX)
	set(GCC_FLAGS -Wall -Wno-sign-compare -g)
	target_compile_options(superswizzle PRIVATE ${GCC_FLAGS} -O3)
endif()
if(MSVC)
	target_compile_options(superswizzle PRIVATE /W4 /EHsc /MP)
endif()

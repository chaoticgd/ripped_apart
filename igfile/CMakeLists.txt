add_executable(igfile
	igfile.cpp
)
target_link_libraries(igfile libra)

if(UNIX)
	set(GCC_FLAGS -Wall -Wno-sign-compare -g)
	target_compile_options(igfile PRIVATE ${GCC_FLAGS} -O3)
endif()
if(MSVC)
	target_compile_options(igfile PRIVATE /W4 /EHsc /MP)
endif()

project(ripped_apart)
cmake_minimum_required(VERSION 3.14)

if(BUILD_STATIC)
	set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
	set(BUILD_SHARED_LIBS OFF)
	set(CMAKE_EXE_LINKER_FLAGS "-static")
endif()

add_subdirectory(superswizzle)
add_subdirectory(thirdparty)

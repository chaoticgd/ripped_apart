project(ripped_apart)
cmake_minimum_required(VERSION 3.14)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
	message(FATAL_ERROR "In-tree build detected. You should do an out-of-tree build instead:\n\tcmake -S . -B build/")
endif()

# Output all the binaries into the root of the build directory.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()

if(BUILD_STATIC)
	set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
	set(BUILD_SHARED_LIBS OFF)
	set(CMAKE_EXE_LINKER_FLAGS "-static")
endif()

add_subdirectory(thirdparty)
include_directories(thirdparty/linmath)

include_directories(.)

add_subdirectory(libra)
add_subdirectory(superswizzle)
add_subdirectory(modeleditor)
add_subdirectory(igfile)

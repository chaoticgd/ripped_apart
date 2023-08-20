project(ripped_apart)
cmake_minimum_required(VERSION 3.14)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
	message(FATAL_ERROR "In-tree build detected. You should do an out-of-tree build instead:\n\tcmake -S . -B build/")
endif()

# Output all the binaries into the root of the bin directory.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")

include(cmake/version_finder.cmake)

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

if(UNIX)
	add_compile_options(-Wall -Wno-sign-compare -g -O3)
elseif(MSVC)
	include_directories(thirdparty/dirent)
	add_compile_options(/W4 /EHsc /MP)
endif()

# Libraries
add_subdirectory(crc)
add_subdirectory(libra)
add_subdirectory(gui)
# Executables
add_subdirectory(superswizzle)
add_subdirectory(modeleditor)
add_subdirectory(igfile)
add_subdirectory(igcrc)
if(UNIX)
	add_subdirectory(test)
endif()
add_executable(dsar dsar.c)
target_link_libraries(dsar libra)
add_executable(dag dag.c)
target_link_libraries(dag libra)
add_executable(genlumptypes genlumptypes.c)
target_link_libraries(genlumptypes crc)
add_executable(extractall extractall.c)
target_link_libraries(extractall libra)
add_executable(printlumpcrcs printlumpcrcs.cpp)
target_link_libraries(printlumpcrcs libra)
add_executable(brutecrc brutecrc.c)
target_link_libraries(brutecrc libra)
add_executable(toc toc.c)
target_link_libraries(toc libra)
add_executable(rebuildtoc rebuildtoc.c)
target_link_libraries(rebuildtoc libra)
add_executable(diffdat diffdat.c)
target_link_libraries(diffdat libra)

# Set the windows codepage to UTF-8.
if(WIN32)
	target_sources(superswizzle PUBLIC ripped_apart.manifest)
	target_sources(igfile PUBLIC ripped_apart.manifest)
	target_sources(igcrc PUBLIC ripped_apart.manifest)
	target_sources(dsar PUBLIC ripped_apart.manifest)
	target_sources(dag PUBLIC ripped_apart.manifest)
	target_sources(extractall PUBLIC ripped_apart.manifest)
	target_sources(toc PUBLIC ripped_apart.manifest)
	target_sources(rebuildtoc PUBLIC ripped_apart.manifest)
	target_sources(diffdat PUBLIC ripped_apart.manifest)
endif()

set(RELEASE_FILES
	$<TARGET_FILE:superswizzle>
	$<TARGET_FILE:igfile>
	$<TARGET_FILE:igcrc>
	$<TARGET_FILE:dsar>
	$<TARGET_FILE:dag>
	$<TARGET_FILE:extractall>
	$<TARGET_FILE:toc>
	$<TARGET_FILE:rebuildtoc>
	$<TARGET_FILE:diffdat>
)
set(RELEASE_DOCS_DIRECTORY "${CMAKE_SOURCE_DIR}/docs")
set(RELEASE_LICENSES_DIRECTORY "${CMAKE_SOURCE_DIR}/licenses")
include(cmake/release_crafter.cmake)

add_library(glad glad/include/glad/glad.h glad/src/glad.c)
target_include_directories(glad PUBLIC glad/include/)
set(GLFW_BUILD_EXAMPLES OFF)
set(GLFW_BUILD_TESTS OFF)
set(GLFW_BUILD_DOCS OFF)
set(GLFW_INSTALL OFF)
add_subdirectory(glfw)
add_subdirectory(lz4/build/cmake)

file(GLOB IMGUI_SOURCES
	cimgui/cimgui.cpp
	cimgui/imgui/imconfig.h
	cimgui/imgui/imgui_demo.cpp
	cimgui/imgui/imgui_draw.cpp
	cimgui/imgui/imgui_internal.h
	cimgui/imgui/imgui_tables.cpp
	cimgui/imgui/imgui_widgets.cpp
	cimgui/imgui/imgui.cpp
	cimgui/imgui/imgui.h
	cimgui/imgui/imstb_rectpack.h
	cimgui/imgui/imstb_textedit.h
	cimgui/imgui/imstb_truetype.h
	cimgui/imgui/misc/cpp/imgui_stdlib.cpp
	cimgui/imgui/backends/imgui_impl_glfw.cpp
	cimgui/imgui/backends/imgui_impl_opengl3.cpp
)

add_library(cimgui STATIC ${IMGUI_SOURCES})

target_compile_definitions(cimgui PUBLIC IMGUI_DISABLE_OBSOLETE_FUNCTIONS=1)
if(WIN32)
	target_compile_definitions(cimgui PUBLIC IMGUI_IMPL_API=extern\t\"C\"\t__declspec\(dllexport\))
	list(APPEND IMGUI_LIBRARIES imm32)
else(WIN32)
	target_compile_definitions(cimgui PUBLIC IMGUI_IMPL_API=extern\t\"C\"\t)
endif(WIN32)

target_include_directories(cimgui PUBLIC cimgui/)
target_include_directories(cimgui PUBLIC cimgui/imgui/)
target_include_directories(cimgui PUBLIC cimgui/generator/output/)
set_target_properties(cimgui PROPERTIES PREFIX "")
target_link_libraries(cimgui glfw ${IMGUI_LIBRARIES})

add_subdirectory(gdeflate)

# zlib
set(SKIP_INSTALL_ALL ON)
add_subdirectory(zlib)
include_directories(zlib)
include_directories(${CMAKE_BINARY_DIR}/thirdparty/zlib)
set(ZLIB_FOUND TRUE)
set(ZLIB_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/thirdparty/zlib ${CMAKE_BINARY_DIR}/thirdparty/zlib)
# ZLIB_INCLUDE_DIRS is used by genout.cmake.in from libpng.
set(ZLIB_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/thirdparty/zlib ${CMAKE_BINARY_DIR}/thirdparty/zlib)
if(WIN32)
	# ZLIB_LIBRARY is used by libzip and must be set at configuration time hence
	# we copy the library file into a fixed location.
	add_custom_target(copy_zlib_lib_hack
		COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:zlibstatic>" "${CMAKE_CURRENT_BINARY_DIR}/zlib.lib"
		BYPRODUCTS "${CMAKE_CURRENT_BINARY_DIR}/zlib.lib"
	)
	set(ZLIB_LIBRARY "${CMAKE_CURRENT_BINARY_DIR}/zlib.lib")
endif()

# libzip
set(LIBZIP_DO_INSTALL OFF CACHE BOOL "")
set(ENABLE_COMMONCRYPTO OFF CACHE BOOL "")
set(ENABLE_GNUTLS OFF CACHE BOOL "")
set(ENABLE_MBEDTLS OFF CACHE BOOL "")
set(ENABLE_WINDOWS_CRYPTO OFF CACHE BOOL "")
set(ENABLE_OPENSSL OFF CACHE BOOL "")
set(ENABLE_BZIP2 OFF CACHE BOOL "")
set(ENABLE_LZMA OFF CACHE BOOL "")
set(ENABLE_ZSTD OFF CACHE BOOL "")
set(BUILD_TOOLS OFF CACHE BOOL "")
set(BUILD_REGRESS OFF CACHE BOOL "")
set(BUILD_EXAMPLES OFF CACHE BOOL "")
set(BUILD_DOC OFF CACHE BOOL "")
add_subdirectory(libzip)
if(WIN32)
	add_dependencies(zip copy_zlib_lib_hack)
endif()

# json-c
cmake_policy(SET CMP0002 NEW)
add_subdirectory(json-c)

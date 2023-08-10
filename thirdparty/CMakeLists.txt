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

add_executable(modeleditor
	modeleditor.c
)
target_link_libraries(modeleditor libra)
target_link_libraries(modeleditor glfw glad cimgui)
target_compile_definitions(modeleditor PUBLIC -DCIMGUI_USE_OPENGL3 -DCIMGUI_USE_GLFW)

if(UNIX)
	set(GCC_FLAGS -Wall -Wno-sign-compare -g)
	target_compile_options(modeleditor PRIVATE ${GCC_FLAGS} -O3)
endif()
if(MSVC)
	target_compile_options(modeleditor PRIVATE /W4 /EHsc /MP)
endif()

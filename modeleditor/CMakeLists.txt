add_executable(modeleditor
	modeleditor.c
	renderer.c
)
target_link_libraries(modeleditor libra)
target_link_libraries(modeleditor glfw glad cimgui)
target_compile_definitions(modeleditor PUBLIC -DCIMGUI_USE_OPENGL3 -DCIMGUI_USE_GLFW)

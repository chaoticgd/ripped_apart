add_library(gui STATIC
	common.c
	common.h
	menu.c
	menu.h
)
target_link_libraries(gui libra glfw glad cimgui)
target_compile_definitions(gui PUBLIC -DCIMGUI_USE_OPENGL3 -DCIMGUI_USE_GLFW)

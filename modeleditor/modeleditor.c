#include <stdio.h>
#include <stdlib.h>
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include <glad/glad.h>
#include <cimgui.h>
#include <cimgui_impl.h>
#include <GLFW/glfw3.h>
#include "../libra/util.h"

GLFWwindow* window;

static void startup();
static void shutdown();

int main(int argc, char** argv) {
	startup();
	while(!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		igNewFrame();
		igShowDemoWindow(NULL);
		igRender();
		glfwMakeContextCurrent(window);
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		glViewport(0, 0, width, height);
		glClearColor(0, 0, 0, 0);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(igGetDrawData());
		glfwSwapBuffers(window);
	}
	shutdown();
}

static void startup() {
	if(!glfwInit()) {
		fprintf(stderr, "Failed to load GLFW.\n");
		abort();
	}
	
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	
	window = glfwCreateWindow(1280, 720, "Ripped Apart Model Editor", NULL, NULL);
	if(!window) {
		fprintf(stderr, "Failed to open GLFW window.\n");
		abort();
	}
	
	glfwMakeContextCurrent(window);
	
	glfwSwapInterval(1);
	
	if(gladLoadGL() == 0) {
		fprintf(stderr, "Failed to load OpenGL.\n");
		abort();
	}
	
	igCreateContext(NULL);
	
	ImGui_ImplGlfw_InitForOpenGL(window, true);
 	ImGui_ImplOpenGL3_Init("#version 430");
	
	igStyleColorsDark(NULL);
}

static void shutdown() {
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	igDestroyContext(NULL);
	
	glfwDestroyWindow(window);
	glfwTerminate();
}

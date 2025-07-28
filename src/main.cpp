#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "CHIP8.h"

int main() {
  CHIP8 Chip8;
  
  // Step 1: setup graphics and input systems
  GLFWwindow* window;
  if(!glfwInit()) {
    std::cout << "GLFW couldn't start" << std::endl;
    return -1;
  }

  window = glfwCreateWindow(640, 480, "EMUL-8 DevBuild", NULL, NULL);
  glfwMakeContextCurrent(window);

  if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    glfwTerminate();
    return -1;
  }

  // Step 2: Initialize system and load program
  Chip8.initialization();
  int programLoaded = Chip8.loadProgram();
  if(programLoaded != 0) {
    std::cout << "Error Accessing ROM" << std::endl;
    return -1;
  }

  // Step 3: Loop CPU cycles
  glClearColor(0.25f, 0.5f, 0.75f, 1.0f);
  while(!glfwWindowShouldClose(window)) {
    glfwPollEvents();
    glClear(GL_COLOR_BUFFER_BIT);
    glfwSwapBuffers(window);
  }
  glfwTerminate();
  return 0;
}
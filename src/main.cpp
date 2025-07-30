#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "CHIP8.h"

CHIP8 Chip8;

const int keyMap[] = {
  GLFW_KEY_1, GLFW_KEY_2, GLFW_KEY_3, GLFW_KEY_4,
  GLFW_KEY_Q, GLFW_KEY_W, GLFW_KEY_E, GLFW_KEY_R,
  GLFW_KEY_A, GLFW_KEY_S, GLFW_KEY_D, GLFW_KEY_F,
  GLFW_KEY_Z, GLFW_KEY_X, GLFW_KEY_C, GLFW_KEY_V,
};

// Updates keypadState. Runs everytime user interacts with keyboard.
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
  bool keyIsPressedDown = !(action == GLFW_RELEASE);

  int i = 0;
  for(int k : keyMap) {
    if(k == key) {
      Chip8.keypadState[i] = static_cast<unsigned char>(keyIsPressedDown);
      std::cout << (int)(Chip8.keypadState[i]) << std::endl;
    }
    i++;
  }
}

int main() {
  // Step 1: setup graphics and input systems
  GLFWwindow* window;
  if(!glfwInit()) {
    std::cout << "GLFW couldn't start" << std::endl;
    return -1;
  }

  window = glfwCreateWindow(640, 320, "EMUL-8 DevBuild", NULL, NULL);
  glfwMakeContextCurrent(window);
  glfwSetKeyCallback(window, key_callback);

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
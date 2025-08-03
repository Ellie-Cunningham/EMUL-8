#include <iostream>
#include <sstream>
#include <fstream>
#include <chrono>
#include <thread>
#include <cstring>
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

const int windowWidth = 640;
const int windowHeight = 360;
const int pixelSize = 10;

// Updates keypadState. Runs everytime user interacts with keyboard.
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
  bool keyIsPressedDown = !(action == GLFW_RELEASE);

  int i = 0;
  for(int k : keyMap) {
    if(k == key) {
      Chip8.keypadState[i] = static_cast<unsigned char>(keyIsPressedDown);
      // std::cout << (int)(Chip8.keypadState[i]) << std::endl;
    }
    i++;
  }
}

void framebufferSizeCallback(GLFWwindow* window, int width, int height) {
  glViewport(0, 0, width, height);
}

std::string readFile(char* filename) {
  std::ifstream file;
  std::stringstream buffer;
  std::string ret;

  file.open(filename);
  if(!file.is_open()) {
    std::cout << "Couldn't read file: " << filename << std::endl;
    return "";
  }

  buffer << file.rdbuf();
  ret = buffer.str();
  file.close();
  return ret;
}

int generateShader(char* filename, GLenum type) {
  std::string shaderSource = readFile(filename);
  const GLchar* shader = shaderSource.c_str();

  int shaderObj = glCreateShader(type);
  glShaderSource(shaderObj, 1, &shader, NULL);
  glCompileShader(shaderObj);

  int shaderCompiled;
  char errorLog[512];
  glGetShaderiv(shaderObj, GL_COMPILE_STATUS, &shaderCompiled);
  if(!shaderCompiled) {
    glGetShaderInfoLog(shaderObj, 512, NULL, errorLog);
    std::cout << "Error in shader compilation: " << errorLog << std::endl;
    return -1;
  }

  return shaderObj;
}

int generateShaderProgram(char* vertexShaderPath, char* fragmentShaderPath, char* geometryShaderPath) {
  int shaderProgram = glCreateProgram();

  int vShader = generateShader(vertexShaderPath, GL_VERTEX_SHADER);
  int fShader = generateShader(fragmentShaderPath, GL_FRAGMENT_SHADER);
  int gShader = generateShader(geometryShaderPath, GL_GEOMETRY_SHADER);

  glAttachShader(shaderProgram, vShader);
  glAttachShader(shaderProgram, fShader);
  glAttachShader(shaderProgram, gShader);
  glLinkProgram(shaderProgram);

  glDeleteShader(vShader);
  glDeleteShader(fShader);
  glDeleteShader(gShader);

  return shaderProgram;
}

int main() {
  // Step 1: setup graphics and input systems
  GLFWwindow* window;
  if(!glfwInit()) {
    std::cout << "GLFW couldn't start" << std::endl;
    return -1;
  }

  window = glfwCreateWindow(screenWidth * pixelSize, screenHeight * pixelSize, "EMUL-8 DevBuild", NULL, NULL);
  glfwMakeContextCurrent(window);
  glfwSetKeyCallback(window, keyCallback);
  glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

  if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cout << "GLAD couldn't start" << std::endl;
    glfwTerminate();
    return -1;
  }

  framebufferSizeCallback(window, screenWidth * pixelSize, screenHeight * pixelSize);

  GLuint shaderProgram = generateShaderProgram("..\\src\\shaders\\main.vert", "..\\src\\shaders\\main.frag", "..\\src\\shaders\\main.geom");
  glUseProgram(shaderProgram);
  glUniform1i(glGetUniformLocation(shaderProgram, "width"), screenWidth);
  glUniform1f(glGetUniformLocation(shaderProgram, "cellWidth"), 2.0f / (float)(screenWidth));
  glUniform1f(glGetUniformLocation(shaderProgram, "cellHeight"), 2.0f / (float)(screenHeight));

  GLuint VAO;
  glGenVertexArrays(1, &VAO);
  glBindVertexArray(VAO);

  GLuint VBO;
  glGenBuffers(1, &VBO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);

  glBufferData(GL_ARRAY_BUFFER, screenPixelCount, Chip8.graphicOutput, GL_DYNAMIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribIPointer(0, 1, GL_BYTE, sizeof(char), 0);

  // Step 2: Initialize system and load program
  Chip8.initialization();
  int programLoaded = Chip8.loadProgram();
  if(programLoaded != 0) {
    std::cout << "Error Accessing ROM" << std::endl;
    return -1;
  }

  // Step 3: Loop CPU cycles
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  while(!glfwWindowShouldClose(window)) {
    glfwPollEvents();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    GLvoid* mapPtr = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
    if (!mapPtr) {
      std::cerr << "Failed to map buffer" << std::endl;
    }
    std::memcpy(mapPtr, Chip8.graphicOutput, screenPixelCount);
    glUnmapBuffer(GL_ARRAY_BUFFER);

    glUseProgram(shaderProgram);
    glDrawArrays(GL_POINTS, 0, screenPixelCount);

    glfwSwapBuffers(window);
    for(int i = 0; i < 10; i++) {
      glfwPollEvents();
      Chip8.CPUCycle();
    }

    if(Chip8.delayTimer > 0) {
      Chip8.delayTimer--;
    }

    if(Chip8.soundTimer > 0) {
      Chip8.soundTimer--;
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(16));
  }

  // Clean up buffers and arrays
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
  glDeleteBuffers(1, &VBO);
  glDeleteVertexArrays(1, &VAO);

  glDeleteProgram(shaderProgram);

  glfwTerminate();
  return 0;
}
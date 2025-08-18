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
HaltState currentHaltState = NOT_HALTING;
unsigned char keypadStateBeforeHalt[16];
int keyPressedDuringHalt = 0;

const int keyMap[] = {
  GLFW_KEY_X, // 0
  GLFW_KEY_1, // 1
  GLFW_KEY_2, // 2
  GLFW_KEY_3, // 3
  GLFW_KEY_Q, // 4
  GLFW_KEY_W, // 5
  GLFW_KEY_E, // 6
  GLFW_KEY_A, // 7
  GLFW_KEY_S, // 8
  GLFW_KEY_D, // 9
  GLFW_KEY_Z, // A
  GLFW_KEY_C, // B
  GLFW_KEY_4, // C
  GLFW_KEY_R, // D
  GLFW_KEY_F, // E
  GLFW_KEY_V  // F
};

const int windowWidth = 640;
const int windowHeight = 360;
const int pixelSize = 10;

// Updates keypadState. Checks conditions to exit states where cpu is halted. Runs everytime user interacts with keyboard.
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
  bool keyIsPressedDown = !(action == GLFW_RELEASE);
  bool keyIsHeldDown = (action == GLFW_REPEAT);

  if(keyIsHeldDown) {
    return;
  }

  int i = 0;
  for(int k : keyMap) {
    if(k == key) {
      Chip8.keypadState[i] = static_cast<unsigned char>(keyIsPressedDown);
      break;
    }
    i++;
  }

  if(currentHaltState == HaltState::AWAITING_KEY_PRESS && keyIsPressedDown) {
    int registerIndex = (Chip8.getLastExecutedOpcode() & 0x0F00) >> 8;
    Chip8.registerValueOverride(registerIndex, i);

    keyPressedDuringHalt = key;
    currentHaltState = HaltState::AWAITING_KEY_RELEASE;
  }

  if(currentHaltState == HaltState::AWAITING_KEY_RELEASE && !keyIsPressedDown && keyPressedDuringHalt == key) {
    currentHaltState = HaltState::NOT_HALTING;
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
  std::fill_n(keypadStateBeforeHalt, 16, 0);

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
    auto startTime = std::chrono::high_resolution_clock::now();
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

    if(currentHaltState == HaltState::NOT_HALTING) {
      for(int i = 0; i < 10; i++) {
        glfwPollEvents();
        currentHaltState = (HaltState)Chip8.CPUCycle();

        if(currentHaltState != HaltState::NOT_HALTING) {
          std::copy(Chip8.keypadState, Chip8.keypadState + 16, keypadStateBeforeHalt);
          break;
        }
      }
    }

    if(Chip8.delayTimer > 0) {
      Chip8.delayTimer--;
    }

    if(Chip8.soundTimer > 0) {
      Chip8.soundTimer--;
    }
    
    std::this_thread::sleep_until(startTime + std::chrono::milliseconds(16));
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
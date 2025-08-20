#include <iostream>
#include <sstream>
#include <fstream>
#include <chrono>
#include <thread>
#include <cstring>
#include <cassert>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <json/json.hpp>
using json = nlohmann::json;
#include "CHIP8.h"

// The miniaudio library contains one reference to MA_ASSERT before it is defined. To avoid issues in compilation it is defined here.
#ifndef MA_ASSERT
#define MA_ASSERT(condition)            assert(condition)
#endif
#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio/miniaudio.h>

CHIP8 Chip8;

/* config.json contains different parameters pertaining to things like audio, graphics, and controls.
 * While a dedicated settings menu doesn't exist I believe this is a nice middle ground of providing
 * user customization without increasing project scope far beyond what I desire.
 */
std::ifstream f("..\\src\\config.json");
json config = json::parse(f);

ma_format audioDeviceFormat = ma_format_f32;
int audioDeviceChannels = 2;
int audioDeviceSampleRate = 48000;

HaltState currentHaltState = NOT_HALTING;
unsigned char keypadStateBeforeHalt[16];
int keyPressedDuringHalt = 0;

int keyMap[16];

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

// Handles reading and writing audio data from ma_device objects.
void dataCallback(ma_device* device, void* output, const void* input, ma_uint32 frameCount) {
  ma_waveform* sineWave;

  MA_ASSERT(device->playback.channels == audioDeviceChannels);

  sineWave = (ma_waveform*)device->pUserData;
  MA_ASSERT(sineWave != NULL);

  ma_waveform_read_pcm_frames(sineWave, output, frameCount, NULL);

  (void)input;
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

  // Step 1: setup graphics, input, and audio systems.
  // Step 1.1: GLFW and GLAD setup.
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

  // Step 1.2: miniaudio setup.
  ma_waveform sineWave;
  ma_device_config deviceConfig;
  ma_device device;
  ma_waveform_config sineWaveConfig;

  deviceConfig = ma_device_config_init(ma_device_type_playback);
  deviceConfig.playback.format = audioDeviceFormat;
  deviceConfig.playback.channels = audioDeviceChannels;
  deviceConfig.sampleRate = audioDeviceSampleRate;
  deviceConfig.dataCallback = dataCallback;
  deviceConfig.pUserData = &sineWave;

  if(ma_device_init(NULL, &deviceConfig, &device) != MA_SUCCESS) {
    std::cout << "miniaudio couldn't open playback device." << std::endl;
    return -1;
  }

  sineWaveConfig = ma_waveform_config_init(
    device.playback.format,
    device.playback.channels,
    device.sampleRate,
    ma_waveform_type_sine,
    config["audio"]["volume"],
    config["audio"]["sineWaveFrequency"]
  );
  ma_waveform_init(&sineWaveConfig, &sineWave);

  // Step 1.3: Keypad setup.
  try {
    int i = 0;
    for(auto& entry : config["controls"].items()) {
      keyMap[i] = (int)((std::string)(entry.value())).c_str()[0];
      i++;
    }
  }
  catch(...) {
    std::cout << "Error getting control keybinds. Entry may have been added or removed from config.json" << std:: endl;
    return -1;
  }

  // Step 2: Initialize Chip8 and load program.
  Chip8.initialization();
  int programLoaded = Chip8.loadProgram(config["general"]["romFileName"]);
  if(programLoaded != 0) {
    std::cout << "Error Accessing ROM" << std::endl;
    return -1;
  }

  // Step 3: Loop CPU cycles.
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
      for(int i = 0; i < config["general"]["cpuCyclesPerFrame"]; i++) {
        glfwPollEvents();
        currentHaltState = (HaltState)Chip8.CPUCycle();

        if(currentHaltState != HaltState::NOT_HALTING) {
          std::copy(Chip8.keypadState, Chip8.keypadState + 16, keypadStateBeforeHalt);
          break;
        }

        // Checks if sound should start playing.
        unsigned short ranOpcode = Chip8.getLastExecutedOpcode();
        bool soundTimerSet = ((ranOpcode & 0xF0FF) == 0xF018);
        if(soundTimerSet && Chip8.soundTimer != 0) {
          if(ma_device_start(&device) != MA_SUCCESS) {
            std::cout << "miniaudio couldn't start playback device." << std::endl;
          }
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

    if(Chip8.soundTimer == 0) {
      if(ma_device_get_state(&device) == ma_device_state_started) {
        ma_device_stop(&device);
      }
    }
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
#include <iostream>
#include <fstream>
#include <random>
#include "CHIP8.h"

// Characters are 4x5, each byte represents a horizontal piece of it's respective character.
const unsigned char CHIP8FontSet[fontSetSize] = {
  0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
  0x20, 0x60, 0x20, 0x20, 0x70, // 1
  0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
  0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
  0x90, 0x90, 0xF0, 0x10, 0x10, // 4
  0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
  0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
  0xF0, 0x10, 0x20, 0x40, 0x40, // 7
  0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
  0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
  0xF0, 0x90, 0xF0, 0x90, 0x90, // A
  0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
  0xF0, 0x80, 0x80, 0x80, 0xF0, // C
  0xE0, 0x90, 0x90, 0x90, 0xE0, // D
  0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
  0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

std::default_random_engine randGenerator;
std::uniform_int_distribution<int> randDistribution(0, 255);

unsigned short CHIP8::getLastExecutedOpcode() {
  try {
    return currentOpcode;
  }
  catch(...) {
    std::cout << "Couldn't get opcode: CHIP8 object may not have been initialized." << std::endl;
    return 0x0000;
  }
}

void CHIP8::registerValueOverride(int registerIndex, int registerValue) {
  V[registerIndex] = (unsigned char)registerValue;
}

void CHIP8::outputScreenToConsole() {
  for(int i = 0; i < screenHeight; i++) {
    for(int j = 0; j < screenWidth; j++) {
      if(graphicOutput[j + i*screenWidth] == 1) {
        std::cout << "@";
      }
      else {
        std::cout << " ";
      }
    }
    std::cout << std::endl;
  }
  std::cout << "________________________________________________________________" << std::endl;
}

void CHIP8::initialization() {
  pc = 0x200; // 0x000 to 0x1FF is reserved for the interpretor.
  currentOpcode = 0;
  I = 0;
  stackPointer = 0;
  delayTimer = 0;
  soundTimer = 0;

  std::fill_n(graphicOutput, screenPixelCount, 0);
  std::fill_n(stack, 16, 0);
  std::fill_n(V, 16, 0);
  std::fill_n(RAM, RAMSize, 0);

  for(int i = 0; i < fontSetSize; i++) {
    RAM[i] = CHIP8FontSet[i];
  }
}

int CHIP8::loadProgram(std::string fileName) {
  // Open ROM file.
  std::fstream fout;
  fout.open("..\\testROM\\" + fileName, std::ios::in | std::ios::binary);
  if(!fout) {
    return -1;
  }

  // Read ROM.
  char ROM[RAMSize - interpretorSize];
  std::fill_n(ROM, RAMSize - interpretorSize, 0);
  fout.read(ROM, sizeof(ROM));
  fout.close();

  // Write ROM into memory.
  for(int i = 0; i < sizeof(ROM)/sizeof(char); i++) {
    RAM[interpretorSize + i] = ROM[i];
  }

  return 0;
}

// Returns whether or not to simulate halting for opcode Fx0A
int CHIP8::CPUCycle() {
  currentOpcode = (RAM[pc] << 8) | RAM[pc + 1];

  const unsigned char xNibble = (currentOpcode & 0x0F00) >> 8; // Second opcode nibble.
  const unsigned char yNibble = (currentOpcode & 0x00F0) >> 4; // Third opcode nibble.
  const unsigned char nNibble = currentOpcode & 0x000F; // Fourth opcode nibble.
  const unsigned char kkByte = currentOpcode & 0x00FF; // Second opcode byte.
  const unsigned short nnn = currentOpcode & 0x0FFF; // Opcode excluding most significant nibble.

  /* Determine and execute given instruction.
   * 
   * All opcodes are labeled. For a more detailed breakdown of their intended function I recommend referring to Gulrak's Variant Opcode Table:
   * https://chip8.gulrak.net/
   */
  switch(currentOpcode & 0xF000) {
    case 0x0000:
      switch(currentOpcode) {
        // 00E0 - CLS
        case 0x00E0:
          std::fill_n(graphicOutput, screenPixelCount, 0);
          break;
        // 00EE - RET
        case 0x00EE:
          stackPointer--;
          pc = stack[stackPointer];
          break;
        // 0nnn - SYS addr (ignored by most compilers)
        default:
          break;
      }
      break;
    // 1nnn - JP addr
    case 0x1000:
      pc = nnn;
      pc -= 2;
      break;
    // 2nnn - CALL addr
    case 0x2000:
      stack[stackPointer] = pc;
      stackPointer++;
      pc = nnn;
      pc -= 2;
      break;
    // 3xkk - SE Vx, byte (Skip Equal)
    case 0x3000:
      if(V[xNibble] == kkByte) {
        pc += 0x2;
      }
      break;
    // 4xkk - SNE Vx, byte (Skip Not Equal) 
    case 0x4000:
      if(V[xNibble] != kkByte) {
        pc += 0x2;
      }
      break;
    // 5xy0 - SE Vx, Vy (Skip Equal)
    case 0x5000:
      if(V[xNibble] == V[yNibble]) {
        pc += 0x2;
      }
      break;
    // 6xkk - LD Vx, byte
    case 0x6000:
      V[xNibble] = kkByte;
      break;
    // 7xkk - ADD Vx, byte
    case 0x7000:
      V[xNibble] += kkByte;
      break;
    case 0x8000:
      switch(nNibble) {
        // 8xy0 - LD Vx, Vy
        case 0x0000:
          V[xNibble] = V[yNibble];
          break;
        // 8xy1 - OR Vx, Vy
        case 0x0001:
          V[xNibble] |= V[yNibble];
          V[15] = 0x00;
          break;
        // 8xy2 - AND Vx, Vy
        case 0x0002:
          V[xNibble] &= V[yNibble];
          V[15] = 0x00;
          break;
        // 8xy3 - XOR Vx, Vy
        case 0x0003:
          V[xNibble] ^= V[yNibble];
          V[15] = 0x00;
          break;
        // 8xy4 - ADD Vx, Vy
        case 0x0004:
          {
            int sum = V[xNibble] + V[yNibble];
            V[15] = (sum > 0xFF);
            V[xNibble] = (unsigned char)sum;
          }
          break;
        // 8xy5 - SUB Vx, Vy
        case 0x0005:
          {
            unsigned char difference = V[xNibble] - V[yNibble];
            V[15] = (V[xNibble] >= V[yNibble]);
            V[xNibble] = difference;
          }
          break;
        // 8xy6 - SHR Vx {, Vy} (Shift Right)
        case 0x0006:
          {
            unsigned char unshifted = V[yNibble];
            V[xNibble] = unshifted >> 1;
            V[15] = ((unshifted & 0x01) == 0x01);
          }
          break;
        // 8xy7 - SUBN Vx, Vy
        case 0x0007:
          {
            unsigned char difference = V[yNibble] - V[xNibble];
            V[xNibble] = difference;
            V[15] = (V[xNibble] <= V[yNibble]);
          }
          break;
        // 8xyE - SHL Vx {, Vy} (Shift Left)
        case 0x000E:
          {
            unsigned char unshifted = V[yNibble];
            V[xNibble] = unshifted << 1;
            V[15] = ((unshifted & 0x80) == 0x80);
          }
          break;
        default:
          break;
      }
      break;
    // 9xy0 - SNE Vx, Vy
    case 0x9000:
      if(V[xNibble] != V[yNibble]) {
        pc += 0x2;
      }
      break;
    // Annn - LD I, addr
    case 0xA000:
      I = nnn;
      break;
    // Bnnn - JP V0, addr
    case 0xB000:
      pc = nnn + V[0];
      pc -= 2;
      break;
    // Cxkk - RND Vx, byte
    case 0xC000:
      V[xNibble] = randDistribution(randGenerator) & kkByte;
      break;
    // Dxyn - DRW Vx, Vy, nibble
    case 0xD000:
      {
        // Carry flag set to 0 by default, 1 if a pixel is erased when drawing.
        V[15] = 0x00;

        // Screen wraps only occur if entire sprite would be drawn off screen.
        const bool horizontalWraparound = ((V[xNibble] % screenWidth) <= (screenWidth - 8));
        const bool verticalWraparound = ((V[yNibble] % screenHeight) <= (screenHeight - nNibble));

        for(int i = 0; i < nNibble; i++) {
            if((V[yNibble] + i) > (screenHeight - 1) && !verticalWraparound) {
              break;
            }

            for(int j = 0; j < 8; j++) {
              if((V[xNibble] + j) > (screenWidth - 1) && !horizontalWraparound) {
                break;
              }

              int currentTargetedPixel = (V[xNibble] + j) % screenWidth + ((V[yNibble] + i) % screenHeight) * screenWidth;
              bool modifyPixel = ((RAM[I + i] >> (7 - j)) & 0x01);
              graphicOutput[currentTargetedPixel] ^= modifyPixel;
              
              if(!graphicOutput[currentTargetedPixel] && modifyPixel) {
                V[15] = 0x01;
              }
            }
        }
      }
      break;
    case 0xE000:
      switch(kkByte) {
        // Ex9E - SKP Vx
        case 0x009E:
          if(keypadState[V[xNibble]]) {
            pc += 0x2;
          }
          break;
        // ExA1 - SKNP Vx
        case 0x00A1:
          if(!keypadState[V[xNibble]]) {
            pc += 0x2;
          }
          break;
        default:
          break;
      }
      break;
    case 0xF000:
      switch(kkByte) {
        // Fx07 - LD Vx, DT
        case 0x0007:
          V[xNibble] = delayTimer;
          break;
        // Fx0A - LD Vx, K
        case 0x000A:
          // CPU cycles are paused until key is pressed and released. remainder of opcode logic handled in the keyCallback method in main.cpp
          pc += 2;
          return (int)HaltState::AWAITING_KEY_PRESS;
          break;
        // Fx15 - LD DT, Vx
        case 0x0015:
          delayTimer = V[xNibble];
          break;
        // Fx18 - LD ST, Vx
        case 0x0018:
          soundTimer = V[xNibble];
          break;
        // Fx1E - ADD I, Vx
        case 0x001E:
          I += V[xNibble];
          break;
        // Fx29 - LD F, Vx
        case 0x0029:
          I = V[xNibble] * 0x5;
          break;
        // Fx33 - LD B, Vx
        case 0x0033:
          RAM[I] = V[xNibble] / 100;
          RAM[I+1] = (V[xNibble] / 10) - (RAM[I] * 10);
          RAM[I+2] = V[xNibble] - (RAM[I] * 100) - (RAM[I+1] * 10);
          break;
        // Fx55 - LD [I], Vx
        case 0x0055:
          // Iterator is j to avoid confusion.
          for(int j = 0; j <= xNibble; j++) {
            RAM[I] = V[j];
            I++;
          }
          break;
        // Fx65 - LD Vx, [I]
        case 0x0065:
          // Iterator is j to avoid confusion.
          for(int j = 0; j <= xNibble; j++) {
            V[j] = RAM[I];
            I++;
          }
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }

  pc += 2;
  return (int)HaltState::NOT_HALTING;
}
#include <iostream>
#include <fstream>
#include "CHIP8.h"

const int screenPixelCount = 64*32;
const int RAMSize = 4096;
const int interpretorSize = 512;
const int fontSetSize = 80;
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

int CHIP8::loadProgram() {
  // Open ROM file.
  std::fstream fout;
  fout.open("testROM\\Pong (1 player).ch8", std::ios::in | std::ios::binary);
  if(!fout) {
    std::cout << "Error Accessing ROM" << std::endl;
    return -1;
  }

  // Read ROM.
  char ROM[RAMSize - interpretorSize];
  std::fill_n(ROM, RAMSize - interpretorSize, 0);
  fout.read(ROM, sizeof(ROM));
  fout.close();

  // Write ROM into memory.
  for(int i = 0; i <sizeof(ROM)/sizeof(char); i++) {
    RAM[interpretorSize + i] = ROM[i];
  }

  return 0;
}

void CHIP8::CPUCycle() {
  currentOpcode = (RAM[pc] << 8) | RAM[pc + 1];

  // Determine and execute given instruction.
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
        // 0nnn - SYS addr
        default:
          pc = currentOpcode & 0x0FFF;
          break;
      }
      break;
    // 1nnn - JP addr
    case 0x1000:
      pc = currentOpcode & 0x0FFF;
      break;
    // 2nnn - CALL addr
    case 0x2000:
      stack[stackPointer] = pc;
      stackPointer++;
      pc = currentOpcode & 0x0FFF;
      break;
    // 3xkk - SE Vx, byte (Skip Equal)
    case 0x3000:
      if(V[(currentOpcode & 0x0F00) >> 8] == currentOpcode & 0x00FF) {
        pc += 0x2;
      }
      break;
    // 4xkk - SNE Vx, byte (Skip Not Equal) 
    case 0x4000:
      if(V[(currentOpcode & 0x0F00) >> 8] != currentOpcode & 0x00FF) {
        pc += 0x2;
      }
      break;
    // 5xy0 - SE Vx, Vy (Skip Equal)
    case 0x5000:
      if(V[(currentOpcode & 0x0F00) >> 8] == V[(currentOpcode & 0x00F0) >> 4])
      break;
    // 6xkk - LD Vx, byte
    case 0x6000:
      V[(currentOpcode & 0x0F00) >> 8] = currentOpcode & 0x00FF;
      break;
    // 7xkk - ADD Vx, byte
    case 0x7000:
      V[(currentOpcode & 0x0F00) >> 8] += currentOpcode & 0x00FF;
      break;
    case 0x8000:
      switch(currentOpcode & 0x000F) {
        // 8xy0 - LD Vx, Vy
        case 0x0000:
          V[(currentOpcode & 0x0F00) >> 8] = V[(currentOpcode & 0x00F0) >> 4];
          break;
        // 8xy1 - OR Vx, Vy
        case 0x0001:
          V[(currentOpcode & 0x0F00) >> 8] = V[(currentOpcode & 0x0F00) >> 8] | V[(currentOpcode & 0x00F0) >> 4];
          break;
        // 8xy2 - AND Vx, Vy
        case 0x0002:
          V[(currentOpcode & 0x0F00) >> 8] = V[(currentOpcode & 0x0F00) >> 8] & V[(currentOpcode & 0x00F0) >> 4];
          break;
        // 8xy3 - XOR Vx, Vy
        case 0x0003:
          V[(currentOpcode & 0x0F00) >> 8] = V[(currentOpcode & 0x0F00) >> 8] ^ V[(currentOpcode & 0x00F0) >> 4];
          break;
        // 8xy4 - ADD Vx, Vy
        case 0x0004:
          unsigned char sum = V[(currentOpcode & 0x0F00) >> 8] + V[(currentOpcode & 0x00F0) >> 4];
          // If resulting sum is less than original value and neither Vx or Vy are 0 (i.e. sum > 255).
          if(sum < V[(currentOpcode & 0x0F00) >> 8] && V[(currentOpcode & 0x0F00) >> 8] != 0x00 && V[(currentOpcode & 0x00F0) >> 4] != 0x00) {
            V[15] = 0x01;
          }
          else {
            V[15] = 0x00;
          }
          V[(currentOpcode & 0x0F00) >> 8] = sum;
          break;
        // 8xy5 - SUB Vx, Vy
        case 0x0005:
          if(V[(currentOpcode & 0x0F00) >> 8] > V[(currentOpcode & 0x00F0) >> 4]) {
            V[15] = 1;
          }
          else {
            V[15] = 0;
          }
          V[(currentOpcode & 0x0F00) >> 8] = V[(currentOpcode & 0x0F00) >> 8] - V[(currentOpcode & 0x00F0) >> 4];
          break;
        // 8xy6 - SHR Vx {, Vy} (Shift Right)
        case 0x0006:
          if(V[(currentOpcode & 0x0F00) >> 8] & 0x01 == 0x01) {
            V[15] = 0x01;
          }
          else {
            V[15] = 0x00;
          }
          V[(currentOpcode & 0x0F00) >> 8] = V[(currentOpcode & 0x0F00) >> 8] >> 1;
          break;
        // 8xy7 - SUBN Vx, Vy
        case 0x0007:
          if (V[(currentOpcode & 0x0F00) >> 8] < V[(currentOpcode & 0x00F0) >> 4]) {
            V[15] = 0x01;
          }
          else {
            V[15] = 0x00;
          }
          V[(currentOpcode & 0x0F00) >> 8] = V[(currentOpcode & 0x0F00) >> 8] - V[(currentOpcode & 0x00F0) >> 4];
          break;
        // 8xyE - SHL Vx {, Vy} (Shift Left)
        case 0x000E:
          if(V[(currentOpcode & 0x0F00) >> 8] & 0x80 == 0x80) {
            V[15] = 0x01;
          }
          else {
            V[15] = 0x00;
          }
          V[(currentOpcode & 0x0F00) >> 8] = V[(currentOpcode & 0x0F00) >> 8] << 1;
          break;
      }
      break;
    // 9xy0 - SNE Vx, Vy
    case 0x9000:
      if(V[(currentOpcode & 0x0F00) >> 8] != V[(currentOpcode & 0x00F0) >> 4]) {
        pc += 0x2;
      }
      break;
    case 0xA000:
      break;
    case 0xB000:
      break;
    case 0xC000:
      break;
    case 0xD000:
      break;
    case 0xE000:
      break;
    case 0xF000:
      break;
    default:
      break;
  }
}
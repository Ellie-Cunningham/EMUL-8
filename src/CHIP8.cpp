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
  fout.open("..\\testROM\\Pong (1 player).ch8", std::ios::in | std::ios::binary);
  if(!fout) {
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
          {
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
          }
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
        default:
          break;
      }
      break;
    // 9xy0 - SNE Vx, Vy
    case 0x9000:
      if(V[(currentOpcode & 0x0F00) >> 8] != V[(currentOpcode & 0x00F0) >> 4]) {
        pc += 0x2;
      }
      break;
    // Annn - LD I, addr
    case 0xA000:
      I = currentOpcode & 0x0FFF;
      break;
    // Bnnn - JP V0, addr
    case 0xB000:
      pc = I + V[0];
      break;
    // Cxkk - RND Vx, byte
    case 0xC000:
      V[(currentOpcode & 0x0F00) >> 8] = randDistribution(randGenerator) & (currentOpcode & 0x00FF);
      break;
    // Dxyn - DRW Vx, Vy, nibble
    case 0xD000:
      for(int i = 0; i < (currentOpcode & 0x000F); i++) {
        for(int j = 0; j < 4; j++) {
          int currentTargetedPixel = V[(currentOpcode & 0x0F00) >> 8] + j + (V[(currentOpcode & 0x0F00) >> 8] + i) * screenWidth;
          graphicOutput[currentTargetedPixel] ^= (RAM[I + i] >> 7 - j) & 0x01;
        }
      }
      break;
    case 0xE000:
      switch(currentOpcode & 0x00FF) {
        // Ex9E - SKP Vx
        case 0x009E:
          if(keypadState[V[(currentOpcode & 0x0F00) >> 8]] != 0x00) {
            pc += 0x2;
          }
          break;
        // ExA1 - SKNP Vx
        case 0x00A1:
          if(keypadState[V[(currentOpcode & 0x0F00) >> 8]] == 0x00) {
            pc += 0x2;
          }
          break;
        default:
          break;
      }
      break;
    case 0xF000:
      switch(currentOpcode & 0x00FF) {
        // Fx07 - LD Vx, DT
        case 0x0007:
          V[(currentOpcode & 0x0F00) >> 8] = delayTimer;
          break;
        // Fx0A - LD Vx, K
        case 0x000A:
          [&]() {
            while(true) {
              for(int i = 0; i < 16; i++) {
                if(keypadState[i]) {
                  V[(currentOpcode & 0x0F00) >> 8] = i;
                  return;
                }
              }
            }
          };
          break;
        // Fx15 - LD DT, Vx
        case 0x0015:
          delayTimer = V[(currentOpcode & 0x0F00) >> 8];
          break;
        // Fx18 - LD ST, Vx
        case 0x0018:
          soundTimer = V[(currentOpcode & 0x0F00) >> 8];
          break;
        // Fx1E - ADD I, Vx
        case 0x001E:
          I += V[(currentOpcode & 0x0F00) >> 8];
          break;
        // Fx29 - LD F, Vx
        case 0x0029:
          I = V[(currentOpcode & 0x0F00) >> 8] * 0x5;
          break;
        // Fx33 - LD B, Vx
        case 0x0033:
          RAM[I] = V[(currentOpcode & 0x0F00) >> 8] / 100;
          RAM[I+1] = (V[(currentOpcode & 0x0F00) >> 8] / 10) - (RAM[I] * 10);
          RAM[I+2] = V[(currentOpcode & 0x0F00) >> 8] - (RAM[I] * 100) - (RAM[I+1] * 10);
          break;
        // Fx55 - LD [I], Vx
        case 0x0055:
          // Iterator is j to avoid confusion.
          for(int j = 0; j <= V[(currentOpcode & 0x0F00) >> 8]; j++) {
            RAM[I + j] = V[j];
          }
          break;
        // Fx65 - LD Vx, [I]
        case 0x0065:
          // Iterator is j to avoid confusion.
          for(int j = 0; j <= V[(currentOpcode & 0x0F00) >> 8]; j++) {
            V[j] = RAM[I + j];
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

}
#include <iostream>
#include <fstream>
#include "CHIP8.h"

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

  std::fill_n(graphicOutput, 64*32, 0);
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
}
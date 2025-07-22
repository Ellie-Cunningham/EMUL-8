#include <iostream>
#include "CHIP8.h"

int main() {
  CHIP8 Chip8;
  
  // Step 1: setup graphics and input systems

  // Step 2: Initialize system and load program
  Chip8.initialization();
  int programLoaded = Chip8.loadProgram();
  if(programLoaded != 0) {
    return -1;
  }

  // Step 3: Loop CPU cycles

  return 0;
}
#ifndef CHIP8_H
#define CHIP8_H

const int screenWidth = 64;
const int screenHeight = 32;
const int screenPixelCount = screenWidth * screenHeight;
const int RAMSize = 4096;
const int interpretorSize = 512;
const int fontSetSize = 80;

class CHIP8 {
  private:
    unsigned char RAM[RAMSize];
    unsigned char V[16]; // CPU registers formaly named V0 - VE with the final register representing a 'carry flag'.
    unsigned short I; // Index register
    unsigned short pc; // Program counter
    unsigned short currentOpcode;

    // When calling a subroutine or jump command, the previous location is stored in a stack to be returned to.
    unsigned short stack[16];
    unsigned short stackPointer;

    // Both timers automatically tick down at 60hz when not 0
    unsigned char delayTimer;
    unsigned char soundTimer;

  public:
    unsigned char graphicOutput[screenPixelCount]; // 64x32 resolution monochrome display.
    unsigned char keypadState[16]; // 4x4 keypad for user input.

    void initialization();
    int loadProgram();
    void CPUCycle();
};
#endif
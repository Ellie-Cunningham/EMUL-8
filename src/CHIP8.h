#ifndef CHIP8_H
#define CHIP8_H

const int screenWidth = 64;
const int screenHeight = 32;
const int screenPixelCount = screenWidth * screenHeight;
const int RAMSize = 4096;
const int interpretorSize = 512;
const int fontSetSize = 80;

enum HaltState {
  NOT_HALTING = 0, // Default state, continue CPU cycles.
  AWAITING_KEY_PRESS = 1, // CPU begins halting, exits into AWAITING_KEY_RELEASE once key is pressed.
  AWAITING_KEY_RELEASE = 2 // CPU continues halting, exits into NOT_HALTING once key is released.
};

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

  public:
    unsigned char graphicOutput[screenPixelCount]; // 64x32 resolution monochrome display.
    unsigned char keypadState[16]; // 4x4 keypad for user input.

    // Both timers automatically tick down at 60hz when not 0
    unsigned char delayTimer;
    unsigned char soundTimer;

    unsigned short getLastExecutedOpcode();
    void registerValueOverride(int registerIndex, int registerValue);
    void outputScreenToConsole();
    void initialization();
    int loadProgram(std::string fileName);
    int CPUCycle();
};
#endif
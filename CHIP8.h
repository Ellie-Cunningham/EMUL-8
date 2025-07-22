#ifndef CHIP8_H
#define CHIP8_H
class CHIP8 {
  private:
    unsigned char RAM[4096];
    unsigned char V[16]; // CPU registers formaly named V0 - VE with the final register representing a 'carry flag'.
    unsigned short I; // Index register
    unsigned short pc; // Program counter
    unsigned short currentOpcode;

    // When calling a subroutine or jump command, the previous location is stored in a stack to be returned to.
    unsigned short stack[16];
    unsigned short stackPointer;

    unsigned char graphicOutput[64*32]; // 64x32 resolution monochrome display.
    unsigned char keypadState[16]; // 4x4 keypad for user input.

    // Both timers automatically tick down at 60hz when not 0
    unsigned char delayTimer;
    unsigned char soundTimer;

  public:
    void initialization();
    int loadProgram();
    void CPUCycle();
};
#endif
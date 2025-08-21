# EMUL-8
## An emulator/interpreter for CHIP-8
CHIP-8 is a programming language originally intended for use on 8-bit microcomputers. CHIP-8 programs are run on a virtual machine (similar to how java and kotlin run on JVM).
This project aims to mimic behavior of the original CHIP-8 interpreter and VM that ran on 1802 based computers like the COSMAC VIP.

## Features
- Proper implimentation of original CHIP-8 interpretor's quirks.
- Audio is actually output rather than being ignored.
- Customizable graphics, audio, and controls through modification of config.json.

## Dependencies
The following libraries are required to compile the project from source:
- [glfw](https://www.glfw.org/)
- [glad](https://glad.dav1d.de/)
- [KHR from the Khronos EGL Registry](https://registry.khronos.org/EGL/)
- [json by nlohmann](https://github.com/nlohmann/json)
- [miniaudio](https://miniaud.io/)
- [stb_image by stb](https://github.com/nothings/stb)

## Known issues
Functionality was verified using [Timendus's chip8-test-suite.](https://github.com/Timendus/chip8-test-suite) Currently 2 opcode's have minor issue's:
- Opcode 8xy4 returns an incorrect value if the carry flag register is used as the y input.
- Opcode 8xy5 returns an incorrect value if the carry flag register is used as the y input.

In my testing with ROMs I've found online this hasn't caused any noticible issues.

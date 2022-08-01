#ifndef VM_H
#define VM_H

#include <algorithm>
#include <cstdint>
#include <iostream>

#include <vector>
using std::vector;

constexpr int CHIP8_WIDTH = 64;
constexpr int CHIP8_HEIGHT = 32;
constexpr int CHIP8_MEMORY = 0x1000;
constexpr uint16_t CHIP8_PROGRAM_START = 0x200;

constexpr int CHIP8_FONT_HEIGHT = 5;
constexpr uint8_t CHIP8_FONT[16 * CHIP8_FONT_HEIGHT] = {
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
    0xF0, 0x90, 0xF0, 0x90, 0x90, // a
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // b
    0xF0, 0x80, 0x80, 0x80, 0xF0, // c
    0xE0, 0x90, 0x90, 0x90, 0xE0, // d
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // e
    0xF0, 0x80, 0xF0, 0x80, 0x80  // f
};

enum Chip8_ErrorType
{
    Chip8_NoError,
    Chip8_InvalidHexDigit,
    Chip8_InvalidKey,
    Chip8_InvalidOpcode,
    Chip8_StackUnderflow,
};

struct Chip8
{
    uint8_t ram[CHIP8_MEMORY] = {}; // Memory
    uint8_t v[16] = {};             // Data registers

    uint16_t i = 0;                    // Address register
    uint16_t pc = CHIP8_PROGRAM_START; // Program counter
    vector<uint16_t> stack;            // Stack for subroutine addresses

    uint8_t sound_tmr = 0; // Sound timer
    uint8_t delay_tmr = 0; // Delay timer

    bool display[CHIP8_WIDTH * CHIP8_HEIGHT] = {}; // Display (pixels can be either on or off)

    bool keys[16] = {};                   // Keyboard state
    uint8_t *awaiting_keypress = nullptr; // Points to the register the pressed key is to be stored in, or NULL if not currently expecting input

    // Error information
    Chip8_ErrorType error_type = Chip8_NoError;
    uint16_t error_addr = 0;
    uint16_t error_opcode = 0;

    // Load the default 4x5px font to address 0.
    void load_default_font()
    {
        std::copy(std::begin(CHIP8_FONT), std::end(CHIP8_FONT), std::begin(ram));
    }

    // Fetch and execute the next instruction.
    void step();

    // Handle key down event.
    void key_down(int key)
    {
        keys[key] = true;
        if (awaiting_keypress)
        {
            *awaiting_keypress = key;
            awaiting_keypress = nullptr;
        }
    }

    // Handle key up event.
    void key_up(int key)
    {
        keys[key] = false;
    }

    // Log information about the error status to an output stream.
    void report_error(std::ostream &);

private:
    // Fetch the next instruction and increment PC.
    uint16_t fetch()
    {
        uint16_t op = ram[pc] << 8 | ram[(pc + 1) & 0x0fff];
        advance();
        return op;
    }

    // Advance the program counter to the next instruction.
    void advance()
    {
        pc += 2;
        pc &= 0x0fff;
    }

    // Indicate that the previous instruction has caused an error. No more instructions will be executed after this.
    void error(Chip8_ErrorType);
};

#endif
#include <algorithm>
#include <iomanip>

#include "vm.h"

void Chip8::step()
{
    if (error_type != Chip8_NoError || awaiting_keypress)
        return;

    // Fetch opcode
    uint16_t op = fetch();

    // For convenience -- many instructions encode used registers, addresses and literal values in this format
    uint8_t &x = v[(op & 0x0f00) >> 8];
    uint8_t &y = v[(op & 0x00f0) >> 4];
    uint16_t addr = op & 0x0fff;
    uint8_t low_byte = op & 0x00ff;

    switch (op & 0xf000)
    {
    case 0x0000:
        if (op == 0x00e0)
        {
            // Clear screen
            std::fill(display, display + CHIP8_WIDTH * CHIP8_HEIGHT, false);
        }
        else if (op == 0x00ee)
        {
            // Return from subroutine
            if (stack.empty())
                return error(Chip8_StackUnderflow);
            pc = stack.back();
            stack.pop_back();
        }
        else
            error(Chip8_InvalidOpcode);
        break;
    case 0x1000:
        // Jump to address
        pc = addr;
        break;
    case 0x2000:
        // Execute subroutine
        stack.push_back(pc);
        pc = addr;
        break;
    case 0x3000:
        // Skip if equal
        if (x == low_byte)
            advance();
        break;
    case 0x4000:
        // Skip if NOT equal
        if (x != low_byte)
            advance();
        break;
    case 0x5000:
        if ((op & 0x000f) != 0)
            return error(Chip8_InvalidOpcode);
        // Skip if two registers equal
        if (x == y)
            advance();
        break;
    case 0x6000:
        // Store literal in register
        x = low_byte;
        break;
    case 0x7000:
        // Add literal to register
        x += low_byte;
        break;
    case 0x8000:
    {
        switch (op & 0x000F)
        {
        case 0x0:
            // Move register
            x = y;
            break;
        case 0x1:
            // OR
            x |= y;
            break;
        case 0x2:
            // AND
            x &= y;
            break;
        case 0x3:
            // XOR
            x ^= y;
            break;
        case 0x4:
            // Add
            x += y;
            v[0xf] = x < y; // set flag on overflow
            break;
        case 0x5:
        {
            // Subtract
            bool no_borrow = x >= y;
            // The reason I don't set vF until after the subtraction is that x could be vF.
            // I haven't seen any documentation on which (result of subtraction vs. borrow flag) is actually prioritized,
            // and this is unlikely to be a problem in actual programs, but I'd rather have it consistent with addition.
            x -= y;
            v[0xf] = no_borrow;
            break;
        }
        case 0x6:
        {
            // Shift right
            bool lsb = x & 0b00000001;
            x = y >> 1;
            v[0xf] = lsb;
            break;
        }
        case 0x7:
        {
            // Subtract, but opposite
            bool no_borrow = y >= x;
            x = y - x;
            v[0xf] = no_borrow;
            break;
        }
        case 0xe:
        {
            // Shift left
            bool msb = x & 0b10000000;
            x = y << 1;
            v[0xf] = msb;
            break;
        }
        default:
            error(Chip8_InvalidOpcode);
        }
        break;
    }
    case 0x9000:
        // Skip if two registers NOT equal
        if ((op & 0x000f) != 0)
            return error(Chip8_InvalidOpcode);
        if (x != y)
            pc += 2;
        break;
    case 0xa000:
        // Set address register
        i = addr;
        break;
    case 0xb000:
        // Jump with offset
        pc = (addr + v[0]) & 0x0fff;
        break;
    case 0xc000:
        // Random number
        x = std::rand() & low_byte;
        break;
    case 0xd000:
        // Draw sprite
        v[0xf] = 0;
        for (int row = 0; row < (op & 0x000f); ++row)
        {
            uint8_t row_byte = ram[(i + row) & 0xfff];
            for (int col = 0; col < 8; ++col)
            {
                int mask = 0x1 << (7 - col);
                bool src = row_byte & mask;
                bool &dest = display[(y + row) % CHIP8_HEIGHT * CHIP8_WIDTH + (x + col) % CHIP8_WIDTH];
                if (src && dest) // Collision detection!
                    v[0xf] = 1;
                dest ^= src; // Sprites are XORed to the screen
            }
        }
        break;
    case 0xe000:
        if (low_byte == 0x9e)
        {
            // Skip if key pressed
            if (x & 0xf0)
                return error(Chip8_InvalidKey);
            if (keys[x])
                advance();
        }
        else if (low_byte == 0xa1)
        {
            // Skip if key NOT pressed
            if (x & 0xf0)
                return error(Chip8_InvalidKey);
            if (!keys[x & 0x0f])
                advance();
        }
        else
            error(Chip8_InvalidOpcode);
        break;
    case 0xf000:
        switch (low_byte)
        {
        case 0x07:
            // Get delay timer
            x = delay_tmr;
            break;
        case 0x0a:
            // Wait for keypress
            awaiting_keypress = &x;
            break;
        case 0x15:
            // Set delay timer
            delay_tmr = x;
            break;
        case 0x18:
            // Set sound timer
            sound_tmr = x;
            break;
        case 0x1e:
            // Add to I
            i += x;
            i &= 0x0fff;
            break;
        case 0x29:
            // Get address to font data
            if (x & 0xf0)
                return error(Chip8_InvalidHexDigit);
            i = x * CHIP8_FONT_HEIGHT;
            break;
        case 0x33:
            // Store binary-coded decimal
            ram[i] = x / 100;
            ram[i + 1] = x / 10 % 10;
            ram[i + 2] = x % 10;
            break;
        case 0x55:
            // Store registers
            for (uint8_t *reg = v;; ++reg)
            {
                ram[i] = *reg;
                ++i &= 0x0fff;
                if (reg == &x)
                    break;
            }
            break;
        case 0x65:
            // Load registers
            for (uint8_t *reg = v;; ++reg)
            {
                *reg = ram[i];
                ++i &= 0x0fff;
                if (reg == &x)
                    break;
            }
            break;
        default:
            error(Chip8_InvalidOpcode);
        }
        break;
    default:
        error(Chip8_InvalidOpcode);
    }
}

void Chip8::error(Chip8_ErrorType type)
{
    error_type = type;
    // PC has already been advanced at this point, so temporarily "roll back"
    pc -= 2;
    pc &= 0x0fff;
    error_addr = pc;
    error_opcode = fetch();
}

void Chip8::report_error(std::ostream &out)
{
    std::ios_base::fmtflags f(out.flags());

    switch (error_type)
    {
    case Chip8_NoError:
        out << "no error";
        break;
    case Chip8_InvalidHexDigit:
        out << "invalid hex digit";
        break;
    case Chip8_InvalidKey:
        out << "invalid key";
        break;
    case Chip8_InvalidOpcode:
        out << "invalid opcode";
        break;
    case Chip8_StackUnderflow:
        out << "stack underflow";
        break;
    }

    out << " at 0x" << std::hex << std::setw(3) << std::setfill('0') << error_addr
        << " (opcode: 0x" << std::setw(4) << error_opcode << ")" << std::endl;
    out.flags(f); // restore flags
}
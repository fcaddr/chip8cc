#ifndef MAIN_H
#define MAIN_H

#include <SDL2/SDL.h>

#include "vm.h"

constexpr int CYCLES_PER_FRAME = 16;

constexpr uint32_t COLOR_ON = 0xff393e41; // ARGB format
constexpr uint32_t COLOR_OFF = 0xfff6f7eb;

constexpr int SCALE = 12;
constexpr int WINDOW_WIDTH = CHIP8_WIDTH * SCALE;
constexpr int WINDOW_HEIGHT = CHIP8_HEIGHT * SCALE;

constexpr int AMPLITUDE = 28000;
constexpr int SAMPLE_RATE = 44100;

// Display an error message, quit SDL and terminate.
void fatal_sdl_error(const char *);

// Map SDL_Keycodes to their representation on the CHIP-8 keyboard (0x0 to 0xf for the 16 keys), using QWERTY layout. Return -1 for unused keys.
int key_index(SDL_Keycode);

// Shamelessly stolen from https://stackoverflow.com/a/45002609.
void audio_callback(void *user_data, uint8_t *raw_buffer, int bytes);

#endif
#include <fstream>
#include <iostream>

#include "main.h"
#include "vm.h"

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        std::cout << "Usage: chip8cc <ROM>" << std::endl;
        return 0;
    }

    std::ifstream file(argv[1], std::ios::binary);
    if (!file)
    {
        std::cerr << "ROM could not be read" << std::endl;
        return 1;
    }
    file.seekg(0, std::ios::end);
    size_t len = file.tellg();
    file.seekg(0, std::ios::beg);
    if (len + CHIP8_PROGRAM_START > CHIP8_MEMORY)
    {
        std::cerr << "ROM too big! (" << len << " bytes)" << std::endl;
        return 1;
    }

    Chip8 vm;
    vm.load_default_font();
    file.read(reinterpret_cast<char *>(&vm.ram[CHIP8_PROGRAM_START]), len);

    // Initialize SDL components

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0)
        fatal_sdl_error("SDL_Init");

    SDL_Window *window = SDL_CreateWindow("Chip-8", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    if (window == nullptr)
        fatal_sdl_error("SDL_CreateWindow");

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (renderer == nullptr)
        fatal_sdl_error("SDL_CreateRenderer");

    SDL_Texture *buffer = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, CHIP8_WIDTH, CHIP8_HEIGHT);
    if (buffer == nullptr)
        fatal_sdl_error("SDL_CreateTexture");

    // Initialize SDL audio

    int sample_nr = 0;

    SDL_AudioSpec want;
    want.freq = SAMPLE_RATE;
    want.format = AUDIO_S16SYS;
    want.channels = 1;
    want.samples = 2048;
    want.callback = audio_callback;
    want.userdata = &sample_nr;

    SDL_AudioSpec have;
    if (SDL_OpenAudio(&want, &have) != 0 || want.format != have.format)
        fatal_sdl_error("SDL_OpenAudio");

    // Main loop

    bool quit = false;
    SDL_Event event;

    while (!quit)
    {
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_QUIT:
                quit = true;
                break;
            case SDL_KEYDOWN:
            {
                int key = key_index(event.key.keysym.sym);
                if (key != -1)
                    vm.key_down(key);
                break;
            }
            case SDL_KEYUP:
            {
                int key = key_index(event.key.keysym.sym);
                if (key != -1)
                    vm.key_up(key);
                break;
            }
            default:
                break;
            }
        }

        // Execute instructions

        for (int i = 0; i < CYCLES_PER_FRAME; i++)
        {
            // std::clog << std::hex << vm.pc << " " << vm.error_type << std::endl;
            vm.step();
        }

        if (vm.error_type != Chip8_NoError)
        {
            std::cerr << "Chip-8 Error: ";
            vm.report_error(std::cerr);
            SDL_Quit();
            return 1;
        }

        // Render pixels to screen

        void *pixels;
        int pitch;
        if (SDL_LockTexture(buffer, nullptr, (void **)&pixels, &pitch) < 0)
            fatal_sdl_error("SDL_LockTexture");

        bool *src = vm.display;
        for (int row = 0; row < CHIP8_HEIGHT; ++row)
        {
            uint32_t *dest = (uint32_t *)((uint8_t *)pixels + row * pitch);
            for (int col = 0; col < CHIP8_WIDTH; ++col)
                *dest++ = *src++ ? COLOR_ON : COLOR_OFF;
        }

        SDL_UnlockTexture(buffer);

        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, buffer, nullptr, nullptr);
        SDL_RenderPresent(renderer);

        // Decrement timers
        if (vm.delay_tmr)
            --vm.delay_tmr;
        if (vm.sound_tmr)
        {
            SDL_PauseAudio(0);
            --vm.sound_tmr;
        }
        else
            SDL_PauseAudio(1);

        // Delay -- this should probably use an actual timer thing but I can't be bothered to set this up
        SDL_Delay(1000 / 60);
    }

    SDL_Quit();

    return 0;
}

void fatal_sdl_error(const char *fn_name)
{
    std::cerr << "SDL Error (" << fn_name << "): " << SDL_GetError() << std::endl;
    SDL_Quit();
    std::exit(1);
}

int key_index(SDL_Keycode key)
{
    switch (key)
    {
    case SDLK_x:
        return 0x0;
    case SDLK_1:
        return 0x1;
    case SDLK_2:
        return 0x2;
    case SDLK_3:
        return 0x3;
    case SDLK_q:
        return 0x4;
    case SDLK_w:
        return 0x5;
    case SDLK_e:
        return 0x6;
    case SDLK_a:
        return 0x7;
    case SDLK_s:
        return 0x8;
    case SDLK_d:
        return 0x9;
    case SDLK_z:
        return 0xa;
    case SDLK_c:
        return 0xb;
    case SDLK_4:
        return 0xc;
    case SDLK_r:
        return 0xd;
    case SDLK_f:
        return 0xe;
    case SDLK_v:
        return 0xf;
    default:
        return -1;
    }
}

void audio_callback(void *user_data, uint8_t *raw_buffer, int bytes)
{
    int16_t *buffer = (int16_t *)raw_buffer;
    int length = bytes / 2; // 2 bytes per sample for AUDIO_S16SYS
    int &sample_nr(*(int *)user_data);

    for (int i = 0; i < length; i++, sample_nr++)
    {
        double time = (double)sample_nr / (double)SAMPLE_RATE;
        buffer[i] = (int16_t)(AMPLITUDE * sin(2.0f * M_PI * 441.0f * time)); // render 441 HZ sine wave
    }
}
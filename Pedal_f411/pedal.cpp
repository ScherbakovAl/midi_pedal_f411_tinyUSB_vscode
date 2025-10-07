#include "pedal.hpp"

void pedal() {

    // USB init
    // LL_TIM_EnableCounter(TIM2); // счётчик
    tud_init(BOARD_TUD_RHPORT);

    LL_mDelay(500);

    {
        uint8_t const cable_num = 0;
        uint8_t note_buf[] = { 0xB0, 0x58, 125, 0x90, 0x3E, 0x36, 0xB0, 0x58, 125, 0x80, 0x3E, 0x36 };
        const int bufsize = sizeof(note_buf);
        tud_midi_stream_write(cable_num, note_buf, bufsize);
        LL_mDelay(1);
    }
    {
        uint8_t const cable_num = 0;
        uint8_t note_buf[] = { 0xB0, 0x58, 125, 0x90, 0x3E, 0x36, 0xB0, 0x58, 125, 0x80, 0x3E, 0x36 };
        const int bufsize = sizeof(note_buf);
        tud_midi_stream_write(cable_num, note_buf, bufsize);
        LL_mDelay(1);
    }

    while (1);
}
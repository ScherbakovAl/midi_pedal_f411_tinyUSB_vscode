#include "stm32f4xx_ll_tim.h"
#include "pedal.hpp"

void pedal() {

    // USB init
    LL_TIM_EnableCounter(TIM2); // счётчик
    tud_init(BOARD_TUD_RHPORT);
int fl = 1;
    while (1) {
        tud_task();
        if (TIM2->CNT > 1000000) {

            if(fl){
                fl = 0;
                GPIOC->BSRR = 0x2000; 
            }
            else{
                fl = 1;
                GPIOC->BSRR = 0x20000000;
            }

            TIM2->CNT = 0;
            
            uint8_t const cable_num = 0;
            uint8_t note_buf[] = { 0xB0, 0x58, 125, 0x90, 0x3E, 0x36, 0xB0, 0x58, 125, 0x80, 0x3E, 0x36 };
            const int bufsize = sizeof(note_buf);
            tud_midi_stream_write(cable_num, note_buf, bufsize);
        }
    }
}

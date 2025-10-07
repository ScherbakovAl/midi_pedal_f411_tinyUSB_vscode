#include "stm32f4xx_ll_tim.h"
#include "pedal.hpp"
#include "board_api.h"

void pedal() {

    LL_TIM_EnableCounter(TIM2); // счётчик
    
    // Инициализируем USB GPIO и тактирование
    board_init_usb();
    
    // Инициализируем TinyUSB (порт 0 для STM32F411)
    tud_init(0);

    int fl = 1;

    uint8_t const cable_num = 0; // MIDI jack associated with USB endpoint
    uint8_t const channel = 0; // 0 for channel 1

    while (1) {
        // Обязательно вызываем tud_task() для обработки USB событий
        tud_task();

        if (TIM2->CNT > 1000000) {
            TIM2->CNT = 0;

            // Переключаем светодиод
            if (fl) {
                fl = 0;
                GPIOC->BSRR = 0x2000;
            }
            else {
                fl = 1;
                GPIOC->BSRR = 0x20000000;
            }

            // Проверяем, что MIDI устройство подключено и готово
            if (tud_mounted() && tud_midi_mounted()) {
                // Отправляем MIDI Note On сообщение (нота A4, velocity 127)
                uint8_t note_on[3] = { 0x90 | channel, 69, 127 };  // Note On, A4 (69), velocity 127
                tud_midi_stream_write(cable_num, note_on, sizeof(note_on));
                
                // Опционально: можно добавить Note Off через некоторое время
                // uint8_t note_off[3] = { 0x80 | channel, 69, 0 };  // Note Off
                // tud_midi_stream_write(cable_num, note_off, sizeof(note_off));
            }
        }
    }
}

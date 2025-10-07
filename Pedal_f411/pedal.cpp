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

            // Проверяем, что устройства подключены и готовы
            if (tud_mounted()) {
                // Отправляем MIDI Note On сообщение (нота A4, velocity 127)
                if (tud_midi_mounted()) {
                    uint8_t note_on[3] = { 0x90 | channel, 69, 127 };  // Note On, A4 (69), velocity 127
                    tud_midi_stream_write(cable_num, note_on, sizeof(note_on));
                }
                
                // Отправляем нажатие клавиши "курсор-влево" через HID
                if (tud_hid_ready()) {
                    // HID Keyboard Report состоит из 8 байт:
                    // [modifier, reserved, key1, key2, key3, key4, key5, key6]
                    //
                    // modifier = 0 (нет Ctrl/Shift/Alt)
                    // keycode[6] = массив из 6 клавиш (стандарт USB HID)
                    // 0x50 = Left Arrow key (стрелка влево)
                    // Остальные позиции = 0 (клавиши не нажаты)
                    uint8_t keycode[6] = { 0x50, 0, 0, 0, 0, 0 };
                    
                    // Отправляем отчет: report_id=0, modifier=0, keycode
                    tud_hid_keyboard_report(0, 0, keycode);
                    
                    // Небольшая задержка перед отпусканием клавиши
                    uint32_t delay_count = 100000;
                    while(delay_count--);
                    
                    // Отпускаем клавишу (отправляем пустой отчет)
                    // NULL = все клавиши отпущены
                    tud_hid_keyboard_report(0, 0, NULL);
                }
            }
        }
    }
}

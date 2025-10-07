// #include "stm32f4xx_ll_tim.h"
#include "pedal.hpp"
#include <deque>

using uint = unsigned int;
using cuint = const uint;

cuint extpr0 = 1;
cuint extpr1 = 2;
cuint extpr2 = 4;
cuint extpr3 = 8;
int32_t t = 0;
int32_t p = 0;
int f = 0;

enum pedal_type {
    a = EXTI_IMR_MR0,
    b = EXTI_IMR_MR1,
    c = EXTI_IMR_MR2,
    d = EXTI_IMR_MR3
};

class pedals {
public:
    pedal_type ped = pedal_type::a;
    uint32_t time = 0;
};

std::deque<pedals> dequePedals;

void pedal() {

    HAL_ADC_Start_IT(&hadc1);
    HAL_TIM_Base_Start(&htim3);
    HAL_Delay(15);
    pwr();
    HAL_TIM_Base_Start(&htim5);
    HAL_TIM_Base_Start_IT(&htim2);
    f = 1;
    GPIOC->BSRR = 0x20000000;

    board_init_usb();
    tud_init(0);

    while (1) {
        auto timer = TIM5->CNT;
        if (!dequePedals.empty()) {
            auto& dP = dequePedals.front();
            if (timer - dP.time > 4200) {
                EXTI->IMR |= (uint32_t)dP.ped;
                tud_hid_keyboard_report(0, 0, NULL);
                dequePedals.pop_front();
                GPIOC->BSRR = 0x20000000;
            }
        }
        tud_task();
        if (timer > 4'094'967'295) {
            TIM5->CNT = 0;
        }
    }
}

    // пометочки..
/*
    int fl = 1;

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
                    uint8_t const cable_num = 0; // MIDI jack associated with USB endpoint
    uint8_t const channel = 0; // 0 for channel 1
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
            }*/


uint8_t u = 0;
uint8_t r = 0;

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc) {
    if (f == 1 && hadc->Instance == ADC1) {
        t = HAL_ADC_GetValue(&hadc1);
        if (t < 300) {
            t = 300;
        }
        if (t - p > 14 || p - t > 14) {
            u = t / 30 - 9;
            if (r != u) {
                uint8_t note_on[3] = { 176, 64, u };
                tud_midi_stream_write(0, note_on, sizeof(note_on));
                r = u;
                TIM2->CNT = 0;
            }
            p = t;
        }
    }
}

void MidiSender2(const uint8_t n, const uint8_t uu) {
    uint8_t note_on[3] = { 0x91, n, uu };
    tud_midi_stream_write(0, note_on, sizeof(note_on));
    r = uu;
    TIM2->CNT = 0;
}

void KeySender(const uint8_t k) {
    if (tud_hid_ready()) {
        uint8_t keycode[6] = { k, 0, 0, 0, 0, 0 };
        tud_hid_keyboard_report(0, 0, keycode);
    }
}


extern "C" {
    void EXTI0_IRQHandler(void) { // отключи в stm32f4xx_it.c
        // EXTI->PR = extpr0;
        // EXTI->IMR &= ~(EXTI_IMR_MR0);
        HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0);
        KeySender(0x50);
        dequePedals.push_back({ pedal_type::a, TIM5->CNT });
        GPIOC->BSRR = 0x2000;
    }

    void EXTI1_IRQHandler(void) {
        // EXTI->PR = extpr1;
        // EXTI->IMR &= ~(EXTI_IMR_MR1);
        HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_1);
        KeySender(0x4F);
        dequePedals.push_back({ pedal_type::b, TIM5->CNT });
        GPIOC->BSRR = 0x2000;
    }

    void EXTI2_IRQHandler(void) {
        // EXTI->PR = extpr2;
        // EXTI->IMR &= ~(EXTI_IMR_MR2);
        HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0);
        dequePedals.push_back({ pedal_type::c, TIM5->CNT });
        MidiSender2(60, 33);
        GPIOC->BSRR = 0x2000;
    }

    void EXTI3_IRQHandler(void) {
        EXTI->PR = extpr3;
        EXTI->IMR &= ~(EXTI_IMR_MR3);
        dequePedals.push_back({ pedal_type::d, TIM5->CNT });
        MidiSender2(61, 44);
        GPIOC->BSRR = 0x2000;
    }
}

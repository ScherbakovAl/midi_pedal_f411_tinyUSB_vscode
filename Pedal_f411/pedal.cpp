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
            if (timer - dP.time > 4000) {
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

void MidiSender(const uint8_t n, const uint8_t uu) {
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
    void EXTI0_IRQHandler(void) { // disable "IRQHandlers" in stm32f4xx_it.c
        EXTI->PR = extpr0;
        EXTI->IMR &= ~(EXTI_IMR_MR0);
        MidiSender(60, 44);
        dequePedals.push_back({ pedal_type::a, TIM5->CNT }); // TODO It's better not to add it to the queue here in the interrupt - it's dangerous! It needs to be redone!
        GPIOC->BSRR = 0x2000;
    }

    void EXTI1_IRQHandler(void) {
        EXTI->PR = extpr1;
        EXTI->IMR &= ~(EXTI_IMR_MR1);
        MidiSender(61, 33);
        dequePedals.push_back({ pedal_type::b, TIM5->CNT });
        GPIOC->BSRR = 0x2000;
    }

    void EXTI2_IRQHandler(void) {
        EXTI->PR = extpr2;
        EXTI->IMR &= ~(EXTI_IMR_MR2);
        dequePedals.push_back({ pedal_type::c, TIM5->CNT });
        KeySender(0x4F);
        GPIOC->BSRR = 0x2000;
    }

    void EXTI3_IRQHandler(void) {
        EXTI->PR = extpr3;
        EXTI->IMR &= ~(EXTI_IMR_MR3);
        dequePedals.push_back({ pedal_type::d, TIM5->CNT });
        KeySender(0x50);
        GPIOC->BSRR = 0x2000;
    }
}

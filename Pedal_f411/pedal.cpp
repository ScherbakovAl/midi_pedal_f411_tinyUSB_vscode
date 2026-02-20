#include "pedal.hpp"

using uint = unsigned int;
using cuint = const uint;

cuint extpr0 = 1;
cuint extpr1 = 2;
cuint extpr2 = 4;
cuint extpr3 = 8;
int32_t t = 0;
int32_t p = 0;
int f = 0;

enum class pedal_type {
    a = EXTI_IMR_MR0,
    b = EXTI_IMR_MR1,
    c = EXTI_IMR_MR2,
    d = EXTI_IMR_MR3
};

enum class pedal_condition {
    none = 0, worked, pressed, free
};

struct pedals {
    pedal_type ped = pedal_type::a;
    uint32_t time = 0;
    pedal_condition condition = pedal_condition::none;
};

static constexpr uint32_t RING_BUF_SIZE = 8u; // должно быть степенью 2 для быстрого вычисления остатка
static_assert((RING_BUF_SIZE& (RING_BUF_SIZE - 1u)) == 0u, "RING_BUF_SIZE must be power of 2");

struct RingBuf {
    pedals buf[RING_BUF_SIZE] = {};
    volatile uint32_t write_idx = 0u; // пишет только ISR
    volatile uint32_t read_idx = 0u; // читает/пишет только main loop

    void push(const pedals& item) {
        uint32_t next = (write_idx + 1u) & (RING_BUF_SIZE - 1u);
        if (next != read_idx) { // буфер не переполнен
            buf[write_idx] = item;
            // Барьер памяти: данные buf[] должны быть записаны ДО обновления write_idx,
            // чтобы main loop не прочитал незаписанный элемент.
            __DMB();
            write_idx = next;
        }
        // Если буфер полон — событие теряется (защита от переполнения).
    }

    bool empty() const {
        return read_idx == write_idx;
    }

    // Возвращает указатель на front-элемент (не удаляет).
    // Вызывать только если !empty().
    pedals& front() {
        return buf[read_idx];
    }

    // Удаляет front-элемент. Вызывать только если !empty().
    void pop_front() {
        // Барьер памяти: убеждаемся, что мы закончили читать buf[] до сдвига read_idx.
        __DMB();
        read_idx = (read_idx + 1u) & (RING_BUF_SIZE - 1u);
    }
};

static RingBuf dequePedals;

uint32_t timeLength(const uint& t1, const uint& t2) {
    uint32_t tOut;
    if (t1 > t2) {
        tOut = (4'294'967'295 - t1) + t2 + 1;
    }
    else {
        tOut = t2 - t1;
    }
    return tOut;
}

void pedal() {

    HAL_ADC_Start_IT(&hadc1);
    HAL_TIM_Base_Start(&htim3);
    HAL_Delay(15);
    pwr();
    HAL_TIM_Base_Start(&htim5); // 100us
    HAL_TIM_Base_Start_IT(&htim2);
    f = 1;
    GPIOC->BSRR = 0x20000000;

    board_init_usb();
    tud_init(0);

    while (1) {
        if (!dequePedals.empty()) {
            auto timer = TIM5->CNT;
            auto& dP = dequePedals.front();
            const uint32_t t = timeLength(dP.time, timer);

            if (t > 800 && dP.condition == pedal_condition::worked) {
                if (dP.ped == pedal_type::a) {
                    if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_RESET) {
                        dP.condition = pedal_condition::pressed;
                        MidiSender(60, 44);
                        GPIOC->BSRR = 0x2000;
                    }
                }
                if (dP.ped == pedal_type::b) {
                    if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1) == GPIO_PIN_RESET) {
                        dP.condition = pedal_condition::pressed;
                        MidiSender(61, 33);
                        GPIOC->BSRR = 0x2000;
                    }
                }
                if (dP.ped == pedal_type::c) {
                    if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_2) == GPIO_PIN_RESET) {
                        dP.condition = pedal_condition::pressed;
                        KeySender(0x4F);
                        GPIOC->BSRR = 0x2000;
                    }
                }
                if (dP.ped == pedal_type::d) {
                    if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_3) == GPIO_PIN_RESET) {
                        dP.condition = pedal_condition::pressed;
                        KeySender(0x50);
                        GPIOC->BSRR = 0x2000;
                    }
                }
            }

            if (t > 2000 && dP.condition == pedal_condition::pressed) {
                EXTI->IMR |= (uint32_t)dP.ped;
                if (dP.ped == pedal_type::c || dP.ped == pedal_type::d) {
                    tud_hid_keyboard_report(0, 0, NULL);
                }
                dequePedals.pop_front();
                GPIOC->BSRR = 0x20000000;
            }

            if (t > 3000 && dP.condition == pedal_condition::worked) {
                EXTI->IMR |= (uint32_t)dP.ped;
                dequePedals.pop_front();
                GPIOC->BSRR = 0x20000000;
            }
        }
        tud_task();
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
        dequePedals.push({ pedal_type::a, TIM5->CNT, pedal_condition::worked });
    }

    void EXTI1_IRQHandler(void) {
        EXTI->PR = extpr1;
        EXTI->IMR &= ~(EXTI_IMR_MR1);
        dequePedals.push({ pedal_type::b, TIM5->CNT, pedal_condition::worked });
    }

    void EXTI2_IRQHandler(void) {
        EXTI->PR = extpr2;
        EXTI->IMR &= ~(EXTI_IMR_MR2);
        dequePedals.push({ pedal_type::c, TIM5->CNT, pedal_condition::worked });
    }

    void EXTI3_IRQHandler(void) {
        EXTI->PR = extpr3;
        EXTI->IMR &= ~(EXTI_IMR_MR3);
        dequePedals.push({ pedal_type::d, TIM5->CNT, pedal_condition::worked });
    }
}

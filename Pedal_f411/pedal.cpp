#include "pedal.hpp"

using uint = unsigned int;
using cuint = const uint;

static constexpr uint32_t LED_ON  = GPIO_PIN_13;                  // BSRR set
static constexpr uint32_t LED_OFF = GPIO_PIN_13 << 16u;           // BSRR reset

static constexpr uint32_t DEBOUNCE_TICKS  =  800u;  // 80 мс  — антидребезг
static constexpr uint32_t RELEASE_TICKS   = 2000u;  // 200 мс — ожидание отпускания
static constexpr uint32_t TIMEOUT_TICKS   = 3000u;  // 300 мс — таймаут ложного срабатывания

int32_t t = 0;
int32_t p = 0;
uint8_t u = 0;
uint8_t r = 0;
volatile int f = 0;

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
    uint32_t time = 0u;
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

static RingBuf vPedals;

static inline void exti_enable(pedal_type ped) {
    __disable_irq();
    EXTI->IMR |= static_cast<uint32_t>(ped);
    __enable_irq();
}

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
    GPIOC->BSRR = LED_OFF;

    board_init_usb();
    tud_init(0);

    while (1) {
        if (!vPedals.empty()) {
            auto now = TIM5->CNT;
            auto& vP = vPedals.front();
            const uint32_t t = timeLength(vP.time, now);

            if (t > DEBOUNCE_TICKS && vP.condition == pedal_condition::worked) {
                if (vP.ped == pedal_type::a) {
                    if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_RESET) {
                        vP.condition = pedal_condition::pressed;
                        MidiSender(60, 44);
                        GPIOC->BSRR = LED_ON;
                    }
                }
                if (vP.ped == pedal_type::b) {
                    if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1) == GPIO_PIN_RESET) {
                        vP.condition = pedal_condition::pressed;
                        MidiSender(61, 33);
                        GPIOC->BSRR = LED_ON;
                    }
                }
                if (vP.ped == pedal_type::c) {
                    if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_2) == GPIO_PIN_RESET) {
                        vP.condition = pedal_condition::pressed;
                        KeySender(0x4F);
                        GPIOC->BSRR = LED_ON;
                    }
                }
                if (vP.ped == pedal_type::d) {
                    if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_3) == GPIO_PIN_RESET) {
                        vP.condition = pedal_condition::pressed;
                        KeySender(0x50);
                        GPIOC->BSRR = LED_ON;
                    }
                }
            }

            if (t > RELEASE_TICKS && vP.condition == pedal_condition::pressed) {
                exti_enable(vP.ped);
                if (vP.ped == pedal_type::c || vP.ped == pedal_type::d) {
                    tud_hid_keyboard_report(0, 0, NULL);
                }
                vPedals.pop_front();
                GPIOC->BSRR = LED_OFF;
            }

            if (t > TIMEOUT_TICKS && vP.condition == pedal_condition::worked) {
                exti_enable(vP.ped);
                vPedals.pop_front();
                GPIOC->BSRR = LED_OFF;
            }
        }
        tud_task();
    }
}

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

void MidiSender(const uint8_t note, const uint8_t velocity) {
    uint8_t note_on[3] = { 0x91, note, velocity };
    tud_midi_stream_write(0, note_on, sizeof(note_on));
    r = velocity;
    TIM2->CNT = 0;
}

void KeySender(const uint8_t command) {
    if (tud_hid_ready()) {
        uint8_t keycode[6] = { command, 0, 0, 0, 0, 0 };
        tud_hid_keyboard_report(0, 0, keycode);
    }
}

extern "C" {
    void EXTI0_IRQHandler(void) { // disable "IRQHandlers" in stm32f4xx_it.c
        EXTI->PR = EXTI_PR_PR0;
        EXTI->IMR &= ~EXTI_IMR_MR0;
        vPedals.push({ pedal_type::a, TIM5->CNT, pedal_condition::worked });
    }

    void EXTI1_IRQHandler(void) {
        EXTI->PR = EXTI_PR_PR1;
        EXTI->IMR &= ~EXTI_IMR_MR1;
        vPedals.push({ pedal_type::b, TIM5->CNT, pedal_condition::worked });
    }

    void EXTI2_IRQHandler(void) {
        EXTI->PR = EXTI_PR_PR2;
        EXTI->IMR &= ~EXTI_IMR_MR2;
        vPedals.push({ pedal_type::c, TIM5->CNT, pedal_condition::worked });
    }

    void EXTI3_IRQHandler(void) {
        EXTI->PR = EXTI_PR_PR3;
        EXTI->IMR &= ~EXTI_IMR_MR3;
        vPedals.push({ pedal_type::d, TIM5->CNT, pedal_condition::worked });
    }
}

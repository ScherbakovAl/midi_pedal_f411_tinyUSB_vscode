#include "pedal.hpp"

// ---------------------------------------------------------------------------
// Константы
// ---------------------------------------------------------------------------

// Биты сброса флагов EXTI Pending Register (PR).
// Используем стандартные CMSIS-макросы вместо магических чисел.
static constexpr uint32_t EXTI_PR0 = EXTI_PR_PR0;
static constexpr uint32_t EXTI_PR1 = EXTI_PR_PR1;
static constexpr uint32_t EXTI_PR2 = EXTI_PR_PR2;
static constexpr uint32_t EXTI_PR3 = EXTI_PR_PR3;

// Светодиод индикации (PC13)
static constexpr uint32_t LED_ON  = GPIO_PIN_13;                  // BSRR set
static constexpr uint32_t LED_OFF = GPIO_PIN_13 << 16u;           // BSRR reset

// Пороги дребезга и таймаутов (единицы — тики TIM5, 100 мкс/тик)
static constexpr uint32_t DEBOUNCE_TICKS  =  800u;  // 80 мс  — антидребезг
static constexpr uint32_t RELEASE_TICKS   = 2000u;  // 200 мс — ожидание отпускания
static constexpr uint32_t TIMEOUT_TICKS   = 3000u;  // 300 мс — таймаут ложного срабатывания

// Пороги фильтрации АЦП
static constexpr int32_t  ADC_MIN         = 300;
static constexpr int32_t  ADC_HYSTERESIS  = 14;
static constexpr uint8_t  MIDI_CC_CHANNEL = 176u;   // 0xB0 — Control Change, канал 1
static constexpr uint8_t  MIDI_CC_NUM     = 64u;    // CC#64 — Sustain Pedal
static constexpr uint8_t  MIDI_NOTE_CH    = 0x91u;  // Note On, канал 2
static constexpr uint8_t  MIDI_CC_SCALE   = 30u;
static constexpr uint8_t  MIDI_CC_OFFSET  = 9u;
static constexpr uint8_t  MIDI_CC_MAX     = 127u;

// ---------------------------------------------------------------------------
// Типы
// ---------------------------------------------------------------------------

enum class pedal_type : uint32_t {
    a = EXTI_IMR_MR0,
    b = EXTI_IMR_MR1,
    c = EXTI_IMR_MR2,
    d = EXTI_IMR_MR3
};

enum class pedal_condition {
    none = 0, worked, pressed, free
};

struct pedals {
    pedal_type      ped       = pedal_type::a;
    uint32_t        time      = 0u;
    pedal_condition condition = pedal_condition::none;
};

// ---------------------------------------------------------------------------
// Кольцевой буфер (lock-free SPSC: ISR пишет, main loop читает)
// ---------------------------------------------------------------------------

static constexpr uint32_t RING_BUF_SIZE = 8u;
static_assert((RING_BUF_SIZE & (RING_BUF_SIZE - 1u)) == 0u,
              "RING_BUF_SIZE must be power of 2");

struct RingBuf {
    pedals           buf[RING_BUF_SIZE] = {};
    volatile uint32_t write_idx = 0u; // пишет только ISR
    volatile uint32_t read_idx  = 0u; // читает/пишет только main loop

    void push(const pedals& item) {
        uint32_t next = (write_idx + 1u) & (RING_BUF_SIZE - 1u);
        if (next != read_idx) {
            buf[write_idx] = item;
            // Барьер памяти: данные buf[] должны быть записаны ДО обновления
            // write_idx, чтобы main loop не прочитал незаписанный элемент.
            __DMB();
            write_idx = next;
        }
        // Если буфер полон — событие теряется (защита от переполнения).
    }

    bool empty() const {
        return read_idx == write_idx;
    }

    // Возвращает ссылку на front-элемент (не удаляет).
    // Вызывать только если !empty().
    pedals& front() {
        return buf[read_idx];
    }

    // Удаляет front-элемент. Вызывать только если !empty().
    void pop_front() {
        // Барьер памяти: убеждаемся, что закончили читать buf[] до сдвига read_idx.
        __DMB();
        read_idx = (read_idx + 1u) & (RING_BUF_SIZE - 1u);
    }
};

static RingBuf dequePedals;

// ---------------------------------------------------------------------------
// Глобальные переменные, разделяемые между main loop и ISR.
// Все должны быть volatile, чтобы компилятор не кешировал их в регистрах.
// ---------------------------------------------------------------------------

static volatile int32_t  adc_raw  = 0;   // последнее сырое значение АЦП
static volatile int32_t  adc_prev = 0;   // предыдущее значение (для гистерезиса)
static volatile int      adc_ready = 0;  // флаг: АЦП инициализировано и готово

// Последнее отправленное значение CC (только для АЦП-педали).
// Не смешивать с velocity нот!
static volatile uint8_t  cc_last = 0;

// ---------------------------------------------------------------------------
// Вспомогательные функции
// ---------------------------------------------------------------------------

// Разность двух тиков таймера с учётом переполнения uint32_t.
// Беззнаковое вычитание само корректно обрабатывает переполнение по стандарту C++.
static inline uint32_t ticksDiff(uint32_t t_start, uint32_t t_now) {
    return t_now - t_start;
}

// Восстановить прерывание для данной педали (атомарно относительно ISR).
// Операция read-modify-write над EXTI->IMR должна быть защищена,
// т.к. ISR может одновременно сбрасывать другой бит.
static inline void exti_enable(pedal_type ped) {
    __disable_irq();
    EXTI->IMR |= static_cast<uint32_t>(ped);
    __enable_irq();
}

// ---------------------------------------------------------------------------
// Основной цикл
// ---------------------------------------------------------------------------

void pedal() {

    HAL_ADC_Start_IT(&hadc1);
    HAL_TIM_Base_Start(&htim3);
    HAL_Delay(15);
    pwr();
    HAL_TIM_Base_Start(&htim5);       // счётчик тиков 100 мкс
    HAL_TIM_Base_Start_IT(&htim2);
    adc_ready = 1;
    GPIOC->BSRR = LED_OFF;

    board_init_usb();
    tud_init(0);

    while (1) {
        if (!dequePedals.empty()) {
            const uint32_t now   = TIM5->CNT;
            auto& dP             = dequePedals.front();
            const uint32_t elapsed = ticksDiff(dP.time, now);

            // --- Антидребезг: проверяем состояние пина после DEBOUNCE_TICKS ---
            if (elapsed > DEBOUNCE_TICKS && dP.condition == pedal_condition::worked) {
                switch (dP.ped) {
                    case pedal_type::a:
                        if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_RESET) {
                            dP.condition = pedal_condition::pressed;
                            MidiSender(60, 44);
                            GPIOC->BSRR = LED_ON;
                        }
                        break;
                    case pedal_type::b:
                        if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1) == GPIO_PIN_RESET) {
                            dP.condition = pedal_condition::pressed;
                            MidiSender(61, 33);
                            GPIOC->BSRR = LED_ON;
                        }
                        break;
                    case pedal_type::c:
                        if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_2) == GPIO_PIN_RESET) {
                            dP.condition = pedal_condition::pressed;
                            KeySender(0x4F);
                            GPIOC->BSRR = LED_ON;
                        }
                        break;
                    case pedal_type::d:
                        if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_3) == GPIO_PIN_RESET) {
                            dP.condition = pedal_condition::pressed;
                            KeySender(0x50);
                            GPIOC->BSRR = LED_ON;
                        }
                        break;
                }
            }
            // --- Ожидание отпускания педали ---
            else if (elapsed > RELEASE_TICKS && dP.condition == pedal_condition::pressed) {
                exti_enable(dP.ped);
                if (dP.ped == pedal_type::c || dP.ped == pedal_type::d) {
                    tud_hid_keyboard_report(0, 0, NULL);
                }
                dequePedals.pop_front();
                GPIOC->BSRR = LED_OFF;
            }
            // --- Таймаут: ложное срабатывание (пин уже отпущен) ---
            else if (elapsed > TIMEOUT_TICKS && dP.condition == pedal_condition::worked) {
                exti_enable(dP.ped);
                dequePedals.pop_front();
                GPIOC->BSRR = LED_OFF;
            }
        }
        tud_task();
    }
}

// ---------------------------------------------------------------------------
// Колбэк АЦП (ISR-контекст)
// ---------------------------------------------------------------------------

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc) {
    if (adc_ready == 1 && hadc->Instance == ADC1) {
        int32_t raw = (int32_t)HAL_ADC_GetValue(&hadc1);
        if (raw < ADC_MIN) {
            raw = ADC_MIN;
        }

        // Гистерезис: отправляем только при значимом изменении
        if ((raw - adc_prev > ADC_HYSTERESIS) || (adc_prev - raw > ADC_HYSTERESIS)) {
            // Масштабирование в диапазон MIDI CC [0..127]
            int32_t cc_val = raw / MIDI_CC_SCALE - MIDI_CC_OFFSET;
            if (cc_val < 0)            cc_val = 0;
            if (cc_val > MIDI_CC_MAX)  cc_val = MIDI_CC_MAX;

            uint8_t cc = (uint8_t)cc_val;
            if (cc_last != cc) {
                uint8_t msg[3] = { MIDI_CC_CHANNEL, MIDI_CC_NUM, cc };
                tud_midi_stream_write(0, msg, sizeof(msg));
                cc_last = cc;
                TIM2->CNT = 0;
            }
            adc_prev = raw;
        }
        adc_raw = raw;
    }
}

// ---------------------------------------------------------------------------
// Отправка MIDI Note On
// ---------------------------------------------------------------------------

void MidiSender(const uint8_t note, const uint8_t velocity) {
    uint8_t msg[3] = { MIDI_NOTE_CH, note, velocity };
    tud_midi_stream_write(0, msg, sizeof(msg));
    // Примечание: cc_last НЕ трогаем — это переменная только для АЦП-педали.
    TIM2->CNT = 0;
}

// ---------------------------------------------------------------------------
// Отправка HID-клавиши
// ---------------------------------------------------------------------------

void KeySender(const uint8_t k) {
    if (tud_hid_ready()) {
        uint8_t keycode[6] = { k, 0, 0, 0, 0, 0 };
        tud_hid_keyboard_report(0, 0, keycode);
    }
}

// ---------------------------------------------------------------------------
// Обработчики прерываний EXTI (педали 0..3)
// Отключаем прерывание сразу, чтобы не получать дребезг.
// Повторно включаем из main loop после обработки.
// ---------------------------------------------------------------------------

extern "C" {
    void EXTI0_IRQHandler(void) { // отключить дублирующий обработчик в stm32f4xx_it.c
        EXTI->PR = EXTI_PR0;
        EXTI->IMR &= ~EXTI_IMR_MR0;
        dequePedals.push({ pedal_type::a, TIM5->CNT, pedal_condition::worked });
    }

    void EXTI1_IRQHandler(void) {
        EXTI->PR = EXTI_PR1;
        EXTI->IMR &= ~EXTI_IMR_MR1;
        dequePedals.push({ pedal_type::b, TIM5->CNT, pedal_condition::worked });
    }

    void EXTI2_IRQHandler(void) {
        EXTI->PR = EXTI_PR2;
        EXTI->IMR &= ~EXTI_IMR_MR2;
        dequePedals.push({ pedal_type::c, TIM5->CNT, pedal_condition::worked });
    }

    void EXTI3_IRQHandler(void) {
        EXTI->PR = EXTI_PR3;
        EXTI->IMR &= ~EXTI_IMR_MR3;
        dequePedals.push({ pedal_type::d, TIM5->CNT, pedal_condition::worked });
    }
}

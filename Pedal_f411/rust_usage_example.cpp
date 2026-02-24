/**
 * @file rust_usage_example.cpp
 * @brief Пример использования Rust функций из C++ кода
 * 
 * Этот файл демонстрирует как постепенно переносить логику с C++ на Rust.
 * Для начала работы с Rust:
 * 1. Установите Rust: curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
 * 2. Добавьте целевую платформу: rustup target add thumbv7em-none-eabihf
 * 3. Соберите проект с Rust: cmake -DCMAKE_BUILD_TYPE=Release ..
 * 4. После успешной сборки Rust кода, можно постепенно переносить логику
 */

#include "logic_rust.h"  // Заголовочный файл с объявлениями Rust функций

// ============================================================================
// Пример 1: Вызов Rust функции из обработчика прерывания
// ============================================================================

/*
// В файле stm32f4xx_it.c или pedal.cpp можно изменить обработчики прерываний:

// Было (C++):
void EXTI0_IRQHandler(void) {
    EXTI->PR = EXTI_PR_PR0;
    EXTI->IMR &= ~EXTI_IMR_MR0;
    vPedals.push({ pedal_type::a, TIM5->CNT, pedal_condition::worked });
}

// Станет (C++ вызывает Rust):
void EXTI0_IRQHandler(void) {
    // Очищаем флаг прерывания на C
    EXTI->PR = EXTI_PR_PR0;
    // Логика обработки на Rust
    rust_exti0_handler(TIM5->CNT);
}
*/

// ============================================================================
// Пример 2: Использование Rust функций для MIDI/USB
// ============================================================================

/*
// Отправка MIDI Note через Rust:
void MidiSender(const uint8_t note, const uint8_t velocity) {
    // Старый код:
    // uint8_t note_on[3] = { MIDI_NOTE_CH, note, velocity };
    // tud_midi_stream_write(0, note_on, sizeof(note_on));
    // TIM2->CNT = 0;
    
    // Новый код (вызов Rust):
    rust_send_midi_note(MIDI_NOTE_CH, note, velocity);
}
*/

// ============================================================================
// Пример 3: Управление LED через Rust
// ============================================================================

/*
// Включение LED:
void led_on() {
    // Старый код:
    // GPIOC->BSRR = LED_ON;
    
    // Новый код:
    rust_led_on();
}

// Выключение LED:
void led_off() {
    // Старый код:
    // GPIOC->BSRR = LED_OFF;
    
    // Новый код:
    rust_led_off();
}
*/

// ============================================================================
// Пример 4: Обработка ADC в Rust
// ============================================================================

/*
// Callback ADC можно перенести на Rust:
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc) {
    if (pwr_flag && hadc->Instance == ADC1) {
        uint32_t adc_raw = HAL_ADC_GetValue(&hadc1);
        
        // Вызов Rust функции для обработки
        rust_adc_callback(adc_raw);
        
        // Или вся логика может быть в Rust:
        // rust_process_adc(adc_raw, &cc_velocity, &cc_velocity_prev, &adc_prev);
    }
}
*/

// ============================================================================
// Пример 5: Проверка состояния педалей через Rust
// ============================================================================

/*
// Чтение состояния педали:
bool is_pedal_pressed(uint8_t pedal_num) {
    return rust_read_pedal_gpio(pedal_num);
}

// Включение EXTI линии:
void enable_pedal_exti(uint8_t pedal_num) {
    rust_exti_enable(pedal_num);
}
*/

// ============================================================================
// Пример 6: Полная инициализация Rust модуля
// ============================================================================

/*
// При старте приложения:
void app_init() {
    // ... инициализация HAL ...
    
    // Инициализация Rust логики
    rust_logic_init();
    
    // ... остальная инициализация ...
}
*/

// ============================================================================
// Тестирование сборки Rust
// ============================================================================

// Для проверки что Rust компилируется, можно добавить вызов:
void test_rust_functions(void) {
    // Тестирование базовых функций (только для отладки)
    // rust_logic_init();  // Раскомментировать для теста
    
    // rust_led_on();
    // rust_led_off();
    
    // uint8_t pedal_state = rust_read_pedal_gpio(0);
}

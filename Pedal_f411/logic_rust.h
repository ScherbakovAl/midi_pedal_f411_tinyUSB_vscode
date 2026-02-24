/**
 * @file logic_rust.h
 * @brief Заголовочный файл для вызова Rust функций из C кода
 * 
 * Этот файл объявляет функции, реализованные в Rust, чтобы их можно было
 * вызывать из C/C++ кода (в том числе из обработчиков прерываний).
 */

#ifndef LOGIC_RUST_H
#define LOGIC_RUST_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// Функции обратного вызова для прерываний EXTI
// =============================================================================

/**
 * @brief Обработчик прерывания EXTI0 (педаль A)
 * @param timestamp Значение таймера TIM5 в момент прерывания
 */
void rust_exti0_handler(uint32_t timestamp);

/**
 * @brief Обработчик прерывания EXTI1 (педаль B)
 * @param timestamp Значение таймера TIM5 в момент прерывания
 */
void rust_exti1_handler(uint32_t timestamp);

/**
 * @brief Обработчик прерывания EXTI2 (педаль C)
 * @param timestamp Значение таймера TIM5 в момент прерывания
 */
void rust_exti2_handler(uint32_t timestamp);

/**
 * @brief Обработчик прерывания EXTI3 (педаль D)
 * @param timestamp Значение таймера TIM5 в момент прерывания
 */
void rust_exti3_handler(uint32_t timestamp);

// =============================================================================
// Функции обратного вызова для ADC
// =============================================================================

/**
 * @brief Callback для завершения ADC преобразования
 * @param raw_value Сырое значение АЦП
 */
void rust_adc_callback(uint32_t raw_value);

// =============================================================================
// Функции для отправки MIDI/USB из Rust
// =============================================================================

/**
 * @brief Отправка MIDI Note On
 * @param channel MIDI канал
 * @param note Нота
 * @param velocity Velocity (громкость)
 */
void rust_send_midi_note(uint8_t channel, uint8_t note, uint8_t velocity);

/**
 * @brief Отправка MIDI Control Change
 * @param cc_num Номер CC
 * @param value Значение CC (0-127)
 */
void rust_send_midi_cc(uint8_t cc_num, uint8_t value);

/**
 * @brief Отправка Keyboard HID репорта
 * @param keycode Код клавиши
 */
void rust_send_keyboard(uint8_t keycode);

/**
 * @brief Освобождение всех клавиш (Key release)
 */
void rust_release_keyboard(void);

// =============================================================================
// Функции для работы с LED
// =============================================================================

/**
 * @brief Включить LED
 */
void rust_led_on(void);

/**
 * @brief Выключить LED
 */
void rust_led_off(void);

// =============================================================================
// Функции для чтения состояния педалей
// =============================================================================

/**
 * @brief Чтение состояния педали через GPIO
 * @param pedal Номер педали (0=A, 1=B, 2=C, 3=D)
 * @return true если нажата, false иначе
 */
bool rust_read_pedal_gpio(uint8_t pedal);

// =============================================================================
// Вспомогательные функции
// =============================================================================

/**
 * @brief Включение EXTI линии
 * @param pedal Номер педали (0-3)
 */
void rust_exti_enable(uint8_t pedal);

/**
 * @brief Инициализация Rust логики
 * Вызывается при старте для настройки Rust модуля
 */
void rust_logic_init(void);

#ifdef __cplusplus
}
#endif

#endif // LOGIC_RUST_H

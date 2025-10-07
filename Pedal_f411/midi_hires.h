/**
 * @file midi_hires.h
 * @brief Hi-Res MIDI (14-бит) функции для STM32 + TinyUSB
 * 
 * Этот файл содержит функции для работы с 14-битными MIDI сообщениями
 * через стандартный MIDI 1.0 протокол (MSB/LSB пары)
 */

#ifndef MIDI_HIRES_H
#define MIDI_HIRES_H

#include <stdint.h>
#include "tusb.h"

#ifdef __cplusplus
extern "C" {
#endif

// Константы для Hi-Res MIDI
#define MIDI_14BIT_MAX      16383   // Максимальное 14-бит значение
#define MIDI_14BIT_CENTER   8192    // Центральное значение для Pitch Bend
#define MIDI_7BIT_MAX       127     // Максимальное 7-бит значение

// CC номера для Hi-Res (MSB)
#define MIDI_CC_BANK_SELECT_MSB     0
#define MIDI_CC_MODULATION_MSB      1
#define MIDI_CC_BREATH_MSB          2
#define MIDI_CC_VOLUME_MSB          7
#define MIDI_CC_PAN_MSB             10
#define MIDI_CC_EXPRESSION_MSB      11

// Соответствующие LSB (MSB + 32)
#define MIDI_CC_BANK_SELECT_LSB     32
#define MIDI_CC_MODULATION_LSB      33
#define MIDI_CC_BREATH_LSB          34
#define MIDI_CC_VOLUME_LSB          39
#define MIDI_CC_PAN_LSB             42
#define MIDI_CC_EXPRESSION_LSB      43

/**
 * @brief Отправка 14-бит Pitch Bend сообщения
 * 
 * @param cable_num MIDI cable номер (обычно 0)
 * @param channel MIDI канал (0-15)
 * @param value Значение pitch bend от -8192 до +8191 (0 = центр)
 * 
 * @note Pitch Bend всегда использует 14-бит разрешение
 */
static inline void midi_send_pitch_bend_14bit(uint8_t cable_num, uint8_t channel, int16_t value) {
    // Преобразуем в беззнаковое 0-16383 (центр = 8192)
    uint16_t unsigned_value = (uint16_t)(value + MIDI_14BIT_CENTER);
    
    // Разделяем на LSB и MSB
    uint8_t lsb = unsigned_value & 0x7F;        // Младшие 7 бит
    uint8_t msb = (unsigned_value >> 7) & 0x7F; // Старшие 7 бит
    
    // Формируем Pitch Bend сообщение
    uint8_t pitch_bend[3] = {
        (uint8_t)(0xE0 | (channel & 0x0F)),  // Pitch Bend + канал
        lsb,                                  // LSB первым!
        msb                                   // MSB вторым
    };
    
    tud_midi_stream_write(cable_num, pitch_bend, 3);
}

/**
 * @brief Отправка 14-бит Control Change сообщения
 * 
 * @param cable_num MIDI cable номер (обычно 0)
 * @param channel MIDI канал (0-15)
 * @param cc_msb Номер CC контроллера (MSB: 0-31)
 * @param value Значение 0-16383 (14-бит)
 * 
 * @note Автоматически отправляет MSB и LSB (cc_msb + 32)
 */
static inline void midi_send_cc_14bit(uint8_t cable_num, uint8_t channel, 
                                      uint8_t cc_msb, uint16_t value) {
    // Ограничиваем значение
    if (value > MIDI_14BIT_MAX) value = MIDI_14BIT_MAX;
    
    uint8_t msb = (value >> 7) & 0x7F;  // Старшие 7 бит
    uint8_t lsb = value & 0x7F;         // Младшие 7 бит
    
    // Отправляем MSB (ВАЖНО: сначала MSB!)
    uint8_t cc_msb_msg[3] = {
        (uint8_t)(0xB0 | (channel & 0x0F)),  // Control Change + канал
        cc_msb & 0x1F,                        // CC номер (0-31)
        msb                                   // Значение MSB
    };
    tud_midi_stream_write(cable_num, cc_msb_msg, 3);
    
    // Отправляем LSB (CC + 32)
    uint8_t cc_lsb_msg[3] = {
        (uint8_t)(0xB0 | (channel & 0x0F)),  // Control Change + канал
        (cc_msb & 0x1F) + 32,                 // CC номер LSB = MSB + 32
        lsb                                   // Значение LSB
    };
    tud_midi_stream_write(cable_num, cc_lsb_msg, 3);
}

/**
 * @brief Отправка 7-бит Control Change сообщения (стандартное)
 * 
 * @param cable_num MIDI cable номер (обычно 0)
 * @param channel MIDI канал (0-15)
 * @param cc_num Номер CC контроллера (0-127)
 * @param value Значение 0-127 (7-бит)
 */
static inline void midi_send_cc_7bit(uint8_t cable_num, uint8_t channel, 
                                     uint8_t cc_num, uint8_t value) {
    uint8_t cc_msg[3] = {
        (uint8_t)(0xB0 | (channel & 0x0F)),  // Control Change + канал
        cc_num & 0x7F,                        // CC номер
        value & 0x7F                          // Значение
    };
    tud_midi_stream_write(cable_num, cc_msg, 3);
}

// ============================================================================
// Специализированные функции для популярных контроллеров
// ============================================================================

/**
 * @brief Отправка Volume (CC 7/39) в 14-бит
 * @param value 0-16383 (0 = тихо, 16383 = громко)
 */
static inline void midi_send_volume_14bit(uint8_t cable_num, uint8_t channel, uint16_t value) {
    midi_send_cc_14bit(cable_num, channel, MIDI_CC_VOLUME_MSB, value);
}

/**
 * @brief Отправка Expression (CC 11/43) в 14-бит
 * @param value 0-16383 (педаль экспрессии)
 */
static inline void midi_send_expression_14bit(uint8_t cable_num, uint8_t channel, uint16_t value) {
    midi_send_cc_14bit(cable_num, channel, MIDI_CC_EXPRESSION_MSB, value);
}

/**
 * @brief Отправка Pan (CC 10/42) в 14-бит
 * @param value 0-16383 (0 = левый, 8192 = центр, 16383 = правый)
 */
static inline void midi_send_pan_14bit(uint8_t cable_num, uint8_t channel, uint16_t value) {
    midi_send_cc_14bit(cable_num, channel, MIDI_CC_PAN_MSB, value);
}

/**
 * @brief Отправка Modulation (CC 1/33) в 14-бит
 * @param value 0-16383 (колесо модуляции)
 */
static inline void midi_send_modulation_14bit(uint8_t cable_num, uint8_t channel, uint16_t value) {
    midi_send_cc_14bit(cable_num, channel, MIDI_CC_MODULATION_MSB, value);
}

// ============================================================================
// Утилиты для преобразования значений
// ============================================================================

/**
 * @brief Преобразование 12-бит ADC в 14-бит MIDI
 * @param adc_value Значение АЦП (0-4095)
 * @return 14-бит MIDI значение (0-16383)
 */
static inline uint16_t midi_adc12_to_14bit(uint16_t adc_value) {
    return (adc_value & 0x0FFF) << 2;  // Сдвиг на 2 бита влево
}

/**
 * @brief Преобразование 10-бит ADC в 14-бит MIDI
 * @param adc_value Значение АЦП (0-1023)
 * @return 14-бит MIDI значение (0-16383)
 */
static inline uint16_t midi_adc10_to_14bit(uint16_t adc_value) {
    return (adc_value & 0x03FF) << 4;  // Сдвиг на 4 бита влево
}

/**
 * @brief Преобразование процентов в 14-бит MIDI
 * @param percent Проценты (0-100)
 * @return 14-бит MIDI значение (0-16383)
 */
static inline uint16_t midi_percent_to_14bit(uint8_t percent) {
    if (percent > 100) percent = 100;
    return (uint16_t)((percent * MIDI_14BIT_MAX) / 100);
}

/**
 * @brief Преобразование 7-бит в 14-бит (расширение разрешения)
 * @param value_7bit 7-бит значение (0-127)
 * @return 14-бит MIDI значение (0-16383)
 */
static inline uint16_t midi_7bit_to_14bit(uint8_t value_7bit) {
    return (uint16_t)(value_7bit & 0x7F) << 7;
}

/**
 * @brief Преобразование 14-бит в 7-бит (потеря разрешения)
 * @param value_14bit 14-бит значение (0-16383)
 * @return 7-бит MIDI значение (0-127)
 */
static inline uint8_t midi_14bit_to_7bit(uint16_t value_14bit) {
    return (uint8_t)((value_14bit >> 7) & 0x7F);
}

// ============================================================================
// Структура для декодирования принятых Hi-Res сообщений
// ============================================================================

typedef struct {
    uint8_t msb_values[32];      // Хранилище MSB значений (CC 0-31)
    uint8_t msb_received[32];    // Флаги получения MSB
} midi_hires_decoder_t;

/**
 * @brief Инициализация декодера Hi-Res MIDI
 */
static inline void midi_hires_decoder_init(midi_hires_decoder_t* decoder) {
    for (int i = 0; i < 32; i++) {
        decoder->msb_values[i] = 0;
        decoder->msb_received[i] = 0;
    }
}

/**
 * @brief Обработка принятого CC сообщения
 * 
 * @param decoder Указатель на структуру декодера
 * @param cc_num Номер CC (0-127)
 * @param value Значение CC (0-127)
 * @param out_value Выходное 14-бит значение (если собрано)
 * @return 1 если собрано полное 14-бит значение, 0 если ещё ждём LSB
 */
static inline int midi_hires_decoder_process(midi_hires_decoder_t* decoder, 
                                             uint8_t cc_num, uint8_t value,
                                             uint16_t* out_value) {
    if (cc_num < 32) {
        // Это MSB
        decoder->msb_values[cc_num] = value;
        decoder->msb_received[cc_num] = 1;
        return 0;  // Ждём LSB
    } 
    else if (cc_num >= 32 && cc_num < 64) {
        // Это LSB
        uint8_t msb_num = cc_num - 32;
        
        if (decoder->msb_received[msb_num]) {
            // Собираем 14-бит значение
            *out_value = ((uint16_t)decoder->msb_values[msb_num] << 7) | (value & 0x7F);
            decoder->msb_received[msb_num] = 0;
            return 1;  // Полное значение готово
        }
    }
    
    return 0;  // Не Hi-Res или неполное сообщение
}

// ============================================================================
// Hi-Res Note On/Off (14-бит velocity через CC 88)
// ============================================================================

/**
 * @brief Отправка Note On с 14-бит velocity (через CC 88)
 *
 * Использует официальный метод MIDI: CC 88 (High Resolution Velocity Prefix)
 *
 * @param cable_num MIDI cable номер (обычно 0)
 * @param channel MIDI канал (0-15)
 * @param note Номер ноты (0-127)
 * @param velocity_14bit Velocity 0-16383 (14-бит)
 *
 * @note Отправляет два сообщения:
 *       1. CC 88 с MSB velocity
 *       2. Note On с LSB velocity
 */
static inline void midi_send_note_on_14bit(uint8_t cable_num, uint8_t channel,
                                           uint8_t note, uint16_t velocity_14bit) {
    // Ограничиваем значение
    if (velocity_14bit > MIDI_14BIT_MAX) velocity_14bit = MIDI_14BIT_MAX;
    
    // Разделяем на MSB и LSB
    uint8_t velocity_msb = (velocity_14bit >> 7) & 0x7F;  // Старшие 7 бит
    uint8_t velocity_lsb = velocity_14bit & 0x7F;         // Младшие 7 бит
    
    // Шаг 1: Отправляем CC 88 (High Resolution Velocity Prefix) с MSB
    uint8_t cc88_msg[3] = {
        (uint8_t)(0xB0 | (channel & 0x0F)),  // Control Change + канал
        88,                                   // CC 88
        velocity_msb                          // MSB velocity
    };
    tud_midi_stream_write(cable_num, cc88_msg, 3);
    
    // Шаг 2: Отправляем Note On с LSB
    uint8_t note_on_msg[3] = {
        (uint8_t)(0x90 | (channel & 0x0F)),  // Note On + канал
        note & 0x7F,                          // Номер ноты
        velocity_lsb                          // LSB velocity
    };
    tud_midi_stream_write(cable_num, note_on_msg, 3);
}

/**
 * @brief Отправка Note Off с 14-бит velocity (через CC 88)
 *
 * @param cable_num MIDI cable номер (обычно 0)
 * @param channel MIDI канал (0-15)
 * @param note Номер ноты (0-127)
 * @param velocity_14bit Release velocity 0-16383 (14-бит)
 *
 * @note Отправляет два сообщения:
 *       1. CC 88 с MSB velocity
 *       2. Note Off с LSB velocity
 */
static inline void midi_send_note_off_14bit(uint8_t cable_num, uint8_t channel,
                                            uint8_t note, uint16_t velocity_14bit) {
    if (velocity_14bit > MIDI_14BIT_MAX) velocity_14bit = MIDI_14BIT_MAX;
    
    uint8_t velocity_msb = (velocity_14bit >> 7) & 0x7F;
    uint8_t velocity_lsb = velocity_14bit & 0x7F;
    
    // CC 88 с MSB
    uint8_t cc88_msg[3] = {
        (uint8_t)(0xB0 | (channel & 0x0F)),
        88,
        velocity_msb
    };
    tud_midi_stream_write(cable_num, cc88_msg, 3);
    
    // Note Off с LSB
    uint8_t note_off_msg[3] = {
        (uint8_t)(0x80 | (channel & 0x0F)),  // Note Off + канал
        note & 0x7F,
        velocity_lsb
    };
    tud_midi_stream_write(cable_num, note_off_msg, 3);
}

/**
 * @brief Отправка стандартного Note On (7-бит velocity)
 *
 * @param cable_num MIDI cable номер (обычно 0)
 * @param channel MIDI канал (0-15)
 * @param note Номер ноты (0-127)
 * @param velocity Velocity 0-127 (7-бит)
 */
static inline void midi_send_note_on_7bit(uint8_t cable_num, uint8_t channel,
                                          uint8_t note, uint8_t velocity) {
    uint8_t note_on_msg[3] = {
        (uint8_t)(0x90 | (channel & 0x0F)),
        note & 0x7F,
        velocity & 0x7F
    };
    tud_midi_stream_write(cable_num, note_on_msg, 3);
}

/**
 * @brief Отправка стандартного Note Off (7-бит velocity)
 *
 * @param cable_num MIDI cable номер (обычно 0)
 * @param channel MIDI канал (0-15)
 * @param note Номер ноты (0-127)
 * @param velocity Release velocity 0-127 (7-бит)
 */
static inline void midi_send_note_off_7bit(uint8_t cable_num, uint8_t channel,
                                           uint8_t note, uint8_t velocity) {
    uint8_t note_off_msg[3] = {
        (uint8_t)(0x80 | (channel & 0x0F)),
        note & 0x7F,
        velocity & 0x7F
    };
    tud_midi_stream_write(cable_num, note_off_msg, 3);
}

/**
 * @brief Отправка Note Off через Note On с velocity=0 (стандартный метод)
 *
 * @param cable_num MIDI cable номер (обычно 0)
 * @param channel MIDI канал (0-15)
 * @param note Номер ноты (0-127)
 *
 * @note Многие устройства предпочитают этот метод для Note Off
 */
static inline void midi_send_note_off_zero_velocity(uint8_t cable_num, uint8_t channel,
                                                    uint8_t note) {
    uint8_t note_off_msg[3] = {
        (uint8_t)(0x90 | (channel & 0x0F)),  // Note On (!)
        note & 0x7F,
        0                                     // velocity = 0 означает Note Off
    };
    tud_midi_stream_write(cable_num, note_off_msg, 3);
}

// ============================================================================
// Декодер для Hi-Res Note On/Off
// ============================================================================

typedef struct {
    uint8_t cc88_received;   // Флаг получения CC 88
    uint8_t cc88_value;      // Значение MSB из CC 88
} midi_hires_note_decoder_t;

/**
 * @brief Инициализация декодера Hi-Res Note
 */
static inline void midi_hires_note_decoder_init(midi_hires_note_decoder_t* decoder) {
    decoder->cc88_received = 0;
    decoder->cc88_value = 0;
}

/**
 * @brief Обработка MIDI сообщения для декодирования Hi-Res Note
 *
 * @param decoder Указатель на структуру декодера
 * @param status Статус байт MIDI сообщения
 * @param data1 Первый байт данных
 * @param data2 Второй байт данных
 * @param out_note Выходной номер ноты (если Note On/Off)
 * @param out_velocity Выходное 14-бит velocity (если собрано)
 * @param out_is_note_on Флаг: 1=Note On, 0=Note Off
 *
 * @return 1 если получено Note On/Off сообщение, 0 если нет
 */
static inline int midi_hires_note_decoder_process(midi_hires_note_decoder_t* decoder,
                                                  uint8_t status, uint8_t data1, uint8_t data2,
                                                  uint8_t* out_note, uint16_t* out_velocity,
                                                  uint8_t* out_is_note_on) {
    uint8_t msg_type = status & 0xF0;
    
    if (msg_type == 0xB0 && data1 == 88) {
        // Получен CC 88 - сохраняем MSB
        decoder->cc88_received = 1;
        decoder->cc88_value = data2;
        return 0;  // Ещё не Note On/Off
    }
    else if (msg_type == 0x90 || msg_type == 0x80) {
        // Note On или Note Off
        *out_note = data1;
        *out_is_note_on = (msg_type == 0x90 && data2 != 0) ? 1 : 0;
        
        if (decoder->cc88_received) {
            // Собираем 14-бит velocity
            *out_velocity = ((uint16_t)decoder->cc88_value << 7) | (data2 & 0x7F);
            decoder->cc88_received = 0;
        } else {
            // Обычный 7-бит velocity, расширяем до 14-бит
            *out_velocity = midi_7bit_to_14bit(data2);
        }
        
        return 1;  // Получено Note сообщение
    }
    
    return 0;
}

#ifdef __cplusplus
}
#endif

#endif // MIDI_HIRES_H
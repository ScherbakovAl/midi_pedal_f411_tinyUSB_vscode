/**
 * @file midi_hires_example.cpp
 * @brief Примеры использования Hi-Res MIDI функций
 * 
 * Этот файл содержит примеры кода для работы с 14-битными MIDI сообщениями
 */

#include "midi_hires.h"
#include "pedal.hpp"

// ============================================================================
// Пример 1: Отправка Pitch Bend с высоким разрешением
// ============================================================================

void example_pitch_bend() {
    uint8_t cable_num = 0;
    uint8_t channel = 0;
    
    // Центральное положение (нет изгиба)
    midi_send_pitch_bend_14bit(cable_num, channel, 0);
    
    // Максимальный изгиб вверх
    midi_send_pitch_bend_14bit(cable_num, channel, 8191);
    
    // Максимальный изгиб вниз
    midi_send_pitch_bend_14bit(cable_num, channel, -8192);
    
    // Небольшой изгиб вверх (плавный)
    midi_send_pitch_bend_14bit(cable_num, channel, 1000);
}

// ============================================================================
// Пример 2: Педаль экспрессии с ADC (12-бит)
// ============================================================================

void example_expression_pedal_adc() {
    uint8_t cable_num = 0;
    uint8_t channel = 0;
    
    // Предположим, у нас есть 12-битный ADC (0-4095)
    uint16_t adc_value = 2048;  // Пример: половина диапазона
    
    // Преобразуем ADC в 14-бит MIDI
    uint16_t midi_value = midi_adc12_to_14bit(adc_value);
    
    // Отправляем как Expression (CC 11/43)
    midi_send_expression_14bit(cable_num, channel, midi_value);
}

// ============================================================================
// Пример 3: Плавное изменение громкости
// ============================================================================

void example_smooth_volume_fade() {
    uint8_t cable_num = 0;
    uint8_t channel = 0;
    
    // Fade in: от 0 до максимума
    for (uint16_t vol = 0; vol <= MIDI_14BIT_MAX; vol += 100) {
        midi_send_volume_14bit(cable_num, channel, vol);
        // Небольшая задержка для плавности
        for (volatile int i = 0; i < 10000; i++);
    }
    
    // Fade out: от максимума до 0
    for (int16_t vol = MIDI_14BIT_MAX; vol >= 0; vol -= 100) {
        midi_send_volume_14bit(cable_num, channel, (uint16_t)vol);
        for (volatile int i = 0; i < 10000; i++);
    }
}

// ============================================================================
// Пример 4: Pan с процентами
// ============================================================================

void example_pan_control() {
    uint8_t cable_num = 0;
    uint8_t channel = 0;
    
    // Левый канал (0%)
    uint16_t left = midi_percent_to_14bit(0);
    midi_send_pan_14bit(cable_num, channel, left);
    
    // Центр (50%)
    uint16_t center = midi_percent_to_14bit(50);
    midi_send_pan_14bit(cable_num, channel, center);
    
    // Правый канал (100%)
    uint16_t right = midi_percent_to_14bit(100);
    midi_send_pan_14bit(cable_num, channel, right);
}

// ============================================================================
// Пример 5: Использование в основном цикле педали
// ============================================================================

void example_pedal_integration() {
    uint8_t cable_num = 0;
    uint8_t channel = 0;
    
    // Предположим, у нас есть функция чтения ADC
    // uint16_t read_pedal_adc();
    
    while (1) {
        tud_task();  // Обработка USB
        
        if (tud_mounted() && tud_midi_mounted()) {
            // Читаем положение педали (12-бит ADC)
            // uint16_t pedal_raw = read_pedal_adc();
            uint16_t pedal_raw = 2048;  // Пример
            
            // Преобразуем в 14-бит MIDI
            uint16_t pedal_14bit = midi_adc12_to_14bit(pedal_raw);
            
            // Отправляем как Expression
            midi_send_expression_14bit(cable_num, channel, pedal_14bit);
            
            // Или как Volume
            // midi_send_volume_14bit(cable_num, channel, pedal_14bit);
            
            // Небольшая задержка
            for (volatile int i = 0; i < 100000; i++);
        }
    }
}

// ============================================================================
// Пример 6: Декодирование принятых Hi-Res сообщений
// ============================================================================

void example_receive_hires() {
    static midi_hires_decoder_t decoder;
    static int decoder_initialized = 0;
    
    if (!decoder_initialized) {
        midi_hires_decoder_init(&decoder);
        decoder_initialized = 1;
    }
    
    // Предположим, мы получили CC сообщение
    uint8_t received_cc = 7;    // Volume MSB
    uint8_t received_value = 64; // Значение
    
    uint16_t hires_value;
    if (midi_hires_decoder_process(&decoder, received_cc, received_value, &hires_value)) {
        // Получено полное 14-бит значение!
        // Можно использовать hires_value (0-16383)
    }
}

// ============================================================================
// Пример 7: Комбинированное использование (педаль + pitch bend)
// ============================================================================

void example_combined_control() {
    uint8_t cable_num = 0;
    uint8_t channel = 0;
    
    // Педаль управляет экспрессией
    uint16_t pedal_position = 8192;  // Середина
    midi_send_expression_14bit(cable_num, channel, pedal_position);
    
    // Одновременно отправляем pitch bend
    int16_t pitch_bend = 2000;  // Небольшой изгиб вверх
    midi_send_pitch_bend_14bit(cable_num, channel, pitch_bend);
    
    // Также можем отправить обычный 7-бит CC
    midi_send_cc_7bit(cable_num, channel, 64, 127);  // Sustain pedal ON
}

// ============================================================================
// Пример 8: Сравнение 7-бит vs 14-бит
// ============================================================================

void example_resolution_comparison() {
    uint8_t cable_num = 0;
    uint8_t channel = 0;
    
    // 7-бит разрешение (128 шагов)
    for (uint8_t i = 0; i <= 127; i++) {
        midi_send_cc_7bit(cable_num, channel, 7, i);  // Volume 7-бит
        for (volatile int d = 0; d < 50000; d++);
    }
    
    // 14-бит разрешение (16384 шага) - гораздо плавнее!
    for (uint16_t i = 0; i <= MIDI_14BIT_MAX; i += 128) {
        midi_send_volume_14bit(cable_num, channel, i);  // Volume 14-бит
        for (volatile int d = 0; d < 50000; d++);
    }
}

// ============================================================================
// Пример 9: Модуляция с высоким разрешением
// ============================================================================

void example_modulation_wheel() {
    uint8_t cable_num = 0;
    uint8_t channel = 0;
    
    // Имитация плавного движения колеса модуляции
    for (uint16_t mod = 0; mod <= MIDI_14BIT_MAX; mod += 50) {
        midi_send_modulation_14bit(cable_num, channel, mod);
        for (volatile int i = 0; i < 10000; i++);
    }
}

// ============================================================================
// Пример 10: Реальное использование в pedal.cpp
// ============================================================================

/*
// Добавьте в pedal.cpp:

#include "midi_hires.h"

void pedal() {
    LL_TIM_EnableCounter(TIM2);
    board_init_usb();
    tud_init(0);

    uint8_t const cable_num = 0;
    uint8_t const channel = 0;

    while (1) {
        tud_task();

        if (TIM2->CNT > 1000000) {
            TIM2->CNT = 0;

            if (tud_mounted() && tud_midi_mounted()) {
                // Вместо простого Note On, используем Hi-Res Expression
                
                // Пример: плавное изменение экспрессии
                static uint16_t expression = 0;
                expression += 1000;
                if (expression > MIDI_14BIT_MAX) expression = 0;
                
                midi_send_expression_14bit(cable_num, channel, expression);
                
                // Или отправляем Pitch Bend
                static int16_t pitch = -8192;
                pitch += 500;
                if (pitch > 8191) pitch = -8192;
                
                midi_send_pitch_bend_14bit(cable_num, channel, pitch);
            }
        }
    }
}
*/

// ============================================================================
// Пример 11: Note On/Off с 14-бит velocity
// ============================================================================

void example_note_on_off_14bit() {
    uint8_t cable_num = 0;
    uint8_t channel = 0;
    uint8_t note = 60;  // Middle C
    
    // Отправляем Note On с 14-бит velocity
    uint16_t velocity_on = 12000;  // Из диапазона 0-16383
    midi_send_note_on_14bit(cable_num, channel, note, velocity_on);
    
    // Задержка (нота звучит)
    for (volatile int i = 0; i < 1000000; i++);
    
    // Отправляем Note Off с 14-бит release velocity
    uint16_t velocity_off = 8000;
    midi_send_note_off_14bit(cable_num, channel, note, velocity_off);
}

// ============================================================================
// Пример 12: Педаль с динамическим velocity
// ============================================================================

void example_pedal_with_velocity() {
    uint8_t cable_num = 0;
    uint8_t channel = 0;
    uint8_t note = 60;
    
    // Предположим, у нас есть датчик силы нажатия (ADC)
    // uint16_t pressure_adc = read_pressure_sensor();  // 0-4095
    uint16_t pressure_adc = 3000;  // Пример
    
    // Преобразуем в 14-бит velocity
    uint16_t velocity_14bit = midi_adc12_to_14bit(pressure_adc);
    
    // Отправляем Note On с высоким разрешением
    midi_send_note_on_14bit(cable_num, channel, note, velocity_14bit);
    
    // ... нота звучит ...
    for (volatile int i = 0; i < 2000000; i++);
    
    // Note Off (можно с velocity=0 для простоты)
    midi_send_note_off_zero_velocity(cable_num, channel, note);
}

// ============================================================================
// Пример 13: Сравнение 7-бит vs 14-бит velocity
// ============================================================================

void example_velocity_resolution_comparison() {
    uint8_t cable_num = 0;
    uint8_t channel = 0;
    uint8_t note = 60;
    
    // 7-бит velocity (128 уровней)
    for (uint8_t vel = 0; vel <= 127; vel++) {
        midi_send_note_on_7bit(cable_num, channel, note, vel);
        for (volatile int i = 0; i < 50000; i++);
        midi_send_note_off_zero_velocity(cable_num, channel, note);
        for (volatile int i = 0; i < 50000; i++);
    }
    
    // 14-бит velocity (16384 уровня) - гораздо плавнее!
    for (uint16_t vel = 0; vel <= MIDI_14BIT_MAX; vel += 128) {
        midi_send_note_on_14bit(cable_num, channel, note, vel);
        for (volatile int i = 0; i < 50000; i++);
        midi_send_note_off_zero_velocity(cable_num, channel, note);
        for (volatile int i = 0; i < 50000; i++);
    }
}

// ============================================================================
// Пример 14: Декодирование принятых Hi-Res Note сообщений
// ============================================================================

void example_receive_hires_notes() {
    static midi_hires_note_decoder_t note_decoder;
    static int decoder_initialized = 0;
    
    if (!decoder_initialized) {
        midi_hires_note_decoder_init(&note_decoder);
        decoder_initialized = 1;
    }
    
    // Предположим, мы получили MIDI сообщения:
    
    // Сообщение 1: CC 88 (MSB velocity)
    uint8_t status1 = 0xB0;  // CC на канале 0
    uint8_t data1_1 = 88;    // CC 88
    uint8_t data2_1 = 94;    // MSB velocity
    
    uint8_t note;
    uint16_t velocity;
    uint8_t is_note_on;
    
    // Обрабатываем CC 88
    if (!midi_hires_note_decoder_process(&note_decoder, status1, data1_1, data2_1,
                                        &note, &velocity, &is_note_on)) {
        // Это был CC 88, ждём Note On
    }
    
    // Сообщение 2: Note On (LSB velocity)
    uint8_t status2 = 0x90;  // Note On на канале 0
    uint8_t data1_2 = 60;    // Note = Middle C
    uint8_t data2_2 = 45;    // LSB velocity
    
    // Обрабатываем Note On
    if (midi_hires_note_decoder_process(&note_decoder, status2, data1_2, data2_2,
                                       &note, &velocity, &is_note_on)) {
        // Получено полное Note On сообщение!
        // note = 60
        // velocity = (94 << 7) | 45 = 12077 (14-бит)
        // is_note_on = 1
        
        if (is_note_on) {
            // Обработка Note On с Hi-Res velocity
        } else {
            // Обработка Note Off
        }
    }
}

// ============================================================================
// Пример 15: Реальное использование в pedal.cpp с velocity
// ============================================================================

/*
// Добавьте в pedal.cpp для педали с датчиком силы:

#include "midi_hires.h"

// Глобальные переменные
static uint8_t note_playing = 0;
static uint8_t current_note = 60;

void pedal() {
    LL_TIM_EnableCounter(TIM2);
    board_init_usb();
    tud_init(0);

    uint8_t const cable_num = 0;
    uint8_t const channel = 0;

    while (1) {
        tud_task();

        if (TIM2->CNT > 100000) {  // Проверяем каждые 100ms
            TIM2->CNT = 0;

            if (tud_mounted() && tud_midi_mounted()) {
                // Читаем датчик силы нажатия (например, FSR или тензодатчик)
                // uint16_t pressure = read_adc_channel(0);  // 0-4095
                uint16_t pressure = 2500;  // Пример
                
                // Порог срабатывания
                const uint16_t threshold = 100;
                
                if (pressure > threshold && !note_playing) {
                    // Педаль нажата - отправляем Note On
                    uint16_t velocity = midi_adc12_to_14bit(pressure);
                    midi_send_note_on_14bit(cable_num, channel, current_note, velocity);
                    note_playing = 1;
                }
                else if (pressure <= threshold && note_playing) {
                    // Педаль отпущена - отправляем Note Off
                    midi_send_note_off_zero_velocity(cable_num, channel, current_note);
                    note_playing = 0;
                }
            }
        }
    }
}
*/

// ============================================================================
// Пример 16: Арпеджиатор с Hi-Res velocity
// ============================================================================

void example_arpeggiator_hires() {
    uint8_t cable_num = 0;
    uint8_t channel = 0;
    
    // Арпеджио: C-E-G-C (до-ми-соль-до)
    uint8_t notes[] = {60, 64, 67, 72};
    uint16_t velocities[] = {8000, 10000, 12000, 14000};  // Нарастающая динамика
    
    for (int i = 0; i < 4; i++) {
        // Note On с Hi-Res velocity
        midi_send_note_on_14bit(cable_num, channel, notes[i], velocities[i]);
        
        // Задержка
        for (volatile int d = 0; d < 500000; d++);
        
        // Note Off
        midi_send_note_off_zero_velocity(cable_num, channel, notes[i]);
        
        // Пауза между нотами
        for (volatile int d = 0; d < 200000; d++);
    }
}

// ============================================================================
// Пример 17: Комбинация Note + Expression для максимальной выразительности
// ============================================================================

void example_note_with_expression() {
    uint8_t cable_num = 0;
    uint8_t channel = 0;
    uint8_t note = 60;
    
    // Отправляем Note On с максимальным velocity
    midi_send_note_on_14bit(cable_num, channel, note, MIDI_14BIT_MAX);
    
    // Плавно уменьшаем громкость через Expression
    for (int16_t expr = MIDI_14BIT_MAX; expr >= 0; expr -= 100) {
        midi_send_expression_14bit(cable_num, channel, (uint16_t)expr);
        for (volatile int i = 0; i < 10000; i++);
    }
    
    // Note Off
    midi_send_note_off_zero_velocity(cable_num, channel, note);
}
*/
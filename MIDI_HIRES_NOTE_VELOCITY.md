# Hi-Res Note Velocity (14-бит) - Руководство

## Проблема стандартного MIDI

Стандартные Note On/Off сообщения в MIDI 1.0:
```
Note On:  [0x90 | channel] [note] [velocity]
Note Off: [0x80 | channel] [note] [velocity]
```

Где `velocity` = 0-127 (7 бит) - это ограничивает динамический диапазон.

## Решение: Hi-Res Velocity через CC

Для получения 14-битного разрешения velocity используется **комбинация Note On + Control Change**:

### Метод 1: CC 88 (High Resolution Velocity Prefix)

Это **официальный** метод из спецификации MIDI:

1. **Сначала** отправляем CC 88 с MSB velocity (старшие 7 бит)
2. **Затем** отправляем Note On с LSB velocity (младшие 7 бит)

```
Шаг 1: CC 88 (MSB)    [0xB0 | channel] [88] [velocity_msb]
Шаг 2: Note On (LSB)  [0x90 | channel] [note] [velocity_lsb]
```

### Метод 2: Два Note On сообщения (альтернативный)

Некоторые синтезаторы используют:

1. **Первое** Note On с velocity = MSB
2. **Второе** Note On той же ноты с velocity = LSB

```
Шаг 1: Note On (MSB)  [0x90 | channel] [note] [velocity_msb]
Шаг 2: Note On (LSB)  [0x90 | channel] [note] [velocity_lsb]
```

⚠️ **Внимание:** Этот метод менее стандартизирован!

## Формирование буфера для отправки

### Вариант A: CC 88 + Note On (Рекомендуется)

```cpp
void send_note_on_14bit_cc88(uint8_t cable_num, uint8_t channel, 
                              uint8_t note, uint16_t velocity_14bit) {
    // Ограничиваем значение
    if (velocity_14bit > 16383) velocity_14bit = 16383;
    
    // Разделяем на MSB и LSB
    uint8_t velocity_msb = (velocity_14bit >> 7) & 0x7F;  // Старшие 7 бит
    uint8_t velocity_lsb = velocity_14bit & 0x7F;         // Младшие 7 бит
    
    // Шаг 1: Отправляем CC 88 с MSB
    uint8_t cc88_msg[3] = {
        0xB0 | (channel & 0x0F),  // Control Change + канал
        88,                        // CC 88 (High Resolution Velocity Prefix)
        velocity_msb               // MSB velocity
    };
    tud_midi_stream_write(cable_num, cc88_msg, 3);
    
    // Шаг 2: Отправляем Note On с LSB
    uint8_t note_on_msg[3] = {
        0x90 | (channel & 0x0F),  // Note On + канал
        note & 0x7F,               // Номер ноты
        velocity_lsb               // LSB velocity
    };
    tud_midi_stream_write(cable_num, note_on_msg, 3);
}
```

### Вариант B: Два Note On (Альтернативный)

```cpp
void send_note_on_14bit_dual(uint8_t cable_num, uint8_t channel, 
                             uint8_t note, uint16_t velocity_14bit) {
    if (velocity_14bit > 16383) velocity_14bit = 16383;
    
    uint8_t velocity_msb = (velocity_14bit >> 7) & 0x7F;
    uint8_t velocity_lsb = velocity_14bit & 0x7F;
    
    // Первое Note On с MSB
    uint8_t note_on_msb[3] = {
        0x90 | (channel & 0x0F),
        note & 0x7F,
        velocity_msb
    };
    tud_midi_stream_write(cable_num, note_on_msb, 3);
    
    // Второе Note On с LSB (очень быстро после первого!)
    uint8_t note_on_lsb[3] = {
        0x90 | (channel & 0x0F),
        note & 0x7F,
        velocity_lsb
    };
    tud_midi_stream_write(cable_num, note_on_lsb, 3);
}
```

## Note Off с 14-бит velocity

### Метод CC 88 + Note Off

```cpp
void send_note_off_14bit_cc88(uint8_t cable_num, uint8_t channel, 
                               uint8_t note, uint16_t velocity_14bit) {
    if (velocity_14bit > 16383) velocity_14bit = 16383;
    
    uint8_t velocity_msb = (velocity_14bit >> 7) & 0x7F;
    uint8_t velocity_lsb = velocity_14bit & 0x7F;
    
    // CC 88 с MSB
    uint8_t cc88_msg[3] = {
        0xB0 | (channel & 0x0F),
        88,
        velocity_msb
    };
    tud_midi_stream_write(cable_num, cc88_msg, 3);
    
    // Note Off с LSB
    uint8_t note_off_msg[3] = {
        0x80 | (channel & 0x0F),  // Note Off + канал
        note & 0x7F,
        velocity_lsb
    };
    tud_midi_stream_write(cable_num, note_off_msg, 3);
}
```

### Альтернатива: Note On с velocity=0 (стандартный способ)

```cpp
void send_note_off_14bit_zero_velocity(uint8_t cable_num, uint8_t channel, 
                                       uint8_t note) {
    // Note On с velocity=0 = Note Off
    uint8_t note_off_msg[3] = {
        0x90 | (channel & 0x0F),  // Note On (!)
        note & 0x7F,
        0                          // velocity = 0 означает Note Off
    };
    tud_midi_stream_write(cable_num, note_off_msg, 3);
}
```

## Полный пример использования

```cpp
#include "tusb.h"

void example_hires_note() {
    uint8_t cable_num = 0;
    uint8_t channel = 0;
    uint8_t note = 60;  // Middle C
    
    // Отправляем Note On с 14-бит velocity
    uint16_t velocity_on = 12000;  // Из диапазона 0-16383
    send_note_on_14bit_cc88(cable_num, channel, note, velocity_on);
    
    // Задержка (нота звучит)
    for (volatile int i = 0; i < 1000000; i++);
    
    // Отправляем Note Off с 14-бит velocity
    uint16_t velocity_off = 8000;  // Release velocity
    send_note_off_14bit_cc88(cable_num, channel, note, velocity_off);
}
```

## Важные замечания

### 1. **Порядок отправки критичен!**
```cpp
// ПРАВИЛЬНО:
send_cc88(velocity_msb);    // Сначала CC 88
send_note_on(velocity_lsb); // Потом Note On

// НЕПРАВИЛЬНО:
send_note_on(velocity_lsb); // Если сначала Note On,
send_cc88(velocity_msb);    // CC 88 будет проигнорирован!
```

### 2. **Совместимость**
- Не все синтезаторы поддерживают CC 88
- Если не поддерживается, используется только LSB (7-бит)
- Проверьте документацию вашего синтезатора

### 3. **Timing (Тайминг)**
Сообщения должны идти **последовательно без задержки**:
```cpp
// Хорошо - сразу друг за другом
tud_midi_stream_write(cable_num, cc88_msg, 3);
tud_midi_stream_write(cable_num, note_on_msg, 3);

// Плохо - с задержкой
tud_midi_stream_write(cable_num, cc88_msg, 3);
delay_ms(10);  // ❌ Слишком долго!
tud_midi_stream_write(cable_num, note_on_msg, 3);
```

### 4. **Альтернатива: Poly Aftertouch**
Для динамического изменения после Note On:
```cpp
// Poly Aftertouch (14-бит через CC)
void send_poly_aftertouch_14bit(uint8_t cable_num, uint8_t channel,
                                uint8_t note, uint16_t pressure) {
    // Используем CC 84 (Portamento Control) или другой
    // Это зависит от синтезатора
}
```

## Сравнение методов

| Метод | Совместимость | Стандартизация | Рекомендация |
|-------|---------------|----------------|--------------|
| CC 88 + Note On | Средняя | Официальный MIDI | ✅ Рекомендуется |
| Два Note On | Низкая | Нестандартный | ⚠️ Только если поддерживается |
| Note On velocity=0 | Высокая | Стандартный | ✅ Для Note Off |

## Практический пример: Педаль с динамикой

```cpp
void pedal_with_velocity() {
    uint8_t cable_num = 0;
    uint8_t channel = 0;
    uint8_t note = 60;
    
    // Читаем силу нажатия с датчика (например, ADC)
    uint16_t pressure_adc = read_pressure_sensor();  // 0-4095
    
    // Преобразуем в 14-бит velocity
    uint16_t velocity_14bit = (pressure_adc << 2);  // 0-16383
    
    // Отправляем Note On с высоким разрешением
    send_note_on_14bit_cc88(cable_num, channel, note, velocity_14bit);
    
    // ... нота звучит ...
    
    // Note Off (можно с velocity=0 для простоты)
    send_note_off_14bit_zero_velocity(cable_num, channel, note);
}
```

## Декодирование принятых Hi-Res Note

```cpp
typedef struct {
    uint8_t cc88_received;
    uint8_t cc88_value;
} hires_note_decoder_t;

void process_midi_message(hires_note_decoder_t* decoder, 
                         uint8_t status, uint8_t data1, uint8_t data2) {
    uint8_t msg_type = status & 0xF0;
    uint8_t channel = status & 0x0F;
    
    if (msg_type == 0xB0 && data1 == 88) {
        // Получен CC 88 - сохраняем MSB
        decoder->cc88_received = 1;
        decoder->cc88_value = data2;
    }
    else if (msg_type == 0x90) {
        // Note On
        uint8_t note = data1;
        uint8_t velocity_lsb = data2;
        
        if (decoder->cc88_received) {
            // Собираем 14-бит velocity
            uint16_t velocity_14bit = (decoder->cc88_value << 7) | velocity_lsb;
            
            // Обрабатываем Note On с Hi-Res velocity
            handle_note_on_hires(channel, note, velocity_14bit);
            
            decoder->cc88_received = 0;
        } else {
            // Обычный 7-бит Note On
            handle_note_on_7bit(channel, note, velocity_lsb);
        }
    }
}
```

## Ссылки

- [MIDI CC 88 Specification](https://www.midi.org/specifications-old/item/table-3-control-change-messages-data-bytes-2)
- [High Resolution Velocity Prefix](https://www.recordingblogs.com/wiki/midi-high-resolution-velocity-prefix-control-change)
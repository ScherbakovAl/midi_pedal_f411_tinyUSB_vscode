# Hi-Res MIDI (14-бит) - Руководство

## Что такое Hi-Res MIDI?

Hi-Res (High Resolution) MIDI использует **14-бит разрешение** вместо стандартных 7-бит. Это достигается путём комбинирования двух MIDI сообщений:
- **MSB (Most Significant Byte)** - старшие 7 бит (биты 7-13)
- **LSB (Least Significant Byte)** - младшие 7 бит (биты 0-6)

Диапазон значений: **0-16383** (вместо 0-127)

## Где используется Hi-Res MIDI?

### 1. **Pitch Bend (Изгиб тона)**
- Стандартно использует 14-бит
- Диапазон: -8192 до +8191 (центр = 8192)
- Формат: `0xE0 | channel, LSB, MSB`

### 2. **Control Change (CC) пары MSB/LSB**
MIDI определяет пары контроллеров для Hi-Res:

| CC MSB | CC LSB | Назначение |
|--------|--------|------------|
| 0      | 32     | Bank Select |
| 1      | 33     | Modulation Wheel |
| 2      | 34     | Breath Controller |
| 7      | 39     | Channel Volume |
| 10     | 42     | Pan |
| 11     | 43     | Expression |
| 64-95  | -      | Только MSB (переключатели) |

## Формирование Hi-Res сообщений

### Pitch Bend (14-бит)

```cpp
// Значение от -8192 до +8191
void send_pitch_bend_14bit(uint8_t cable_num, uint8_t channel, int16_t value) {
    // Преобразуем в беззнаковое 0-16383 (центр = 8192)
    uint16_t unsigned_value = (uint16_t)(value + 8192);
    
    // Разделяем на LSB (младшие 7 бит) и MSB (старшие 7 бит)
    uint8_t lsb = unsigned_value & 0x7F;        // Биты 0-6
    uint8_t msb = (unsigned_value >> 7) & 0x7F; // Биты 7-13
    
    // Формируем MIDI сообщение Pitch Bend
    uint8_t pitch_bend[3] = {
        0xE0 | channel,  // Pitch Bend + канал
        lsb,             // Младшие 7 бит
        msb              // Старшие 7 бит
    };
    
    tud_midi_stream_write(cable_num, pitch_bend, 3);
}

// Пример использования:
// Центр (нет изгиба)
send_pitch_bend_14bit(0, 0, 0);

// Максимальный изгиб вверх
send_pitch_bend_14bit(0, 0, 8191);

// Максимальный изгиб вниз
send_pitch_bend_14bit(0, 0, -8192);
```

### Control Change Hi-Res (14-бит)

```cpp
// Отправка 14-бит CC (например, Volume)
void send_cc_14bit(uint8_t cable_num, uint8_t channel, 
                   uint8_t cc_msb, uint16_t value) {
    // value: 0-16383
    uint8_t msb = (value >> 7) & 0x7F;  // Старшие 7 бит
    uint8_t lsb = value & 0x7F;         // Младшие 7 бит
    
    // Сначала отправляем MSB
    uint8_t cc_msb_msg[3] = {
        0xB0 | channel,  // Control Change + канал
        cc_msb,          // Номер CC (MSB)
        msb              // Значение MSB
    };
    tud_midi_stream_write(cable_num, cc_msb_msg, 3);
    
    // Затем отправляем LSB (CC + 32)
    uint8_t cc_lsb_msg[3] = {
        0xB0 | channel,  // Control Change + канал
        cc_msb + 32,     // Номер CC (LSB) = MSB + 32
        lsb              // Значение LSB
    };
    tud_midi_stream_write(cable_num, cc_lsb_msg, 3);
}

// Примеры использования:

// Volume (CC 7/39) - значение 10000 из 16383
send_cc_14bit(0, 0, 7, 10000);

// Modulation (CC 1/33) - максимум
send_cc_14bit(0, 0, 1, 16383);

// Pan (CC 10/42) - центр (8192)
send_cc_14bit(0, 0, 10, 8192);
```

## Важные моменты

### 1. **Порядок отправки**
Всегда отправляйте **сначала MSB, потом LSB**:
```cpp
// ПРАВИЛЬНО:
send_cc_msb();  // CC 7 (Volume MSB)
send_cc_lsb();  // CC 39 (Volume LSB)

// НЕПРАВИЛЬНО:
send_cc_lsb();  // Может вызвать скачок значения
send_cc_msb();
```

### 2. **Совместимость**
Если устройство не поддерживает LSB:
- Оно использует только MSB (7-бит разрешение)
- LSB игнорируется
- Всё равно работает, но с меньшей точностью

### 3. **Оптимизация**
Если не нужна высокая точность:
```cpp
// Отправляем только MSB (7-бит, 0-127)
void send_cc_7bit(uint8_t cable_num, uint8_t channel, 
                  uint8_t cc_num, uint8_t value) {
    uint8_t cc_msg[3] = {
        0xB0 | channel,
        cc_num,
        value & 0x7F
    };
    tud_midi_stream_write(cable_num, cc_msg, 3);
}
```

## Пример: Педаль экспрессии с Hi-Res

```cpp
// Педаль экспрессии (CC 11) с 14-бит разрешением
void send_expression_pedal(uint8_t cable_num, uint8_t channel, 
                          uint16_t adc_value) {
    // ADC: 0-4095 (12-бит) -> MIDI: 0-16383 (14-бит)
    uint16_t midi_value = (adc_value << 2);  // Масштабируем до 14-бит
    
    // Отправляем как Hi-Res CC
    send_cc_14bit(cable_num, channel, 11, midi_value);
}

// Использование:
uint16_t pedal_position = read_adc();  // 0-4095
send_expression_pedal(0, 0, pedal_position);
```

## Преобразование значений

### Из 14-бит в 7-бит (MSB only):
```cpp
uint8_t value_7bit = value_14bit >> 7;
```

### Из 7-бит в 14-бит:
```cpp
uint16_t value_14bit = value_7bit << 7;
```

### Из ADC (12-бит) в 14-бит:
```cpp
uint16_t value_14bit = adc_12bit << 2;
```

### Из процентов (0-100%) в 14-бит:
```cpp
uint16_t value_14bit = (percent * 16383) / 100;
```

## Декодирование принятых Hi-Res сообщений

```cpp
// Буфер для хранения MSB до получения LSB
static uint8_t cc_msb_values[128] = {0};
static bool cc_msb_received[128] = {false};

void process_midi_cc(uint8_t cc_num, uint8_t value) {
    if (cc_num < 32) {
        // Это MSB
        cc_msb_values[cc_num] = value;
        cc_msb_received[cc_num] = true;
    } 
    else if (cc_num >= 32 && cc_num < 64) {
        // Это LSB
        uint8_t msb_num = cc_num - 32;
        
        if (cc_msb_received[msb_num]) {
            // Собираем 14-бит значение
            uint16_t value_14bit = (cc_msb_values[msb_num] << 7) | value;
            
            // Обрабатываем Hi-Res значение
            handle_hires_cc(msb_num, value_14bit);
            
            cc_msb_received[msb_num] = false;
        }
    }
}
```

## Тестирование

Для проверки Hi-Res MIDI используйте:
- **MIDI-OX** (Windows) - показывает MSB/LSB
- **MIDI Monitor** (macOS) - отображает 14-бит значения
- **Protokol** (macOS) - детальный анализ MIDI

## Ссылки

- [MIDI 1.0 Specification](https://www.midi.org/specifications)
- [Control Change Messages](https://www.midi.org/specifications-old/item/table-3-control-change-messages-data-bytes-2)
- [14-bit MIDI Controllers](https://www.recordingblogs.com/wiki/midi-control-change-message)
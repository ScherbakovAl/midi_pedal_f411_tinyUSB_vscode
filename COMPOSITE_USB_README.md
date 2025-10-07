# Composite USB Device: MIDI + HID Keyboard

## Описание
Проект настроен как composite USB устройство, которое одновременно работает как:
- **MIDI устройство** - для отправки MIDI сообщений
- **HID клавиатура** - для отправки нажатий клавиш

## Измененные файлы

### 1. `Pedal_f411/tusb_config.h`
- Включен HID класс: `CFG_TUD_HID = 1`
- Добавлен размер буфера HID: `CFG_TUD_HID_EP_BUFSIZE = 16`

### 2. `Pedal_f411/usb_descriptors.c`
- Добавлен HID интерфейс в перечисление интерфейсов
- Добавлен HID Report Descriptor для клавиатуры
- Обновлены дескрипторы конфигурации (Full Speed и High Speed)
- Добавлен endpoint для HID: `EPNUM_HID = 0x82`
- Реализованы callback функции для HID:
  - `tud_hid_descriptor_report_cb()`
  - `tud_hid_get_report_cb()`
  - `tud_hid_set_report_cb()`

### 3. `Pedal_f411/pedal.cpp`
- Добавлена отправка нажатия клавиши "курсор-влево" (HID keycode 0x50)
- Используется `tud_hid_keyboard_report()` для отправки HID событий
- Реализовано нажатие и отпускание клавиши с задержкой

## Как это работает

При каждом срабатывании таймера (каждую секунду) устройство:
1. Отправляет MIDI Note On сообщение (нота A4, velocity 127)
2. Отправляет нажатие клавиши "курсор-влево" через HID клавиатуру
3. После небольшой задержки отпускает клавишу

## USB дескрипторы

Устройство определяется в системе как:
- **Vendor ID**: 0xCafe
- **Product ID**: 0x400C (автоматически генерируется на основе включенных классов)
- **Manufacturer**: SCHE
- **Product**: SCHE MIDI Pedal
- **Интерфейсы**:
  - Interface 0: HID Keyboard
  - Interface 1: MIDI Audio Control
  - Interface 2: MIDI Streaming

## Endpoints

- **EP0**: Control endpoint (bidirectional)
- **EP1 IN/OUT**: MIDI (0x81/0x01)
- **EP2 IN**: HID Keyboard (0x82)

## Сборка проекта

```bash
cmake --build build
```

Проект успешно собирается с использованием:
- STM32F411
- TinyUSB library
- ARM GCC toolchain

## Использование

После прошивки устройство будет определяться в операционной системе как два отдельных устройства:
1. MIDI интерфейс (для работы с MIDI приложениями)
2. HID клавиатура (для эмуляции нажатий клавиш)

Каждую секунду устройство автоматически отправляет:
- MIDI ноту A4
- Нажатие клавиши "стрелка влево"
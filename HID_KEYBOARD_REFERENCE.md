# HID Keyboard Reference - Справочник по HID клавиатуре

## Структура HID Keyboard Report

HID клавиатура использует стандартный формат отчета (report) из **8 байт**:

```c
uint8_t report[8] = {
    modifier,    // Байт 0: Модификаторы (Ctrl, Shift, Alt и т.д.)
    reserved,    // Байт 1: Зарезервирован (всегда 0x00)
    keycode[0],  // Байт 2: Первая нажатая клавиша
    keycode[1],  // Байт 3: Вторая нажатая клавиша
    keycode[2],  // Байт 4: Третья нажатая клавиша
    keycode[3],  // Байт 5: Четвертая нажатая клавиша
    keycode[4],  // Байт 6: Пятая нажатая клавиша
    keycode[5]   // Байт 7: Шестая нажатая клавиша
};
```

### Почему именно 6 клавиш?

**6 клавиш (keycode)** - это стандарт USB HID для клавиатуры, определенный в спецификации USB HID:
- Позволяет одновременно нажимать до 6 обычных клавиш
- Плюс модификаторы (Ctrl, Shift, Alt и т.д.) независимо
- Это называется **6-Key Rollover (6KRO)**

**Можно ли другое количество?**
- Да, но нужно изменить HID Report Descriptor
- Стандартные клавиатуры используют 6 клавиш
- Игровые клавиатуры могут использовать NKRO (N-Key Rollover) - любое количество клавиш

## Что такое Modifier (Модификатор)?

**Modifier** - это первый байт отчета, битовая маска для специальных клавиш:

| Бит | Значение | Клавиша |
|-----|----------|---------|
| 0   | 0x01     | Left Ctrl |
| 1   | 0x02     | Left Shift |
| 2   | 0x04     | Left Alt |
| 3   | 0x08     | Left GUI (Win/Cmd) |
| 4   | 0x10     | Right Ctrl |
| 5   | 0x20     | Right Shift |
| 6   | 0x40     | Right Alt |
| 7   | 0x80     | Right GUI (Win/Cmd) |

### Примеры использования модификаторов:

```c
// Нажать Ctrl+C
uint8_t modifier = 0x01;  // Left Ctrl
uint8_t keycode[6] = { 0x06, 0, 0, 0, 0, 0 };  // 'C' = 0x06

// Нажать Shift+A (заглавная A)
uint8_t modifier = 0x02;  // Left Shift
uint8_t keycode[6] = { 0x04, 0, 0, 0, 0, 0 };  // 'A' = 0x04

// Нажать Ctrl+Shift+Esc
uint8_t modifier = 0x03;  // Left Ctrl + Left Shift (0x01 | 0x02)
uint8_t keycode[6] = { 0x29, 0, 0, 0, 0, 0 };  // Esc = 0x29
```

## Что такое Report (Отчет)?

**Report** - это пакет данных, который USB устройство отправляет хосту (компьютеру):
- Содержит информацию о текущем состоянии клавиатуры
- Отправляется при каждом изменении (нажатие/отпускание клавиши)
- Для клавиатуры это 8 байт данных

## HID Keyboard Keycodes (Коды клавиш)

### Буквы (Letters)
```
0x04 = A    0x05 = B    0x06 = C    0x07 = D
0x08 = E    0x09 = F    0x0A = G    0x0B = H
0x0C = I    0x0D = J    0x0E = K    0x0F = L
0x10 = M    0x11 = N    0x12 = O    0x13 = P
0x14 = Q    0x15 = R    0x16 = S    0x17 = T
0x18 = U    0x19 = V    0x1A = W    0x1B = X
0x1C = Y    0x1D = Z
```

### Цифры (Numbers)
```
0x1E = 1    0x1F = 2    0x20 = 3    0x21 = 4
0x22 = 5    0x23 = 6    0x24 = 7    0x25 = 8
0x26 = 9    0x27 = 0
```

### Специальные клавиши (Special Keys)
```
0x28 = Enter
0x29 = Escape
0x2A = Backspace
0x2B = Tab
0x2C = Space
0x2D = - (Minus)
0x2E = = (Equal)
0x2F = [ (Left Bracket)
0x30 = ] (Right Bracket)
0x31 = \ (Backslash)
0x33 = ; (Semicolon)
0x34 = ' (Apostrophe)
0x35 = ` (Grave)
0x36 = , (Comma)
0x37 = . (Period)
0x38 = / (Slash)
0x39 = Caps Lock
```

### Функциональные клавиши (Function Keys)
```
0x3A = F1     0x3B = F2     0x3C = F3     0x3D = F4
0x3E = F5     0x3F = F6     0x40 = F7     0x41 = F8
0x42 = F9     0x43 = F10    0x44 = F11    0x45 = F12
```

### Клавиши навигации (Navigation Keys)
```
0x4A = Home
0x4B = Page Up
0x4C = Delete
0x4D = End
0x4E = Page Down
0x4F = Right Arrow  (→)
0x50 = Left Arrow   (←)  ← ИСПОЛЬЗУЕТСЯ В ПРОЕКТЕ
0x51 = Down Arrow   (↓)
0x52 = Up Arrow     (↑)
```

### Клавиши Numpad
```
0x53 = Num Lock
0x54 = Numpad /
0x55 = Numpad *
0x56 = Numpad -
0x57 = Numpad +
0x58 = Numpad Enter
0x59 = Numpad 1
0x5A = Numpad 2
0x5B = Numpad 3
0x5C = Numpad 4
0x5D = Numpad 5
0x5E = Numpad 6
0x5F = Numpad 7
0x60 = Numpad 8
0x61 = Numpad 9
0x62 = Numpad 0
0x63 = Numpad .
```

### Системные клавиши (System Keys)
```
0x46 = Print Screen
0x47 = Scroll Lock
0x48 = Pause
0x49 = Insert
```

## Примеры использования в коде

### Пример 1: Отправка одной клавиши (текущий проект)
```c
// Отправить "стрелка влево"
uint8_t keycode[6] = { 0x50, 0, 0, 0, 0, 0 };
tud_hid_keyboard_report(0, 0, keycode);  // report_id=0, modifier=0

// Отпустить клавишу
tud_hid_keyboard_report(0, 0, NULL);
```

### Пример 2: Отправка нескольких клавиш одновременно
```c
// Нажать A, B, C одновременно
uint8_t keycode[6] = { 0x04, 0x05, 0x06, 0, 0, 0 };  // A, B, C
tud_hid_keyboard_report(0, 0, keycode);
```

### Пример 3: Использование модификаторов
```c
// Ctrl+Alt+Delete
uint8_t modifier = 0x01 | 0x04;  // Left Ctrl + Left Alt
uint8_t keycode[6] = { 0x4C, 0, 0, 0, 0, 0 };  // Delete
tud_hid_keyboard_report(0, modifier, keycode);
```

### Пример 4: Печать текста "Hello"
```c
// H
uint8_t keycode1[6] = { 0x0B, 0, 0, 0, 0, 0 };
tud_hid_keyboard_report(0, 0x02, keycode1);  // Shift + H
tud_hid_keyboard_report(0, 0, NULL);  // Release

// e
uint8_t keycode2[6] = { 0x08, 0, 0, 0, 0, 0 };
tud_hid_keyboard_report(0, 0, keycode2);
tud_hid_keyboard_report(0, 0, NULL);

// l
uint8_t keycode3[6] = { 0x0F, 0, 0, 0, 0, 0 };
tud_hid_keyboard_report(0, 0, keycode3);
tud_hid_keyboard_report(0, 0, NULL);

// l
tud_hid_keyboard_report(0, 0, keycode3);
tud_hid_keyboard_report(0, 0, NULL);

// o
uint8_t keycode4[6] = { 0x12, 0, 0, 0, 0, 0 };
tud_hid_keyboard_report(0, 0, keycode4);
tud_hid_keyboard_report(0, 0, NULL);
```

## Функция tud_hid_keyboard_report()

```c
bool tud_hid_keyboard_report(
    uint8_t report_id,      // ID отчета (обычно 0)
    uint8_t modifier,       // Битовая маска модификаторов
    uint8_t keycode[6]      // Массив из 6 кодов клавиш
);
```

**Параметры:**
- `report_id`: Идентификатор отчета (для клавиатуры обычно 0)
- `modifier`: Битовая маска модификаторов (Ctrl, Shift, Alt и т.д.)
- `keycode`: Массив из 6 кодов нажатых клавиш (0x00 = не нажата)

**Возвращает:**
- `true` если отчет успешно отправлен
- `false` если устройство не готово

## Важные замечания

1. **Всегда отпускайте клавиши**: После отправки нажатия нужно отправить пустой отчет
   ```c
   tud_hid_keyboard_report(0, 0, NULL);  // Отпустить все клавиши
   ```

2. **Задержка между нажатиями**: Компьютеру нужно время для обработки
   ```c
   // Нажать
   tud_hid_keyboard_report(0, 0, keycode);
   delay_ms(10);  // Небольшая задержка
   // Отпустить
   tud_hid_keyboard_report(0, 0, NULL);
   ```

3. **Максимум 6 обычных клавиш**: Можно нажать до 6 обычных клавиш + все модификаторы

4. **Порядок не важен**: Клавиши в массиве могут быть в любом порядке

5. **Нулевые значения игнорируются**: 0x00 означает "клавиша не нажата"

## Ссылки на спецификации

- USB HID Usage Tables: https://www.usb.org/sites/default/files/hut1_21_0.pdf
- USB HID Specification: https://www.usb.org/hid
- TinyUSB HID Documentation: https://docs.tinyusb.org/en/latest/reference/hid.html
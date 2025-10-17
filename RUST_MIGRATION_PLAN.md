# План миграции STM32F411 MIDI Pedal на Rust

## 📋 Анализ существующего проекта

### Текущая архитектура (C/C++)
- **MCU**: STM32F411xE (Cortex-M4F, 100 MHz)
- **Периферия**:
  - USB OTG FS (Full Speed 12 Mbps)
  - ADC1 для экспрессии педали
  - 4x GPIO (PA0-PA3) с EXTI прерываниями
  - TIM2, TIM3, TIM5 для тайминга
  - RTC для управления питанием
  - GPIO PC13 для LED индикации

### Функциональность
1. **USB Composite Device**:
   - MIDI интерфейс (EP 0x01/0x81)
   - HID Keyboard (EP 0x82)
   
2. **MIDI**:
   - Педали 1-2: Note On/Off (ноты 60, 61)
   - ADC педаль: CC 64 (Sustain Pedal)
   
3. **HID Keyboard**:
   - Педали 3-4: клавиши → и ← (0x4F, 0x50)

4. **Управление питанием**:
   - Standby режим с wake-up по ADC

### Текущий стек технологий
- **USB**: TinyUSB
- **HAL**: STM32 HAL (C)
- **Build**: CMake + ARM GCC
- **Язык**: C17 + C++23

---

## 🦀 Rust экосистема для STM32

### Базовые крейты

#### 1. HAL и PAC
- **`stm32f4xx-hal`** (v0.21+) - Hardware Abstraction Layer
  - GPIO с type-state pattern
  - ADC с DMA поддержкой
  - Таймеры и прерывания
  - Управление тактированием
  
- **`stm32f4`** - Peripheral Access Crate (PAC)
  - Прямой доступ к регистрам
  - Генерируется из SVD файлов

#### 2. USB стек
- **`usb-device`** (v0.3+) - USB фреймворк
  - Общий интерфейс для USB device
  - Composite device поддержка
  
- **`synopsys-usb-otg`** (v0.4+) - USB OTG драйвер для STM32F4
  - Поддержка DWC2 контроллера
  - Совместимость с usb-device

#### 3. USB классы
- **`usbd-hid`** (v0.7+) - HID класс
  - Keyboard, mouse support
  - Custom HID descriptors
  
- **`usbd-midi`** (v0.2+) - MIDI класс
  - MIDI streaming
  - Поддержка bulk endpoints

#### 4. Embedded инфраструктура
- **`cortex-m`** - Cortex-M процессоры
- **`cortex-m-rt`** - Runtime для Cortex-M
- **`panic-halt`** или `panic-probe` - Panic handlers
- **`heapless`** - Коллекции без heap (Vec, Queue и т.д.)
- **`embedded-hal`** - Общий HAL трейты

### Альтернативные решения

#### Embassy Framework (async/await)
- **`embassy-stm32`** - Async HAL для STM32
- **`embassy-usb`** - Async USB стек
- **Плюсы**: современный async подход, меньше кода
- **Минусы**: сложнее отладка, больший binary size

#### RTIC (Real-Time Interrupt-driven Concurrency)
- **`rtic`** (v2.x) - Concurrency framework
- **Плюсы**: zero-cost abstractions, статический анализ
- **Минусы**: крутая кривая обучения

---

## 🎯 Выбор архитектуры

### Рекомендуемый подход: `stm32f4xx-hal` + `usb-device`

**Обоснование**:
1. ✅ Зрелая экосистема с большим комьюнити
2. ✅ Хорошая документация и примеры
3. ✅ Прямое соответствие текущей архитектуре
4. ✅ Проще миграция и отладка
5. ✅ Меньше зависимостей

**Альтернатива для будущего**: Embassy framework
- Можно мигрировать позже для улучшения производительности
- Async подход упростит сложную логику

---

## 📁 Структура нового проекта

```
midi_pedal_f411_rust/
├── Cargo.toml              # Конфигурация проекта
├── .cargo/
│   └── config.toml         # Настройки сборки и линкера
├── memory.x                # Карта памяти (заменяет .ld файл)
├── build.rs                # Build script
├── src/
│   ├── main.rs             # Точка входа
│   ├── usb/
│   │   ├── mod.rs          # USB модуль
│   │   ├── descriptors.rs  # USB дескрипторы
│   │   ├── midi.rs         # MIDI функциональность
│   │   └── hid.rs          # HID Keyboard
│   ├── pedal/
│   │   ├── mod.rs          # Педали модуль
│   │   ├── gpio.rs         # GPIO и прерывания
│   │   └── adc.rs          # ADC обработка
│   ├── power.rs            # Управление питанием
│   └── peripherals.rs      # Инициализация периферии
├── .embed.toml             # probe-rs конфигурация
└── README.md
```

---

## 🔧 Детальный план реализации

### Фаза 1: Настройка проекта (1-2 дня)

#### 1.1 Создание Cargo проекта
```bash
cargo new --bin midi_pedal_f411_rust
cd midi_pedal_f411_rust
```

#### 1.2 Cargo.toml зависимости
```toml
[dependencies]
cortex-m = "0.7"
cortex-m-rt = "0.7"
stm32f4xx-hal = { version = "0.21", features = ["stm32f411"] }
usb-device = "0.3"
usbd-hid = "0.7"
usbd-midi = "0.2"
synopsys-usb-otg = { version = "0.4", features = ["fs"] }
heapless = "0.8"
panic-halt = "0.2"

[profile.release]
opt-level = "z"      # Оптимизация размера
lto = true           # Link Time Optimization
codegen-units = 1
```

#### 1.3 Настройка .cargo/config.toml
```toml
[target.thumbv7em-none-eabihf]
runner = "probe-rs run --chip STM32F411CEUx"
rustflags = [
  "-C", "link-arg=-Tlink.x",
]

[build]
target = "thumbv7em-none-eabihf"
```

#### 1.4 Создание memory.x
```
MEMORY
{
  FLASH : ORIGIN = 0x08000000, LENGTH = 512K
  RAM : ORIGIN = 0x20000000, LENGTH = 128K
}
```

### Фаза 2: Инициализация периферии (2-3 дня)

#### 2.1 Базовая инициализация (src/main.rs)
```rust
#![no_std]
#![no_main]

use cortex_m_rt::entry;
use panic_halt as _;
use stm32f4xx_hal::{pac, prelude::*};

#[entry]
fn main() -> ! {
    let dp = pac::Peripherals::take().unwrap();
    let cp = cortex_m::Peripherals::take().unwrap();
    
    // Настройка тактирования: 96 MHz (USB требует точной частоты)
    let rcc = dp.RCC.constrain();
    let clocks = rcc.cfgr
        .use_hse(25.MHz())
        .sysclk(96.MHz())
        .require_pll48clk()
        .freeze();
    
    // TODO: Инициализация периферии
    
    loop {}
}
```

#### 2.2 GPIO и EXTI прерывания (src/pedal/gpio.rs)
```rust
use stm32f4xx_hal::{
    gpio::{Edge, Input, Pin, PullUp},
    pac::EXTI,
    interrupt,
};

pub struct PedalPins {
    pub pedal0: Pin<'A', 0, Input<PullUp>>,
    pub pedal1: Pin<'A', 1, Input<PullUp>>,
    pub pedal2: Pin<'A', 2, Input<PullUp>>,
    pub pedal3: Pin<'A', 3, Input<PullUp>>,
}

impl PedalPins {
    pub fn setup_interrupts(&mut self, syscfg: &mut SYSCFG, exti: &mut EXTI) {
        self.pedal0.make_interrupt_source(syscfg);
        self.pedal0.enable_interrupt(exti);
        self.pedal0.trigger_on_edge(exti, Edge::Rising);
        // Аналогично для остальных
    }
}
```

#### 2.3 ADC для экспрессии (src/pedal/adc.rs)
```rust
use stm32f4xx_hal::{
    adc::{Adc, config::AdcConfig},
    gpio::{Analog, Pin},
};

pub struct ExpressionPedal {
    adc: Adc<ADC1>,
    pin: Pin<'B', 0, Analog>,
    last_value: u16,
}

impl ExpressionPedal {
    pub fn read(&mut self) -> Option<u8> {
        let value = self.adc.read(&mut self.pin).unwrap();
        // Фильтрация и преобразование в MIDI CC (0-127)
        if (value as i16 - self.last_value as i16).abs() > 14 {
            self.last_value = value;
            Some((value / 30).saturating_sub(9) as u8)
        } else {
            None
        }
    }
}
```

### Фаза 3: USB инфраструктура (3-4 дня)

#### 3.1 USB дескрипторы (src/usb/descriptors.rs)
```rust
use usb_device::descriptor::{lang_id::LangID, StringDescriptors};

pub const VID: u16 = 0xCAFE;
pub const PID: u16 = 0x400C;

pub fn create_string_descriptors() -> StringDescriptors {
    StringDescriptors::new(LangID::EN)
        .manufacturer("SCHE")
        .product("SCHE MIDI Pedal")
        .serial_number("F411-MIDI-001")
}
```

#### 3.2 Composite USB Device (src/usb/mod.rs)
```rust
use usb_device::{
    bus::UsbBusAllocator,
    prelude::*,
};
use usbd_hid::descriptor::KeyboardReport;
use usbd_midi::data::midi::message::Message;

pub struct UsbDevices<'a> {
    pub device: UsbDevice<'a, UsbBus>,
    pub midi: MidiClass<'a, UsbBus>,
    pub hid: HIDClass<'a, UsbBus>,
}

impl<'a> UsbDevices<'a> {
    pub fn new(usb_bus: &'a UsbBusAllocator<UsbBus>) -> Self {
        let midi = MidiClass::new(usb_bus);
        let hid = HIDClass::new(usb_bus, KeyboardReport::desc(), 10);
        
        let device = UsbDeviceBuilder::new(usb_bus, UsbVidPid(VID, PID))
            .manufacturer("SCHE")
            .product("SCHE MIDI Pedal")
            .composite_with_iads()
            .build();
            
        Self { device, midi, hid }
    }
    
    pub fn poll(&mut self) -> bool {
        self.device.poll(&mut [&mut self.midi, &mut self.hid])
    }
}
```

#### 3.3 MIDI функциональность (src/usb/midi.rs)
```rust
pub fn send_note_on(midi: &mut MidiClass<UsbBus>, note: u8, velocity: u8) {
    let message = Message::NoteOn(Channel1, note, velocity);
    midi.send_message(message).ok();
}

pub fn send_cc(midi: &mut MidiClass<UsbBus>, cc: u8, value: u8) {
    let message = Message::ControlChange(Channel1, cc, value);
    midi.send_message(message).ok();
}
```

#### 3.4 HID Keyboard (src/usb/hid.rs)
```rust
use usbd_hid::descriptor::{KeyboardReport, KeyboardUsage};

pub fn send_key(hid: &mut HIDClass<UsbBus>, key: KeyboardUsage) {
    let report = KeyboardReport {
        modifier: 0,
        reserved: 0,
        keycodes: [key as u8, 0, 0, 0, 0, 0],
    };
    hid.push_input(&report).ok();
}

pub fn release_keys(hid: &mut HIDClass<UsbBus>) {
    let report = KeyboardReport::default();
    hid.push_input(&report).ok();
}
```

### Фаза 4: Обработка педалей и прерываний (2-3 дня)

#### 4.1 Обработка прерываний
```rust
use cortex_m::interrupt::Mutex;
use core::cell::RefCell;

static PEDAL_QUEUE: Mutex<RefCell<Option<heapless::Deque<PedalEvent, 8>>>> = 
    Mutex::new(RefCell::new(None));

#[derive(Clone, Copy)]
pub struct PedalEvent {
    pub pedal_id: u8,
    pub timestamp: u32,
}

#[interrupt]
fn EXTI0() {
    cortex_m::interrupt::free(|cs| {
        if let Some(queue) = PEDAL_QUEUE.borrow(cs).borrow_mut().as_mut() {
            queue.push_back(PedalEvent {
                pedal_id: 0,
                timestamp: get_timestamp(),
            }).ok();
        }
    });
    // Очистить флаг прерывания
}
```

#### 4.2 Основной цикл обработки
```rust
fn main() -> ! {
    // ... инициализация ...
    
    loop {
        // USB polling
        if usb.poll() {
            // Обработка USB событий
        }
        
        // Обработка очереди педалей
        if let Some(event) = pedal_queue.pop_front() {
            handle_pedal_event(event, &mut usb);
        }
        
        // Чтение ADC
        if let Some(cc_value) = expression.read() {
            usb.send_cc(64, cc_value);
        }
        
        // Таймауты для release keys
        check_timeouts(&mut usb);
    }
}
```

### Фаза 5: Управление питанием (1 день)

#### 5.1 Power management (src/power.rs)
```rust
use stm32f4xx_hal::pac::{PWR, RCC};

pub fn check_standby_mode(pwr: &PWR, adc_value: u16) {
    if !pwr.csr.read().sbf().bit_is_set() {
        enter_standby_mode(pwr);
    } else if adc_value < 2000 {
        enter_standby_mode(pwr);
    }
}

fn enter_standby_mode(pwr: &PWR) {
    pwr.cr.modify(|_, w| w.cwuf().set_bit());
    pwr.csr.modify(|_, w| w.ewup().set_bit());
    pwr.cr.modify(|_, w| w.pdds().set_bit());
    cortex_m::asm::wfi();
}
```

### Фаза 6: Сборка и прошивка (1 день)

#### 6.1 Установка инструментов
```bash
# Rust toolchain для ARM Cortex-M4F
rustup target add thumbv7em-none-eabihf

# probe-rs для прошивки и отладки
cargo install probe-rs-tools

# cargo-binutils для анализа бинарника
cargo install cargo-binutils
rustup component add llvm-tools-preview
```

#### 6.2 Сборка
```bash
# Debug сборка
cargo build

# Release сборка (оптимизированная)
cargo build --release

# Проверка размера
cargo size --release -- -A
```

#### 6.3 Прошивка
```bash
# Через probe-rs (ST-Link, J-Link)
cargo run --release

# Или напрямую
probe-rs run --chip STM32F411CEUx target/thumbv7em-none-eabihf/release/midi_pedal_f411_rust
```

#### 6.4 Отладка
```bash
# RTT (Real-Time Transfer) для логов
probe-rs attach --chip STM32F411CEUx

# GDB отладка
probe-rs gdb --chip STM32F411CEUx target/thumbv7em-none-eabihf/release/midi_pedal_f411_rust
```

---

## 📊 Сравнение C/C++ vs Rust

### Преимущества Rust

| Аспект | C/C++ | Rust | Выигрыш |
|--------|-------|------|---------|
| **Безопасность памяти** | Manual management | Borrow checker | ✅ Нет UB |
| **Concurrency** | Ручная синхронизация | Send/Sync traits | ✅ Безопасность в compile-time |
| **Размер кода** | ~30KB | ~25-35KB | ≈ Сопоставимо |
| **Производительность** | Отличная | Отличная | ≈ Одинаково |
| **Типобезопасность** | Слабая | Сильная | ✅ Меньше ошибок |
| **Zero-cost abstractions** | Частично | Полностью | ✅ Выразительнее |
| **Управление зависимостями** | Git submodules | Cargo | ✅ Проще |

### Потенциальные сложности

1. **Кривая обучения**: 
   - Borrow checker требует привыкания
   - Embedded специфика (no_std, no heap)

2. **Экосистема**:
   - Меньше готовых примеров чем для C
   - Некоторые периферийные драйверы могут быть незрелыми

3. **Размер бинарника**:
   - Может быть больше из-за monomorphization
   - Решается оптимизацией и cargo-bloat

4. **Время компиляции**:
   - Rust компилируется дольше C
   - Но incremental compilation помогает

---

## 🎯 Этапы миграции

### Этап 1: Proof of Concept (1 неделя)
- [x] Базовая инициализация STM32F411
- [ ] USB enumeration как composite device
- [ ] Одна педаль → MIDI Note On
- [ ] Тестирование на реальном железе

### Этап 2: Полная функциональность (2 недели)
- [ ] Все 4 педали с прерываниями
- [ ] ADC для экспрессии → MIDI CC
- [ ] HID Keyboard функциональность
- [ ] Debouncing и таймауты
- [ ] LED индикация

### Этап 3: Оптимизация (1 неделя)
- [ ] Оптимизация размера бинарника
- [ ] Профилирование производительности
- [ ] Управление питанием
- [ ] Low power режимы

### Этап 4: Тестирование и документация (1 неделя)
- [ ] Интеграционное тестирование
- [ ] Стресс-тестирование USB
- [ ] Документация кода
- [ ] README и примеры

---

## 📝 Рекомендации по реализации

### Best Practices

1. **Используйте type-state pattern для GPIO**:
   ```rust
   // Компилятор не даст использовать неинициализированный пин
   let pin: Pin<'A', 0, Input<PullUp>> = gpioa.pa0.into_pull_up_input();
   ```

2. **Критические секции для shared state**:
   ```rust
   cortex_m::interrupt::free(|cs| {
       // Атомарный доступ к shared данным
   });
   ```

3. **heapless для коллекций**:
   ```rust
   // Очередь без heap allocation
   let mut queue: heapless::Deque<Event, 16> = heapless::Deque::new();
   ```

4. **defmt для отладочных логов**:
   ```rust
   defmt::info!("MIDI Note: {} Velocity: {}", note, velocity);
   ```

5. **Модульная архитектура**:
   - Каждый модуль отвечает за свою функциональность
   - Используйте traits для абстракций

### Потенциальные проблемы и решения

| Проблема | Решение |
|----------|---------|
| USB enumeration fails | Проверить тактирование USB (48 MHz обязательно) |
| EXTI не срабатывает | Включить SYSCFG clock, настроить NVIC priorities |
| ADC шум | Использовать DMA + averaging |
| Heap overflow | Использовать только heapless коллекции |
| Stack overflow | Увеличить stack size в memory.x |

---

## 🔗 Полезные ресурсы

### Документация
- [Embedded Rust Book](https://docs.rust-embedded.org/book/)
- [stm32f4xx-hal docs](https://docs.rs/stm32f4xx-hal/)
- [usb-device docs](https://docs.rs/usb-device/)
- [Discovery Book (STM32F3)](https://docs.rust-embedded.org/discovery/)

### Примеры проектов
- [stm32f4xx-hal examples](https://github.com/stm32-rs/stm32f4xx-hal/tree/master/examples)
- [usb-device examples](https://github.com/rust-embedded-community/usb-device/tree/master/examples)
- [keyberon](https://github.com/TeXitoi/keyberon) - Rust USB клавиатура

### Инструменты
- [probe-rs](https://probe.rs/) - Отладка и прошивка
- [cargo-embed](https://probe.rs/docs/tools/cargo-embed/) - Упрощенная прошивка
- [defmt](https://defmt.ferrous-systems.com/) - Эффективное логирование

---

## 📈 Ожидаемые результаты

### Метрики успеха
- ✅ Полная функциональная совместимость с C версией
- ✅ Размер прошивки ≤ 40 KB (текущая ~30 KB)
- ✅ Нет runtime ошибок (память, concurrency)
- ✅ Время отклика педалей < 1 мс
- ✅ USB stable enumeration
- ✅ Поддержка всех MIDI/HID функций

### Timeline (для одного разработчика)
- **Базовая реализация**: 2-3 недели
- **Полная функциональность**: 4-5 недель
- **Production-ready**: 6-8 недель

---

## 🚀 Следующие шаги

1. **Подтвердите план** - готовы ли начать реализацию?
2. **Выберите подход**:
   - Поэтапная миграция (безопаснее)
   - Полная переписывание (чище)
3. **Настройте окружение**:
   ```bash
   rustup target add thumbv7em-none-eabihf
   cargo install probe-rs-tools
   ```
4. **Создайте репозиторий** и начните с Фазы 1

---

## 💡 Дополнительные возможности (после базовой миграции)

1. **Конфигурация через USB**:
   - Custom HID endpoint для настройки
   - Сохранение в EEPROM/Flash

2. **DFU (Device Firmware Update)**:
   - Bootloader на Rust
   - OTA обновления

3. **Advanced MIDI**:
   - MPE (MIDI Polyphonic Expression)
   - SysEx сообщения

4. **LCD дисплей**:
   - Отображение текущих настроек
   - embedded-graphics

5. **Bluetooth MIDI**:
   - BLE через дополнительный модуль
   - Беспроводная педаль
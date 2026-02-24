// logic_rust.rs - Rust модуль для логики обработки педалей
// Этот файл демонстрирует примеры функций для постепенного перехода с C/C++ на Rust
//
// Для сборки требуется:
//   rustup target add thumbv7em-none-eabihf

#![no_std]

// Константы-идентификаторы педалей (соответствуют C++ enum pedal_type)
// Используем простые константы вместо enum для совместимости с C
pub const PEDAL_A: u8 = 0;
pub const PEDAL_B: u8 = 1;
pub const PEDAL_C: u8 = 2;
pub const PEDAL_D: u8 = 3;

// Константы-идентификаторы состояний педали
pub const COND_NONE: u8 = 0;
pub const COND_WORKED: u8 = 1;
pub const COND_PRESSED: u8 = 2;
pub const COND_FREE: u8 = 3;

// Константы из C++ кода
pub const DEBOUNCE_TICKS: u32 = 800u32;
pub const RELEASE_TICKS: u32 = 2000u32;
pub const TIMEOUT_TICKS: u32 = 3000u32;
pub const ADC_MIN: u32 = 300u32;
pub const ADC_HYSTERESIS: u32 = 14u32;
pub const MIDI_CC_SCALE: u8 = 30u8;
pub const MIDI_CC_OFFSET: u8 = 9u8;
pub const MIDI_CC_CHANNEL: u8 = 176u8;
pub const MIDI_CC_NUM: u8 = 64u8;
pub const MIDI_NOTE_CH: u8 = 0x91u8;
pub const MIDI_CC_MAX: u8 = 127u8;

// ============================================================================
// ВНЕШНИЕ ФУНКЦИИ (HAL, LL, TinyUSB) - вызываются из Rust
// ============================================================================

// Определения внешних функций HAL
extern "C" {
    // ADC функции
    pub fn HAL_ADC_Start_IT(hadc: *mut core::ffi::c_void);
    pub fn HAL_ADC_GetValue(hadc: *mut core::ffi::c_void) -> u32;
    
    // TIM функции
    pub fn HAL_TIM_Base_Start(htim: *mut core::ffi::c_void);
    pub fn HAL_TIM_Base_Start_IT(htim: *mut core::ffi::c_void);
    
    // GPIO функции
    pub fn HAL_GPIO_ReadPin(GPIOx: *mut core::ffi::c_void, GPIO_Pin: u16) -> u32;
    
    // Delay
    pub fn HAL_Delay(Delay: u32);
    
    // Power функции
    pub fn HAL_PWR_DisableWakeUpPin(UPWR_Pin: u32);
    pub fn HAL_PWR_EnableWakeUpPin(UPWR_Pin: u32);
    pub fn HAL_PWR_EnterSTANDBYMode();
    
    // USB функции TinyUSB
    pub fn tud_init(rhport: u8);
    pub fn tud_task();
    pub fn tud_midi_stream_write(itf: u8, data: *const u8, len: usize) -> usize;
    pub fn tud_hid_keyboard_report(itf: u8, modifier: u8, keycode: *const u8) -> bool;
    pub fn tud_hid_ready() -> bool;
    pub fn tud_disconnect();
    pub fn tud_int_handler(rhport: u8);
    
    // Board API
    pub fn board_init_usb();
    
    // Регистры
    pub static mut TIM5: TIM_TypeDef;
    pub static mut TIM2: TIM_TypeDef;
    pub static mut EXTI: EXTI_TypeDef;
    pub static mut GPIOC: GPIO_TypeDef;
    pub static mut GPIOA: GPIO_TypeDef;
}

// Определения типов регистров (STM32F4xx Reference Manual, Table 58)
#[repr(C)]
pub struct TIM_TypeDef {
    pub CR1: u32,     // 0x00
    pub CR2: u32,     // 0x04
    pub SMCR: u32,    // 0x08
    pub DIER: u32,    // 0x0C
    pub SR: u32,      // 0x10
    pub EGR: u32,    // 0x14
    pub CCMR1: u32,   // 0x18
    pub CCMR2: u32,   // 0x1C
    pub CCER: u32,    // 0x20
    pub CNT: u32,     // 0x24 - счётчик
    pub PSC: u32,     // 0x28 - предделитель
    pub ARR: u32,     // 0x2C - автоперезагрузка
}

#[repr(C)]
pub struct EXTI_TypeDef {
    pub IMR: u32,
    pub EMR: u32,
    pub RTSR: u32,
    pub FTSR: u32,
    pub SWIER: u32,
    pub PR: u32,
    _reserved: [u32; 2],
}

#[repr(C)]
pub struct GPIO_TypeDef {
    pub MODER: u32,
    pub OTYPER: u32,
    pub OSPEEDR: u32,
    pub PUPDR: u32,
    pub IDR: u32,
    pub ODR: u32,
    pub BSRR: u32,
    pub LCKR: u32,
    pub AFR: [u32; 2],
}

// ============================================================================
// ФУНКЦИИ ДЛЯ ВЫЗОВА ИЗ ПРЕРЫВАНИЙ (C -> Rust)
// ============================================================================

// Функция обратного вызова для EXTI0 прерывания
// Эта функция будет вызываться из C обработчика прерывания
#[no_mangle]
pub extern "C" fn rust_exti0_handler(timestamp: u32) {
    let _ = timestamp;
    // Пример логики: отключаем прерывание и добавляем в очередь
    unsafe {
        EXTI.PR = 1 << 0;  // Очищаем флаг прерывания
        EXTI.IMR &= !(1 << 0);  // Отключаем прерывание
        
        // Здесь можно добавить логику обработки
        // Например, push в кольцевой буфер
    }
}

// Аналогичные функции для других каналов
#[no_mangle]
pub extern "C" fn rust_exti1_handler(timestamp: u32) {
    let _ = timestamp;
    unsafe {
        EXTI.PR = 1 << 1;
        EXTI.IMR &= !(1 << 1);
    }
}

#[no_mangle]
pub extern "C" fn rust_exti2_handler(timestamp: u32) {
    let _ = timestamp;
    unsafe {
        EXTI.PR = 1 << 2;
        EXTI.IMR &= !(1 << 2);
    }
}

#[no_mangle]
pub extern "C" fn rust_exti3_handler(timestamp: u32) {
    let _ = timestamp;
    unsafe {
        EXTI.PR = 1 << 3;
        EXTI.IMR &= !(1 << 3);
    }
}

// ============================================================================
// ФУНКЦИИ ОБРАТНОГО ВЫЗОВА ADC (C -> Rust)
// ============================================================================

// Обработчик завершения ADC преобразования
#[no_mangle]
pub extern "C" fn rust_adc_callback(raw_value: u32) {
    // Пример логики обработки ADC значения
    // Подавление шума
    let _filtered = if raw_value < ADC_MIN { ADC_MIN } else { raw_value };
    
    // Пример вычисления velocity (должно быть в static переменной)
    // cc_velocity = filtered / MIDI_CC_SCALE - MIDI_CC_OFFSET;
    
    // Здесь можно добавить логику аналогичную C++ коду в HAL_ADC_ConvCpltCallback
}

// ============================================================================
// ФУНКЦИИ ДЛЯ ОТПРАВКИ MIDI/USB ИЗ RUST
// ============================================================================

// Отправка MIDI Note On
#[no_mangle]
pub extern "C" fn rust_send_midi_note(channel: u8, note: u8, velocity: u8) {
    // MIDI Note On: status = 0x90 | (channel & 0x0F)
    let status: u8 = 0x90u8 | (channel & 0x0F);
    let data: [u8; 3] = [status, note, velocity];
    unsafe {
        let ptr = data.as_ptr();
        tud_midi_stream_write(0, ptr, 3);
        // Сброс таймера TIM2
        TIM2.CNT = 0;
    }
}

// Отправка MIDI CC (Control Change)
#[no_mangle]
pub extern "C" fn rust_send_midi_cc(cc_num: u8, value: u8) {
    let data: [u8; 3] = [MIDI_CC_CHANNEL, cc_num, value];
    unsafe {
        let ptr = data.as_ptr();
        tud_midi_stream_write(0, ptr, 3);
        TIM2.CNT = 0;
    }
}

// Отправка Keyboard HID репорта
#[no_mangle]
pub extern "C" fn rust_send_keyboard(keycode: u8) {
    unsafe {
        if tud_hid_ready() {
            let keycodes = [keycode, 0, 0, 0, 0, 0];
            tud_hid_keyboard_report(0, 0, keycodes.as_ptr());
        }
    }
}

// Освобождение клавиши
#[no_mangle]
pub extern "C" fn rust_release_keyboard() {
    unsafe {
        tud_hid_keyboard_report(0, 0, core::ptr::null());
    }
}

// ============================================================================
// ФУНКЦИИ ДЛЯ РАБОТЫ С LED
// ============================================================================

const LED_PIN: u32 = 1 << 13;  // GPIO_PIN_13

#[no_mangle]
pub extern "C" fn rust_led_on() {
    unsafe {
        GPIOC.BSRR = LED_PIN;
    }
}

#[no_mangle]
pub extern "C" fn rust_led_off() {
    unsafe {
        GPIOC.BSRR = LED_PIN << 16;  // BSRR reset
    }
}

// ============================================================================
// ФУНКЦИИ ДЛЯ ЧТЕНИЯ СОСТОЯНИЯ ПЕДАЛЕЙ
// ============================================================================

// Чтение состояния педали (GPIO)
// pedal: 0=A, 1=B, 2=C, 3=D
#[no_mangle]
pub extern "C" fn rust_read_pedal_gpio(pedal: u8) -> bool {
    unsafe {
        let pin: u16 = match pedal {
            0 => 1 << 0,   // GPIO_PIN_0
            1 => 1 << 1,   // GPIO_PIN_1
            2 => 1 << 2,   // GPIO_PIN_2
            3 => 1 << 3,   // GPIO_PIN_3
            _ => return false,
        };
        
        let state = HAL_GPIO_ReadPin(core::ptr::addr_of_mut!(GPIOA) as *mut core::ffi::c_void, pin);
        state == 0  // Active low (GPIO_PIN_RESET)
    }
}

// ============================================================================
// ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ
// ============================================================================

// Вычисление разницы во времени с учетом переполнения
#[inline(always)]
pub fn time_length(t1: u32, t2: u32) -> u32 {
    if t1 > t2 {
        (u32::MAX - t1) + t2 + 1
    } else {
        t2 - t1
    }
}

// Включение EXTI линии
// pedal: 0=A, 1=B, 2=C, 3=D
#[no_mangle]
pub extern "C" fn rust_exti_enable(pedal: u8) {
    unsafe {
        core::arch::asm!("cpsid i", options(nostack, nomem));
        EXTI.IMR |= 1 << pedal;
        core::arch::asm!("cpsie i", options(nostack, nomem));
    }
}

// ============================================================================
// ТЕСТОВАЯ ФУНКЦИЯ (для проверки компиляции)
// ============================================================================

#[no_mangle]
pub extern "C" fn rust_logic_init() {
    // Пример инициализации - аналог начала функции pedal() в C++
    unsafe {
        // Запуск ADC
        // HAL_ADC_Start_IT(&hadc1);
        
        // Запуск таймеров
        // HAL_TIM_Base_Start(&htim3);
        
        // Инициализация USB
        // board_init_usb();
        // tud_init(0);
    }
}

// ============================================================================
// PANIC HANDLER (обязателен для no_std staticlib)
// ============================================================================

#[panic_handler]
fn panic(_info: &core::panic::PanicInfo) -> ! {
    loop {}
}

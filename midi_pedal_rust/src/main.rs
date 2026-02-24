#![no_std]
#![no_main]

//! MIDI Pedal Controller for STM32F411
//! Ported from C++ to Rust

use core::arch::asm;
use core::mem::MaybeUninit;
use core::ptr;

// Import entry from cortex-m-rt
use cortex_m_rt::entry;

// Panic handler
use panic_halt as _;

// Register addresses
const RCC_BASE: u32 = 0x40023800;
const GPIOA_BASE: u32 = 0x40020000;
const GPIOB_BASE: u32 = 0x40020400;
const GPIOC_BASE: u32 = 0x40020800;
const EXTI_BASE: u32 = 0x40013C00;
const ADC1_BASE: u32 = 0x40012000;
const TIM2_BASE: u32 = 0x40000000;
const TIM3_BASE: u32 = 0x40000400;
const TIM5_BASE: u32 = 0x40000C00;

// Register offsets
const RCC_AHB1ENR: u32 = 0x30;
const RCC_APB2ENR: u32 = 0x44;
const GPIO_BSRR: u32 = 0x18;
const GPIO_IDR: u32 = 0x10;
const GPIO_MODER: u32 = 0x00;
const GPIO_PUPDR: u32 = 0x0C;
const EXTI_IMR: u32 = 0x00;
const EXTI_RTSR: u32 = 0x08;
const EXTI_PR: u32 = 0x14;
const ADC_SR: u32 = 0x00;
const ADC_DR: u32 = 0x40;
const ADC_CR1: u32 = 0x04;
const ADC_CR2: u32 = 0x08;
const ADC_SMPR2: u32 = 0x1C;
const ADC_SQR3: u32 = 0x34;
const TIM_CR1: u32 = 0x00;
const TIM_CR2: u32 = 0x04;
const TIM_DIER: u32 = 0x0C;
const TIM_PSC: u32 = 0x28;
const TIM_ARR: u32 = 0x2C;
const TIM_CNT: u32 = 0x24;
const TIM_SR: u32 = 0x10;

// Constants
const LED_ON: u32 = 1 << 13;
const LED_OFF: u32 = 1 << (13 + 16);

const RIGHT_ARROW: u8 = 0x4F;
const LEFT_ARROW: u8 = 0x50;

const DEBOUNCE_TICKS: u32 = 800;
const RELEASE_TICKS: u32 = 2000;
const TIMEOUT_TICKS: u32 = 3000;

const ADC_MIN: u32 = 300;
const ADC_HYSTERESIS: u32 = 14;
const MIDI_CC_SCALE: u8 = 30;
const MIDI_CC_OFFSET: u8 = 9;
const MIDI_CC_NUM: u8 = 64;

const RING_BUF_SIZE: usize = 8;

// Pedal types
#[derive(Clone, Copy)]
enum PedalType {
    A, B, C, D,
}

impl PedalType {
    fn to_exti_mask(&self) -> u32 {
        match self {
            PedalType::A => 1 << 0,
            PedalType::B => 1 << 1,
            PedalType::C => 1 << 2,
            PedalType::D => 1 << 3,
        }
    }
}

#[derive(Clone, Copy, PartialEq)]
enum PedalCondition {
    None, Worked, Pressed, Free,
}

#[derive(Clone, Copy)]
struct PedalEvent {
    ped: PedalType,
    time: u32,
    condition: PedalCondition,
}

struct RingBuffer {
    buf: [MaybeUninit<PedalEvent>; RING_BUF_SIZE],
    write_idx: usize,
    read_idx: usize,
}

impl RingBuffer {
    const fn new() -> Self {
        Self {
            buf: [MaybeUninit::uninit(); RING_BUF_SIZE],
            write_idx: 0,
            read_idx: 0,
        }
    }
    
    fn push(&mut self, item: PedalEvent) {
        let next = (self.write_idx + 1) & (RING_BUF_SIZE - 1);
        if next != self.read_idx {
            unsafe { asm!("dmb") };
            self.buf[self.write_idx].write(item);
            self.write_idx = next;
        }
    }
    
    fn empty(&self) -> bool {
        self.read_idx == self.write_idx
    }
    
    fn front(&self) -> &PedalEvent {
        unsafe { &*self.buf[self.read_idx].as_ptr() }
    }
    
    fn pop_front(&mut self) {
        if !self.empty() {
            unsafe { asm!("dmb") };
            self.read_idx = (self.read_idx + 1) & (RING_BUF_SIZE - 1);
        }
    }
}

// Global state
static mut PEDAL_BUFFER: RingBuffer = RingBuffer::new();
static mut ADC_RAW: u32 = 0;
static mut ADC_PREV: u32 = 0;
static mut CC_VELOCITY: u8 = 0;
static mut CC_VELOCITY_PREV: u8 = 0;
static mut PWR_FLAG: bool = false;

// Register access helpers
#[inline]
fn read_reg(addr: u32) -> u32 {
    unsafe { ptr::read_volatile(addr as *const u32) }
}

#[inline]
fn write_reg(addr: u32, val: u32) {
    unsafe { ptr::write_volatile(addr as *mut u32, val) }
}

#[inline]
fn set_bits(addr: u32, mask: u32) {
    write_reg(addr, read_reg(addr) | mask);
}

#[inline]
fn clear_bits(addr: u32, mask: u32) {
    write_reg(addr, read_reg(addr) & !mask);
}

// Time difference handling overflow
fn time_length(t1: u32, t2: u32) -> u32 {
    if t1 > t2 { (0xFFFFFFFF - t1) + t2 + 1 } else { t2 - t1 }
}

// Enable EXTI
#[inline]
fn exti_enable(ped: PedalType) {
    set_bits(EXTI_BASE + EXTI_IMR, ped.to_exti_mask());
}

// MIDI/Keyboard send placeholders
#[inline]
fn midi_sender(_note: u8, _velocity: u8) {
    write_reg(TIM2_BASE + TIM_CNT, 0);
}

#[inline]
fn midi_cc_send(_cc_num: u8, _value: u8) {
    write_reg(TIM2_BASE + TIM_CNT, 0);
}

#[inline]
fn key_sender(_command: u8) {}

// LED control
#[inline]
fn led_on() { write_reg(GPIOC_BASE + GPIO_BSRR, LED_ON); }
#[inline]
fn led_off() { write_reg(GPIOC_BASE + GPIO_BSRR, LED_OFF); }

// GPIO read
fn read_pedal_pin(pin: u32) -> bool {
    (read_reg(GPIOA_BASE + GPIO_IDR) & (1 << pin)) != 0
}

// ADC callback
fn adc_callback(value: u32) {
    unsafe {
        if PWR_FLAG {
            ADC_RAW = value.min(ADC_MIN);
            let diff = if ADC_RAW > ADC_PREV { ADC_RAW - ADC_PREV } else { ADC_PREV - ADC_RAW };
            if diff > ADC_HYSTERESIS {
                CC_VELOCITY = (ADC_RAW / MIDI_CC_SCALE as u32).saturating_sub(MIDI_CC_OFFSET as u32) as u8;
                if CC_VELOCITY != CC_VELOCITY_PREV {
                    midi_cc_send(MIDI_CC_NUM, CC_VELOCITY);
                    CC_VELOCITY_PREV = CC_VELOCITY;
                    write_reg(TIM2_BASE + TIM_CNT, 0);
                }
                ADC_PREV = ADC_RAW;
            }
        }
    }
}

// Process pedal events
fn process_pedal_events() {
    unsafe {
        let buffer = &mut PEDAL_BUFFER;
        
        while !buffer.empty() {
            let now = read_reg(TIM5_BASE + TIM_CNT);
            let event = *buffer.front(); // Copy the value
            let elapsed = time_length(event.time, now);
            
            match event.condition {
                PedalCondition::Worked => {
                    if elapsed > DEBOUNCE_TICKS {
                        let pin_pressed = match event.ped {
                            PedalType::A => read_pedal_pin(0),
                            PedalType::B => read_pedal_pin(1),
                            PedalType::C => read_pedal_pin(2),
                            PedalType::D => read_pedal_pin(3),
                        };
                        if pin_pressed {
                            match event.ped {
                                PedalType::A => { midi_sender(60, 44); led_on(); }
                                PedalType::B => { midi_sender(61, 33); led_on(); }
                                PedalType::C => { key_sender(RIGHT_ARROW); led_on(); }
                                PedalType::D => { key_sender(LEFT_ARROW); led_on(); }
                            }
                        }
                    }
                }
                PedalCondition::Pressed => {
                    if elapsed > RELEASE_TICKS {
                        exti_enable(event.ped);
                        buffer.pop_front();
                        led_off();
                    }
                }
                _ => { buffer.pop_front(); }
            }
            
            if event.condition == PedalCondition::Worked && elapsed > TIMEOUT_TICKS {
                exti_enable(event.ped);
                buffer.pop_front();
                led_off();
            }
        }
    }
}

// USB task placeholder
fn usb_device_task() {}

// Interrupt handlers using vector table
// Note: These must match the actual interrupt vector table for STM32F411
#[inline]
pub fn exti0_handler() {
    write_reg(EXTI_BASE + EXTI_PR, 1 << 0);
    clear_bits(EXTI_BASE + EXTI_IMR, 1 << 0);
    let time = read_reg(TIM5_BASE + TIM_CNT);
    unsafe { PEDAL_BUFFER.push(PedalEvent { ped: PedalType::A, time, condition: PedalCondition::Worked }); }
}

#[inline]
pub fn exti1_handler() {
    write_reg(EXTI_BASE + EXTI_PR, 1 << 1);
    clear_bits(EXTI_BASE + EXTI_IMR, 1 << 1);
    let time = read_reg(TIM5_BASE + TIM_CNT);
    unsafe { PEDAL_BUFFER.push(PedalEvent { ped: PedalType::B, time, condition: PedalCondition::Worked }); }
}

#[inline]
pub fn exti2_handler() {
    write_reg(EXTI_BASE + EXTI_PR, 1 << 2);
    clear_bits(EXTI_BASE + EXTI_IMR, 1 << 2);
    let time = read_reg(TIM5_BASE + TIM_CNT);
    unsafe { PEDAL_BUFFER.push(PedalEvent { ped: PedalType::C, time, condition: PedalCondition::Worked }); }
}

#[inline]
pub fn exti3_handler() {
    write_reg(EXTI_BASE + EXTI_PR, 1 << 3);
    clear_bits(EXTI_BASE + EXTI_IMR, 1 << 3);
    let time = read_reg(TIM5_BASE + TIM_CNT);
    unsafe { PEDAL_BUFFER.push(PedalEvent { ped: PedalType::D, time, condition: PedalCondition::Worked }); }
}

#[inline]
pub fn adc_handler() {
    if read_reg(ADC1_BASE + ADC_SR) & 0x2 != 0 {
        adc_callback(read_reg(ADC1_BASE + ADC_DR));
    }
}

#[inline]
pub fn tim2_handler() {
    write_reg(TIM2_BASE + TIM_SR, 0);
}

// Main entry
#[entry]
fn main() -> ! {
    // Wait for stabilization
    unsafe {
        for _ in 0..100_000 { asm!("nop"); }
    }
    
    // Enable GPIO clocks
    set_bits(RCC_BASE + RCC_AHB1ENR, 0x7); // GPIOA, GPIOB, GPIOC
    
    // Small delay
    unsafe {
        for _ in 0..100 { asm!("nop"); }
    }
    
    // LED off - PC13 as output (bits 26-27 = 01)
    let moder = read_reg(GPIOC_BASE + GPIO_MODER);
    write_reg(GPIOC_BASE + GPIO_MODER, (moder & !(0x3 << 26)) | (0x1 << 26));
    led_off();
    
    // Pedals pull-up on PA0-PA3 (PUPDR bits 0-1 = 01 for each)
    let pupdr = read_reg(GPIOA_BASE + GPIO_PUPDR);
    write_reg(GPIOA_BASE + GPIO_PUPDR, (pupdr & !0xFF) | 0x55);
    
    // Enable EXTI for lines 0-3
    set_bits(EXTI_BASE + EXTI_IMR, 0xF);
    // Rising edge trigger
    set_bits(EXTI_BASE + EXTI_RTSR, 0xF);
    
    // Enable ADC1 clock
    set_bits(RCC_BASE + RCC_APB2ENR, 1 << 8);
    
    // ADC config - 12-bit, single conversion
    let cr1 = read_reg(ADC1_BASE + ADC_CR1);
    write_reg(ADC1_BASE + ADC_CR1, cr1 & !0x3); // res bits = 00 (12-bit)
    
    let cr2 = read_reg(ADC1_BASE + ADC_CR2);
    write_reg(ADC1_BASE + ADC_CR2, cr2 | (1 << 30) | (1 << 10)); // ADON=1, EOCS=1
    
    // Sample time for channel 8 (PB0) - 480 cycles (bits 24-26 = 111)
    let smpr2 = read_reg(ADC1_BASE + ADC_SMPR2);
    write_reg(ADC1_BASE + ADC_SMPR2, smpr2 | (7 << 24));
    
    // Channel 8 as first in sequence
    write_reg(ADC1_BASE + ADC_SQR3, 8);
    
    // Trigger from TIM3
    let cr2_2 = read_reg(ADC1_BASE + ADC_CR2);
    write_reg(ADC1_BASE + ADC_CR2, cr2_2 & !(0x7 << 24) | (0 << 24)); // EXTSEL = 000 (TIM3_TRGO)
    
    // Configure TIM3
    set_bits(RCC_BASE + RCC_APB2ENR, 1 << 1); // TIM3 clock (actually APB1)
    // TIM3 on APB1 = 96MHz / 96 = 1MHz, period 8000 = 8ms
    write_reg(TIM3_BASE + TIM_PSC, 95);
    write_reg(TIM3_BASE + TIM_ARR, 8000);
    // Master mode - update triggers ADC
    write_reg(TIM3_BASE + TIM_CR2, (read_reg(TIM3_BASE + TIM_CR2) & !(0x7 << 4)) | (2 << 4)); // MMS = 010
    // Enable
    set_bits(TIM3_BASE + TIM_CR1, 1);
    
    // Configure TIM2
    set_bits(RCC_BASE + 0x1C, 1 << 0); // TIM2 clock (APB1)
    write_reg(TIM2_BASE + TIM_PSC, 9599);
    write_reg(TIM2_BASE + TIM_ARR, 6000000);
    set_bits(TIM2_BASE + TIM_DIER, 1); // Update interrupt
    set_bits(TIM2_BASE + TIM_CR1, 1);
    
    // Configure TIM5 (32-bit counter)
    set_bits(RCC_BASE + 0x1C, 1 << 3); // TIM5 clock (APB1)
    write_reg(TIM5_BASE + TIM_PSC, 9599);
    write_reg(TIM5_BASE + TIM_ARR, 0xFFFFFFFF);
    set_bits(TIM5_BASE + TIM_CR1, 1);
    
    // Start ADC
    let cr2_adc = read_reg(ADC1_BASE + ADC_CR2);
    write_reg(ADC1_BASE + ADC_CR2, cr2_adc | (1 << 30) | (1 << 31)); // SWSTART
    
    unsafe { PWR_FLAG = true; }
    
    loop {
        process_pedal_events();
        usb_device_task();
    }
}

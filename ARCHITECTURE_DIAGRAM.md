# Архитектура MIDI Pedal на Rust

## Диаграмма компонентов системы

```mermaid
graph TB
    subgraph Hardware["🔧 Hardware Layer"]
        MCU[STM32F411CEU6<br/>Cortex-M4F @ 96MHz]
        USB_PHY[USB OTG FS PHY]
        PEDALS[4x Pedals<br/>PA0-PA3]
        ADC_PIN[Expression Pedal<br/>ADC PA4]
        LED[LED PC13]
    end

    subgraph Firmware["⚙️ Firmware Layer - Rust"]
        subgraph HAL["HAL Layer"]
            PAC[stm32f4<br/>Peripheral Access Crate]
            HAL_CRATE[stm32f4xx-hal<br/>Hardware Abstraction]
        end
        
        subgraph USB["USB Stack"]
            USB_DEVICE[usb-device<br/>Core Framework]
            USB_OTG[synopsys-usb-otg<br/>DWC2 Driver]
            MIDI_CLASS[usbd-midi<br/>MIDI Class]
            HID_CLASS[usbd-hid<br/>HID Keyboard]
        end
        
        subgraph App["Application Logic"]
            MAIN[Main Loop<br/>Event Processing]
            PEDAL_MGR[Pedal Manager<br/>Debounce & Queue]
            ADC_MGR[ADC Manager<br/>Expression Control]
            PWR_MGR[Power Manager<br/>Standby Mode]
        end
    end

    subgraph Output["🎵 Output"]
        MIDI_OUT[MIDI Messages<br/>Notes & CC]
        HID_OUT[HID Reports<br/>Keyboard Keys]
    end

    MCU --> PAC
    PAC --> HAL_CRATE
    HAL_CRATE --> USB_OTG
    HAL_CRATE --> PEDAL_MGR
    HAL_CRATE --> ADC_MGR
    HAL_CRATE --> PWR_MGR
    
    USB_PHY --> USB_OTG
    USB_OTG --> USB_DEVICE
    USB_DEVICE --> MIDI_CLASS
    USB_DEVICE --> HID_CLASS
    
    PEDALS --> PEDAL_MGR
    ADC_PIN --> ADC_MGR
    LED --> MAIN
    
    PEDAL_MGR --> MAIN
    ADC_MGR --> MAIN
    PWR_MGR --> MAIN
    
    MAIN --> MIDI_CLASS
    MAIN --> HID_CLASS
    
    MIDI_CLASS --> MIDI_OUT
    HID_CLASS --> HID_OUT
```

## Поток данных педалей

```mermaid
sequenceDiagram
    participant Pedal as 🎹 Pedal
    participant EXTI as EXTI Handler
    participant Queue as Event Queue
    participant Main as Main Loop
    participant USB as USB Stack
    participant Host as 💻 USB Host

    Pedal->>EXTI: Press (Rising Edge)
    activate EXTI
    EXTI->>Queue: Push Event {id, timestamp}
    EXTI->>EXTI: Disable EXTI
    deactivate EXTI
    
    loop Main Loop
        Main->>Queue: Pop Event?
        alt Event Available
            Queue->>Main: Event {id, timestamp}
            Main->>USB: Send MIDI/HID
            USB->>Host: USB Packet
            Main->>Main: Schedule Re-enable
        end
        
        Main->>Main: Check Timeout
        alt Timeout Reached
            Main->>USB: Release Keys
            USB->>Host: Key Release
            Main->>EXTI: Re-enable Interrupt
        end
    end
```

## Диаграмма состояний USB

```mermaid
stateDiagram-v2
    [*] --> Detached
    Detached --> Attached: USB Cable Connect
    Attached --> Powered: VBUS Detected
    Powered --> Default: USB Reset
    Default --> Address: Set Address
    Address --> Configured: Set Configuration
    
    Configured --> Ready: Enumeration Complete
    
    state Ready {
        [*] --> Idle
        Idle --> SendingMIDI: MIDI Event
        SendingMIDI --> Idle: Sent
        Idle --> SendingHID: HID Event
        SendingHID --> Idle: Sent
    }
    
    Ready --> Suspended: No Activity 3ms
    Suspended --> Ready: Resume Signal
    
    Ready --> Detached: USB Disconnect
    Suspended --> Detached: USB Disconnect
```

## Архитектура модулей

```mermaid
graph LR
    subgraph src["📁 src/"]
        MAIN_RS[main.rs<br/>Entry Point]
        PERIPH[peripherals.rs<br/>Init & Config]
        POWER[power.rs<br/>Power Mgmt]
        
        subgraph usb["📁 usb/"]
            USB_MOD[mod.rs<br/>USB Manager]
            USB_DESC[descriptors.rs<br/>Descriptors]
            USB_MIDI[midi.rs<br/>MIDI Logic]
            USB_HID[hid.rs<br/>HID Logic]
        end
        
        subgraph pedal["📁 pedal/"]
            PEDAL_MOD[mod.rs<br/>Pedal Manager]
            PEDAL_GPIO[gpio.rs<br/>GPIO & EXTI]
            PEDAL_ADC[adc.rs<br/>ADC Reader]
        end
    end
    
    MAIN_RS --> PERIPH
    MAIN_RS --> POWER
    MAIN_RS --> USB_MOD
    MAIN_RS --> PEDAL_MOD
    
    USB_MOD --> USB_DESC
    USB_MOD --> USB_MIDI
    USB_MOD --> USB_HID
    
    PEDAL_MOD --> PEDAL_GPIO
    PEDAL_MOD --> PEDAL_ADC
```

## Карта памяти

```mermaid
graph TB
    subgraph FLASH["🔵 FLASH - 512 KB"]
        VECTOR[Vector Table<br/>0x08000000]
        CODE[Application Code<br/>~25-35 KB]
        RODATA[Const Data & Strings]
        UNUSED_F[Unused ~470KB]
    end
    
    subgraph RAM["🟢 RAM - 128 KB"]
        STACK[Stack<br/>~16 KB]
        BSS[BSS - Uninit Data<br/>USB Buffers]
        DATA[Data - Init Globals<br/>State Variables]
        HEAP[Optional Heap<br/>Not Used]
        UNUSED_R[Free RAM ~100KB]
    end
    
    FLASH -.Copy.-> DATA
    
    style FLASH fill:#e3f2fd
    style RAM fill:#e8f5e9
    style HEAP fill:#ffebee
```

## Временная диаграмма обработки событий

```mermaid
gantt
    title Timing Diagram - Pedal Press Event
    dateFormat X
    axisFormat %L ms
    
    section Hardware
    Pedal Press           :0, 1
    EXTI Trigger          :1, 2
    
    section Interrupt
    EXTI Handler          :2, 3
    Queue Push            :2, 3
    Disable EXTI          :3, 4
    
    section Main Loop
    USB Poll              :4, 5
    Queue Check           :5, 6
    Send MIDI/HID         :6, 7
    USB Transfer          :7, 10
    
    section Timeout
    Debounce Wait         :10, 14
    Key Release           :14, 15
    Re-enable EXTI        :15, 16
```

## Сравнение архитектур C++ vs Rust

| Компонент | C++ (Current) | Rust (Planned) |
|-----------|---------------|----------------|
| **HAL** | STM32 HAL (C) | stm32f4xx-hal |
| **USB** | TinyUSB | usb-device + synopsys-usb-otg |
| **MIDI** | Custom (TinyUSB) | usbd-midi |
| **HID** | Custom (TinyUSB) | usbd-hid |
| **Concurrency** | Manual (deque + interrupts) | Safe abstractions (Mutex + RefCell) |
| **Memory Safety** | Manual | Borrow checker |
| **Build System** | CMake | Cargo |
| **Размер кода** | ~30 KB | ~25-35 KB (estimated) |

## Приоритеты прерываний

```
Priority 0 (Highest)
    └── (Reserved for critical)

Priority 2
    ├── EXTI0 (Pedal 1)
    ├── EXTI1 (Pedal 2)
    ├── EXTI2 (Pedal 3)
    └── EXTI3 (Pedal 4)

Priority 6
    └── OTG_FS (USB)

Priority 15 (Lowest)
    └── SysTick
```

## Конфигурация тактирования

```mermaid
graph LR
    HSE[HSE Crystal<br/>25 MHz] --> PLL[PLL]
    PLL --> |PLLM=25| DIV1[÷25]
    DIV1 --> |1 MHz| MUL[×192]
    MUL --> |192 MHz| DIV2[÷2]
    DIV2 --> SYSCLK[SYSCLK<br/>96 MHz]
    
    PLL --> |PLLQ=4| USB_CLK[USB Clock<br/>48 MHz]
    
    SYSCLK --> AHB[AHB Bus<br/>96 MHz]
    AHB --> APB1[APB1<br/>48 MHz]
    AHB --> APB2[APB2<br/>96 MHz]
    
    style USB_CLK fill:#ffeb3b
    style SYSCLK fill:#4caf50
```

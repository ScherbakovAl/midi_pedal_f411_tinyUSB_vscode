# MIDI Pedal STM32F411 + TinyUSB

USB MIDI/HID pedal based on STM32F411, implemented using TinyUSB library.

## 📋 Description

The device operates as a composite USB device, combining:
- **MIDI interface** - for sending MIDI messages (notes, CC)
- **HID keyboard** - for emulating key presses

Project features:
- 4 pedals with interrupt handling (EXTI0-EXTI3)
- Analog input (ADC) for expression pedal
- MIDI Note On/Off and Control Change transmission
- Keyboard key press emulation

## 🖼️ Photos

### Device
![Device](IMG_20251010_233841.jpg)

### Schematic
![Schematic](Снимок%20экрана%20от%202025-10-10%2018-33-08.png)

## 🔧 Technical Specifications

- **MCU**: STM32F411xE
- **USB**: Full Speed (12 Mbps)
- **Library**: TinyUSB
- **Compiler**: ARM GCC
- **Build System**: CMake

## 📁 Project Structure

```
├── Pedal_f411/          # Main application code
│   ├── pedal.cpp        # Main pedal logic
│   ├── power.cpp        # Power management
│   ├── board_api.c      # BSP for TinyUSB
│   └── usb_descriptors.c # USB descriptors
├── Core/                # STM32 HAL and initialization
├── Drivers/             # CMSIS and HAL drivers
├── tinyusb/             # TinyUSB library
└── cmake/               # CMake configuration
```

## 🚀 Build

```bash
# Configuration
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build

# Flash
# Use your favorite programmer (ST-Link, J-Link, etc.)
```

## 🎹 Functionality

### MIDI
- **Pedals 1-2**: Send MIDI Note On (notes 60, 61)
- **Expression pedal**: MIDI CC 64 (Sustain Pedal)
- Hi-Res MIDI (14-bit) support - see [`MIDI_HIRES_GUIDE.md`](MIDI_HIRES_GUIDE.md)

### HID Keyboard
- **Pedals 3-4**: Emulate "→" and "←" keys
- Details in [`HID_KEYBOARD_REFERENCE.md`](HID_KEYBOARD_REFERENCE.md)

## 📚 Documentation

- [`COMPOSITE_USB_README.md`](COMPOSITE_USB_README.md) - Composite USB device setup
- [`MIDI_HIRES_GUIDE.md`](MIDI_HIRES_GUIDE.md) - Hi-Res MIDI (14-bit) guide
- [`MIDI_HIRES_NOTE_VELOCITY.md`](MIDI_HIRES_NOTE_VELOCITY.md) - Hi-Res Note Velocity
- [`HID_KEYBOARD_REFERENCE.md`](HID_KEYBOARD_REFERENCE.md) - HID keyboard reference
- [`SCH_pedal_2025-10-10.pdf`](SCH_pedal_2025-10-10.pdf) - Device schematic

## 🔌 USB Descriptors

- **VID**: 0xCafe
- **PID**: 0x400C
- **Manufacturer**: SCHE
- **Product**: SCHE MIDI Pedal
- **Endpoints**:
  - EP0: Control
  - EP1 IN/OUT: MIDI (0x81/0x01)
  - EP2 IN: HID Keyboard (0x82)

## ⚙️ Configuration

Main settings in [`Pedal_f411/tusb_config.h`](Pedal_f411/tusb_config.h):
```c
#define CFG_TUD_MIDI 1
#define CFG_TUD_HID 1
#define CFG_TUD_HID_EP_BUFSIZE 16
```

## 📝 License

Project uses:
- STM32 HAL (BSD-3-Clause)
- TinyUSB (MIT License)
- CMSIS (Apache-2.0)

## 👤 Author

SCHE - 2025
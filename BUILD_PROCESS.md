# Процесс сборки проекта MIDI Pedal Firmware

Документ описывает этапы сборки проекта с использованием CMake для микроконтроллера STM32F411 с поддержкой TinyUSB и Rust.

## Общая архитектура сборки

Проект представляет собой гибридную C/C++/Rust прошивку для STM32F411 (ARM Cortex-M4F). CMake управляет сборкой следующих компонентов:

```
┌─────────────────────────────────────────────────────────────┐
│                    CMakeLists.txt (главный)                  │
├─────────────────────────────────────────────────────────────┤
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────────┐   │
│  │ gcc-arm-     │  │   rust.cmake │  │ tinyusb/src/     │   │
│  │ none-eabi    │  │              │  │ CMakeLists.txt   │   │
│  │ .cmake       │  │              │  │                  │   │
│  └──────────────┘  └──────────────┘  └──────────────────┘   │
├─────────────────────────────────────────────────────────────┤
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────────┐   │
│  │ STM32CubeMX  │  │ Rust (cargo) │  │ TinyUSB          │   │
│  │ drivers      │  │ logic_rust.rs│  │ библиотеки       │   │
│  └──────────────┘  └──────────────┘  └──────────────────┘   │
└─────────────────────────────────────────────────────────────┘
```

---

## Этап 1: Конфигурация CMake (Configure)

### 1.1 Подключение тулчейна

В [`CMakeLists.txt`](CMakeLists.txt:11) первым делом подключается тулчейн файл:

```cmake
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/gcc-arm-none-eabi.cmake)
```

Этот файл ([`cmake/gcc-arm-none-eabi.cmake`](cmake/gcc-arm-none-eabi.cmake:1)) настраивает кросс-компиляцию:

| Параметр | Значение |
|----------|----------|
| `CMAKE_SYSTEM_NAME` | Generic |
| `CMAKE_SYSTEM_PROCESSOR` | arm |
| Компилятор | `arm-none-eabi-gcc` |
| Линкер | `arm-none-eabi-g++` |
| CPU | cortex-m4 |
| FPU | fpv4-sp-d16 (hard float ABI) |

Флаги компиляции ([`cmake/gcc-arm-none-eabi.cmake`](cmake/gcc-arm-none-eabi.cmake:25)):
```cmake
-mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard
```

### 1.2 Подключение Rust

Файл [`cmake/rust.cmake`](cmake/rust.cmake:1) выполняет:

1. **Поиск Rust инструментов**: `rustc`, `cargo`, `rustup`
2. **Проверка целевой платформы**: требуется `thumbv7em-none-eabihf` (ARM Cortex-M4F с FPU)
3. **Проверка установки**: если target не установлен, выводится предупреждение

```cmake
# Проверка установленных целей
rustup target list --installed
```

### 1.3 Настройка стандартов языков

```cmake
set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 23)
```

### 1.4 Настройка оптимизации

CMakeLists.txt настраивает флаги оптимизации в зависимости от `CMAKE_BUILD_TYPE`:

| Build Type | C/C++ Flags | Linker Flags |
|------------|-------------|--------------|
| Debug | `-Og -g -fno-omit-frame-pointer` | (по умолчанию) |
| Release | `-Ofast -fno-stack-protector -ffunction-sections -fdata-sections` | `-flto --gc-sections` |
| MinSizeRel | `-Os -fno-stack-protector -ffunction-sections -fdata-sections` | `--gc-sections` |
| RelWithDebInfo | `-O2 -g -fno-stack-protector -ffunction-sections -fdata-sections` | `-flto --gc-sections` |

---

## Этап 2: Генерация промежуточных директорий

### 2.1 Создание директории сборки

```bash
mkdir -p build && cd build
cmake ..
```

CMake создаёт:
- `build/CMakeCache.txt` - кэш конфигурации
- `build/CMakeFiles/` - метаданные сборки
- `build/rules/` - правила генерации

---

## Этап 3: Сборка TinyUSB

### 3.1 Подключение TinyUSB

В [`CMakeLists.txt`](CMakeLists.txt:73):
```cmake
include("tinyusb/src/CMakeLists.txt")
```

### 3.2 Определение функций

Ф [`tinyusb/src/CMakeLists.txt`](tinyusb/src/CMakeLists.txt:7) определяется функция `tinyusb_target_add()`:

```cmake
function(tinyusb_target_add TARGET)
    target_sources(${TARGET} PRIVATE
        ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/tusb.c
        ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/common/tusb_fifo.c
        ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/device/usbd.c
        ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/device/usbd_control.c
        ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/class/midi/midi_device.c
        # ... другие классы
    )
```

Эта функция добавляет исходные файлы TinyUSB к основному исполняемому файлу.

---

## Этап 4: Создание исполняемого файла

### 4.1 Объявление проекта

```cmake
project(midi_pedal_f411_tinyUSB_vscode)
enable_language(C CXX ASM)
add_executable(${CMAKE_PROJECT_NAME})
```

Создаётся пустая цель исполняемого файла.

---

## Этап 5: Сборка Rust кода

### 5.1 Конфигурация Rust

В [`cmake/rust.cmake`](cmake/rust.cmake:44) функция `add_rust_file()`:

```cmake
function(add_rust_file FILE_PATH)
    # Определение профиля сборки
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        set(RUST_BUILD_FLAGS "--release")
        set(RUST_PROFILE_DIR "release")
    else()
        set(RUST_BUILD_FLAGS "")
        set(RUST_PROFILE_DIR "debug")
    endif()
    
    # Путь к библиотеке
    set(RUST_LIB ${CMAKE_SOURCE_DIR}/target/${RUST_TARGET}/${RUST_PROFILE_DIR}/liblogic_rust.a)
```

### 5.2 Сборка через Cargo

CMake настраивает команду сборки:

```cmake
add_custom_command(
    OUTPUT ${RUST_LIB}
    COMMAND ${CARGO_EXECUTABLE} build ${RUST_BUILD_FLAGS} --target ${RUST_TARGET}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    DEPENDS ${RUST_DEPS}
    COMMENT "Building Rust library..."
)
```

Rust компилируется с профилем из [`Cargo.toml`](Cargo.toml:1):

**Release профиль:**
```toml
[profile.release]
opt-level = 3
lto = true
codegen-units = 1
panic = "abort"
strip = true
```

**Debug профиль:**
```toml
[profile.dev]
opt-level = 0
debug = true
panic = "abort"
```

### 5.3 Связывание Rust с CMake

```cmake
# Создание imported библиотеки
add_library(rust_logic_lib STATIC IMPORTED GLOBAL)
set_target_properties(rust_logic_lib PROPERTIES
    IMPORTED_LOCATION ${RUST_LIB}
)

# Линковка
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE rust_logic_lib)
```

---

## Этап 6: Сборка STM32 HAL драйверов

### 6.1 Подключение CubeMX

```cmake
add_subdirectory(cmake/stm32cubemx)
```

### 6.2 Создание интерфейсной библиотеки

В [`cmake/stm32cubemx/CMakeLists.txt`](cmake/stm32cubemx/CMakeLists.txt:78):

```cmake
add_library(stm32cubemx INTERFACE)
target_include_directories(stm32cubemx INTERFACE ${MX_Include_Dirs})
target_compile_definitions(stm32cubemx INTERFACE ${MX_Defines_Syms})
```

Определения компилятора ([`cmake/stm32cubemx/CMakeLists.txt`](cmake/stm32cubemx/CMakeLists.txt:5)):
```cmake
set(MX_Defines_Syms 
    USE_HAL_DRIVER 
    STM32F411xE
    $<$<CONFIG:Debug>:DEBUG>
)
```

### 6.3 Сборка объектной библиотеки драйверов

```cmake
add_library(STM32_Drivers OBJECT)
target_sources(STM32_Drivers PRIVATE ${STM32_Drivers_Src})
target_link_libraries(STM32_Drivers PUBLIC stm32cubemx)
```

Исходные файлы HAL драйверов:
- `stm32f4xx_hal_adc.c`, `stm32f4xx_hal_adc_ex.c`, `stm32f4xx_ll_adc.c`
- `stm32f4xx_hal_rcc.c`, `stm32f4xx_hal_rcc_ex.c`
- `stm32f4xx_hal_gpio.c`
- `stm32f4xx_hal_tim.c`, `stm32f4xx_hal_tim_ex.c`
- `stm32f4xx_hal_pcd.c`, `stm32f4xx_hal_pcd_ex.c`
- `stm32f4xx_hal_pwr.c`, `stm32f4xx_hal_pwr_ex.c`
- `stm32f4xx_hal_flash.c`, `stm32f4xx_hal_flash_ex.c`
- `stm32f4xx_hal_exti.c`, `stm32f4xx_hal_cortex.c`
- `stm32f4xx_hal_rtc.c`, `stm32f4xx_hal_rtc_ex.c`
- `stm32f4xx_ll_usb.c`

---

## Этап 7: Добавление исходных файлов проекта

### 7.1 Приложение STM32CubeMX

```cmake
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    Core/Src/main.c
    Core/Src/gpio.c
    Core/Src/adc.c
    Core/Src/rtc.c
    Core/Src/tim.c
    Core/Src/usb_otg.c
    Core/Src/stm32f4xx_it.c
    Core/Src/stm32f4xx_hal_msp.c
    Core/Src/sysmem.c
    Core/Src/syscalls.c
    startup_stm32f411xe.s
)
```

### 7.2 Пользовательские файлы

```cmake
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    Pedal_f411/pedal.cpp
    Pedal_f411/power.cpp
    Pedal_f411/board_api.c
)
```

### 7.3 TinyUSB файлы

```cmake
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    Pedal_f411/usb_descriptors.c
    tinyusb/src/tusb.c
    tinyusb/src/class/midi/midi_device.c
    tinyusb/src/portable/synopsys/dwc2/dcd_dwc2.c
    tinyusb/src/portable/synopsys/dwc2/dwc2_common.c
)
```

---

## Этап 8: Настройка include директорий

```cmake
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Pedal_f411
    ${CMAKE_CURRENT_SOURCE_DIR}/tinyusb/src
    ${CMAKE_CURRENT_SOURCE_DIR}/tinyusb/hw/bsp
    # Из stm32cubemx:
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/STM32F4xx_HAL_Driver/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/STM32F4xx_HAL_Driver/Inc/Legacy
    ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/CMSIS/Device/ST/STM32F4xx/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/CMSIS/Include
)
```

---

## Этап 9: Определения компилятора

```cmake
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    CFG_TUSB_MCU=OPT_MCU_STM32F4
)
```

---

## Этап 10: Линковка

### 10.1 Линковка библиотек

```cmake
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
    stm32cubemx      # Интерфейсная библиотека
    STM32_Drivers    # Объектная библиотека HAL
    rust_logic_lib   # Скомпилированная Rust библиотека
)
```

### 10.2 Линковщик

Линковщик настраивается в [`cmake/gcc-arm-none-eabi.cmake`](cmake/gcc-arm-none-eabi.cmake:38):

```cmake
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -T \"${CMAKE_SOURCE_DIR}/STM32F411XX_FLASH.ld\"")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --specs=nano.specs")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-Map=${CMAKE_PROJECT_NAME}.map")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--print-memory-usage")
```

Параметры:
- `-T STM32F411XX_FLASH.ld` - скрипт линковщика (распределение памяти)
- `--specs=nano.specs` - использование newlib nano (уменьшенный размер)
- `-Wl,-Map=...map` - генерация MAP файла
- `-Wl,--gc-sections` - удаление неиспользуемых секций

---

## Этап 11: Генерация исполняемого файла

### 11.1 Компиляция

```bash
# Для каждого исходного файла:
arm-none-eabi-gcc -c -mcpu=cortex-m4 ... source.c -o object.o
arm-none-eabi-g++ -c -mcpu=cortex-m4 ... source.cpp -o object.o

# Для Rust:
cargo build --release --target thumbv7em-none-eabihf
```

### 11.2 Линковка

```bash
arm-none-eabi-g++ object1.o object2.o ... liblogic_rust.a -o firmware.elf
```

### 11.3 Конвертация в HEX/BIN

```bash
arm-none-eabi-objcopy firmware.elf -O ihex firmware.hex
arm-none-eabi-objcopy firmware.elf -O binary firmware.bin
```

---

## Порядок сборки

```
1. CMake Configure
   ├── Подключение gcc-arm-none-eabi.cmake
   ├── Подключение rust.cmake
   └── Проверка Rust

2. CMake Build
   ├── [PRIVATE] Сборка Rust библиотеки (liblogic_rust.a)
   │   └── cargo build --target thumbv7em-none-eabihf
   │
   ├── [STM32_Drivers] Компиляция HAL драйверов
   │   ├── stm32f4xx_hal_*.c → object.o
   │   └── system_stm32f4xx.c → object.o
   │
   ├── [midi_pedal_f411_tinyUSB_vscode] Компиляция
   │   ├── Core/Src/*.c → object.o
   │   ├── Pedal_f411/*.c,*.cpp → object.o
   │   ├── tinyusb/src/*.c → object.o
   │   └── startup_stm32f411xe.s → object.o
   │
   └── [midi_pedal_f411_tinyUSB_vscode] Линковка
       ├── object.o + liblogic_rust.a + STM32_Drivers
       ├── Скрипт STM32F411XX_FLASH.ld
       └── → firmware.elf
```

---

## Команды сборки

### Конфигурация
```bash
# Debug сборка
cmake -B build -DCMAKE_BUILD_TYPE=Debug

# Release сборка
cmake -B build -DCMAKE_BUILD_TYPE=Release
```

### Сборка
```bash
cmake --build build
```

### Очистка
```bash
cmake --build build --clean-first
# или
rm -rf build
```

---

## Требования

- **ARM Toolchain**: `arm-none-eabi-gcc`
- **Rust**: с установленным target `thumbv7em-none-eabihf`
  ```bash
  rustup target add thumbv7em-none-eabihf
  ```
- **CMake**: версия 3.28+

---

## Генерируемые файлы

| Файл | Описание |
|------|----------|
| `firmware.elf` | ELF исполняемый файл для отладки |
| `firmware.hex` | HEX файл для прошивки |
| `firmware.bin` | Бинарный образ для прошивки |
| `firmware.map` | Карта памяти после линковки |
| `compile_commands.json` | Для IDE (clangd и др.) |

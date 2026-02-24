# cmake/rust.cmake - CMake конфигурация для Rust
# Требует установленного Rust с поддержкой thumbv7em-none-eabihf
#
# Установка:
# rustup target add thumbv7em-none-eabihf

# Проверка наличия Rust
find_program(RUSTC_EXECUTABLE rustc)
find_program(CARGO_EXECUTABLE cargo)
find_program(RUSTUP_EXECUTABLE rustup)

if(NOT RUSTC_EXECUTABLE OR NOT CARGO_EXECUTABLE)
    message(WARNING "Rust not found. Rust code will not be compiled.")
    message(WARNING "Install Rust from: https://rustup.rs/")
    set(RUST_AVAILABLE FALSE)
else()
    set(RUST_AVAILABLE TRUE)

    # Целевая платформа для STM32F4 (ARM Cortex-M4F с FPU)
    if(NOT DEFINED RUST_TARGET)
        set(RUST_TARGET "thumbv7em-none-eabihf")
    endif()

    # Проверка установленных целей через rustup
    if(RUSTUP_EXECUTABLE)
        execute_process(
            COMMAND ${RUSTUP_EXECUTABLE} target list --installed
            OUTPUT_VARIABLE INSTALLED_TARGETS
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )
        if(NOT INSTALLED_TARGETS MATCHES "${RUST_TARGET}")
            message(STATUS "Rust target ${RUST_TARGET} is not installed.")
            message(STATUS "Run: rustup target add ${RUST_TARGET}")
        endif()
    endif()

    message(STATUS "Rust found: ${RUSTC_EXECUTABLE}")
    message(STATUS "Rust target: ${RUST_TARGET}")
endif()

# Функция для сборки Rust файла через add_custom_command
# Обеспечивает инкрементальную пересборку при изменении .rs файлов
function(add_rust_file FILE_PATH)
    if(NOT RUST_AVAILABLE)
        message(STATUS "Rust not available, skipping: ${FILE_PATH}")
        return()
    endif()

    if(NOT EXISTS ${CMAKE_SOURCE_DIR}/Cargo.toml)
        message(FATAL_ERROR "Cargo.toml not found in ${CMAKE_SOURCE_DIR}")
    endif()

    # Определяем флаги и путь к выходной библиотеке
    if(CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "MinSizeRel" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
        set(RUST_BUILD_FLAGS "--release")
        set(RUST_PROFILE_DIR "release")
    else()
        set(RUST_BUILD_FLAGS "")
        set(RUST_PROFILE_DIR "debug")
    endif()

    set(RUST_LIB ${CMAKE_SOURCE_DIR}/target/${RUST_TARGET}/${RUST_PROFILE_DIR}/liblogic_rust.a)

    # Собираем список зависимостей: все .rs файлы + Cargo.toml + .cargo/config.toml
    file(GLOB_RECURSE RUST_SOURCES "${CMAKE_SOURCE_DIR}/Pedal_f411/*.rs")
    set(RUST_DEPS
        ${RUST_SOURCES}
        ${CMAKE_SOURCE_DIR}/Cargo.toml
        ${CMAKE_SOURCE_DIR}/.cargo/config.toml
    )

    message(STATUS "Rust library output: ${RUST_LIB}")
    message(STATUS "Rust sources: ${RUST_SOURCES}")

    # Команда сборки — запускается при cmake --build если зависимости изменились
    add_custom_command(
        OUTPUT ${RUST_LIB}
        COMMAND ${CARGO_EXECUTABLE} build ${RUST_BUILD_FLAGS} --target ${RUST_TARGET}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        DEPENDS ${RUST_DEPS}
        COMMENT "Building Rust library (${RUST_PROFILE_DIR})..."
        VERBATIM
    )

    # Цель, которая зависит от выходного файла
    add_custom_target(rust_lib ALL
        DEPENDS ${RUST_LIB}
    )

    # Основной таргет зависит от rust_lib
    add_dependencies(${CMAKE_PROJECT_NAME} rust_lib)

    # Создаём IMPORTED таргет для Rust библиотеки
    # Это позволяет CMake не требовать существования файла на этапе конфигурации
    add_library(rust_logic_lib STATIC IMPORTED GLOBAL)
    set_target_properties(rust_logic_lib PROPERTIES
        IMPORTED_LOCATION ${RUST_LIB}
    )
    # Таргет зависит от сборки Rust
    add_dependencies(rust_logic_lib rust_lib)

    # Линкуем через IMPORTED таргет с явным ключевым словом PRIVATE
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE rust_logic_lib)

    message(STATUS "Rust integration configured. Library: ${RUST_LIB}")
endfunction()

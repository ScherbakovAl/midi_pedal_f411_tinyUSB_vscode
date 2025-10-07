/*
 * board_api.c
 * Простая реализация board API для STM32F411
 */

#include "board_api.h"
#include "stm32f4xx_hal.h"
#include <string.h>
#include <stdio.h>

// Инициализация USB GPIO и тактирования для TinyUSB
void board_init_usb(void) {
    // Включаем тактирование GPIOA
    __HAL_RCC_GPIOA_CLK_ENABLE();
    
    // Настраиваем PA11 и PA12 как альтернативные функции для USB
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_11 | GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Alternate = GPIO_AF10_OTG_FS;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    // Включаем тактирование USB OTG FS
    __HAL_RCC_USB_OTG_FS_CLK_ENABLE();
    
    // Настраиваем прерывание USB с приоритетом
    HAL_NVIC_SetPriority(OTG_FS_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(OTG_FS_IRQn);
}

// Получить серийный номер устройства из уникального ID STM32
size_t board_usb_get_serial(uint16_t* serial_str, size_t max_chars) {
    // STM32F4 имеет 96-битный уникальный ID по адресу 0x1FFF7A10
    uint32_t* uid = (uint32_t*)UID_BASE;

    // Формируем серийный номер из уникального ID
    char serial_temp[32];
    // snprintf(serial_temp, sizeof(serial_temp), "%08lX%08lX%08lX", uid[0], uid[1], uid[2]);

    size_t len = strlen(serial_temp);
    if (len > max_chars) {
        len = max_chars;
    }

    // Конвертируем в UTF-16
    for (size_t i = 0; i < len; i++) {
        serial_str[i] = serial_temp[i];
    }

    return len;
}
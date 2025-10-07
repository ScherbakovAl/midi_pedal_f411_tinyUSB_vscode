/*
 * board_api.c
 * Простая реализация board API для STM32F411
 */

#include "board_api.h"
#include "stm32f4xx.h"
#include <string.h>
#include <stdio.h>

// Получить серийный номер устройства из уникального ID STM32
size_t board_usb_get_serial(uint16_t* serial_str, size_t max_chars) {
    // STM32F4 имеет 96-битный уникальный ID по адресу 0x1FFF7A10
    uint32_t* uid = (uint32_t*)UID_BASE;
    
    // Формируем серийный номер из уникального ID
    char serial_temp[32];
    snprintf(serial_temp, sizeof(serial_temp), "%08lX%08lX%08lX", 
             uid[0], uid[1], uid[2]);
    
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
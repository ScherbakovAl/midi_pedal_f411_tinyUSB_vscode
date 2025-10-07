/*
 * board_api.h
 * Заголовочный файл для board API
 */

#ifndef BOARD_API_H_
#define BOARD_API_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

// Получить серийный номер устройства
size_t board_usb_get_serial(uint16_t* serial_str, size_t max_chars);

#ifdef __cplusplus
}
#endif

#endif /* BOARD_API_H_ */
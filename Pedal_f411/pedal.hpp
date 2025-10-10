#pragma once

#include "main.h"
#include "adc.h"
#include "tim.h"
#include "stm32f4xx_hal.h"
#include "tusb_config.h"
#include "tusb.h"
#include "board_api.h"
#include "power.h"

#ifdef __cplusplus
extern "C" {
#endif

    void pedal();
    void MidiSender2(const uint8_t n, const uint8_t uu);
    void KeySender(const uint8_t k);

#ifdef __cplusplus
}
#endif
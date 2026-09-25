/*
 * This file is part of Betaflight.
 *
 * Betaflight is free software. You can redistribute this software and/or
 * modify it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 */

#pragma once

#ifndef TARGET_BOARD_IDENTIFIER
#define TARGET_BOARD_IDENTIFIER "2040"
#endif

#ifndef USBD_PRODUCT_STRING
#define USBD_PRODUCT_STRING "Betaflight - RP2040"
#endif

// Betaflight App currently recognises Raspberry Pi Pico VCP 2E8A:0009.  The
// Pico SDK's RP2040-specific 000A PID enumerates on the host but is filtered
// out by both the web and desktop device pickers.
#ifndef USBD_PID
#define USBD_PID 0x0009
#endif

#define UARTHARDWARE_MAX_PINS 8
#define MAX_SPI_PIN_SEL 4

#include "../common/target_RP2040.h"

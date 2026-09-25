/*
 * This file is part of Betaflight.
 *
 * Betaflight is free software. You can redistribute this software and/or
 * modify it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 */

#pragma once

// Keep the first RP2040 bring-up single-core.  Core 1 remains available for a
// later performance experiment once flash writes and interrupt ownership have
// been qualified.

#define USE_UART0
#define USE_UART1
#define UART_TRAIT_AF_PORT 1

#define USE_SPI
#define SPIDEV_COUNT 2
#define USE_SPI_DEVICE_0
#define USE_SPI_DEVICE_1
#define USE_SPI_DMA_ENABLE_LATE

#define USE_I2C
#define I2CDEV_COUNT 2
#define USE_I2C_DEVICE_0
#define USE_I2C_DEVICE_1

#define USE_ADC
#define USE_VCP

#define USE_SERIALRX_CRSF

#undef USE_SOFTSERIAL1
#undef USE_SOFTSERIAL2
#undef USE_TRANSPONDER
#undef USE_TIMER
#undef USE_RCC

#define CONFIG_IN_FLASH

#define FLASH_CONFIG_STREAMER_BUFFER_SIZE FLASH_PAGE_SIZE
#define FLASH_CONFIG_BUFFER_TYPE uint8_t

#undef USE_DMA_SPEC
#undef USE_DSHOT_BITBANG

// RP2040 has two PIO blocks.  Keep DShot on PIO0 and reserve PIO1 for future
// expansion; this target intentionally has no framebuffer OSD or LED strip.
#define PIO_DSHOT_INDEX 0
#define PIO_UART_INDEX 1
#define PIO_LEDSTRIP_INDEX 1
#define PIO_OSD_INDEX 1

#ifdef REMOVE_MSP_DISPLAYPORT
#undef USE_MSP_DISPLAYPORT
#endif

#undef USE_RX_PWM
#undef USE_RX_PPM
#undef USE_RX_CC2500
#undef USE_SERIALRX_GHST
#undef USE_SERIALRX_IBUS
#undef USE_SERIALRX_JETIEXBUS
#undef USE_SERIALRX_SPEKTRUM
#undef USE_SERIALRX_SUMD
#undef USE_SERIALRX_SUMH
#undef USE_SERIALRX_XBUS
#undef USE_SERIALRX_FPORT

#undef USE_TELEMETRY_GHST
#undef USE_TELEMETRY_FRSKY_HUB
#undef USE_TELEMETRY_HOTT
#undef USE_TELEMETRY_IBUS
#undef USE_TELEMETRY_IBUS_EXTENDED
#undef USE_TELEMETRY_JETIEXBUS
#undef USE_TELEMETRY_LTM
#undef USE_TELEMETRY_MAVLINK
#undef USE_TELEMETRY_SMARTPORT
#undef USE_TELEMETRY_SRXL

// The generic 4-way implementation bit-bangs the motor GPIOs and does not
// depend on an STM32 timer peripheral.  Keep both bootloader transports so
// BLHeli/AM32 configurators can use MSP ESC passthrough.
#undef USE_MULTI_GYRO
#undef USE_RANGEFINDER_HCSR04
#undef USE_VTX_RTC6705
#undef USE_VTX_RTC6705_SOFTSPI
#undef USE_SRXL
#undef USE_SPEKTRUM
#undef USE_SPEKTRUM_BIND
#undef USE_SERIAL_PASSTHROUGH
#undef USE_RPM_LIMIT

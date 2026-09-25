/*
 * This file is part of Betaflight.
 *
 * Betaflight is free software. You can redistribute this software
 * and/or modify this software under the terms of the GNU General
 * Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later
 * version.
 *
 * Betaflight is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this software.
 *
 * If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdint.h>
#include <string.h>

#include "platform.h"

#include "common/maths.h"
#include "common/time.h"
#include "drivers/system.h"
#include "drivers/time.h"

#include "drivers/io.h"
#include "drivers/light_led.h"
#include "drivers/sound_beeper.h"

#include "platform/multicore.h"

#include "hardware/clocks.h"
#include "hardware/timer.h"
#include "hardware/watchdog.h"
#include "pico/bootrom.h"
#include "pico/unique_id.h"
// flash.h used by PICO QSPI helpers is included where needed in PICO bus/flash code

///////////////////////////////////////////////////

// SystemInit and SystemCoreClock variables/functions,
// as per the pico-sdk CMSIS device system source

uint32_t SystemCoreClock; /* System Clock Frequency (Core Clock)*/

void SystemCoreClockUpdate (void)
{
    SystemCoreClock = clock_get_hz(clk_sys);
}

void __attribute__((constructor)) SystemInit (void)
{
    SystemCoreClockUpdate();
}

////////////////////////////////////////////////////

void systemReset(void)
{
    bprintf("*** PICO systemReset ***");
    //TODO: check

#if 1
#ifdef USE_MULTICORE
    // Reset core 1
    multicore_reset_core1();
#endif
    watchdog_reboot(0, 0, 0);
#else
    // this might be fine
    __disable_irq();
    NVIC_SystemReset();
#endif
}

uint32_t systemUniqueId[3] = { 0 };

// cycles per microsecond
static uint32_t usTicks = 0;
static float usTicksInv = 0.0f;

#ifdef RP2350
// These are defined in pico-sdk headers as volatile uint32_t types.
#define PICO_DWT_CTRL   m33_hw->dwt_ctrl
#define PICO_DWT_CYCCNT m33_hw->dwt_cyccnt
#define PICO_DEMCR      m33_hw->demcr
#else
// Cortex-M0+ has no DWT cycle counter.  Extend its 24-bit SysTick counter in
// software; at 125 MHz this adds only about eight interrupts per second.
#define RP2040_SYSTICK_PERIOD (1u << 24)
static volatile uint32_t rp2040CycleBase;

void isr_systick(void)
{
    rp2040CycleBase += RP2040_SYSTICK_PERIOD;
}
#endif

void cycleCounterInit(void)
{
    // TODO check clock_get_hz(clk_sys) is the clock for CPU cycles
    usTicks = SystemCoreClock / 1000000;
    usTicksInv = 1e6f / SystemCoreClock;

#ifdef RP2350
    // Global DWT enable
    PICO_DEMCR |= M33_DEMCR_TRCENA_BITS;

    // Reset and enable cycle counter
    PICO_DWT_CYCCNT = 0;
    PICO_DWT_CTRL |= M33_DWT_CTRL_CYCCNTENA_BITS;
#else
    rp2040CycleBase = 0;
    SysTick->CTRL = 0;
    SysTick->LOAD = RP2040_SYSTICK_PERIOD - 1;
    SysTick->VAL = 0;
    NVIC_SetPriority(SysTick_IRQn, 0);
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;

    // VAL reloads on the first processor-clock edge after enabling.  Waiting
    // here prevents the public counter from appearing to jump backwards once.
    while (SysTick->VAL == 0) {
        __NOP();
    }
#endif
}

void systemInit(void)
{
    //TODO: implement

    SystemInit();

    cycleCounterInit();

    // load the unique id into a local array
    pico_unique_board_id_t id;
    pico_get_unique_board_id(&id);
    memcpy(&systemUniqueId, &id.id, MIN(sizeof(systemUniqueId), (uint32_t)PICO_UNIQUE_BOARD_ID_SIZE_BYTES));


#ifdef USE_MULTICORE
    multicoreStart();
#endif // USE_MULTICORE
}

void systemResetToBootloader(bootloaderRequestType_e requestType)
{
    switch (requestType) {
    case BOOTLOADER_REQUEST_ROM:
        rom_reset_usb_boot_extra(-1, 0, false);
        break;
    case BOOTLOADER_REQUEST_FLASH:
    default:
        systemReset();
    }
}

// We can make use of time_us_64 if BF defines USE_64BIT_TIME in future, but that will require some changes
STATIC_ASSERT(sizeof(timeMs_t) == sizeof(uint32_t), timeMs_t_is_32_bit_failed);
STATIC_ASSERT(sizeof(timeUs_t) == sizeof(uint32_t), timeUs_t_is_32_bit_failed);

// Return system uptime in milliseconds (rollover in 49 days)
timeMs_t millis(void)
{
    return (timeMs_t)(time_us_64() / 1000);
}

// Return system uptime in micros (rollover in 71 mins)
timeUs_t micros(void)
{
    return time_us_32();
}

timeUs_t microsISR(void)
{
    return micros();
}

void delayMicroseconds(uint32_t us)
{
    sleep_us(us);
}

void delay(uint32_t ms)
{
    sleep_ms(ms);
}

uint32_t getCycleCounter(void)
{
#ifdef RP2350
    return PICO_DWT_CYCCNT;
#else
    uint32_t baseBefore;
    uint32_t baseAfter;
    uint32_t current;
    uint32_t pendingBefore;
    uint32_t pendingAfter;

    // Make the read coherent with both a SysTick rollover and its ISR.  If the
    // exception is pending (for example inside a critical section), account
    // for the rollover that the ISR has not added yet.
    do {
        baseBefore = rp2040CycleBase;
        pendingBefore = SCB->ICSR & SCB_ICSR_PENDSTSET_Msk;
        current = SysTick->VAL;
        pendingAfter = SCB->ICSR & SCB_ICSR_PENDSTSET_Msk;
        baseAfter = rp2040CycleBase;
    } while (baseBefore != baseAfter || pendingBefore != pendingAfter);

    if (pendingAfter) {
        baseBefore += RP2040_SYSTICK_PERIOD;
    }

    return baseBefore + (RP2040_SYSTICK_PERIOD - 1u - current);
#endif
}

// Conversion routines copied from platform/common/stm32/system.c
int32_t clockCyclesToMicros(int32_t clockCycles)
{
    return clockCycles / usTicks;
}

float clockCyclesToMicrosf(int32_t clockCycles)
{
    return clockCycles * usTicksInv;
}

// Note that this conversion is signed as this is used for periods rather than absolute timestamps
int32_t clockCyclesTo10thMicros(int32_t clockCycles)
{
    return 10 * clockCycles / (int32_t)usTicks;
}

// Note that this conversion is signed as this is used for periods rather than absolute timestamps
int32_t clockCyclesTo100thMicros(int32_t clockCycles)
{
    return 100 * clockCycles / (int32_t)usTicks;
}

uint32_t clockMicrosToCycles(uint32_t micros)
{
    return micros * usTicks;
}

static void indicate(uint8_t count, uint16_t duration)
{
    if (count) {
        LED1_ON;
        LED0_OFF;

        while (count--) {
            LED1_TOGGLE;
            LED0_TOGGLE;
            BEEP_ON;
            delay(duration);

            LED1_TOGGLE;
            LED0_TOGGLE;
            BEEP_OFF;
            delay(duration);
        }
    }
}

void indicateFailure(failureMode_e mode, int codeRepeatsRemaining)
{
    while (codeRepeatsRemaining--) {
        indicate(WARNING_FLASH_COUNT, WARNING_FLASH_DURATION_MS);

        delay(WARNING_PAUSE_DURATION_MS);

        indicate(mode + 1, WARNING_CODE_DURATION_LONG_MS);

        delay(1000);
    }
}

void failureMode(failureMode_e mode)
{
    indicateFailure(mode, 10);

#ifdef DEBUG
    systemReset();
#else
    systemResetToBootloader(BOOTLOADER_REQUEST_ROM);
#endif
}

static void unusedPinInit(IO_t io)
{
    if (IOGetOwner(io) == OWNER_FREE) {
        IOConfigGPIO(io, 0);
    }
}

void unusedPinsInit(void)
{
    IOTraversePins(unusedPinInit);
}

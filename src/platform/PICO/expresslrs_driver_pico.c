/*
 * This file is part of Betaflight.
 *
 * Betaflight is free software. You can redistribute this software and/or
 * modify it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 */

#include <stdbool.h>
#include <stdint.h>

#include "platform.h"

#ifdef USE_RX_EXPRESSLRS

#include "build/debug.h"

#include "common/maths.h"

#include "drivers/timer.h"
#include "drivers/rx/expresslrs_driver.h"

#include "hardware/timer.h"
#include "pico/time.h"

#define ELRS_TIMER_INTERVAL_US_DEFAULT 20000u
#define ELRS_TICK_TOCK_COUNT 2u

// Pico's default alarm pool uses alarm 3.  Reserve alarm 2 for the receiver so
// phase lock does not share scheduling state with sleep/time APIs.
#ifndef PICO_EXPRESSLRS_ALARM_NUM
#define PICO_EXPRESSLRS_ALARM_NUM 2u
#endif

typedef enum {
    ELRS_TIMER_TICK,
    ELRS_TIMER_TOCK,
} elrsTimerTickTock_e;

typedef struct {
    bool initialised;
    volatile bool running;
    volatile elrsTimerTickTock_e tickTock;
    volatile uint32_t intervalUs;
    volatile int32_t frequencyOffsetUs;
    volatile int32_t phaseShiftUs;
    volatile uint64_t nextAlarmAtUs;
} elrsPicoTimerState_t;

static elrsPicoTimerState_t timerState = {
    .tickTock = ELRS_TIMER_TOCK,
    .intervalUs = ELRS_TIMER_INTERVAL_US_DEFAULT,
};

static int32_t phaseShiftMin;
static int32_t phaseShiftMax;

static void expressLrsPicoScheduleNext(uint32_t periodUs)
{
    timerState.nextAlarmAtUs += periodUs;

    const uint64_t now = time_us_64();
    if (timerState.nextAlarmAtUs <= now) {
        // Do not let one delayed interrupt create a burst of catch-up IRQs.
        timerState.nextAlarmAtUs = now + 10u;
    }

    // A true return means the target elapsed while it was being programmed.
    // Retry with a small margin so a missed deadline cannot stall the receiver.
    while (hardware_alarm_set_target(PICO_EXPRESSLRS_ALARM_NUM, from_us_since_boot(timerState.nextAlarmAtUs))) {
        timerState.nextAlarmAtUs = time_us_64() + 10u;
    }
}

static void expressLrsPicoAlarmCallback(uint alarmNum)
{
    if (alarmNum != PICO_EXPRESSLRS_ALARM_NUM || !timerState.running) {
        return;
    }

    int32_t adjustedPeriod;
    if (timerState.tickTock == ELRS_TIMER_TICK) {
        adjustedPeriod = (int32_t)(timerState.intervalUs / ELRS_TICK_TOCK_COUNT) + timerState.frequencyOffsetUs;
        expressLrsOnTimerTickISR();
        timerState.tickTock = ELRS_TIMER_TOCK;
    } else {
        adjustedPeriod = (int32_t)(timerState.intervalUs / ELRS_TICK_TOCK_COUNT) + timerState.phaseShiftUs + timerState.frequencyOffsetUs;
        timerState.phaseShiftUs = 0;
        expressLrsOnTimerTockISR();
        timerState.tickTock = ELRS_TIMER_TICK;
    }

    expressLrsPicoScheduleNext(MAX(adjustedPeriod, 1));
}

void expressLrsTimerDebug(void)
{
    DEBUG_SET(DEBUG_RX_EXPRESSLRS_PHASELOCK, 2, timerState.frequencyOffsetUs);
    DEBUG_SET(DEBUG_RX_EXPRESSLRS_PHASELOCK, 3, timerState.phaseShiftUs);
}

void expressLrsUpdateTimerInterval(uint16_t intervalUs)
{
    timerState.intervalUs = intervalUs;
    phaseShiftMax = intervalUs / ELRS_TICK_TOCK_COUNT;
    phaseShiftMin = -phaseShiftMax;
}

void expressLrsUpdatePhaseShift(int32_t newPhaseShift)
{
    timerState.phaseShiftUs = constrain(newPhaseShift, phaseShiftMin, phaseShiftMax);
}

void expressLrsTimerIncreaseFrequencyOffset(void)
{
    timerState.frequencyOffsetUs++;
}

void expressLrsTimerDecreaseFrequencyOffset(void)
{
    timerState.frequencyOffsetUs--;
}

void expressLrsTimerResetFrequencyOffset(void)
{
    timerState.frequencyOffsetUs = 0;
}

bool expressLrsTimerIsRunning(void)
{
    return timerState.running;
}

void expressLrsTimerStop(void)
{
    timerState.running = false;
    hardware_alarm_cancel(PICO_EXPRESSLRS_ALARM_NUM);
}

void expressLrsTimerResume(void)
{
    timerState.tickTock = ELRS_TIMER_TOCK;
    timerState.running = true;
    timerState.nextAlarmAtUs = time_us_64();
    expressLrsPicoScheduleNext(timerState.intervalUs / ELRS_TICK_TOCK_COUNT);
}

void expressLrsInitialiseTimer(const timerHardware_t *timHw, timerOvrHandlerRec_t *timerUpdateCb)
{
    UNUSED(timHw);
    UNUSED(timerUpdateCb);

    if (!timerState.initialised) {
        hardware_alarm_claim(PICO_EXPRESSLRS_ALARM_NUM);
        timerState.initialised = true;
    }

    expressLrsUpdateTimerInterval(ELRS_TIMER_INTERVAL_US_DEFAULT);
}

void expressLrsTimerEnableIRQs(const timerHardware_t *timer)
{
    UNUSED(timer);
    hardware_alarm_set_callback(PICO_EXPRESSLRS_ALARM_NUM, expressLrsPicoAlarmCallback);
}

#endif

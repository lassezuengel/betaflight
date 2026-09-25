# Experimental RP2040 support

The `RP2040FPV` configuration is the first board using this platform target. It
is intended for bring-up and evaluation, not flight testing yet.

Build it from the Betaflight repository root:

```sh
make -j4 CCACHE= fwo CONFIG=RP2040FPV
```

The resulting UF2 can be copied to the RP2040 ROM BOOTSEL drive. Remove all
propellers before flashing or testing motor outputs.

The initial port deliberately uses conservative settings: one RP2040 core,
125 MHz system clock, execute-in-place flash, a 1.6 kHz PID loop, DShot300, and
an ELRS 250 Hz default. Gyro alignment, motor order/direction, LED polarity, and
VBAT scaling must be verified on the physical board before flight.

Suggested bring-up order:

1. Confirm USB CDC and CLI operation with `version`, `status`, and `tasks`.
2. Confirm the BMI270 is detected and verify every axis by moving the board.
3. Test save/reboot and configuration persistence.
4. Bind the SX1280 receiver and inspect link statistics at a low packet rate.
5. Verify every motor output with props removed.

For an Acro-only configuration, setting `acc_hardware = NONE` leaves the
BMI270 gyro running but disables Betaflight's accelerometer and attitude tasks.
The BMI270 driver also powers down its accelerometer block. The Setup tab's 3D
model will stop updating in this mode because it displays estimated attitude,
not raw gyro activity; use the Sensors tab or `status`/`tasks` to verify gyro
operation.

## Integrated ExpressLRS receiver

The integrated SX1280 receiver does not use ExpressLRS's three-power-cycle
binding gesture. With an empty UID it enters binding mode automatically. To
re-enter binding mode later, use the CLI command `bind_rx` or press the
active-low user/bind button on GPIO4. The blue LED blinks rapidly in binding
mode and stays on while valid receiver packets are arriving.

This build uses the ExpressLRS v3 over-the-air format by default. The radio
transmitter module must use the same major OTA version and a 2.4 GHz regulatory
domain compatible with `expresslrs_domain` (`ISM2400` by default).

## ESC passthrough

Motor outputs use RP2040 GPIO12 through GPIO15 (`PA12` through `PA15` in
Betaflight resource notation). Serial 4-way MSP passthrough is enabled for
BLHeli and AM32 tools. During passthrough, the motor pins are temporarily moved
from PIO DShot to software-controlled GPIO and are restored when it exits.

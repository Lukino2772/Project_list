# 74HC 14-pin logic tester

STM32F401RCT6 firmware for the fixed-power 14-pin socket in
`Schematic_Of_The_Device.asc`. Pin 14 is 3.3 V through 47 ohms; pin 7 is ground.
Each signal requires its own 1 kohm series resistor. PB0 senses socket VCC.
The resistor's upstream supply must equal MCU VDDA for the current estimate
in this firmware to be meaningful.

## Operation

- Short StartKey press: identify/test a chip.
- Hold 2–5 seconds, then release: zero the current estimate with an **empty** socket.
- Hold at least 5 seconds, then release: cycle the software threshold (3/5/8/10 mA).
- `I` and `P` on a PASS screen are idle and peak sampled current in mA. Peak
  includes the matching device test and final idle check, not rejected candidates.
  These are estimates, not precision current measurements or transient capture.
- A failed zero calibration preserves the last successful zero. Settings and zero
  are RAM-only and reset on restart.

The firmware sweeps every gate's truth table while holding the other gates at
both output polarities, checking all outputs with both weak pull directions.
For 125/126 buffers it also disables one gate at a time and checks that the
other gates remain enabled. This detects some cross-gate and enable coupling;
it is not exhaustive coverage of every possible multi-pin fault.

The table supports 20 logic/pinout entries. DC truth tables cannot distinguish
00 from 132, 04 from 14, or otherwise prove the exact logic family or speed grade.
Only use known-compatible 3.3 V CMOS parts with this power pinout. This is not a
16-pin, sequential-logic, or universal 74-series tester.

## Hardware limits

There is **no software-controlled DUT power switch** in this schematic. A fault
stops stimulus and returns signal pins to input/pull-down, but VCC stays on.
Power off before inserting/removing a chip, including after a fault. The 47 ohm
resistor and sampled software threshold do not provide an active current cutoff.
Wrong candidates can also produce contention, so a threshold trip is not proof
that the inserted IC is defective.

Open-drain stimulus uses weak MCU pull-ups. Rising-edge speed depends on fixture
capacitance; long wiring can violate ordinary HC input transition requirements.
A DC PASS does not establish timing compliance or Schmitt-trigger behavior.

## Regression checks

Run `python3 tests/run_tests.py` with host GCC available. It compiles the driver
with `-O2 -Wall -Wextra -Werror -Wformat-truncation=2` against a mock HAL and checks
20 ideal device models, stuck/open outputs, selected cross-gate and OE faults,
empty socket, supply/ADC faults, calibration retention and LCD/current reporting.
The models reuse the pinout table, so they do not independently validate physical
pinouts. Tests are outside CubeIDE's configured firmware source directories.

CubeMX configuration and generated setup are unchanged. Build/flash with
STM32CubeIDE and confirm behavior on the actual fixture; host tests do not replace
an ARM firmware build or hardware validation.

Reference checks: [STM32F401xB/xC datasheet](https://www.st.com/resource/en/datasheet/stm32f401vc.pdf)
(VREFINT calibration and ADC timing),
[74HC125](https://assets.nexperia.com/documents/data-sheet/74HC_HCT125.pdf) and
[74HC126](https://assets.nexperia.com/documents/data-sheet/74HC_HCT126.pdf)
(enable polarity and high-impedance operation).

# 74HC logic tester

The STM32F401RCT6 checks static logic on compatible 3.3 V, 14-pin parts with
VCC on pin 14 and ground on pin 7. Insert the chip with the notch aligned and
press Start once. The LCD shows when the test is running, the matching part
or reason to recheck, and when the socket supply is off. Parts with identical
logic and pinouts share a result, such as 74HC00/132 and 74HC04/14.

Connect the socket per the schematic: PB0 goes to ZIF1 and PB1 to ZIF13.
PB13 drives the 2N3906 base through R7. The 10 kΩ pull-up keeps the active-low
switch off at reset. Keep all 1 kΩ signal resistors and the 47 Ω supply resistor.
The firmware switches power on during a test and off before displaying its
result. The 47 Ω resistor limits current. The fixture has no voltage or
current feedback, so it cannot detect or report an overload directly.

## PC app (Windows)

1. Build and flash the CubeIDE project. Plug the STM32 USB data port into the PC.
2. From the project directory run `py -m pip install -r pc/requirements.txt`.
3. Run `py pc/tester.py`. Select the tester's COM port, click **Connect**, and
   click **Run Test** or use the board's button. The board can also work by itself.

The USB CDC protocol accepts newline-terminated `STATUS` and `RUN`.
Responses have the form `HC1,state,part,count` followed by a newline.
Only one test runs at a time. Query STATUS for the latest result if a
notification was missed. The USB clock configuration assumes a 25 MHz HSE.

## Regression checks

Run `python3 tests/run_tests.py` with host GCC. The fixture mock uses the
firmware's device table and does not independently verify physical pinouts.

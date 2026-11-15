# 🔌 Embedded Linux Device Drivers & HAL

![C11](https://img.shields.io/badge/C-11-blue) ![Embedded Linux](https://img.shields.io/badge/Embedded-Linux-orange) ![ARM](https://img.shields.io/badge/ARM-Cortex--M%2F64-red) ![Build](https://img.shields.io/badge/build-passing-success)

Low-level Linux device-driver and hardware-abstraction (HAL) examples for **GPIO, I2C, SPI and UART** on ARM platforms — with a simulated sensor register file, signed temperature decoding, and full bus-transaction logging.

*Personal project based on professional embedded Linux / board bring-up experience — no employer code.*

## ✨ Highlights
- I2C sensor demo: WHO_AM_I `0x48` detected, temperature decoded at **25.50 °C** (and −10.00 °C negative case)
- Every GPIO / I2C / SPI / UART transaction is logged, so the output reads like a real bring-up session
- Final self-check prints **PASS**

## Code, Build & Sample Output

Real, runnable code — not just a description. `src/hal.h` / `src/hal.c` implement a
simulated-register HAL in C11 that mirrors a real ARM embedded-Linux driver layer:
GPIO write/read/toggle, an I2C write/read transaction to a simulated sensor
register file (7-bit address `0x48`, `WHO_AM_I` register `0x00`, temperature
registers `0x01`/`0x02` in TMP102-style 12-bit encoding), an SPI transfer, and a
UART transmit — with every bus transaction logged. `src/main.c` is a demo on top
of it. It builds and runs on any host with gcc; no hardware is needed.

### Build & run

```bash
make            # builds build/hal_demo with gcc -std=c11 -Wall -Wextra
make run        # builds and runs the demo
# or directly:
gcc -std=c11 -Wall -Wextra -Isrc -o build/hal_demo src/main.c src/hal.c
./build/hal_demo
```

### Sample output (actual captured run)

```text
$ gcc -std=c11 -Wall -Wextra -o build/hal_demo src/main.c src/hal.c
(compiled with no errors or warnings)

$ ./build/hal_demo
=== Embedded Linux HAL demo (simulated registers) ===
[HAL] init: GPIO/I2C/SPI/UART simulated peripherals ready
[I2C]  read  addr=0x48 reg=0x00 len=1 data=[0x48]
Sensor WHO_AM_I = 0x48 (expected 0x48) -> OK, sensor detected
[I2C]  read  addr=0x48 reg=0x01 len=2 data=[0x19 0x80]
Temperature = 25.50 C
[SIM]  sensor temperature registers set to MSB=0xF6 LSB=0x00
[I2C]  read  addr=0x48 reg=0x01 len=2 data=[0xF6 0x00]
Temperature = -10.00 C
[GPIO] write pin=5 level=HIGH (latch=0x0020)
[GPIO] write pin=7 level=LOW (latch=0x0020)
[GPIO] read  pin=5 level=HIGH
[GPIO] read  pin=5 level=HIGH
[GPIO] write pin=5 level=LOW (latch=0x0000)
[GPIO] read  pin=5 level=LOW
[GPIO] write pin=5 level=HIGH (latch=0x0020)
[GPIO] read  pin=5 level=HIGH
[SPI]  transfer len=4 tx=[0x9F 0x00 0x00 0x00] rx=[0xA5 0xFF 0xFF 0xFF]
[UART] tx (50 bytes): "HAL demo complete: sensor OK, GPIO toggled, SPI OK"
--- Summary ---
WHO_AM_I      : 0x48
Last temp     : -10.00 C
GPIO pin 5    : HIGH (after 2 toggles)
SPI rx[0]     : 0xA5
UART message  : "HAL demo complete: sensor OK, GPIO toggled, SPI OK"
Result        : PASS
[HAL] deinit: peripherals shut down
```

Output is deterministic (identical across runs) and is also saved verbatim in
`sample_output.txt`.


---
*Created by Rohith Reddy Chinthala — Embedded Software Engineer at Honeywell, Saint Louis, MO.*

# Reverse Gear Input

The cab unit switches to the rear camera when the van is in reverse. It detects reverse from the +12 V reverse-light supply. This document describes the input circuit between the reverse-light wire and a GPIO on the Waveshare ESP32-P4 board.

> **Not built or tested yet.** Check the values against your van and the parts you buy. Work on the vehicle wiring only if you are competent to do so, and fuse every tap into the vehicle wiring.

## Requirements

- The vehicle supply is nominally 12 V. It is 9 V to 15 V in normal use, and it has fast spikes much higher than that, for example at engine start and load dump.
- The board GPIOs accept 0 V to 3.3 V only. A direct connection to 12 V destroys the ESP32-P4.
- The van ground and the board ground can be at slightly different voltages, and they carry noise.

An optocoupler meets these requirements. The 12 V side and the 3.3 V side have no electrical connection, so spikes and ground noise do not reach the board.

## Circuit

```text
Reverse light +12 V
      │
     [F1]  0.5 A fuse (fuse tap or in-line fuse at the tap point)
      │
      ├──────────┐
      │         [D2] SMBJ18A TVS diode (cathode to +12 V)
     [D1]        │
   1N4007        │
   (anode up)    │
      │          │
     [R1] 2.2 kΩ, 0.5 W
      │          │
    U1 pin 1 (LED anode)
    U1 pin 2 (LED cathode)
      │          │
Vehicle ground ──┴─────────────

                    Board 3.3 V
                        │
                       [R2] 10 kΩ
                        │
GPIO4 ──────────────────┼────── U1 pin 4 (collector)
                        │
                       [C1] 100 nF (optional, to board GND)
                        │
Board GND ──────────────┴────── U1 pin 3 (emitter)
```

U1 is a PC817 optocoupler (or a similar one with a current transfer ratio of at least 50 %).

## How It Works

- In reverse, current flows through D1, R1 and the optocoupler LED. The optocoupler transistor turns on and pulls GPIO4 to 0 V. The input is **active low**.
- When the van is not in reverse, R2 pulls GPIO4 to 3.3 V.
- D1 blocks reverse polarity. D2 clamps spikes above about 20 V. F1 protects the vehicle wire if a part fails short.
- C1 and the software debounce remove short noise pulses.

## Component Values

| Part | Value | Reason |
| --- | --- | --- |
| R1 | 2.2 kΩ, 0.5 W | LED current is about 3 mA at 9 V and about 6 mA at 14.4 V. The PC817 LED is rated for 50 mA. 0.5 W tolerates short spikes up to the TVS clamp voltage. |
| R2 | 10 kΩ | 0.33 mA when the transistor is on. With a transfer ratio of 50 % and 3 mA LED current, the transistor can sink 1.5 mA, so it saturates. |
| D1 | 1N4007 | Reverse polarity protection. |
| D2 | SMBJ18A | Stand-off voltage 18 V, above the normal supply. Clamps load dump spikes. |
| F1 | 0.5 A | Protects the tapped vehicle wire. |
| C1 | 100 nF | Filters fast noise. Optional. |

## Firmware Settings

The input is in the "Cab dashboard" menu of `idf.py menuconfig`:

| Option | Default | Purpose |
| --- | --- | --- |
| Reverse input GPIO | 4 | The GPIO for the optocoupler output. -1 turns the reverse input off. |
| Reverse input is active low | Yes | Matches the circuit above. |
| Reverse debounce | 50 ms | The input must be stable for this time before the firmware accepts a change. |

GPIO4 and GPIO5 are free on the 40-pin header. Check the pin position on the board silkscreen. Refer to [Waveshare ESP32-P4 board notes](waveshare_p4_43.md).

## Camera Power

Most reversing cameras get their power from the reverse-light supply too. Such a camera is off when the van is not in reverse, so the manual "Camera" button shows "No camera signal" while you drive forward. To use the camera at other times, power the camera from an ignition-switched supply instead.

## See Also

- [Waveshare ESP32-P4 board notes](waveshare_p4_43.md)

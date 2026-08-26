# SEDS BPHC — Avionics Round 1 Induction
**Project:** Athena's Intern (Project Odysseus)  
**Author:** Saptarshi Nandi  
**BITS ID:** [2026A4PS0609H]  

---

## Overview
This repository contains the source code, implementation details, and circuit schematics for the SEDS BPHC Avionics Subsystem Round 1 Induction Tasks.

---

## Task 1: Finding the Sea Floor

### Approach & Implementation
1. **Data Ingestion & Formatting:** Loaded the telemetry time-series dataset using Pandas. Handled non-numeric strings (`#VALUE!` at Point 97) safely using `pd.to_numeric(errors='coerce')` and mapped negative depth coordinates to positive scalar values representing distance below the sea surface.
2. **Outlier Filtering (Point 151 Glitch):** The -1271.1 m sensor spike was detected using a 7-point rolling median with a 3-standard-deviation ($3\sigma$) threshold filter. Corrupted points were replaced with NaN and bridged using linear interpolation.
3. **Noise Reduction:** Applied a 5-point rolling moving average filter to smooth random high-frequency acoustic sensor jitter while preserving true seabed bathymetry.
4. **Real-Time Telemetry Animation:** Rendered an animated bathymetric graph updating at 1 Hz ($1000\text{ ms}$ interval) using `matplotlib.animation.FuncAnimation`, with an inverted Y-axis (sea surface at $0\text{ m}$) and a live telemetry HUD.

### Telemetry Graph
![Task 1 Graph](task1_graph.png)

---

## Task 2: Keeping Watch Over Odysseus

### System Architecture & State Machine
The onboard safety system was designed in Tinkercad Circuits using an Arduino Uno, 16x2 LCD screen, 3-pin Ping ultrasonic sensor, ambient light sensor (LDR), push button, alert LED, and piezo buzzer.

The system runs a non-blocking Finite State Machine (FSM) utilizing `millis()`:
* **OPEN SEA:** Default navigation state; vessel sails safely.
* **ANCHOR DROPPED:** Push button toggles anchor state with 50 ms software debouncing. While anchored, danger timers are reset, and the vessel is fully protected from environmental hazards.
* **STORM:** Triggered when light levels fall below 512. Actuates a 250 ms blinking red alert LED. If the storm lasts continuously for 5 seconds without dropping anchor, the ship transitions to `WRECKED`.
* **CHARYBDIS:** Triggered when an obstacle is detected within 100 cm. Sounds a 1 kHz tone on the piezo buzzer. Continuous exposure for 5 seconds results in `WRECKED`.
* **WRECKED:** Terminal latch state reached after 5 seconds of unmitigated danger.
* **Precedence Rule:** Whichever danger state is entered first retains priority and its active timer continues running.

### Pin Mapping
| Component | Arduino Pin | Notes |
| :--- | :--- | :--- |
| **LCD RS** | Pin 12 | Register Select |
| **LCD E** | Pin 11 | Enable |
| **LCD DB4 – DB7** | Pins 5, 4, 3, 2 | 4-Bit Parallel Data Bus |
| **Ultrasonic Sensor** | Pin 9 | 3-Pin SIG (Trig/Echo shared) |
| **Light Sensor (LDR)** | Pin A0 | $10\text{ k}\Omega$ Pull-Down Voltage Divider |
| **Push Button** | Pin 7 | Internal `INPUT_PULLUP` |
| **Storm LED** | Pin 13 | $220\text{ }\Omega$ Current-Limiting Resistor |
| **Buzzer** | Pin 10 | PWM Tone Generation |

### Circuit & Simulation Preview
![Task 2 Circuit Schematic](task2_circuit.png)

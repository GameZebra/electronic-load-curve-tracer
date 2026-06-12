# Electronic Load Curve Tracer

This project is an open-source programmable electronic load designed for real-time IV curve tracing of photovoltaic (PV) sources.

It is primarily developed as a measurement and evaluation tool for Maximum Power Point Tracking (MPPT) algorithms, but it can also be used as a general-purpose electronic load for power electronics testing and characterization.

---

## Key Features

- Real-time IV curve acquisition
- Programmable electronic load control
- Designed for PV panel characterization
- MPPT algorithm evaluation under real operating conditions
- General-purpose laboratory electronic load functionality

---

## Operating Modes (Mode Hierarchy and Analysis)

In power electronics, **Constant Current (CC)** is the fundamental operating mode of an electronic load. All other modes are derived from it through software control implemented in the microcontroller.

### 1. Constant Current (CC) — Base Mode
The fundamental analog control mode.

The microcontroller sets a reference voltage via a DAC (Digital-to-Analog Converter), which is compared by an operational amplifier (Op-Amp). The Op-Amp drives the MOSFET gate to maintain a precise constant current through the load.

---

### 2. Constant Power (CP) — Derived from CC
A software-controlled sub-mode of CC.

The system continuously measures voltage ($V$) and computes the required current using:

$ I = \frac{P_{target}}{V} $

The CC reference is dynamically updated to maintain constant power.

---

### 3. Constant Resistance (CR) — Derived from CC
Another software-defined extension of CC.

The microcontroller measures voltage ($V$) and computes current using:

$ I = \frac{V}{R_{target}} $

The CC loop is adjusted accordingly.

---

### 4. Constant Voltage (CV)
Can be implemented either:
- Analogly (hardware feedback loop), or
- Digitally using a fast PID controller in the MCU

The controller adjusts current to maintain a stable output voltage.

---

### 5. IV Curve Sweeping (Curve Tracer Mode)
An automated measurement routine.

The system starts from open-circuit conditions ($I = 0$) and gradually increases current in discrete steps until reaching short-circuit current ($I_{sc}$).

At each step, voltage ($V$) and current ($I$) are sampled and logged, producing a full IV characteristic curve.

---

## Safety and Protection Systems

Proper protection mechanisms are critical in early prototypes to prevent MOSFET failure or thermal runaway.

### 1. Over-Temperature Protection (OTP)
A thermistor (NTC) is placed on the MOSFET heatsink.

If temperature exceeds ~70°C, the system shuts down the load immediately.

---

### 2. Over-Power Protection (OPP)
If the input source exceeds safe operating limits, the firmware disables the load to prevent component damage.

---

### 3. Reverse Polarity Protection
Protects against incorrect PV panel connection.

Implemented using a high-power diode or equivalent protection circuit.

---

## Project Scope

This project includes:

- Power electronics hardware design (MOSFET-based load stage)
- Embedded firmware for control and measurement
- Calibration procedures for accurate sensing
- PC software for IV curve visualization and data logging
- Mechanical enclosure design (FreeCAD)
- Experimental validation under varying irradiance conditions

---

## Notes

This project is part of a broader research effort on photovoltaic energy harvesting and MPPT optimization techniques.

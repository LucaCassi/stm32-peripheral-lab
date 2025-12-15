# Dual Motor PWM Synchronization using HRTIM (STM32G474)

## Overview

This project demonstrates a **hardware-level, fully deterministic generation of six complementary PWM phases**
using the **HRTIM peripheral of the STM32G474** microcontroller.

The design targets **dual three-phase motor / inverter control** and focuses on:

- complementary PWM generation
- deterministic start/stop behavior
- dead-time insertion handled entirely in hardware
- live parameter tuning from the debugger
- clean separation between application logic and low-level timing

Although the PWM outputs are suitable for real power stages, this project is intentionally
kept **open-loop and debug-driven**, making it ideal as a reference for HRTIM bring-up.

---

## Why STM32G474 and not STM32H753?

The STM32H753 includes an HRTIM, but only **five waveform timers (A–E)**.
A single three-phase motor requires **three timers**, one per phase.

Using the STM32G474 allows:
- **six independent HRTIM timers (A–F)**
- a natural mapping:
  - Motor 1 → Timers A, B, C
  - Motor 2 → Timers D, E, F

This makes the design symmetrical, scalable, and easier to reason about.

---

## System Clock Configuration

- SYSCLK: **170 MHz**
- HRTIM1 clock: **170 MHz**
- No prescaling on master or individual HRTIM timers

This results in a PWM tick resolution of ~5.9 ns before any high-resolution interpolation.

---

## HRTIM Architecture

### Timer Allocation

| Timer | Function |
|-----|----------|
| A | Motor 1 – Phase U |
| B | Motor 1 – Phase V |
| C | Motor 1 – Phase W |
| D | Motor 2 – Phase U |
| E | Motor 2 – Phase V |
| F | Motor 2 – Phase W |

Each timer generates **two outputs**:
- Output 1 → high-side gate
- Output 2 → low-side gate (complementary)

---

## PWM Generation Principle (Key Concept)

HRTIM outputs are **not PWM channels** in the classical sense.

Each output is defined by:
- one or more **SET events**
- one or more **RESET events**

### Complementary PWM logic used in this project

For each timer:

- **Output 1 (high-side)**
  - SET at period start
  - RESET at compare match (CMP1)

- **Output 2 (low-side)**
  - SET at compare match (CMP1)
  - RESET at period start

This creates **true complementary waveforms**.
Dead-time is then inserted automatically by the HRTIM hardware.

Polarity is set to **Active High** for both outputs.
Complementarity is achieved through event configuration, not polarity inversion.

---

## PWM Parameters

- PWM period: **17000 ticks**
- Initial duty cycle: **50%**
- Duty cycle limits: **10% – 90%**

Duty limits avoid pathological corner cases:
- CMP = 0 or CMP = PERIOD
- outputs permanently ON or OFF
- ambiguous dead-time behavior

---

## Dead-Time Configuration

- Dead-time enabled for all timers
- Rising and falling dead-time set conservatively (~1 µs)

This value is intentionally large:
- safe for oscilloscope bring-up
- avoids shoot-through on any realistic power stage

It can be reduced later based on gate driver and MOSFET characteristics.

---

## High-Resolution Mode

High-resolution interpolation is **available but not required** for motor control PWM frequencies.
The project does not rely on sub-tick edge positioning.

This reflects a conscious design choice:
- high-resolution is essential for high-frequency SMPS and phase-shifted converters
- classical motor control rarely benefits from picosecond-level edge placement

---

## Application Architecture

The firmware is organized around a minimal and explicit **application state machine**.

Two states are defined:

- `STATE_STOP`
- `STATE_RUN`

In `STATE_STOP`:
- all HRTIM outputs are disabled
- all counters are stopped
- the power stage is kept in a safe, inactive condition

In `STATE_RUN`:
- all PWM timers (A–F) are started synchronously
- all outputs are enabled simultaneously
- duty cycles are applied and updated in real time

The application state is stored in a `volatile` global variable (`appState`), which can be modified live from the debugger or later by real control logic.

The function `App_Run()` is called continuously from the main loop and:
- detects state transitions
- starts or stops the HRTIM accordingly
- updates PWM duty cycles when required

This structure keeps timing-critical logic deterministic and independent from application-level decisions.

---

## Debug-Driven Control

This project is intentionally **debug-driven** to simplify bring-up and validation.

All PWM duty cycles are exposed as global `volatile` variables:

- `dbg_m1_u_pct`, `dbg_m1_v_pct`, `dbg_m1_w_pct`
- `dbg_m2_u_pct`, `dbg_m2_v_pct`, `dbg_m2_w_pct`

Each variable represents the duty cycle of one inverter leg, expressed in percent.

Features of the debug interface:

- duty cycles can be modified live from STM32CubeIDE
- values are automatically clamped to the range **10% – 90%**
- changes are applied using HRTIM preload registers, avoiding glitches
- compare registers are updated only when values actually change

An additional debug flag is provided:

- `dbg_outputs_enable`

When this flag is cleared:
- HRTIM counters keep running
- all PWM outputs are disabled

This is useful for safe testing, oscilloscope probing, and early hardware bring-up.

---

## Start and Stop Strategy

A deterministic start/stop sequence is essential for synchronized multi-phase systems.

### Start Sequence (transition to STATE_RUN)

1. All outputs are disabled
2. PWM compare values are updated
3. All HRTIM counters (Timers A–F) are started simultaneously using a mask
4. All outputs are enabled simultaneously

This guarantees:
- aligned phase start across all six timers
- repeatable behavior after every STOP/RUN cycle

### Stop Sequence (transition to STATE_STOP)

1. All PWM outputs are disabled first
2. All HRTIM counters are stopped

Disabling outputs before stopping counters is safer for real power stages and avoids unintended switching.

---

## What This Project Is and Is Not

### This project **is**:
- a clean reference for HRTIM-based PWM generation
- a deterministic multi-phase timing example
- suitable as a foundation for motor control experiments

### This project **is not**:
- a closed-loop motor controller
- a current-regulated inverter
- production-ready firmware

---

## Notes on the C Code

- Start reading the code from `App_Run()` to understand the control flow.
- All HRTIM access is centralized and explicit.
- Debug-only features are clearly separated from application logic.
- If a real control loop is added, debug-driven duty updates should be disabled or isolated.

---

## Final Remarks

The code is intentionally simple and designed to be read, modified, and extended.


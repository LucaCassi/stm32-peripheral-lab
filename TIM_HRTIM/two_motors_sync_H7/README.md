# Synchronized Dual Motor Timers on STM32H753

## Overview
This project demonstrates how to achieve **deterministic, hardware-level synchronization of two PWM timers** on an **STM32H753** microcontroller, targeting **dual motor control** applications.

The core idea is to use:

- **TIM2** as a *master timer*, configured only to generate a synchronization trigger (TRGO)
- **TIM1** and **TIM8** as *slave advanced timers*, each driving a three-phase PWM set

Both slave timers start **on the same hardware event**, ensuring repeatable phase alignment that can be verified on an oscilloscope. The project is designed to be simple to test on GPIO pins, yet directly scalable to real power stages and motors.

Although the implementation is tested on a **custom (non-public) board**, the configuration is intentionally generic and easily portable to other STM32H7 (and even G4) devices featuring advanced timers.

---

## Target MCU and Tools

- **MCU**: STM32H753 (STM32H7 series)
- **IDE**: STM32CubeIDE
- **Configuration tool**: STM32CubeMX
- **Language**: C (HAL-based)
- **Verification**: Oscilloscope (PWM outputs / debug pins)

---

## Project Goal

The goal of this project is to show:

- How to **start two independent PWM generators at exactly the same instant**
- How to **re-synchronize them deterministically** after a STOP/RUN transition
- How to do this **entirely in hardware**, without relying on software timing or delays

This is a very common requirement in:

- dual motor drives
- multi-axis control
- power electronics with multiple synchronized converters

---

## Clock Configuration

The first mandatory step is configuring the system clock.

In this project:

- **HSE (external crystal / resonator)** is used as the high-speed clock source
- HSE frequency is set to **8 MHz** (adjust to your hardware)
- The PLL is configured to generate a **SYSCLK of 480 MHz**

CubeMX automatically updates bus prescalers and flash latency after this step.

> Note: If your board does not include an external crystal, you can use the internal oscillator instead. Precision is slightly reduced, but this project works perfectly fine nonetheless.

---

## Timer Architecture

### Master Timer – TIM2

TIM2 is used **only as a synchronization source**, not to generate PWM.

Key settings:

- Clock source: Internal
- Prescaler: **0**
- Counter period (ARR): **1**
- Counter mode: Up
- CKD: No division
- Auto-reload preload: Disabled

**Trigger Output (TRGO):**

- Enabled
- Trigger event selection: **Update Event**

This configuration generates an update event almost immediately after TIM2 is started. That update event is routed internally as TRGO to the slave timers.

> Using PSC=0 and ARR=1 is unusual for a "normal" timer, but ideal here: the master fires as soon as possible and introduces virtually no startup latency.

---

### Slave Timers – TIM1 and TIM8

TIM1 and TIM8 are **advanced-control timers**, ideal for motor control thanks to:

- complementary PWM outputs (CHx / CHxN)
- dead-time insertion
- break input support

Both timers are configured identically.

Key settings:

- Clock source: Internal
- Slave mode: **Trigger Mode**
- Trigger source: **ITR connected to TIM2_TRGO**

> The exact ITR index (ITR0–ITR3) depends on the MCU. CubeMX shows the correct mapping explicitly (e.g. "ITR1 (TIM2_TRGO)"). Always trust CubeMX or the reference manual.

Trigger mode is used (instead of reset mode) because only a **single synchronization event** is needed per RUN cycle.

---

## PWM Configuration

Each advanced timer drives a three-phase PWM set:

- Channels: CH1, CH2, CH3
- Optional complementary outputs: CH1N, CH2N, CH3N

General guidelines:

- TIM1 and TIM8 use the **same prescaler and ARR** to guarantee identical PWM frequency
- Auto-reload preload is **enabled** on PWM timers
- Initial duty cycle is set to **50%**
- Duty cycles are constrained between **10% and 90%** for safety

Duty cycles can be **modified live from the debugger**, which is extremely useful during bring-up and oscilloscope testing.

---

## Dead-Time and Break Configuration

Dead-time and break settings are **application-dependent** and must always be checked against the power module datasheet.

In this project:

- Dead-time is set conservatively (large value)
- CH and CHN polarities are both **active-high** (standard configuration)
- OSSR and OSSI are enabled to keep outputs in a defined safe state
- Automatic output is disabled (manual re-enable after faults)

> Never reduce dead-time or relax break protection before fully understanding the power stage characteristics.

---

## Software Architecture

The application is structured around a very small state machine:

- **STATE_STOP**: PWM outputs disabled
- **STATE_RUN**: PWM outputs running

A single function, `App_Run()`, is called in the main loop and:

- detects state transitions
- starts or stops timers accordingly
- continuously applies debug-controlled PWM duties in RUN state

The synchronization logic is encapsulated in:

- `MotorTimers_SyncStart()`
- `MotorTimers_Stop()`

All state and duty variables are declared as **volatile globals**, making them fully accessible and modifiable from the STM32CubeIDE debugger.

---

## Verification

Synchronization can be verified by:

- probing one PWM channel from TIM1 and one from TIM8
- observing aligned rising edges on an oscilloscope
- toggling STOP/RUN from the debugger and checking repeatability

Optional test signals (PWM or GPIO) can be enabled to act as timing markers.

---

## Notes on the C Code (Important)

1. **Read the code top-down**: start from `main.c`, then follow `App_Run()` to understand the state machine and timer control flow.
2. This project intentionally avoids interrupts for synchronization. All alignment is done in **hardware**, making the behavior deterministic and robust.

---

## Final Remarks

This project is intentionally small, focused, and hardware-oriented. Its purpose is to demonstrate **clean timer synchronization**, a foundational skill for advanced motor control and power electronics applications on STM32 microcontrollers.


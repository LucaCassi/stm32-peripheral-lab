
Each subfolder is a **standalone STM32 project**, built around the same functional requirement but using a different ADC data retrieval mechanism.

---

## Project Overview

### 1. ADC_Polling
**Single conversion – polling-based data retrieval**

- ADC conversion is started manually
- The CPU actively waits for conversion completion (`HAL_ADC_PollForConversion`)
- The result is read synchronously

**Why it matters**
- Simplest and most explicit ADC usage model
- Ideal for:
  - Debugging
  - Low-frequency measurements
  - Learning ADC configuration step-by-step

**Trade-offs**
- CPU blocking
- Not scalable for real-time or high-frequency acquisition

---

### 2. ADC_Interrupt
**Single conversion – interrupt-driven data retrieval**

- ADC conversion completion triggers an interrupt
- Conversion result is handled inside the ADC callback
- CPU is free while conversion is ongoing

**Why it matters**
- Introduces event-driven acquisition
- Shows how ADC integrates with NVIC and callbacks
- Common pattern in real-time embedded systems

**Trade-offs**
- Slightly more complex control flow
- Still one conversion at a time

---

### 3. ADC_DMA
**Single conversion – DMA-based data transfer**

- ADC conversion result is transferred automatically via DMA
- No CPU intervention during data movement
- Callback signals end of transfer

**Why it matters**
- Foundation for high-throughput ADC applications
- Demonstrates ADC–DMA coupling
- Essential for:
  - Continuous acquisition
  - Multi-channel scans
  - Data logging systems

**Trade-offs**
- Higher configuration complexity
- Overkill for very simple or low-rate use cases

---

## Design Philosophy

- **One concept per project**  
  Each example focuses on *only one ADC acquisition method*.

- **Minimal but correct HAL configuration**  
  No unnecessary peripherals or abstractions.

- **Heavily commented code**  
  Emphasis on *why* each configuration choice is made.

---

## Notes on Scope

- All projects use:
  - **Single ADC channel**
  - **Single conversion (non-continuous)**
- No filtering, oversampling, or multi-channel scans are included here by design.

A **separate, more advanced project** will cover:
- Continuous conversion
- High-frequency sampling
- Buffered DMA acquisition
- Data logging and asynchronous communication

---

## References

The structure and concepts of these examples are inspired by and adapted from tutorials available on:

- https://deepbluembedded.com/

The code has been reworked, simplified where appropriate, and documented with a focus on **clarity, correctness, and professional embedded development practices**.


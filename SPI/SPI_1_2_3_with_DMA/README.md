# STM32H753 – SPI Configuration and Abstraction Example

## Overview

This project is focused on the configuration and use of multiple SPI peripherals on the **STM32H753ZIT** microcontroller.

The primary goal of this project is **not** to provide a complete end-application, but to demonstrate:
- Correct and conscious configuration of SPI peripherals using **STM32CubeMX**
- Management of different SPI use cases on the same MCU
- A clean and readable application-level SPI abstraction layer
- Awareness of real-world constraints when deploying SPI communication in industrial systems

The project has been initialized and generated using STM32CubeMX, and all peripheral settings are intentionally kept explicit and traceable to the `.ioc` file.

---

## Target Platform

- **Microcontroller**: STM32H753ZIT (Cortex-M7)
- **Clock Source**: External HSE (8 MHz)
- **Development Environment**: STM32CubeIDE
- **HAL Driver**: STM32Cube FW_H7

The MCU is configured to operate at high performance (480 MHz CPU clock), reflecting realistic industrial and high-throughput embedded systems.

---

## SPI Peripheral Overview

This project configures and enables **three independent SPI peripherals**, each intentionally set up with different parameters to represent common real-world communication scenarios.

| SPI | Mode | Data Size | CPOL / CPHA | Baudrate | Transfer Method | Typical Use Case |
|----|----|----|----|----|----|----|
| SPI1 | Master | 8-bit | Mode 1 | ~1.5 Mbit/s | Polling | Standard sensor / peripheral |
| SPI2 | Master | 16-bit | Mode 3 | ~3.0 Mbit/s | Polling | High-throughput or legacy device |
| SPI3 | Master | 8-bit | Mode 3 | ~1.5 Mbit/s | DMA | Continuous or burst transfers |

Each SPI instance uses:
- Manual **GPIO-controlled Chip Select**
- Full-duplex configuration
- Explicit pin assignment and clock source selection

---

## CubeMX Configuration Philosophy

The configuration has been designed with the following principles:

### 1. Explicit Peripheral Settings

All relevant SPI parameters are intentionally configured and not left at defaults:
- Clock polarity and phase (CPOL / CPHA)
- Data frame size
- Baud rate prescaler
- FIFO threshold
- NSS behavior (software-managed CS)

This makes the configuration **self-documenting** and suitable for technical review.

---

### 2. Manual Chip Select Management

All SPI instances use:
- `NSS = Software`
- Chip Select handled via GPIO

This reflects the most common real-world scenario where:
- Devices require non-standard CS timing
- Multiple transactions must occur under a single CS assertion
- SPI slaves do not fully comply with automatic NSS behavior

---

### 3. Multiple SPI Clock Domains

SPI clocks are derived from the PLL-based SPI123 clock domain, allowing:
- High and stable SPI clock frequencies
- Independent tuning of SPI speed without impacting other peripherals

This mirrors typical industrial designs where SPI timing margins must be tightly controlled.

---

### 4. DMA Usage on SPI3

SPI3 is configured with:
- Dedicated RX and TX DMA streams
- High priority DMA channels
- Interrupt-driven completion handling

This configuration is suitable for:
- High data-rate streams
- Periodic bulk transfers
- Reducing CPU load in real-time applications

The project explicitly acknowledges that on STM32H7-class MCUs, DMA usage may require **cache coherency management**, which is left as an application-level responsibility.

---

## Application-Level SPI Abstraction

In addition to the CubeMX-generated code, the project includes a **generic SPI application layer** (`spi_app.c / spi_app.h`) providing:

- A simple device structure containing:
  - SPI handle
  - Chip Select GPIO
- Blocking (polling-based) SPI transfers
- Non-blocking DMA-based transfers
- Explicit and readable CS handling
- Minimal internal state tracking for DMA operations

The abstraction is intentionally kept:
- Lightweight
- Readable
- Easy to adapt to different SPI devices

No assumptions are made about the connected SPI slave.

---

## Design Notes on Code Origin and Intent

The SPI application code included in this project is **generic and device-agnostic**.

Its structure and design are inspired by patterns commonly observed in:
- Industrial embedded systems
- Production firmware
- Real-world SPI driver implementations

However:
- The code is **not copied from any proprietary source**
- It is **not tied to any specific sensor, board, or protocol**
- It serves as an educational and demonstrative abstraction

Any real deployment of SPI communication **requires careful, application-specific work**, including but not limited to:
- Slave device timing requirements
- Initialization and reset sequences
- Command and data framing
- Error handling and recovery strategies
- Cache coherency considerations (on high-performance MCUs)

This project intentionally focuses on **infrastructure and configuration correctness**, not on application logic.

---

## Scope and Limitations

This repository demonstrates:
- How to correctly configure and enable SPI peripherals using STM32CubeMX
- How to manage multiple SPI interfaces on a single MCU
- How to structure a clean and maintainable SPI abstraction layer

It does **not** attempt to:
- Implement a complete SPI protocol
- Model a specific SPI device
- Provide production-ready application logic

Those aspects are highly dependent on the target hardware and system requirements and must be addressed on a per-project basis.


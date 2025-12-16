# STM32 ADC – Timer-Triggered Scan with DMA, Oversampling and Injected Conversions

This project is a **revisited and cleaned-up version of an ADC application I previously developed**, restructured with a strong focus on **peripheral configuration clarity** rather than on application-level logic.

---

## Overview

The application configures **ADC1** to perform:

- **Regular group conversions**
  - Multi-channel scan (5 channels)
  - Timer-triggered sampling at **1 kHz**
  - Circular DMA transfer
  - Hardware oversampling

- **Injected group conversions**
  - Independent, higher-priority conversion path
  - Differential input
  - Software-triggered
  - Interrupt-based data retrieval
  - Independent oversampling configuration

The system is designed to highlight how different ADC features interact at the **peripheral level**, rather than to present a finalized end-product.

---

## ADC Regular Group Configuration

### Channels and Scan Sequence
- **ADC1 Regular Group**
- **5 ranks**, in fixed order:
  1. IN2 – Differential
  2. IN3 – Single-ended
  3. IN4 – Single-ended
  4. IN8 – Single-ended
  5. IN9 – Single-ended

Each rank has an explicitly chosen sampling time, reflecting a realistic scenario where input source impedance and signal quality differ between channels.

---

### Triggering and Timing
- **External trigger**: `TIM2 TRGO (Update Event)`
- **Trigger edge**: Rising
- **Sampling frequency**: **1 kHz**, fully hardware-driven

This guarantees:
- Deterministic sampling period
- No dependency on CPU execution timing
- Clean separation between data acquisition and application logic

---

### Data Transfer
- **DMA mode**: Circular
- **Data width**: 12-bit
- **Memory increment**: Enabled
- **Overrun behavior**: Data overwritten

The DMA configuration is intentionally minimal and robust, suitable for continuous acquisition without CPU intervention.

---

### Oversampling (Regular Group)
- **Oversampling ratio**: 8×
- **Right bit shift**: 3

This results in a true hardware-averaged output, improving noise performance and effective resolution without altering the external sampling rate.

---

## ADC Injected Group Configuration

The injected group is configured as a **logically independent conversion path**, intended for urgent, diagnostic, or safety-related measurements.

### Key Characteristics
- **Single injected rank**
- **Differential input channel**
- **Software-triggered**
- **Interrupt-driven completion**
- **Independent oversampling** (4× with right shift 2)

Injected conversions can preempt regular conversions, demonstrating ADC priority handling and multi-context acquisition.

---

## Timing and Bandwidth Considerations

This configuration is intentionally designed to operate **close to the available ADC conversion bandwidth**.

When the injected group is triggered at high rate (e.g. **16 kHz**) while the regular group runs periodically at **1 kHz** with multi-channel scan and hardware oversampling enabled, the overall ADC usage approaches the theoretical conversion limit. In this worst-case scenario, the remaining timing margin is on the order of **single-digit percentage (~7%)**.

This is a **deliberate design choice**, aimed at:
- Exploring ADC scheduling limits
- Evaluating regular vs injected preemption behavior
- Understanding how oversampling, sampling time, and trigger rates interact

Such a configuration highlights an important system-level consideration:  
**heavy ADC usage directly reduces the available DMA bandwidth for other peripherals** (e.g. additional ADC instances, SPI, UART, or high-throughput data streams).

As a consequence, this setup should be considered **application-specific and deployment-dependent**. Any real system integrating additional DMA-driven peripherals would require a careful rebalancing of sampling rates, oversampling factors, or DMA priorities.

---

## Interrupt and Priority Design

- **ADC global interrupt** enabled
- **ADC IRQ priority** higher than DMA IRQ

This ensures injected conversion completion is serviced promptly, even during continuous DMA-driven regular acquisitions.

---

## Design Intent and Scope

This project is **not presented as a drop-in, production-ready firmware**.

While the configuration is internally consistent and theoretically functional, the **number of ADC, DMA, clock, and trigger parameters involved is large and highly interdependent**.  
As a result:

> Each real deployment would require **careful, application-specific tuning** of timing, sampling times, oversampling ratios, trigger sources, and memory handling.

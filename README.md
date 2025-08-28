# ⚙️ STM32 Peripheral Lab

This repository explores a set of **advanced STM32 peripherals** through modular demos,  
each focusing on performance tuning, edge configurations, and real-world use cases.

---

## ✨ Covered Peripherals
- **SPI** — high-speed streaming with DMA  
- **FDCAN** — reliable real-time communication for robotics/automotive  
- **Advanced Timers** — synchronized PWM for multi-motor control  
- **ADC/DAC** — high-performance data acquisition and signal generation  
- **LTDC** — embedded display interface for HMI systems  

*(Additional experiments: DMA tricks, I2C sensor interfacing, USART with double-buffer DMA, USB CDC.)*

---

## 🧩 Repository Layout
stm32-peripheral-lab/
├─ spi/
├─ fdcan/
├─ timers-advanced/
├─ adc-dac/
├─ ltdc/
└─ common/


Each folder contains:
- **Source code** (CubeMX + HAL examples)
- **README.md** (setup, goals, results, measurements)
- **Documentation** (waveforms, scope screenshots, test logs)

---

## 📚 Hardware
- **Primary board**: STM32 Nucleo-G474RE (ADC/DAC, timers, FDCAN)  
- **Optional**: STM32F746G-DISCO (for LTDC/DSI with integrated display), but I will use a dedicated board from my job  
- **Lab equipment**: Oscilloscope & logic analyzer for timing validation

---

## 📬 Contact & Collaboration
This lab is designed for **learning, demonstration, and collaboration**.  
Engineers and students experimenting with STM32 peripherals are welcome to share improvements or new demos.

📧 **luca99.cassi@gmail.com**

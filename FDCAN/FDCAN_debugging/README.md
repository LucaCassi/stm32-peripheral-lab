# FDCAN_debugging (STM32G4 / FDCAN1) — Minimal CAN Debug Sender + Error Counters

This project is a small, *portfolio-friendly* extraction of a bigger debugging firmware used on a custom board in an industrial system where the CAN network showed **intermittent, non-deterministic communication issues**.

The goal of this simplified code is to:
- continuously transmit a Classic CAN frame (8 bytes, Standard ID),
- enable FDCAN interrupt notifications,
- collect basic **bus/controller error statistics** (warning, error passive, bus-off, TX FIFO full),
- and perform a minimal **application-level sequence check** on RX frames (byte `[0]` as counter).

---

## Hardware / MCU

- **MCU**: STM32G474VET6 (STM32G4 family)
- **CAN peripheral**: **FDCAN1**
- **Pins (CubeMX / .ioc)**:
  - `PD0` = `FDCAN1_RX`
  - `PD1` = `FDCAN1_TX`

> Note: This firmware assumes an external CAN transceiver is present on the board (e.g., TJA105x / MCP256x / etc.) and a properly terminated CAN bus.

---

## CubeMX / .ioc Configuration Summary

### 1) FDCAN1 mode and timing
From the provided `.ioc`:

- **Classic CAN** is used (no CAN FD in the module TX header).
- **Auto Retransmission**: `ENABLE`
- **Nominal bitrate**: ~`500 kbit/s` (CubeMX computed `499999`; I kept the values coherent with the system under analysis)
- **Nominal timing**:
  - `NominalPrescaler = 20`
  - `NominalTimeSeg1 = 13`
  - `NominalTimeSeg2 = 3`
  - `NominalSyncJumpWidth = 3`
- **FDCAN kernel clock**: `170 MHz` (see `RCC.FDCANFreq_Value=170000000`)

> The `.ioc` also contains "Data*" timing fields, but the code configures **Classic CAN** (`FDFormat = FDCAN_CLASSIC_CAN`) and **BRS off**, so only the *nominal* phase is relevant.

### 2) Filters
- `StdFiltersNbr = 2`
- `ExtFiltersNbr = 2`

> This project does not explicitly show filter programming in `can_module.c`. Filter setup may be generated in `fdcan.c` by CubeMX (`MX_FDCAN1_Init()`), depending on how CubeMX was configured.

### 3) NVIC / interrupts
Enabled in `.ioc`:
- `FDCAN1_IT0_IRQn = enabled`
- `FDCAN1_IT1_IRQn = enabled`

These are required for the HAL callbacks used by the module:
- `HAL_FDCAN_ErrorStatusCallback(...)`
- `HAL_FDCAN_RxFifo0Callback(...)`

### 4) Clock setup (high-level)
- System clock is derived from **HSE + PLL**.
- `SYSCLK = 170 MHz` (typical for STM32G4 performance config).

---

## Project Structure

- `Core/Src/main.c`  
  Initializes HAL + clocks + GPIO + `MX_FDCAN1_Init()`, then uses the CAN module in a simple loop.
- `can_module.[ch]`  
  Minimal helper module:
  - configures a fixed TX header (Classic CAN, DLC=8, Standard ID),
  - starts FDCAN,
  - activates notifications (RX FIFO0 new message + error status),
  - maintains error counters and RX sequence checks.

---

## How it works

### Main loop behavior (`main.c`)
After init:
1. `can_module_init(CAN_MODULE_FDCAN1, 0x201);`
2. In the infinite loop:
   - wait `1 ms`
   - transmit `CAN_Tx` (8 bytes) via `can_module_transmit(...)`

This produces a **~1 kHz** stream of CAN frames, useful to stress the bus and reproduce sporadic issues.

### TX frame format
- Standard ID: `0x201` (set in `main.c`)
- DLC: 8 bytes
- Data: `CAN_Tx = {0,1,2,3,4,56,7,8}` (constant in this simplified version)

### Error counters and callbacks
The module counts:
- TX FIFO full conditions (when enqueue fails),
- controller protocol states:
  - Warning
  - Error Passive
  - Bus-Off
- other unexpected conditions.

RX FIFO0 callback:
- reads one message from FIFO0,
- checks if `data[0]` is sequential vs the previous message (`last_msg_idx + 1` with wrap 255→0),
- increments a firmware-level counter on mismatch (useful to detect drops/reordering at application level).

---

## How to use

1. Open the project in **STM32CubeIDE**.
2. Ensure `MX_FDCAN1_Init()` is generated and matches your board’s FDCAN transceiver wiring.
3. Connect the board to a CAN bus with:
   - correct termination (typically 120Ω at both ends),
   - at least one other node (or analyzer) to provide ACKs.
4. Build and flash.
5. Observe behavior using:
   - a CAN analyzer (PCAN/Kvaser/etc.), and/or
   - a debugger watch on:
     - `can_module_error[...][...]`
     - `can_error`
     - `err_num`

Suggested debug variables to watch:
- `can_module_error[CAN_MODULE_FDCAN1][CAN_MODULE_FIFO_FULL]`
- `can_module_error[CAN_MODULE_FDCAN1][CAN_MODULE_ERROR_WARNING]`
- `can_module_error[CAN_MODULE_FDCAN1][CAN_MODULE_ERROR_PASSIVE]`
- `can_module_error[CAN_MODULE_FDCAN1][CAN_MODULE_ERROR_BUS_OFF]`
- `can_module_error[CAN_MODULE_FDCAN1][CAN_MODULE_ERROR_FW]`

---

## Typical interpretation of the counters

- **WARNING / ERROR_PASSIVE increasing quickly**
  - often indicates missing ACK (node alone on the bus, unplugged transceiver, wrong bitrate, wiring issue).
- **BUS_OFF increasing**
  - severe bus error rate; investigate physical layer, bitrate mismatch, termination, EMC noise.
- **FIFO_FULL increasing**
  - transmit load too high for current bus/arbitration conditions; frames cannot be enqueued fast enough.
- **ERROR_FW increasing**
  - application-level drop/reorder (e.g., RX overruns, missed reads, priority/latency issues).

---

## Notes / limitations of this simplified version

- Only **FDCAN1** is effectively used (FDCAN2 is present but not wired in code).
- The payload is constant in `main.c` (for real sequence testing you typically increment byte `[0]` each TX).
- Filtering and some low-level peripheral details are assumed to be handled in CubeMX-generated `fdcan.c`.

---

## License
ST default header applies. If you plan to publish this as a portfolio repo, consider adding an explicit LICENSE file and a short disclaimer about the original industrial context.

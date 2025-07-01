# NXP S32K358 LPUART Controller Documentation

## Overview

The NXP S32K358 LPUART (Low Power Universal Asynchronous Receiver/Transmitter) is a QEMU device model that emulates the UART peripheral of the S32K358 microcontroller. It supports asynchronous serial communication with programmable baud rates, interrupt-driven operation, and integration with QEMU's character device backend.

---

## Header File: `nxps32k358_lpuart.h`

### Key Definitions

- **Register Offsets**: Defines the memory-mapped offsets for LPUART registers
- **Bit Masks**: Provides bit-level definitions for control and status registers
- **Reset Values**: Default values for registers after reset

### Key Constants

- **Status Register (STAT) Bits**:
  - `LPUART_STAT_TDRE`: Transmit Data Register Empty flag
  - `LPUART_STAT_RDRF`: Receive Data Register Full flag
- **Control Register (CTRL) Bits**:
  - `LPUART_CTRL_TE`: Transmitter Enable
  - `LPUART_CTRL_RE`: Receiver Enable
  - `LPUART_CTRL_RIE`: Receive Interrupt Enable
  - `LPUART_CTRL_TIE`: Transmit Interrupt Enable
- **Baud Rate Register (BAUD) Fields**:
  - `LPUART_BAUD_OSR_MASK`: Over Sampling Ratio field mask
  - `LPUART_BAUD_SBR_MASK`: Baud Rate Modulo Divisor field mask
- **Reset Values**:
  - `LPUART_BAUD_RESET`: Default BAUD register value
  - `LPUART_STAT_RESET`: Default STAT register value
  - `LPUART_CONTROL_RESET`: Default CTRL register value

### Key Structures

- **`NXPS32K358LPUARTState`**: Represents the state of the LPUART device:
  - **Parent Object**: `SysBusDevice parent_obj`
  - **Memory Region**: `MemoryRegion iomem`
  - **Clock Source**: `Clock *clk`
  - **Character Backend**: `CharBackend chr` for serial I/O
  - **IRQ Line**: `qemu_irq irq`
  - **Registers**:
    - `uint32_t baud_rate_config`: Baud Rate Register
    - `uint32_t lpuart_cr`: Control Register
    - `uint32_t lpuart_sr`: Status Register
    - `uint32_t lpuart_dr`: Data Register
    - `uint32_t lpuart_gb`: Global Register

---

## Source File: `nxps32k358_lpuart.c`

### Debug Logging

The driver uses macros `DB_PRINT_L`, `DB_PRINT`, and `DB_PRINT_READ` to conditionally emit debug logs at different verbosity levels. These wrap `qemu_log()` and are controlled by `NXP_LPUART_DEBUG`.

### Key Functions

#### `nxps32k358_lpuart_calculate_baud_rate()`

- **Purpose**: Computes the actual baud rate from the BAUD register configuration
- **Functionality**:
  - Extracts SBR (Baud Rate Modulo Divisor) and OSR (Over Sampling Ratio) values from the BAUD register
  - Retrieves the module clock frequency using `clock_get_hz()`
  - Calculates baud rate using formula: `baud = clock_freq / ((OSR + 1) * SBR)`
  - Returns 0 if divisor is zero (invalid configuration)

#### `nxps32k358_lpuart_update_params()`

- **Purpose**: Configures the serial backend with current baud rate settings
- **Functionality**:
  - Calls `nxps32k358_lpuart_calculate_baud_rate()` to get current baud rate
  - Populates a `QEMUSerialSetParams` structure with the calculated rate
  - Applies parameters to the character backend using `CHR_IOCTL_SERIAL_SET_PARAMS`

#### `nxps32k358_lpuart_update_irq()`

- **Purpose**: Manages the IRQ line state based on interrupt enable flags and status conditions
- **Functionality**:
  - Checks if any enabled interrupt condition is active (TX empty, RX full, etc.)
  - Asserts IRQ line if conditions met, deasserts otherwise
  - Uses bitmask: `(s->lpuart_sr & s->lpuart_cr)` to determine active interrupts

#### `nxps32k358_lpuart_can_receive()`

- **Purpose**: Determines if the UART can accept incoming data
- **Functionality**:
  - Returns 0 (cannot receive) if Receive Data Register Full (RDRF) flag is set
  - Returns 1 (can receive) otherwise
  - Called automatically by QEMU's character backend system

#### `nxps32k358_lpuart_receive()`

- **Purpose**: Handles incoming data from the character backend
- **Functionality**:
  - Checks if receiver is enabled (`LPUART_CTRL_RE` bit set)
  - Stores received byte in data register (`lpuart_dr`)
  - Sets Receive Data Register Full (RDRF) status flag
  - Triggers interrupt update via `nxps32k358_lpuart_update_irq()`

#### `nxps32k358_lpuart_reset()`

- **Purpose**: Resets the device to default state
- **Functionality**:
  - Initializes registers to their reset values
  - Clears pending interrupts
  - Calls `nxps32k358_lpuart_update_irq()` to synchronize IRQ state

#### `nxps32k358_lpuart_read()`

- **Purpose**: Handles register read operations
- **Functionality**:
  - Implements read behavior for all memory-mapped registers
  - Special handling for DATA register:
    - Clears RDRF status flag after read
    - Notifies character backend to accept more input
    - Updates IRQ state
  - Logs unimplemented register accesses

#### `nxps32k358_lpuart_write()`

- **Purpose**: Handles register write operations
- **Functionality**:
  - Implements write behavior for all memory-mapped registers
  - Special handling for:
    - GLOBAL register: Triggers reset if `LPUART_GLOBAL_RST_MASK` bit set
    - BAUD register: Updates serial parameters
    - CTRL register: Updates interrupt state
    - DATA register: Transmits character via character backend
  - Logs unimplemented register accesses

#### `nxps32k358_lpuart_realize()`

- **Purpose**: Finalizes device initialization
- **Functionality**:
  - Verifies clock source is connected
  - Sets up character backend handlers:
    - `nxps32k358_lpuart_can_receive` for flow control
    - `nxps32k358_lpuart_receive` for data input
  - Enables backend to accept input immediately

#### `nxps32k358_lpuart_init()`

- **Purpose**: Initializes device instance
- **Functionality**:
  - Initializes IRQ line
  - Maps 16KB memory region for registers
  - Initializes clock input
  - Sets up memory region operations (`nxps32k358_lpuart_ops`)

### Memory Region Operations

- **Read**: Implemented via `nxps32k358_lpuart_read()`
- **Write**: Implemented via `nxps32k358_lpuart_write()`
- **Endianness**: `DEVICE_NATIVE_ENDIAN`
- **Region Size**: 0x4000 (16KB)

### Property Definitions

- **chardev**: Specifies the character device backend for serial I/O

---

## Register Map

| Register | Offset | Description                  |
|----------|--------|------------------------------|
| VERID    | 0x00   | Version ID                   |
| PARAM    | 0x04   | Parameter information        |
| GLOBAL   | 0x08   | Global Control               |
| PINCFG   | 0x0C   | Pin Configuration            |
| BAUD     | 0x10   | Baud Rate Configuration      |
| STAT     | 0x14   | Status Register              |
| CTRL     | 0x18   | Control Register             |
| DATA     | 0x1C   | Data Register                |
| MATCH    | 0x20   | Match Address                |
| MODIR    | 0x24   | Modem/IRDA Configuration     |
| FIFO     | 0x28   | FIFO Configuration           |
| WATER    | 0x2C   | Watermark Configuration      |

---

## Key Features

### Baud Rate Configuration

- Programmable baud rate via SBR (13-bit divisor) and OSR (5-bit oversampling ratio)
- Automatic parameter update when BAUD register is modified
- Direct clock input from SoC clock tree

### Interrupt Handling

- Configurable interrupt sources:
  - Transmit Data Register Empty (TIE)
  - Transmission Complete (TCIE)
  - Receive Data Register Full (RIE)
- Automatic IRQ line management based on enabled interrupts and status flags

### Character Backend Integration

- Seamless integration with QEMU's character device system
- Supports various backends: stdio, TCP, PTY, file, etc.
- Automatic flow control via `can_receive` callback

### Reset Behavior

- Software reset via GLOBAL register
- Registers initialize to documented reset values
- Clears all status flags and pending interrupts

### Input/Output Handling

- Synchronous character transmission
- Interrupt-driven reception
- Status flags automatically updated during I/O operations
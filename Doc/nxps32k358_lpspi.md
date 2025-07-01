# NXP S32K358 LPSPI Controller Documentation

## Overview

The NXP S32K358 LPSPI (Low Power Serial Peripheral Interface) is a QEMU device model that emulates the SPI peripheral of the S32K358 microcontroller. It supports full-duplex SPI communication with 4-word deep TX/RX FIFOs, 1-4096 bit frame size support (limited to 32-bit words), full interrupt support, configurable watermarks, and proper clock configuration with FreeRTOS compatibility.

---

## Header File: `nxps32k358_lpspi.h`

### Key Definitions

-   **Type Definition**: `TYPE_NXPS32K358_LPSPI` - QEMU object type string for the device.
-   **Register Offsets**: Defines the memory-mapped offsets for the LPSPI registers (`S32K_LPSPI_*`).
-   **Bit Masks**: Provides bit-level definitions for control and status registers.
-   **Shift Macros**: Provides bit position shift values for multi-bit fields.
-   **FIFO Depth**: Defines the FIFO depth and capacity.

### Key Constants

-   **Control Register (CR) Bits**:
    -   `LPSPI_CR_MEN`: Module Enable.
    -   `LPSPI_CR_RST`: Software Reset.
    -   `LPSPI_CR_RTF`: Reset Transmit FIFO.
    -   `LPSPI_CR_RRF`: Reset Receive FIFO.
-   **Status Register (SR) Bits**:
    -   `LPSPI_SR_TDF`: Transmit Data Flag.
    -   `LPSPI_SR_RDF`: Receive Data Flag.
    -   `LPSPI_SR_WCF`: Word Complete Flag.
    -   `LPSPI_SR_FCF`: Frame Complete Flag.
    -   `LPSPI_SR_TCF`: Transfer Complete Flag.
    -   `LPSPI_SR_TEF`: Transmit Error Flag.
    -   `LPSPI_SR_REF`: Receive Error Flag.
    -   `LPSPI_SR_DMF`: Data Match Flag.
    -   `LPSPI_SR_MBF`: Module Busy Flag.
-   **Receive Status Register (RSR) Bits**:
    -   `LPSPI_RSR_RXEMPTY`: RX FIFO Empty Flag.
-   **Transmit Command Register (TCR) Bits**:
    -   `TCR_FRAMESZ_MASK`: Frame Size field (12 bits, 1-4096 bit frames).
    -   `TCR_PCS_MASK`: Peripheral Chip Select field.
    -   `TCR_TXMSK`: Transmit Data Mask.
    -   `TCR_RXMSK`: Receive Data Mask.
    -   `TCR_LSBF`: LSB First transfer mode.
    -   `TCR_CONT`: Continuous transfer mode.
    -   `TCR_CONTC`: Continuing Command.
    -   `TCR_PRESCALE_MASK`: Clock prescaler field.
    -   `TCR_CPOL`: Clock Polarity (used for clock config).
    -   `TCR_CPHA`: Clock Phase (used for clock config).
-   **FIFO Configuration**:
    -   `LPSPI_FIFO_WORD_DEPTH`: 4 words deep.
    -   `LPSPI_FIFO_BYTE_CAPACITY`: 16 bytes total capacity.
-   **Clock Configuration (CCR1) Bits**:
    -   `CCR1_SCKSET_MASK`: SCK setup time.
    -   `CCR1_SCKHLD_MASK`: SCK hold time.

### Complete Macro Reference

#### Register Offset Macros

-   `S32K_LPSPI_VERID` (0x00): Version ID register offset
-   `S32K_LPSPI_PARAM` (0x04): Parameter register offset
-   `S32K_LPSPI_CR` (0x10): Control register offset
-   `S32K_LPSPI_SR` (0x14): Status register offset
-   `S32K_LPSPI_IER` (0x18): Interrupt enable register offset
-   `S32K_LPSPI_DER` (0x1C): DMA enable register offset
-   `S32K_LPSPI_CFGR0` (0x20): Configuration register 0 offset
-   `S32K_LPSPI_CFGR1` (0x24): Configuration register 1 offset
-   `S32K_LPSPI_CCR` (0x40): Clock configuration register offset
-   `S32K_LPSPI_CCR1` (0x44): Clock configuration register 1 offset
-   `S32K_LPSPI_FCR` (0x58): FIFO control register offset
-   `S32K_LPSPI_FSR` (0x5C): FIFO status register offset
-   `S32K_LPSPI_TCR` (0x60): Transmit command register offset
-   `S32K_LPSPI_TDR` (0x64): Transmit data register offset
-   `S32K_LPSPI_RSR` (0x70): Receive status register offset
-   `S32K_LPSPI_RDR` (0x74): Receive data register offset

#### TCR Register Field Shift Macros

-   `TCR_FRAMESZ_SHIFT` (0): Frame size field bit position
-   `TCR_PCS_SHIFT` (24): Peripheral chip select field bit position
-   `TCR_PRESCALE_SHIFT` (27): Clock prescaler field bit position

#### FSR Register Field Shift Macros

-   `FSR_TXCOUNT_SHIFT` (0): TX FIFO count field bit position
-   `FSR_RXCOUNT_SHIFT` (16): RX FIFO count field bit position

#### FCR Register Field Macros

-   `LPSPI_FCR_TXWATER_SHIFT` (0): TX watermark field bit position
-   `LPSPI_FCR_TXWATER_MASK`: TX watermark field mask
-   `LPSPI_FCR_RXWATER_SHIFT` (16): RX watermark field bit position
-   `LPSPI_FCR_RXWATER_MASK`: RX watermark field mask

#### CFGR1 Register Field Macros

-   `LPSPI_CFGR1_PINCFG_SHIFT` (8): Pin configuration field bit position
-   `LPSPI_CFGR1_PINCFG_MASK`: Pin configuration field mask
-   `LPSPI_CFGR1_PCSPOL_SHIFT` (24): PCS polarity field bit position
-   `CFGR1_PCSPOL_SHIFT` (8): Alternate PCS polarity field bit position

#### CCR1 Register Field Shift Macros

-   `CCR1_SCKSET_SHIFT` (0): SCK setup time field bit position
-   `CCR1_SCKHLD_SHIFT` (8): SCK hold time field bit position

### Key Structures

-   **`NXPS32K358LPSPIState`**: Represents the state of the LPSPI device, including:
    -   **Parent Object**: `SysBusDevice parent_obj`.
    -   **Clock Input**: `Clock *clk` for clock frequency management.
    -   **Memory Region**: `MemoryRegion mmio`.
    -   **SSIBus**: `SSIBus *ssi`.
    -   **IRQ Line**: `qemu_irq irq`.
    -   **Chip Select Lines**: `uint8_t num_cs_lines` and `qemu_irq *cs_lines`.
    -   **FIFO Buffers**: `Fifo8 tx_fifo` and `Fifo8 rx_fifo`.
    -   **Registers**: All 16 LPSPI registers (VERID, PARAM, CR, SR, etc.).
    -   **State Variables**:
        -   `bool busy`: Transfer busy state.
        -   `uint8_t tx_watermark, rx_watermark`: FIFO watermark levels.
        -   `uint16_t frame_size`: Current frame size in bits.
        -   `bool continuous_mode`: Continuous transfer mode.
        -   `uint64_t input_clk`: Input clock frequency.

---

## Source File: `nxps32k358_lpspi.c`

### Debug Logging

The driver uses a macro `DB_PRINT_L` to conditionally emit debug logs. This macro wraps `qemu_log()` and only logs when `NXP_LPSPI_ERR_DEBUG` is enabled. It is primarily used for tracing register accesses, FIFO operations, and error conditions.

### Key Functions

### Key Functions

#### `lpspi_update_clock_config()`

-   **Purpose**: Updates the SPI clock configuration based on the current TCR and CCR1 register values, calculating the actual SCK frequency and logging the clock parameters for debugging.

-   **Functionality**:
    -   Reads the input clock frequency from the clock source.
    -   Calculates prescaler division from TCR register.
    -   Extracts SCK setup and hold times from CCR1 register.
    -   Computes final SCK frequency: `input_clk / (prescaler * (setup + hold + 2))`.
    -   Extracts CPOL and CPHA settings from TCR register.
    -   Logs complete clock configuration for verification.

#### `lpspi_update_status()`

-   **Purpose**: Updates FIFO status flags and word counts based on current frame size and FIFO contents, ensuring proper TDF/RDF flag management with watermark support.

-   **Functionality**:
    -   Calculates frame size in bytes from TCR register (1-4096 bits, limited to 32-bit words).
    -   Computes TX and RX word counts based on frame size.
    -   Updates FSR register with word counts.
    -   Sets TDF flag when TX FIFO words ≤ watermark and has space.
    -   Sets RDF flag when RX FIFO words > watermark and has data.
    -   Updates RSR register with RX FIFO empty status.

#### `lpspi_flush_txfifo()`

-   **Purpose**: Handles SPI data transfers by moving data from TX FIFO to RX FIFO via the SPI bus, with proper chip select management and frame size validation.

-   **Functionality**:
    -   Validates frame size (1-4096 bits) and calculates bytes per frame.
    -   Checks if sufficient data exists in TX FIFO and space in RX FIFO.
    -   Extracts and validates chip select value from TCR register.
    -   Asserts appropriate chip select line.
    -   Transfers data in frame-sized chunks via `ssi_transfer()`.
    -   Handles MSB/LSB first mode and continuous transfer mode.
    -   Deasserts chip select and updates status flags.
    -   Clears MBF flag when TX FIFO becomes empty.

#### `nxps32k358_lpspi_do_reset()`

-   **Purpose**: Resets the LPSPI device to proper S32K358 default values as specified in the reference manual.

-   **Functionality**:
    -   Sets VERID to `0x04040007` (S32K358 specific version).
    -   Sets PARAM to `0x00040404` (4-word FIFOs, 4 PCS lines).
    -   Sets TCR to `0x0000001F` (32-bit default frame size).
    -   Initializes status flags: TDF=1, TCF=1 (ready for transmission).
    -   Sets RSR to RXEMPTY (receive FIFO empty).
    -   Resets both TX and RX FIFOs and deasserts all CS lines.
    -   Updates clock configuration and interrupt status.

#### `nxps32k358_lpspi_realize()`

-   **Purpose**:
    Initializes the NXPS32K358 LPSPI device during its realization phase, setting up all required hardware structures such as memory regions, interrupt lines, chip select GPIOs, and FIFO buffers. This function ensures the device is fully integrated into the QEMU virtual hardware environment.

-   **Functionality**:
    Performs the following initialization steps in sequence:

1. **Memory Region Initialization**:

    - Initializes the memory-mapped I/O region for the device with `memory_region_init_io()`.
    - Binds the region to the device state (`OBJECT(s)`) and register access callbacks (`nxps32k358_lpspi_ops`).
    - Specifies the size of the region via `S32K_LPSPI_REG_MAX_OFFSET`.
    - Registers the region with the system bus using `sysbus_init_mmio()` to allow access by other components.

2. **SPI Bus Creation**:

    - Creates and attaches a serial peripheral interface (SPI) bus to the device using `ssi_create_bus()`.
    - Associates the bus with the QEMU device object (`dev`) and assigns it the name `"spi"` for identification.

3. **IRQ Initialization**:

    - Initializes the IRQ line (Interrupt request line) with `sysbus_init_irq()`, enabling the device to signal interrupts to the virtual CPU.

4. **Chip Select (CS) Line Configuration**:

    - Allocates memory for chip select GPIO lines based on `s->num_cs_lines` using `g_new0()`.
    - Initializes these lines as GPIO outputs with `qdev_init_gpio_out_named()`, tagging them with the name `"cs"` for each chip select.

5. **FIFO Buffer Initialization**:

    - Initializes the transmit (TX) FIFO using `fifo8_create()` with a predefined capacity (`LPSPI_FIFO_BYTE_CAPACITY`).
    - Initializes the receive (RX) FIFO using the same capacity, ensuring symmetric buffer sizing for full-duplex transfers.

6. **Error Handling**:

    - Ensures each subsystem (memory, bus, IRQ, GPIOs, FIFOs) is initialized in order.
    - If any step fails, the realization halts and logs an appropriate error to aid in debugging and diagnosis.

-   **Debug Logging**:

    -   Provides logs for memory region creation, IRQ setup, GPIO configuration, and FIFO initialization.
    -   Logs errors in case of allocation or setup failures to assist in tracing device bring-up issues.

#### `nxps32k358_lpspi_reset()`

-   **Purpose**:
    This function resets the NXPS32K358 LPSPI device to its default state. It is typically called during system initialization or when a reset condition is triggered.
-   **Functionality**:
    -   Calls `nxps32k358_lpspi_do_reset()` to perform the reset operation.

#### `nxps32k358_lpspi_read()`

-   **Purpose**:  
    Reads data from the specified memory-mapped register of the NXPS32K358 LPSPI device.

-   **Functionality**:

    -   Determines the register to read based on the provided `addr` offset.
    -   Returns the value of the corresponding register.
    -   Handles special cases:
        -   For the `RDR` (Receive Data Register), it reads 4 bytes from the RX FIFO, combines them into a 32-bit value, and updates the `RDR` register.
        -   If the RX FIFO has fewer than 4 bytes, it returns `0`.
    -   Logs an error if the `addr` does not match any valid register offset.

-   **Key Steps**:

    1. Calls `lpspi_update_status()` to ensure the device state is up-to-date.
    2. Uses a `switch` statement to map the `addr` to the corresponding register.
    3. For the `RDR` case:
        - Checks if the RX FIFO contains at least 4 bytes.
        - Pops 4 bytes from the RX FIFO, combines them into a 32-bit value, and updates the `RDR` register.
        - Calls `lpspi_flush_txfifo()` to handle any pending SPI transfers.
    4. Logs an error for invalid `addr` values.

-   **Registers Handled**:

    -   `VERID`, `PARAM`, `CR`, `SR`, `IER`, `DER`, `CFGR0`, `CFGR1`, `CCR`, `CCR1`, `FCR`, `FSR`, `TCR`, `RSR`, `RDR`.

-   **Error Handling**:

    -   Logs an error message if the `addr` is invalid.

-   **Return Value**:

    -   The value of the specified register, or `0` for invalid reads or empty RX FIFO.

#### `nxps32k358_lpspi_write()`

-   **Purpose**:  
    Writes data to the specified memory-mapped register of the NXPS32K358 LPSPI device.

-   **Functionality**:

    -   Determines the register to write to based on the provided `addr` offset.
    -   Updates the value of the corresponding register.
    -   Handles special cases:
        -   Logs an error if attempting to write to a read-only register.
        -   Resets the device if the `RST` bit in the `CR` register is set.
        -   Resets the FIFOs if the `RTF` or `RRF` bits in the `CR` register are set.
        -   Handles FIFO operations for the `TDR` (Transmit Data Register).
        -   Updates the IRQ line after each write.

-   **Key Steps**:

    1. Uses a `switch` statement to map the `addr` to the corresponding register.
    2. For read-only registers (`VERID`, `PARAM`, `FSR`, `RSR`, `RDR`):
        - Logs an error and ignores the write.
    3. For the `CR` (Control Register):
        - Resets the device if the `RST` bit is set.
        - Resets the FIFOs if the `RSTF` bit is set.
        - Updates the `CR` register value.
    4. For the `SR` (Status Register):
        - Clears the specified bits in the `SR` register.
    5. For the `TCR` (Transmit Command Register):
        - Updates the `TCR` register value.
        - If the module is enabled (`MEN` bit in `CR`), checks the FIFO state and flushes the TX FIFO.
    6. For the `TDR` (Transmit Data Register):
        - Writes data to the TX FIFO if the module is enabled.
        - Logs an error if the TX FIFO is full or the module is not enabled.
    7. For other writable registers (`IER`, `DER`, `CFGR0`, `CFGR1`, `CCR`, `CCR1`, `FCR`):
        - Updates the corresponding register value.
    8. Logs an error for invalid `addr` values.
    9. Calls `lpspi_update_irq()` to update the IRQ line.

-   **Registers Handled**:

    -   **Read-Only**: `VERID`, `PARAM`, `FSR`, `RSR`, `RDR`.
    -   **Writable**: `CR`, `SR`, `TCR`, `TDR`, `IER`, `DER`, `CFGR0`, `CFGR1`, `CCR`, `CCR1`, `FCR`.

-   **Error Handling**:

    -   Logs an error message if attempting to write to a read-only register.
    -   Logs an error if the TX FIFO is full or the module is not enabled when writing to `TDR`.
    -   Logs an error for invalid `addr` values.

#### `nxps32k358_lpspi_class_init()`

-   **Purpose**: This function initializes the class-level properties of the NXP S32K358 LPSPI device type, preparing it for use within the QEMU device model infrastructure.

-   **Functionality**:

    -   **Device Class Casting**:

        -   Casts the provided `ObjectClass` to a `DeviceClass` to access standard device initialization hooks.

    -   **Realize Function Assignment**:

        -   Sets the `realize` function pointer to `nxps32k358_lpspi_realize`, defining how the device is instantiated and initialized at runtime.

    -   **Legacy Reset Handler**:

        -   Assigns `nxps32k358_lpspi_reset` as the legacy reset function, enabling the device to support system resets.

    -   **Device Properties Setup**:

        -   Registers the property list of the device by calling `device_class_set_props` with `nxps32k358_lpspi_properties`.

    -   **VMState Description Assignment**:

        -   Sets the `vmsd` field to `vmstate_nxps32k358_lpspi`, allowing the device to support VM snapshot and migration by describing how its state should be saved and restored.

#### `nxps32k358_lpspi_register_types()`

-   **Purpose**: Registers the NXP S32K358 LPSPI device type with the QEMU type system so it can be instantiated and used during emulation.

-   **Functionality**:

    -   Calls `type_register_static()` with the `nxps32k358_lpspi_info` structure to make the device type available to the QEMU object model.

### Key Constants

#### Memory Region Operations

-   **Read**: Implemented via `nxps32k358_lpspi_read()`.
-   **Write**: Implemented via `nxps32k358_lpspi_write()`.
-   **Endianess**: Default to little-endian for 32-bit accesses.
-   **Minimum Access Size**: 4 bytes.
-   **Maximum Access Size**: 4 bytes.

### VMState Description

-   **Name**: The name of the VMState structure, set to "nxps32k358_lpspi".
-   **Version**: The version of the VMState structure, set to 1.
-   **Fields**: An array of VMStateField structures that describe the individual fields to be saved and restored during migration. These fields include:
    -   tx_fifo: Transmit FIFO buffer.
    -   rx_fifo: Receive FIFO buffer.
    -   lpspi_verid: Version ID register.
    -   lpspi_param: Parameter register.
    -   lpspi_cr: Control register.
    -   lpspi_sr: Status register.
    -   lpspi_ier: Interrupt enable register.
    -   lpspi_der: DMA enable register.
    -   lpspi_cfgr0: Configuration register 0.
    -   lpspi_cfgr1: Configuration register 1.
    -   lpspi_ccr: Clock configuration register.
    -   lpspi_ccr1: Clock configuration register 1.
    -   lpspi_fcr: FIFO control register.
    -   lpspi_fsr: FIFO status register.
    -   lpspi_tcr: Transmit command register.
    -   lpspi_tdr: Transmit data register.
    -   lpspi_rsr: Receive status register.
    -   lpspi_rdr: Receive data register.
    -   VMSTATE_END_OF_LIST(): Marks the end of the field list.

### Property Definitions

-   **num-cs-lines**: Specifies the number of chip select lines available for the LPSPI device. This property is defined with a default value of 1, indicating that at least one chip select line is required.

---

## Register Map

| Register | Offset | Description                    |
| -------- | ------ | ------------------------------ |
| VERID    | 0x00   | Version ID                     |
| PARAM    | 0x04   | Parameter information          |
| CR       | 0x10   | Control Register               |
| SR       | 0x14   | Status Register                |
| IER      | 0x18   | Interrupt Enable Register      |
| DER      | 0x1C   | DMA Enable Register            |
| CFGR0    | 0x20   | Configuration Register 0       |
| CFGR1    | 0x24   | Configuration Register 1       |
| CCR      | 0x40   | Clock Configuration Register   |
| CCR1     | 0x44   | Clock Configuration Register 1 |
| FCR      | 0x58   | FIFO Control Register          |
| FSR      | 0x5C   | FIFO Status Register           |
| TCR      | 0x60   | Transmit Command Register      |
| TDR      | 0x64   | Transmit Data Register         |
| RSR      | 0x70   | Receive Status Register        |
| RDR      | 0x74   | Receive Data Register          |

---

## Key Features

### Advanced FIFO Management

-   4-word deep TX and RX FIFOs (16 bytes each).
-   Frame-size aware word counting (1-4096 bit frames, limited to 32-bit words).
-   Watermark-based TDF/RDF flag generation.
-   Automatic flushing when sufficient data is available and module is enabled.
-   Proper FIFO reset via RTF/RRF control bits.

### Comprehensive Interrupt Handling

-   Full status flag support: TDF, RDF, WCF, FCF, TCF, TEF, REF, DMF, MBF.
-   Watermark-based interrupt generation.
-   Configurable interrupt enables via IER register.
-   Proper IRQ assertion/deassertion based on enabled conditions.

### Advanced Clock Configuration

-   Input clock frequency management via Clock object.
-   Configurable prescaler division (TCR register).
-   SCK setup and hold time configuration (CCR1 register).
-   Real-time SCK frequency calculation and logging.
-   CPOL/CPHA clock polarity and phase support.

### Frame Size and Transfer Modes

-   Support for 1-4096 bit frame sizes (configurable via TCR).
-   MSB/LSB first transfer modes (LSBF bit).
-   Continuous transfer mode support (CONT/CONTC bits).
-   Transmit/Receive data masking (TXMSK/RXMSK bits).
-   Proper frame-size validation and byte-per-frame calculation.

### Chip Select Management

-   Multiple chip select lines support (configurable num_cs_lines).
-   PCS field validation against available CS lines.
-   Automatic CS assertion/deassertion during transfers.
-   Configurable CS polarity (future extension ready).

### S32K358-Specific Features

-   Correct VERID (0x04040007) and PARAM (0x00040404) values.
-   Default 32-bit frame size (TCR = 0x0000001F).
-   Proper reset state initialization per S32K358 reference manual.
-   FreeRTOS compatibility with 80MHz clock validation.

---

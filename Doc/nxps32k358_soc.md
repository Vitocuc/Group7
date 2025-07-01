# NXP S32K358 SoC Documentation

## Overview

The SoC files defines and implement the **NXP S32K358 System on Chip (SoC) model** for QEMU. These files describe the overall microcontroller, integrating the ARM Cortex-M7 CPU, memory regions (Flash, SRAM, TCM), and all on-chip peripherals (such as LPUARTs and LPSPIs) into a single, unified device.

This SoC model serves several key purposes:

-   Provide a complete hardware model of the S32K358 microcontroller, allowing QEMU to emulate the real chip's behavior.
-   Aggregate and connect all internal components (CPU, memory, peripherals) so that software running in QEMU interacts with them as it would on real hardware.
-   Set up the memory map, clock tree, and interrupt routing for the entire chip.
-   Serve as the foundation for board-level emulation, enabling the simulation of real-world embedded systems using the S32K358.
-   Allow QEMU to recognize, instantiate, and manage the SoC as a device, making it possible to run and debug firmware or operating systems designed for the S32K358 microcontroller.

## Header File: `nxps32k358_soc.h`

### Key Definitions

-   **`TYPE_NXPS32K358_SOC`**: The type name for the SoC device, defined as `"nxps32k358-soc"`.
-   **`NXP_NUM_LPUARTS`**: The number of LPUART peripherals (16).
-   **`NXP_NUM_LPSPIS`**: The number of LPSPI peripherals (6).

### Memory Region Base Addresses and Sizes

-   **`CODE_FLASH_BASE_ADDRESS`**: Base address for the code flash memory (0x00400000).
-   **`CODE_FLASH_BLOCK_SIZE`**: Size of each code flash block (2 MB).
-   **`DATA_FLASH_BASE_ADDRESS`**: Base address for the data flash memory (0x10000000).
-   **`DATA_FLASH_SIZE`**: Size of the data flash region (128 KB).
-   **`SRAM_BASE_ADDRESS`**: Base address for the SRAM region (0x20400000).
-   **`SRAM_BLOCK_SIZE`**: Size of each SRAM block (256 KB).
-   **`DTCM_BASE_ADDRESS`**: Base address for the Data Tightly Coupled Memory (DTCM) (0x20000000).
-   **`DTCM_SIZE`**: Size of the DTCM region (128 KB + 1 byte, note: the +1 is unusual and might be an error).
-   **`ITCM_BASE_ADDRESS`**: Base address for the Instruction Tightly Coupled Memory (ITCM) (0x00000000).
-   **`ITCM_SIZE`**: Size of the ITCM region (64 KB).

### Key Structures

-   **`NXPS32K358State`**: Represents the state of the SoC device, including:
    -   **Parent Object**: `SysBusDevice parent_obj`.
    -   **ARMv7M State**: `ARMv7MState armv7m` for the Cortex-M7 core.
    -   **SYSCFG**: `NXPS32K358SYSCFGState syscfg` for the system configuration controller.
    -   **LPUARTs**: Array of `NXPS32K358LPUARTState lpuarts[NXP_NUM_LPUARTS]`.
    -   **LPSPIs**: Array of `NXPS32K358LPSPIState lpspis[NXP_NUM_LPSPIS]`.
    -   **ADC IRQs**: `OrIRQState *adc_irqs` enables interrupt handling for the ADC (Analog to Digital Converter) peripherals.
    -   **Memory Regions**:
        -   `code_flash_0`, `code_flash_1`, `code_flash_2`, `code_flash_3`: Code flash blocks.
        -   `data_flash`: Data flash region.
        -   `sram_0`, `sram_1`, `sram_2`: SRAM blocks.
        -   `dtcm`: DTCM region.
        -   `itcm`: ITCM region.
    -   **Clocks**:
        -   `sysclk`: Main system clock.
        -   `refclk`: Reference clock (derived from `sysclk` with a divisor of 8).
        -   `aips_plat_clk`: AIPS platform clock (80 MHz).
        -   `aips_slow_clk`: AIPS slow clock (40 MHz).

---

## Source File: `nxps32k358_soc.c`

### Key Functions

#### `create_unimplemented_devices()`

-   **Purpose**: Creates unimplemented devices for a large set of peripherals that are present in the actual hardware but not yet implemented in QEMU. This ensures that accesses to these regions do not cause bus errors and can be logged for debugging. It maps the memory regions of these peripherals to a "unimplemented" device, allowing the system to continue functioning without crashing.

-   **Functionality**:

    -   Calls `create_unimplemented_device()` for each peripheral, specifying:
        -   A name string (for identification in logs).
        -   The base address of the peripheral.
        -   The size of the memory region (typically 0x4000, 16KB, but some are 64KB).

-   **Note**: The function covers a wide range of peripherals including timers, analog to digital converters, communication devices, DMA, memory/bus, security (erm0, erm1,fccu_m, mc_rgm, stcu, selftest_gpr), and other type of devices.

#### `nxps32k358_soc_initfn()`

-   **Purpose**: Initializes the SoC device and its child objects during instance creation.

-   **Functionality**:
    -   Initializes the ARMv7-M object (`armv7m`).
    -   Initializes the system configuration controller (`syscfg`).
    -   Initializes the input clocks:
        -   `sysclk`: Main system clock.
        -   `refclk`: Reference clock (derived from `sysclk`).
        -   `aips_plat_clk`: AIPS platform clock.
        -   `aips_slow_clk`: AIPS slow clock.
    -   Initializes the LPUART and LPSPI child objects in arrays.

#### `nxps32k358_soc_realize()`

-   **Purpose**: Realizes the SoC device, including setting up memory regions, clocks, and realizing child devices. This function is called when the device is instantiated and performs the main setup, after the initialization the purpose of the realization is to connect all components together and prepare the SoC in the QEMU environment so that it can be used for emulation.

-   **Functionality**:
    -   **Clock Setup**:
        -   Checks that `sysclk` is connected (by board code) and `refclk` is not externally connected.
        -   Sets the `refclk` to run at `sysclk` frequency divided by 8.
        -   Sets the frequencies for `aips_plat_clk` (80 MHz) and `aips_slow_clk` (40 MHz). Clocks for Advanced IP bus Pheripheral Subsystems, bus used in NXP microcontrollers.
    -   **Memory Region Setup**:
        -   Initializes and maps the code flash blocks (4 blocks of 2 MB each at base address 0x00400000).
        -   Initializes and maps the data flash region (128 KB at 0x10000000).
        -   Initializes and maps the SRAM blocks (3 blocks of 256 KB at 0x20400000).
        -   Initializes and maps the DTCM (128 KB + 1 at 0x20000000) and ITCM (64 KB at 0x00000000).
-                       -   Sets the memory link to the system memory.
                        -   Realizes the ARMv7-M object.

    -   **System Configuration Controller (SYSCFG)**:

        -   Connects the SYSCFG's clock input to the main system clock (`sysclk`).
        -   Realizes (initializes and activates) the SYSCFG device so it becomes part of the emulated hardware.
        -   Maps the SYSCFG's memory-mapped I/O region to address `0x40013800` in the system's memory space.

    -   **SYSCFG Setup**:
        -   Connects the `sysclk` to the SYSCFG device.
        -   Realizes the SYSCFG and maps it at address 0x40013800.
    -   **LPUART Setup**:
        -   For each LPUART:
            -   Sets the character device (for serial output).
            -   Connects the appropriate clock (`aips_plat_clk` for LPUARTs 0,1,8 and `aips_slow_clk` for the others).
            -   Realizes the device and maps it to its base address (from `lpuart_addr` array).
            -   Connects the IRQ (from `lpuart_irq` array) to the ARMv7-M NVIC.
    -   **LPSPI Setup**:
        -   For each LPSPI:
            -   Realizes the device and maps it to its base address (from `lpspi_addr` array).
            -   Connects the IRQ (from `lpspi_irq` array) to the ARMv7-M NVIC.
    -   **Unimplemented Devices**:
        -   Calls `create_unimplemented_devices()` to cover the rest of the peripherals.

#### `nxps32k358_soc_class_init()`

-   **Purpose**: Initializes the class for the SoC device.

-   **Functionality**:
    -   Sets the `realize` method to `nxps32k358_soc_realize`.

#### `nxps32k358_soc_types()`

-   **Purpose**: Registers the SoC device type with QEMU.

-   **Functionality**:
    -   Calls `type_register_static()` with the `nxps32k358_soc_info` structure.

### Key Constants

#### Peripheral Base Addresses and IRQs

-   **LPUART Base Addresses**: Array `lpuart_addr` with 16 base addresses.
-   **LPUART IRQs**: Array `lpuart_irq` with 7 IRQ numbers (note: the array has 7 entries but there are 16 LPUARTs; this might be an error).
-   **LPSPI Base Addresses**: Array `lpspi_addr` with 6 base addresses.
-   **LPSPI IRQs**: Array `lpspi_irq` with 6 IRQ numbers.

### Memory Region Setup

The SoC sets up the following memory regions:

| Region     | Base Address | Size (per block) | Blocks | Total Size |
| ---------- | ------------ | ---------------- | ------ | ---------- |
| Code Flash | 0x00400000   | 2 MB             | 4      | 8 MB       |
| Data Flash | 0x10000000   | 128 KB           | 1      | 128 KB     |
| SRAM       | 0x20400000   | 256 KB           | 3      | 768 KB     |
| DTCM       | 0x20000000   | 128 KB + 1       | 1      | 128 KB + 1 |
| ITCM       | 0x00000000   | 64 KB            | 1      | 64 KB      |

### Clock Setup

-   **`sysclk`**: Must be provided by the board. This is the main CPU clock.
-   **`refclk`**: Derived from `sysclk` by dividing by 8. Used internally by the CPU and peripherals.
-   **`aips_plat_clk`**: Set to 80 MHz. Used by some peripherals (LPUARTs 0,1,8).
-   **`aips_slow_clk`**: Set to 40 MHz. Used by other peripherals (LPUARTs 2-7,9-15).

---

## Key Features

### Memory Map

The SoC provides a comprehensive memory map including:

-   **Code Flash**: 8 MB in 4 blocks, starting at 0x00400000.
-   **Data Flash**: 128 KB at 0x10000000.
-   **SRAM**: 768 KB in 3 blocks, starting at 0x20400000.
-   **DTCM**: 128 KB at 0x20000000 (plus 1 byte, which might be a mistake).
-   **ITCM**: 64 KB at 0x00000000.

### Peripheral Integration

-   **ARM Cortex-M7**: The main CPU core.
-   **SYSCFG**: System configuration controller at 0x40013800.
-   **16 LPUARTs**: Mapped at addresses from the `lpuart_addr` array, with IRQs from `lpuart_irq` (note: the IRQ array only has 7 entries, so this might be incomplete).
-   **6 LPSPIs**: Mapped at addresses from the `lpspi_addr` array, with IRQs from `lpspi_irq`.

### Unimplemented Peripherals

A large set of peripherals are marked as unimplemented, ensuring that accesses to their memory regions do not cause bus errors. These include:

-   Timers (PIT, STM, SWT)
-   Communication interfaces (FlexCAN, FlexIO, SAI, EMAC, GMAC)
-   Analog (ADC)
-   Safety and security (ERM, BCU, WKPU)
-   Clock and reset (CMU, MC_RGM, PLL)
-   And many more.

### Clock Management

The SoC has a flexible clock setup with multiple derived clocks for different peripheral groups.

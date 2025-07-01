# NXP S32K358 Discovery Documentation

## Overview

The `nxps32k358discovery.c` file implements the **NXP S32K358 Discovery board** machine model for QEMU. This file defines how the physical development board is emulated, integrating the NXP S32K358 SoC, clock sources, and firmware loading mechanisms. Its primary role is to:

-   Configure the board-level hardware environment.
-   Instantiate and connect the S32K358 SoC device.
-   Load firmware/kernel images into the emulated flash memory.
-   Set up fixed-frequency clocks required by the SoC.

---

## Key Definitions

-   **`SYSCLK_FRQ`**:  
    Fixed frequency of the main system clock (`24 MHz`), defined as `24000000ULL` (unsigned long long). This clock drives the SoC's primary timing source.

---

## Key Functions

>

### `nxp_s32k358discovery_init(MachineState *machine)`

-   **Purpose**:  
    Initializes the Discovery board hardware during QEMU machine startup.
-   **Functionality**:
    1. **Clock Setup**:
        - Creates a fixed-frequency clock `sysclk` at 24 MHz using `clock_new()`.
        - Connects this clock to the SoC's `sysclk` input via `qdev_connect_clock_in()`.
    2. **SoC Initialization**:
        - Instantiates the S32K358 SoC device (`TYPE_NXPS32K358_SOC`).
        - Attaches the SoC as a child of the machine using `object_property_add_child()`.
        - Realizes the SoC device with `sysbus_realize_and_unref()`.
    3. **Firmware Loading**:
        - Loads a kernel/firmware image into the SoC's _code flash memory_:
            - Base Address: `CODE_FLASH_BASE_ADDRESS` (`0x00400000`).
            - Size: `CODE_FLASH_BLOCK_SIZE * 4` (8 MB total).
        - Uses `armv7m_load_kernel()` to handle ARMv7-M CPU boot mechanics.

### `nxp_s32k358discovery_machine_init(MachineClass *mc)`

-   **Purpose**:  
    Configures the QEMU machine class for the Discovery board.
-   **Functionality**:
    -   Sets machine metadata:
        -   Description: `"NXP NXPS32K358 (Cortex-M7)"`.
        -   Valid CPU types: Only `ARM_CPU_TYPE_NAME("cortex-m7")` is allowed.
        -   Disables unused peripherals: Floppy, CD-ROM, parallel port (`no_floppy=1`, `no_cdrom=1`, `no_parallel=1`).
    -   Registers the board initialization function `nxp_s32k358discovery_init` as the machine's entry point.

### `DEFINE_MACHINE("nxps32k358evb", ...)`

-   **Purpose**:  
    Registers the board with QEMU's machine registry.
-   **Key Detail**:  
    The machine is named `"nxps32k358evb"`. When selected (e.g., via `-M nxps32k358evb`), QEMU uses this board configuration.

---

## Key Features

1. **Machine Creation**:
    - QEMU initializes the machine using `nxps32k358evb`.
2. **Board Setup**:
    - The 24 MHz system clock (`sysclk`) is created and connected to the SoC.
    - The SoC device is instantiated and realized (triggering its internal setup).
3. **Firmware Execution**:
    - If a kernel is provided (e.g., `-kernel <firmware.bin>`), it is loaded at `0x00400000` (start of code flash).
    - The ARMv7-M CPU begins execution from this address.

---

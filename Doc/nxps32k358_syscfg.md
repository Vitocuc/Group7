# NXP S32K358 SYSCFG (System Configuration Controller) Documentation

## Overview

The `nxps32k358_syscfg.c` file implements QEMU's **System Configuration Controller (SYSCFG)** for the NXP S32K358 SoC. This device manages critical system-level configurations including memory remapping, security settings, and system control/status registers. Key responsibilities include:

-   Handling register read/write operations
-   Resetting registers to hardware defaults
-   Validating clock dependencies
-   Supporting VM state migration

---

## Key Definitions

### **Base Address**

-   `SYSCFG_BASE_ADDR` (`0x40268000`)  
    Memory-mapped I/O base address for SYSCFG registers.

### **Register Offsets**

-   `SYSCFG_CFGR1` (`0x00`): Configuration Register 1 (peripheral/I/O settings)
-   `SYSCFG_MEMRMP` (`0x04`): Memory Remap Register (memory space configuration)
-   `SYSCFG_SCSR` (`0x08`): System Control/Status Register (runtime status flags)
-   `SYSCFG_CFGR2` (`0x0C`): Configuration Register 2 (secondary settings)
-   `SYSCFG_SKR` (`0x10`): Security Key Register (security/access control)

### **Register Bitmasks**

-   `ACTIVABLE_BITS_MEMRP` (`0x000000FF`):  
    Writable bits in `MEMRMP` (reserved bits protected)
-   `ACTIVABLE_BITS_CFGR1` (`0x0000FFFF`):  
    Writable bits in `CFGR1` (reserved bits protected)
-   `ACTIVABLE_BITS_SKR` (`0x000000FF`):  
    Writable bits in `SKR` (security key limited to 8 bits)

### **Device Type**

-   `TYPE_NXPS32K358_SYSCFG` (`"nxps32k358-syscfg"`)  
    QEMU device type identifier for SYSCFG.

---

## Key Functions

### `nxps32k358_syscfg_hold_reset(Object *obj, ResetType type)`

-   **Purpose**:  
    Resets all SYSCFG registers to their hardware default values during power-on or system reset.
-   **Functionality**:  
    Sets registers to predefined initial states:
    -   `memrmp = 0x00000000` (no memory remapping)
    -   `cfgr1 = 0x7C000001` (default I/O and peripheral config)
    -   `scsr = 0x00000000` (clear system status)
    -   `cfgr2 = 0x00000000` (reset configuration flags)
    -   `skr = 0x00000000` (clear security key)

### `nxps32k358_syscfg_read(void *opaque, hwaddr addr, unsigned int size)`

-   **Purpose**:  
    Handles read operations from SYSCFG registers.
-   **Functionality**:
    1.  Uses `addr` to identify the target register
    2.  Returns current value of:
        -   `SYSCFG_MEMRMP` (memory remap settings)
        -   `SYSCFG_CFGR1` (configuration flags)
        -   `SYSCFG_SCSR` (system status)
        -   `SYSCFG_CFGR2` (secondary config)
        -   `SYSCFG_SKR` (security key)
    3.  Logs "guest error" for invalid/unimplemented addresses

### `nxps32k358_syscfg_write(void *opaque, hwaddr addr, uint64_t value, unsigned int size)`

-   **Purpose**:  
    Handles write operations to SYSCFG registers.
-   **Functionality**:
    1.  Masks `value` with register-specific bitmask (e.g., `ACTIVABLE_BITS_MEMRP`)
    2.  Updates register based on `addr`:
        -   Blocks unsupported features (memory remap/CFGRx) with "unimplemented" warnings
        -   Allows direct writes to `SCSR` and `CFGR2`
        -   Filters `SKR` writes to 8 valid bits
    3.  Logs "guest error" for invalid addresses

### `nxps32k358_syscfg_init(Object *obj)`

-   **Purpose**:  
    Initializes SYSCFG device during QEMU object creation.
-   **Functionality**:
    1.  Maps a **1KB memory region** (`0x400` bytes) for register access
    2.  Registers MMIO operations using `nxps32k358_syscfg_ops`
    3.  Initializes clock input `clk` for device timing

### `nxps32k358_syscfg_realize(DeviceState *dev, Error **errp)`

-   **Purpose**:  
    Validates device configuration during final setup.
-   **Functionality**:
    1.  Checks if clock input `clk` is connected
    2.  Throws error: `"SYSCFG: clk input must be connected"` if unconfigured
    3.  Aborts device initialization on failure

### `nxps32k358_syscfg_class_init(ObjectClass *klass, const void *data)`

-   **Purpose**:  
    Configures SYSCFG device class properties.
-   **Functionality**:
    1.  Binds `realize` method to `nxps32k358_syscfg_realize`
    2.  Registers reset handler `nxps32k358_syscfg_hold_reset`
    3.  Attaches VMState descriptor for migration support

### `nxps32k358_syscfg_register_types(void)`

-   **Purpose**:  
    Registers the SYSCFG device type with QEMU's type system.
-   **Functionality**:
    1.  Defines device type `TYPE_NXPS32K358_SYSCFG`
    2.  Specifies parent class (`TYPE_SYS_BUS_DEVICE`)
    3.  Sets instance size and initialization methods

---

## Key Features

1. **Guarded Register Access**:

    - Masks writes with `ACTIVABLE_BITS_*` to protect reserved bits
    - Blocks unsupported operations (e.g., memory remap) with warnings

2. **Reset Control**:

    - Implements full hardware reset sequence for registers

3. **Clock Dependency**:

    - Mandatory clock connection enforced during `realize()`

4. **VMState Support**:

    - Includes all registers + clock state in migration snapshots:
        ```c
        VMSTATE_UINT32(memrmp, ...),
        VMSTATE_UINT32(cfgr1, ...),
        ...
        VMSTATE_CLOCK(clk, ...)
        ```

5. **QEMU Integration**:
    - Inherits from SysBusDevice (MMIO/IRQ support)
    - Native-endian 32-bit register access

---

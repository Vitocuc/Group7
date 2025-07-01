# LPSPI Communication Demo Script

## Overview

Simple bash script to demonstrate the working LPSPI implementation in the NXP S32K358 QEMU emulation. Shows successful LPSPI peripheral configuration with 4-word deep TX/RX FIFOs, 1-4096 bit frame size support (limited to 32-bit words), full interrupt support, and proper 8-bit frame size operation.

---

## Usage

```bash
# Run the demo
cd LPSPI_Testing/scripts
./demo_spi_communication.sh
```

The script automatically:

1. Checks for required files (QEMU binary and ELF)
2. Changes to the correct directory (`/qemu/build`)
3. Runs QEMU with the exact command
4. Displays the QEMU debug output

## Expected Output

-   `lpspi_update_clock_config` messages show SPI clock configuration
-   `nxps32k358_lpspi_write` messages show register writes to LPSPI
-   `MASTER=1` confirms SPI master mode configuration
-   Clock frequency properly set to 20MHz

## Technical Details

The script executes this exact command:

```bash
cd /home/iaco/Polito/CAOS/Group7/qemu/build
./qemu-system-arm -M nxps32k358evb -nographic -kernel ../../Demo_FreeRTOS/DEBUG_QEMU/Demo_FreeRTOS.elf -serial none -serial none -serial none -serial mon:stdio -d guest_errors
```

## Files Validated

-   `qemu/hw/ssi/nxps32k358_lpspi.c`
-   `Demo_FreeRTOS/DEBUG_QEMU/Demo_FreeRTOS.elf`

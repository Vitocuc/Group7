# SPI Configuration Fix - 8-bit Frame Size Correction

## Critical Issue Discovered

Through QEMU comparison with the working manual version, we discovered that our FreeRTOS implementation had a **fundamental SPI configuration error**.

## Problem Analysis

### QEMU Output Comparison

**❌ Our Version (Broken)**:
```
nxps32k358_lpspi_write: TCR write: 0xc0a00000, calculated frame_size: 1
lpspi_flush_txfifo: SPI transfer: tx=0x000000b1 -> rx=0x00000000
```

**✅ Working Manual Version**:
```
nxps32k358_lpspi_write: TCR write: 0x00000007, calculated frame_size: 8
lpspi_flush_txfifo: SPI transfer (loopback): tx=0x00000010 -> rx=0x00000010
```

### Key Differences Identified

| Aspect | Our Version | Working Version | Impact |
|--------|-------------|----------------|---------|
| **Frame Size** | 1 bit | 8 bits | Data transfer broken |
| **TCR Value** | 0xc0a00000 | 0x00000007 | Wrong register config |
| **Loopback** | No (rx=0x00) | Yes (tx=rx) | No data return |
| **Data Pattern** | 0xb1, 0xa1... | 0x10, 0x11... | Pattern mismatch |
| **PINCFG** | 0 | 1 | Pin configuration |

## Root Cause

The SPI configuration files were generated with incorrect parameters:

1. **Frame Size**: Set to `1U` instead of `8U`
2. **TCR WIDTH**: Set to `0U` instead of `7U` (WIDTH = frame_size - 1)

## Files Modified

### 1. SPI Configuration File

**File**: `Demo_FreeRTOS/generate/src/Lpspi_Ip_Sa_PBcfg.c`

#### Master Device Configuration
```c
// BEFORE (1-bit transfers)
const Lpspi_Ip_ExternalDeviceType Lpspi_Ip_DeviceAttributes_SpiExternalDevice_0_Instance_2 =
{
    2U,  /* Instance */
    (uint32)(LPSPI_CCR_SCKPCS(39U) | LPSPI_CCR_PCSSCK(39U) | LPSPI_CCR_SCKDIV(38U) | LPSPI_CCR_DBT(38U)),
    (uint32)(LPSPI_TCR_WIDTH(0U) | LPSPI_TCR_CPOL(1U) | LPSPI_TCR_CPHA(1U) | LPSPI_TCR_PRESCALE(0U) | LPSPI_TCR_PCS(0U) | LPSPI_TCR_CONT(1U))
    // ...
};

// AFTER (8-bit transfers) ✅
const Lpspi_Ip_ExternalDeviceType Lpspi_Ip_DeviceAttributes_SpiExternalDevice_0_Instance_2 =
{
    2U,  /* Instance */
    (uint32)(LPSPI_CCR_SCKPCS(39U) | LPSPI_CCR_PCSSCK(39U) | LPSPI_CCR_SCKDIV(38U) | LPSPI_CCR_DBT(38U)),
    (uint32)(LPSPI_TCR_WIDTH(7U) | LPSPI_TCR_CPOL(1U) | LPSPI_TCR_CPHA(1U) | LPSPI_TCR_PRESCALE(0U) | LPSPI_TCR_PCS(0U) | LPSPI_TCR_CONT(1U))
    // ...
};
```

#### Device Parameters Configuration
```c
// BEFORE (1-bit frame size)
static Lpspi_Ip_DeviceParamsType Lpspi_Ip_DeviceParamsCfg[2U] =
{
    {
        (uint8)1U, /* Frame size */
        (boolean)TRUE, /*Lsb */
        (uint32)0U  /* Default Data */
        // ...
    },
    // ...
};

// AFTER (8-bit frame size) ✅
static Lpspi_Ip_DeviceParamsType Lpspi_Ip_DeviceParamsCfg[2U] =
{
    {
        (uint8)8U, /* Frame size */
        (boolean)TRUE, /*Lsb */
        (uint32)0U  /* Default Data */
        // ...
    },
    // ...
};
```

### 2. Application Code Updates

**File**: `Demo_FreeRTOS/src/main.c`

#### Data Pattern Update
```c
// BEFORE: Pattern that didn't match working version
for (uint16_t i = 0; i < SPI_BUFFER_SIZE; i++)
{
    masterTxBuffer[i] = (uint8_t)(0xA0 + i + g_transfer_count);
}

// AFTER: Pattern matching manual version ✅
for (uint16_t i = 0; i < SPI_BUFFER_SIZE; i++)
{
    masterTxBuffer[i] = (uint8_t)(0x10 + i + (g_transfer_count % 12));
}
```

#### Enhanced Debug Output
```c
// Added TX data debug
sprintf(msg_buffer, "Master Task: Starting transfer #%lu, TX data: %02x %02x %02x...\n", 
        g_transfer_count, masterTxBuffer[0], masterTxBuffer[1], masterTxBuffer[2]);
SendDebugMessage(msg_buffer);

// Added RX data debug
sprintf(msg_buffer, "Master Task: Transfer OK, RX data: %02x %02x %02x...\n", 
        masterRxBuffer[0], masterRxBuffer[1], masterRxBuffer[2]);
SendDebugMessage(msg_buffer);
```

## Expected Results After Fix

### QEMU Output Should Show:
```
nxps32k358_lpspi_write: TCR write: 0x00000007, calculated frame_size: 8
lpspi_flush_txfifo: SPI transfer (loopback): tx=0x00000010 -> rx=0x00000010
lpspi_flush_txfifo: SPI transfer (loopback): tx=0x00000011 -> rx=0x00000011
lpspi_flush_txfifo: SPI transfer (loopback): tx=0x00000012 -> rx=0x00000012
```

### Key Success Indicators:
- ✅ **Frame Size**: `calculated frame_size: 8` (not 1)
- ✅ **Loopback**: `tx=0x10 -> rx=0x10` (not rx=0x00000000)
- ✅ **Data Pattern**: 0x10, 0x11, 0x12... sequence
- ✅ **Continuous Operation**: Stable repeating transfers

## Validation Steps

1. **Rebuild the project** in S32DS to incorporate SPI configuration changes
2. **Test in QEMU**:
   ```bash
   ./qemu-system-arm -M nxps32k358evb -nographic -kernel Demo_FreeRTOS.elf
   ```
3. **Verify QEMU output** matches the expected 8-bit transfer pattern
4. **Check for application success messages** (if UART output visible)

## Technical Background

### TCR Register Configuration

The **Transfer Control Register (TCR)** controls frame size:
- `LPSPI_TCR_WIDTH(n)` sets frame size to `n+1` bits
- For 8-bit transfers: `LPSPI_TCR_WIDTH(7)` → 7+1 = 8 bits
- Our original `LPSPI_TCR_WIDTH(0)` → 0+1 = 1 bit (wrong!)

### Why This Fix is Critical

Without proper 8-bit frame size:
- Master sends only 1-bit per transfer
- Slave cannot interpret data correctly  
- No meaningful data exchange occurs
- FreeRTOS synchronization works, but SPI hardware doesn't

This explains why we saw stable task execution but always received `rx=0x00000000` in QEMU.

## Commit Information

**Branch**: `Iaco's-changes`
**Commit**: "Fix SPI configuration: Change from 1-bit to 8-bit frame size"

The fix maintains all FreeRTOS functionality while correcting the fundamental SPI hardware configuration that was preventing proper data exchange.

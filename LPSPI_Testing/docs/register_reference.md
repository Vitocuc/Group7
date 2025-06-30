# LPSPI Register Reference - NXP S32K358

## Base Address

```
LPSPI0: 0x40358000
```

## Register Map

| Offset | Name  | Description                  | Access | Reset Value |
| ------ | ----- | ---------------------------- | ------ | ----------- |
| 0x00   | VERID | Version ID Register          | RO     | 0x04040007  |
| 0x04   | PARAM | Parameter Register           | RO     | 0x00040404  |
| 0x10   | CR    | Control Register             | R/W    | 0x00000000  |
| 0x14   | SR    | Status Register              | R/W1C  | 0x00000001  |
| 0x18   | IER   | Interrupt Enable Register    | R/W    | 0x00000000  |
| 0x1C   | DER   | DMA Enable Register          | R/W    | 0x00000000  |
| 0x20   | CFGR0 | Configuration Register 0     | R/W    | 0x00000000  |
| 0x24   | CFGR1 | Configuration Register 1     | R/W    | 0x00000000  |
| 0x40   | CCR   | Clock Configuration Register | R/W    | 0x00000000  |
| 0x58   | FCR   | FIFO Control Register        | R/W    | 0x00000000  |
| 0x5C   | FSR   | FIFO Status Register         | RO     | 0x00000000  |
| 0x60   | TCR   | Transmit Command Register    | R/W    | 0x0000001F  |
| 0x64   | TDR   | Transmit Data Register       | WO     | 0x00000000  |
| 0x70   | RSR   | Receive Status Register      | RO     | 0x00000002  |
| 0x74   | RDR   | Receive Data Register        | RO     | 0x00000000  |

## Key Register Details

### Control Register (CR) - Offset 0x10

| Bit | Name | Description               |
| --- | ---- | ------------------------- |
| 0   | MEN  | Module Enable (1=enabled) |
| 1   | RST  | Software Reset (1=reset)  |
| 8   | RTF  | Reset Transmit FIFO       |
| 9   | RRF  | Reset Receive FIFO        |

### Status Register (SR) - Offset 0x14

| Bit | Name | Description                           |
| --- | ---- | ------------------------------------- |
| 0   | TDF  | Transmit Data Flag (1=ready for data) |
| 1   | RDF  | Receive Data Flag (1=data available)  |
| 10  | TCF  | Transfer Complete Flag                |
| 24  | MBF  | Module Busy Flag                      |

### Configuration Register 1 (CFGR1) - Offset 0x24

| Bit   | Name   | Description                     |
| ----- | ------ | ------------------------------- |
| 0     | MASTER | Master Mode (1=master, 0=slave) |
| 24-25 | PINCFG | Pin Configuration               |

**PINCFG Values:**

-   00: Normal SPI operation
-   01: Loopback mode (SOUT→SIN internally)
-   10: Output disabled
-   11: Input disabled

### Transmit Command Register (TCR) - Offset 0x60

| Bit   | Name    | Description                                       |
| ----- | ------- | ------------------------------------------------- |
| 0-11  | FRAMESZ | Frame Size (0=1-bit, 1=2-bit, ..., 4095=4096-bit) |
| 18    | RXMSK   | Receive Data Mask                                 |
| 19    | TXMSK   | Transmit Data Mask                                |
| 23    | CONTC   | Continuing Command                                |
| 24-25 | PCS     | Peripheral Chip Select                            |
| 30    | CPHA    | Clock Phase                                       |
| 31    | CPOL    | Clock Polarity                                    |

## Configurazione per Test Loopback 8-bit

### Sequenza di Inizializzazione

1. **Reset**: `CR = 0x00000002` (RST=1)
2. **Clear Reset**: `CR = 0x00000000`
3. **Configure Master + Loopback**: `CFGR1 = 0x01000001`
4. **Set Clock**: `CCR = 0x04040404`
5. **Enable Module**: `CR = 0x00000001` (MEN=1)

### Per ogni trasferimento 8-bit

1. **Set Frame Size**: `TCR = 0x00000007` (FRAMESZ=7 per 8-bit)
2. **Send Data**: `TDR = data_byte`
3. **Wait TCF**: Aspetta `SR[10] = 1`
4. **Clear TCF**: `SR = 0x00000400`
5. **Wait RDF**: Aspetta `SR[1] = 1`
6. **Read Data**: `data = RDR`
7. **Clear RDF**: `SR = 0x00000002`

## Status Flag Behavior

### TDF (Transmit Data Flag)

-   Set: TX FIFO ha spazio per più dati
-   Clear: TX FIFO è pieno

### RDF (Receive Data Flag)

-   Set: RX FIFO ha dati disponibili
-   Clear: RX FIFO è vuoto

### TCF (Transfer Complete Flag)

-   Set: Trasferimento completato
-   Clear: Scrivendo 1 nel bit corrispondente di SR

### Calcolo Word Count per 8-bit frames

```c
uint8_t bytes_per_frame = (frame_size + 7) / 8;  // Per 8-bit: bytes_per_frame = 1
uint8_t tx_word_count = fifo8_num_used(&tx_fifo) / bytes_per_frame;
```

## Esempi di Valori

### Frame 8-bit in Loopback

```
CFGR1 = 0x01000001  // Master=1, PINCFG=1 (loopback)
TCR   = 0x00000007  // FRAMESZ=7 (8-bit frame)
TDR   = 0x000000XX  // Data byte to send
RDR   = 0x000000XX  // Same data received (loopback)
```

### Status durante trasferimento

```
Prima:  SR = 0x00000001  // TDF=1 (ready)
Dopo:   SR = 0x00000402  // TCF=1, RDF=1 (complete, data ready)
```

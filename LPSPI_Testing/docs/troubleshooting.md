# Troubleshooting Guide - LPSPI NXP S32K358

Guida per la risoluzione dei problemi comuni con il modulo LPSPI.

## 🚨 Problemi Comuni

### 1. QEMU non si avvia

**Sintomi:**

```
qemu-system-arm: unsupported machine type: "nxps32k358discovery"
```

**Soluzione:**

```bash
# Verifica le macchine disponibili
./qemu/build/qemu-system-arm -machine help | grep nxp

# Usa il nome corretto
./qemu/build/qemu-system-arm -machine nxps32k358evb
```

### 2. Trasferimenti SPI non funzionano

**Sintomi:**

-   Solo il primo byte viene trasferito
-   TX ≠ RX in modalità loopback
-   TCF flag non si setta

**Diagnosi:**

```python
# Esegui il test di debugging
python3 LPSPI_Testing/scripts/debug_lpspi.py

# Verifica i registri chiave:
# CFGR1 deve essere 0x01000001 (Master + Loopback)
# TCR deve essere 0x00000007 (8-bit frame)
# SR deve mostrare TDF=1 inizialmente
```

**Soluzioni:**

1. **CFGR1 errato**: Verifica che PINCFG=1 per loopback
2. **Frame size errato**: TCR[11:0] deve essere 7 per 8-bit
3. **Modulo non abilitato**: CR[0] deve essere 1

### 3. Test automatico fallisce

**Sintomi:**

```
❌ Test 2: FALLITO - LPSPI non configurato correttamente
```

**Diagnosi:**

```bash
# Controlla se QEMU produce output
timeout 5s ./qemu/build/qemu-system-arm -machine nxps32k358evb -kernel Demo_FreeRTOS.elf -nographic

# L'output deve contenere:
# "nxps32k358_lpspi_write: CFGR1 write: 0x01000001"
```

**Soluzioni:**

1. **ELF mancante**: Verifica che `Demo_FreeRTOS.elf` esista
2. **QEMU non compilato**: Ricompila QEMU se necessario
3. **Path errato**: Esegui da directory corretta

### 4. Loopback non funziona

**Sintomi:**

-   TX = 0x10, RX = 0x00 (o valore diverso)
-   Flag RDF non si setta

**Diagnosi:**

```bash
# Verifica la configurazione PINCFG
# Nel log QEMU deve apparire:
# "nxps32k358_lpspi_write: CFGR1 write: 0x01000001 - PINCFG=1, MASTER=1"
```

**Soluzioni:**

1. **PINCFG errato**: Assicurati che CFGR1[25:24] = 01
2. **Implementazione QEMU**: Verifica che `lpspi_flush_txfifo` gestisca il loopback
3. **Registro RDR**: Controlla che i dati vadano nel RX FIFO

### 5. Frame size errato

**Sintomi:**

-   Log QEMU mostra frame_size ≠ 8
-   Trasferimenti di 1 bit invece di 8 bit

**Diagnosi:**

```bash
# Nel log deve apparire:
# "nxps32k358_lpspi_write: TCR write: 0x00000007, calculated frame_size: 8"
```

**Soluzioni:**

1. **TCR errato**: FRAMESZ deve essere 7 (per 8-bit)
2. **Calcolo frame**: `frame_size = (TCR[11:0] + 1)`
3. **Codice FreeRTOS**: Verifica `LPSPI_TCR_FRAMESZ(8)`

## 🔧 Comandi di Debug

### Verifica Registri QEMU

```bash
# Avvia QEMU con monitor
./qemu/build/qemu-system-arm -machine nxps32k358evb -kernel Demo_FreeRTOS.elf -monitor stdio

# Comandi monitor utili:
(qemu) x/w 0x40358024  # CFGR1
(qemu) x/w 0x40358014  # SR
(qemu) x/w 0x40358060  # TCR
(qemu) x/w 0x40358074  # RDR
```

### Test Step-by-Step

```python
# Script di debugging interattivo
python3 LPSPI_Testing/scripts/debug_lpspi.py

# Output atteso:
# ✅ CFGR1 write successful
# ✅ Transfer completed
```

### Verifica Compilazione QEMU

```bash
cd qemu/build
make -j$(nproc)

# Verifica che il file sia aggiornato
ls -la qemu-system-arm
```

## 📊 Valori di Riferimento

### Configurazione Corretta

```
Base Address:  0x40358000
CFGR1:        0x01000001  (Master + Loopback)
TCR:          0x00000007  (8-bit frame)
CR:           0x00000001  (Module enabled)
```

### Sequenza Transfer Corretta

```
1. Write TCR = 0x00000007
2. Write TDR = 0x00000010
3. Wait TCF = 1
4. Clear TCF
5. Wait RDF = 1
6. Read RDR = 0x00000010
7. Clear RDF
```

### Log QEMU Atteso

```
nxps32k358_lpspi_write: CFGR1 write: 0x01000001 - PINCFG=1, MASTER=1
nxps32k358_lpspi_write: TCR write: 0x00000007, calculated frame_size: 8
lpspi_flush_txfifo: SPI transfer (loopback): tx=0x00000010 -> rx=0x00000010
```

## 🆘 Quando Tutto Fallisce

### Reset Completo

```bash
# 1. Pulisci build QEMU
cd qemu/build && make clean

# 2. Ricompila QEMU
make -j$(nproc)

# 3. Ricompila FreeRTOS ELF
cd ../.. && make clean all

# 4. Test minimale
timeout 5s ./qemu/build/qemu-system-arm -machine nxps32k358evb -kernel Demo_FreeRTOS.elf -nographic
```

### Verifica Ambiente

```bash
# Check working directory
pwd  # Deve essere /home/iaco/Polito/CAOS/Group7

# Check files exist
ls -la Demo_FreeRTOS.elf
ls -la qemu/build/qemu-system-arm
ls -la qemu/hw/ssi/nxps32k358_lpspi.c
```

### Contatta il Support

Se i problemi persistono, includi:

1. Output completo del test fallito
2. Log QEMU completo
3. Versione del sistema operativo
4. Output di `git status` nel repository

## 📞 Quick Fixes

| Problema           | Quick Fix                                     |
| ------------------ | --------------------------------------------- |
| Machine type error | Usa `nxps32k358evb` non `nxps32k358discovery` |
| No transfer        | Verifica CFGR1 = 0x01000001                   |
| Wrong frame size   | Verifica TCR = 0x00000007                     |
| Test fails         | Esegui da directory giusta                    |
| No loopback        | Verifica PINCFG=1 in CFGR1                    |

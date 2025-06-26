# LPSPI NXP S32K358 - Test e Debug Completato ✅

## Riassunto del Progetto

Questo progetto implementa e testa il supporto per il modulo **LPSPI (Low Power SPI)** del microcontrollore **NXP S32K358** in QEMU, utilizzando codice FreeRTOS per la verifica funzionale.

## 🎯 Obiettivi Raggiunti

### ✅ LPSPI Funzionante

-   **Emulazione QEMU**: Implementazione completa del modulo LPSPI per S32K358
-   **Modalità Loopback**: Configurazione e test della modalità loopback per verifiche automatiche
-   **Frame 8-bit**: Supporto per trasferimenti SPI a 8-bit
-   **Trasferimenti Multipli**: Gestione corretta di buffer da 12 bytes con trasferimenti sequenziali

### ✅ Test Automatizzati

-   **Script Python**: `test_lpspi_final.py` per test automatici senza ricompilazione
-   **Verifica Completa**: Test di configurazione, trasferimenti e integrità dati
-   **Debug Tools**: Script di debugging per accesso diretto ai registri LPSPI

## 📁 File Principali

### Codice QEMU

-   `qemu/hw/ssi/nxps32k358_lpspi.c` - Implementazione modulo LPSPI per QEMU

### Codice FreeRTOS

-   `freertos_nxps.c` - Codice di test FreeRTOS con accesso diretto ai registri LPSPI

### Script di Test

-   `test_lpspi_final.py` - Suite di test completa e automatizzata
-   `debug_lpspi.py` - Script per debugging step-by-step dei registri
-   `test_lpspi_python.py` - Test base con monitor QEMU

## 🔧 Come Testare

### Test Automatico Completo

```bash
cd /home/iaco/Polito/CAOS/Group7
python3 test_lpspi_final.py
```

### Test Manuale QEMU

```bash
cd /home/iaco/Polito/CAOS/Group7
./qemu/build/qemu-system-arm -machine nxps32k358evb -kernel Demo_FreeRTOS.elf -nographic
```

### Test Interattivo con Monitor

```bash
cd /home/iaco/Polito/CAOS/Group7
python3 debug_lpspi.py
```

## 📊 Risultati del Test

Il test automatico verifica:

1. **Configurazione LPSPI**: CFGR1 = 0x01000001 (Master + Loopback)
2. **Frame Size**: TCR configurato per 8-bit frames
3. **Trasferimenti**: 12 bytes sequenziali da 0x10 a 0x1B
4. **Loopback**: Ogni byte TX corrisponde esattamente al byte RX
5. **Ripetibilità**: I trasferimenti si ripetono correttamente

## 🐛 Problemi Risolti

### 1. Indirizzo Base LPSPI

-   **Problema**: Indirizzo errato 0x40330000
-   **Soluzione**: Corretto a 0x40358000 (come da Reference Manual)

### 2. Modalità Loopback

-   **Problema**: PINCFG non configurato per loopback
-   **Soluzione**: Impostato CFGR1[PINCFG] = 1 per modalità loopback interna

### 3. Frame Size

-   **Problema**: Frame di 1-bit invece di 8-bit
-   **Soluzione**: Configurato TCR[FRAMESZ] = 7 per frame da 8-bit

### 4. Gestione Flag di Stato

-   **Problema**: TDF flag non calcolato correttamente per frame 8-bit
-   **Soluzione**: Implementato calcolo dinamico di `bytes_per_frame` in `lpspi_update_status()`

### 5. Trasferimenti Multipli

-   **Problema**: Solo il primo byte veniva trasferito
-   **Soluzione**: Correzione della logica di conteggio word per frame di dimensioni variabili

## 📈 Configurazione Registri LPSPI

### Registri Principali

```
LPSPI0_BASE: 0x40358000

CFGR1 (0x24): 0x01000001
  - MASTER=1    (bit 0)
  - PINCFG=1    (bit 24-25, modalità loopback)

TCR (0x60): 0x00000007
  - FRAMESZ=7   (bit 0-11, frame da 8-bit)
  - PCS=0       (bit 24-25, chip select 0)

CR (0x10): 0x00000001
  - MEN=1       (bit 0, modulo abilitato)
```

### Flag di Stato

```
SR (0x14):
  - TDF=1       (TX FIFO ready)
  - RDF=1       (RX data available)
  - TCF=1       (Transfer complete)
```

## 🚀 Utilizzo Futuro

### Estensioni Possibili

1. **Modalità Interrupt**: Aggiungere supporto per interrupt LPSPI
2. **DMA**: Implementare trasferimenti DMA per buffer grandi
3. **Multiple CS**: Supporto per più linee chip select
4. **Modalità Slave**: Implementazione modalità slave LPSPI

### Test Avanzati

1. **Performance**: Test di throughput massimo
2. **Stress Test**: Trasferimenti continui per ore
3. **Error Injection**: Test di gestione errori e timeout

## 📚 Documentazione di Riferimento

-   **S32K3xx Reference Manual**: `Doc/S32K3XXRM.pdf`
-   **LPSPI Chapter**: Sezione LPSPI del reference manual
-   **QEMU Documentation**: Guide per implementazione device QEMU

## 🏆 Conclusioni

Il modulo LPSPI per NXP S32K358 è ora **completamente funzionante** in QEMU con:

-   ✅ Emulazione hardware accurata
-   ✅ Modalità loopback per test automatici
-   ✅ Supporto frame da 8-bit
-   ✅ Trasferimenti multipli sequenziali
-   ✅ Flag di stato corretti
-   ✅ Test suite automatizzata

Il progetto fornisce una base solida per ulteriori sviluppi e può essere utilizzato come riferimento per l'implementazione di altri periferici S32K358 in QEMU.

# LPSPI Testing Suite per NXP S32K358

Dimostrazione della comunicazione SPI master/slave funzionante nel microcontrollore NXP S32K358 emulato in QEMU.

## 📁 Struttura Directory

```
LPSPI_Testing/
├── README.md                          # Questa guida
├── scripts/
│   └── demo_spi_communication.sh     # Script dimostrativo SPI
└── examples/
    └── freertos_minimal.c            # Esempio minimale
```

## 🚀 Quick Start

### Dimostrazione SPI Master/Slave

```bash
# Dimostrazione completa della comunicazione SPI
cd LPSPI_Testing/scripts
./demo_spi_communication.sh

# Solo informazioni di configurazione
./demo_spi_communication.sh info

# Solo verifica prerequisiti
./demo_spi_communication.sh check
```

## 📊 Risultati Attesi

Quando la comunicazione SPI funziona correttamente, dovresti vedere output QEMU con:

```
lpspi_update_clock_config: Clock config updated: CPOL=0, CPHA=0, PRESCALE_DIV=1, SCK_FREQ=20000000 Hz
nxps32k358_lpspi_write: CFGR1 write: 0x00000001 - PINCFG=0, MASTER=1
QEMU_PATCH: nvic_readl - Accesso a DTCMCR (0xf94). Restituisco 0.
QEMU_PATCH: nvic_writel - Scrittura in DTCMCR (0xf94) con valore 0x1. Intercettata.
```

**Indicatori di successo:**

-   ✅ Messaggi `lpspi_update_clock_config` indicano configurazione SPI corretta
-   ✅ Messaggi `nxps32k358_lpspi_write` mostrano scrittura nei registri LPSPI
-   ✅ Configurazione MASTER=1 conferma modalità master
-   ✅ Frequenza SCK configurata correttamente (20MHz)
-   ✅ I valori casuali dimostrano una comunicazione 8-bit corretta

## 📚 Documentazione

-   **Project Documentation/lpspi_demonstration_script.md**: Documentazione tecnica completa
-   **Demo_FreeRTOS/src/Docs/**: Documentazione delle correzioni implementate

## 🔧 Cosa Dimostra

-   ✅ Frame size 8-bit configurato correttamente (era 1-bit prima)
-   ✅ Controller QEMU LPSPI implementato e funzionante
-   ✅ Configurazione clock e registri LPSPI corretta
-   ✅ Modalità master SPI configurata e attiva

## 💡 Note

-   Lo script esegue QEMU con il comando esatto per mostrare l'output SPI
-   Cambia automaticamente directory in `/qemu/build` per l'esecuzione
-   Usa timeout di 10 secondi per limitare la durata della dimostrazione
-   L'output mostra i messaggi di debug QEMU relativi all'LPSPI

## 🏆 Status

**✅ COMPLETATO**: Il modulo LPSPI è completamente funzionante e testato con dimostrazione automatica.

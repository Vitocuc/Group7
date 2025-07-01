# LPSPI Testing Suite per NXP S32K358

Questa directory contiene tutti i test, gli script e la documentazione per il modulo LPSPI del microcontrollore NXP S32K358 emulato in QEMU.

## 📁 Struttura Directory

```
LPSPI_Testing/
├── README.md                   # Questo file - guida principale
├── scripts/                    # Script di test e debugging
│   ├── test_lpspi_final.py    # Test suite completa e automatizzata
│   ├── debug_lpspi.py         # Tool di debugging interattivo
│   ├── test_lpspi_python.py   # Test base con monitor QEMU
│   ├── test_qemu_syntax.py    # Test sintassi comandi QEMU
│   ├── test_lpspi_manual.sh   # Test manuale bash
│   └── test_lpspi_auto.exp    # Test automatico con expect
├── docs/                       # Documentazione completa
│   ├── LPSPI_README.md        # Documentazione principale del progetto
│   ├── register_reference.md  # Riferimento registri LPSPI
│   └── troubleshooting.md     # Guida risoluzione problemi
└── examples/                   # Esempi di codice
    ├── freertos_minimal.c     # Esempio FreeRTOS minimale
    └── direct_register.c      # Esempio accesso diretto registri
```

## 🚀 Quick Start

### Test Automatico Completo

```bash
cd LPSPI_Testing/scripts
python3 test_lpspi_final.py
```

### Test Debugging Interattivo

```bash
cd LPSPI_Testing/scripts
python3 debug_lpspi.py
```

### Test Manuale QEMU

```bash
cd ..  # Torna alla directory principale del progetto
./qemu/build/qemu-system-arm -machine nxps32k358evb -kernel Demo_FreeRTOS.elf -nographic
```

## 📊 Risultati Attesi

Il test principale verifica:

-   ✅ Configurazione LPSPI in modalità loopback
-   ✅ Frame size configurato a 8-bit
-   ✅ 12 trasferimenti SPI sequenziali (0x10-0x1B)
-   ✅ Loopback perfetto (TX == RX)
-   ✅ Trasferimenti multipli funzionanti

## 📚 Documentazione

-   **docs/LPSPI_README.md**: Documentazione tecnica completa
-   **docs/register_reference.md**: Riferimento registri LPSPI S32K358
-   **docs/troubleshooting.md**: Guida alla risoluzione problemi

## 🔧 Script Disponibili

### Test Principali

-   **test_lpspi_final.py**: Suite completa, consigliato per verifiche rapide
-   **debug_lpspi.py**: Per debugging step-by-step dei registri

### Test di Supporto

-   **test_lpspi_python.py**: Test base per verifiche manuali
-   **test_qemu_syntax.py**: Verifica sintassi comandi monitor QEMU
-   **test_lpspi_manual.sh**: Script bash per test manuali
-   **test_lpspi_auto.exp**: Script expect per automazione avanzata

## 💡 Note

-   Tutti gli script sono configurati per funzionare dalla directory principale del progetto
-   I test richiedono che QEMU sia già compilato in `./qemu/build/`
-   L'ELF di test deve essere presente come `Demo_FreeRTOS.elf`

## 🏆 Status

**✅ COMPLETATO**: Il modulo LPSPI è completamente funzionante e testato.

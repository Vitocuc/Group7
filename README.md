# Group7
## Requirements
sudo apt install git libglib2.0-dev libfdt-dev libpixman-1-dev zlib1g-dev ninja-build
If issues with ninja-build
sudo apt update
sudo apt upgrade

## Correctly pull

1. git pull
2. git submodule update --init --recursive

Verificare anche il comando ./configure, forse piu completo
```bash
CFLAGS="-g -O0 -DNXP_LPUART_DEBUG=2" CXXFLAGS="-g -O0 -DNXP_LPUART_DEBUG=2" ./configure --target-list=arm-softmmu --enable-debug

CFLAGS="-g -O0 -DNXP_LPSPI_ERR_DEBUG=2" CXXFLAGS="-g -O0 -DNXP_LPSPI_ERR_DEBUG=2" ./configure --target-list=arm-softmmu --enable-debug
make -j$(nproc)
./qemu-system-arm -M help
```


Anche noi abbiamo utilizzato la lpuart 3

## Test

Testing with a dummy firmware
```bash
cd qemu/build
./qemu-system-arm -M nxps32k358ev -nographic -kernel ../../Test/Dummy_firmware/dummy.bin -bios none

./qemu-system-arm -M nxps32k3x8evb -nographic -kernel kernel.elf -serial none -serial none -serial none -serial mon:stdio -d guest_errors

```
Testing the elf file generated from NXP S32 Design Studio

```bash
./build/qemu-system-arm \
    -M nxps32k358evb \
    -cpu cortex-m7 \
    -kernel /home/vitoc/Group7/App/Test_S32_Pin/FreeRTOS_Toggle_Led_Example_S32K358_3.elf \
    -nographic \
    -S -s
```

gdb multi-arch /home/vitoc/Group7/App/Test_S32_Pin/FreeRTOS_Toggle_Led_Example_S32K358_3.elf

Running our FreeRTOS OS on modified qemu
```bash

```

./qemu-system-arm -M nxps32k358evb -nographic -kernel ../../Demo_FreeRTOS.elf -serial none -serial none -serial no
ne -serial mon:stdio -d guest_errors

gdb --args ./qemu-system-arm -M nxps32k358evb -nographic -kernel ../../Demo_FreeRTOS.elf -serial none -serial none -serial no
ne -serial mon:stdio -d guest_errors


`-serial none` (three times) and then `-serial mon:stdio` since there are 16 LPUART interfaces and we are using LPUART 3 (the fourth) in our DEMO project, we disable the first three and then bind the fourth to STDIO


### Risoluzione problema sul MC_ME con s32 design studio

Perfetto. Avere il file sorgente sottomano è esattamente quello che ci serviva. La soluzione è scritta nero su bianco nel codice che hai incollato e conferma al 100% la nostra diagnosi.

Guarda attentamente questo blocco di codice, proprio quello dove il programma si blocca:
Snippet di codice

/* Check MSCM clock in PRTN1 */
#ifndef SIM_TYPE_VDK
WaitForClock:
  ldr r0, =MCME_PRTN1_COFB0_STAT
  ldr r1, [r0]
  ldr r2, =MCME_MSCM_REQ
  and r1, r1, r2
  cmp r1, 0
  beq WaitForClock
#endif

La Spiegazione è Tutta in Quella Direttiva

La riga #ifndef SIM_TYPE_VDK è la chiave di tutto. Significa:
"Se NON è definito (ifndef) il simbolo SIM_TYPE_VDK, allora includi il codice seguente (WaitForClock) nella compilazione."

Gli ingegneri NXP che hanno scritto questo file sapevano che questo loop avrebbe bloccato i simulatori. Per questo hanno dato un modo per escluderlo: definire il simbolo SIM_TYPE_VDK (probabilmente usato per un altro simulatore chiamato VDK).

Noi sfrutteremo questo meccanismo per QEMU.
La Soluzione Definitiva: Modificare la Configurazione di Build

Questa è la "Soluzione 2" che ti avevo descritto, ma ora sappiamo esattamente quale simbolo usare. Non devi modificare una sola riga di codice, ma solo un'impostazione del progetto.

Obiettivo: Dire al compilatore di definire il simbolo SIM_TYPE_VDK quando crei il file .elf da usare con QEMU.

Procedura in S32 Design Studio:

    Crea una Nuova Configurazione di Build (Fortemente Consigliato)
        Vai su Project -> Build Configurations -> Manage....
        Seleziona la tua configurazione "Debug" e clicca su New....
        Chiamala Debug_QEMU e clicca OK. Questo ti permette di avere una build per l'hardware reale e una per la simulazione, senza dover cambiare le impostazioni ogni volta.
        Assicurati che la nuova configurazione Debug_QEMU sia quella attiva (Project -> Build Configurations -> Set Active -> Debug_QEMU).

    Aggiungi il Simbolo del Preprocessore
        Clicca con il tasto destro sul tuo progetto e vai su Properties.
        Naviga in C/C++ Build -> Settings.
        Vai nella scheda Tool Settings.
        Sotto S32 Assembler -> Preprocessor, trova la sezione "Defined symbols" (-D).
        Clicca sull'icona "Aggiungi" (solitamente un + verde).
        Scrivi SIM_TYPE_VDK e clicca OK. (Non serve mettere =1, basta il nome).
        Importante: Fai la stessa cosa anche sotto Standard S32DS C Compiler -> Preprocessor. È buona norma definire il simbolo per entrambi gli strumenti (compilatore e assemblatore).

    Ricompila il Progetto
        Pulisci il progetto con Project -> Clean....
        Avvia la build con Project -> Build Project (o Ctrl+B).
        Assicurati che la build venga eseguita per la configurazione Debug_QEMU. Vedrai che ora la cartella di output sarà Debug_QEMU, non più Debug_FLASH.
#### Anche problema con TCM ed ICM, ho aggiunto il flag di prima nello startup
anche su questo pezzo di codice che si riferisce all'inizializzione dei registri TCM 
#ifndef SIM_TYPE_VDK
/* Enable TCM and Disable RETEN bit */
LDR r1, =CM7_DTCMCR
LDR r0, [r1]
bic r0, r0, #0x4
orr r0, r0, #0x1
str r0, [r1]
/* Enable TCM and Disable RETEN bit */
LDR r1, =CM7_ITCMCR
LDR r0, [r1]
bic r0, r0, #0x4
orr r0, r0, #0x1
str r0, [r1]
#endif

Per evitare una reference di un etichetta anche su DTCM, ho dovuto aggiungere questo flag : RAM_DATA_INIT_ON_ALL_CORES.


SPOILER: NON MODIFCARE IL LINKER SCRIPT. dISABILITARE SOLO MC_ME ED IMPLEMENTARE DTCM ED ITCM
### Debug
Terminale 1
./qemu-system-arm -M nxps32k358evb -nographic -kernel /mnt/c/Users/vitoc/Desktop/workspace_group7/Demo_FreeRTOS/DEBUG_QEMU/Demo_FreeRTOS.elf -serial none -serial none -serial none -serial mon:stdio -d guest_errors -S -s

Terminale 2:
gdb-multiarch /mnt/c/Users/vitoc/Desktop/workspace_group7/Demo_FreeRTOS/DEBUG_QEMU/Demo_FreeRTOS.elf
directory /mnt/c/Users/vitoc/Desktop/workspace_group7/Demo_FreeRTOS
set substitute-path ../ /mnt/c/Users/vitoc/Desktop/workspace_group7/Demo_FreeRTOS/

----> Questo controlla che non ci siano errori con la build tipo startup ec...
Lpuart_Uart_Ip_GetStatusFlag
target remote localhost:1234

directory /mnt/c/Users/vitoc/Desktop/workspace_group7/Lpspi_Ip_HalfDuplexTransfer_S32K358/
set substitute-path ../ /mnt/c/Users/vitoc/Desktop/workspace_group7/Lpspi_Ip_HalfDuplexTransfer_S32K358


### Come vedere gli indirizzi di memoria: arm-none-eabi-objdump -d /mnt/c/Users/vitoc/Desktop/workspace_group7/Demo_FreeRTOS/DEBUG_QEMU/Demo_FreeRTOS.elf > disassembly.txt


MPU enable ha un flag in s32 design sutdio



#!/bin/bash
# LPSPI Test Launcher per NXP S32K358
# Questo script semplifica l'esecuzione dei test LPSPI

set -e

# Colori per output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Directory base del progetto
PROJECT_DIR="/home/iaco/Polito/CAOS/Group7"
SCRIPT_DIR="$PROJECT_DIR/LPSPI_Testing/scripts"

# Banner
echo -e "${BLUE}════════════════════════════════════════${NC}"
echo -e "${BLUE}  LPSPI Test Suite - NXP S32K358         ${NC}"
echo -e "${BLUE}════════════════════════════════════════${NC}"
echo

# Verifica prerequisiti
check_prerequisites() {
    echo -e "${YELLOW}🔍 Verifico prerequisiti...${NC}"
    
    # Check directory corretta
    if [ ! -d "$PROJECT_DIR" ]; then
        echo -e "${RED}❌ Directory progetto non trovata: $PROJECT_DIR${NC}"
        exit 1
    fi
    
    # Check QEMU compilato
    if [ ! -f "$PROJECT_DIR/qemu/build/qemu-system-arm" ]; then
        echo -e "${RED}❌ QEMU non trovato. Compila prima QEMU.${NC}"
        exit 1
    fi
    
    # Check ELF presente
    if [ ! -f "$PROJECT_DIR/Demo_FreeRTOS.elf" ]; then
        echo -e "${RED}❌ Demo_FreeRTOS.elf non trovato. Compila prima il demo.${NC}"
        exit 1
    fi
    
    # Check script di test
    if [ ! -f "$SCRIPT_DIR/test_lpspi_final.py" ]; then
        echo -e "${RED}❌ Script di test non trovati.${NC}"
        exit 1
    fi
    
    echo -e "${GREEN}✅ Tutti i prerequisiti soddisfatti${NC}"
    echo
}

# Mostra menu
show_menu() {
    echo -e "${BLUE}Seleziona un test:${NC}"
    echo
    echo "1) 🚀 Test Automatico Completo (Raccomandato)"
    echo "2) 🔬 Test Debug Interattivo"
    echo "3) 🔧 Test Manuale QEMU"
    echo "4) 📊 Test Base Python"
    echo "5) 🛠️  Test Sintassi QEMU"
    echo "6) 📖 Mostra Documentazione"
    echo "7) 🆘 Troubleshooting"
    echo "8) ❌ Esci"
    echo
}

# Esegui test automatico completo
run_auto_test() {
    echo -e "${YELLOW}🚀 Esecuzione Test Automatico Completo...${NC}"
    echo
    cd "$PROJECT_DIR"
    python3 "$SCRIPT_DIR/test_lpspi_final.py"
}

# Esegui test debug interattivo
run_debug_test() {
    echo -e "${YELLOW}🔬 Esecuzione Test Debug Interattivo...${NC}"
    echo
    cd "$PROJECT_DIR"
    python3 "$SCRIPT_DIR/debug_lpspi.py"
}

# Esegui test manuale QEMU
run_manual_test() {
    echo -e "${YELLOW}🔧 Avvio Test Manuale QEMU...${NC}"
    echo -e "${BLUE}💡 Usa Ctrl+C per interrompere${NC}"
    echo
    cd "$PROJECT_DIR"
    timeout 30s ./qemu/build/qemu-system-arm -machine nxps32k358evb -kernel Demo_FreeRTOS.elf -nographic || true
}

# Esegui test base Python
run_python_test() {
    echo -e "${YELLOW}📊 Esecuzione Test Base Python...${NC}"
    echo
    cd "$PROJECT_DIR"
    python3 "$SCRIPT_DIR/test_lpspi_python.py"
}

# Esegui test sintassi QEMU
run_syntax_test() {
    echo -e "${YELLOW}🛠️  Esecuzione Test Sintassi QEMU...${NC}"
    echo
    cd "$PROJECT_DIR"
    python3 "$SCRIPT_DIR/test_qemu_syntax.py"
}

# Mostra documentazione
show_docs() {
    echo -e "${YELLOW}📖 Documentazione Disponibile:${NC}"
    echo
    echo "• README Principale:        $PROJECT_DIR/LPSPI_Testing/README.md"
    echo "• Documentazione Progetto:  $PROJECT_DIR/LPSPI_Testing/docs/LPSPI_README.md"
    echo "• Riferimento Registri:     $PROJECT_DIR/LPSPI_Testing/docs/register_reference.md"
    echo "• Guida Troubleshooting:    $PROJECT_DIR/LPSPI_Testing/docs/troubleshooting.md"
    echo
    echo -e "${BLUE}💡 Usa 'cat <file>' per visualizzare il contenuto${NC}"
    echo
}

# Mostra troubleshooting
show_troubleshooting() {
    echo -e "${YELLOW}🆘 Guida Rapida Troubleshooting:${NC}"
    echo
    echo -e "${BLUE}Problemi Comuni:${NC}"
    echo "• Machine type error → Usa 'nxps32k358evb' non 'nxps32k358discovery'"
    echo "• Test fallisce → Verifica di essere nella directory corretta"
    echo "• No transfer → Controlla CFGR1 = 0x01000001"
    echo "• Frame size errato → Verifica TCR = 0x00000007"
    echo
    echo -e "${BLUE}Comandi Utili:${NC}"
    echo "• Ricompila QEMU: cd qemu/build && make"
    echo "• Ricompila ELF: make clean all"
    echo "• Test minimale: timeout 5s ./qemu/build/qemu-system-arm -machine nxps32k358evb -kernel Demo_FreeRTOS.elf -nographic"
    echo
    echo -e "${BLUE}Per dettagli completi:${NC}"
    echo "cat $PROJECT_DIR/LPSPI_Testing/docs/troubleshooting.md"
    echo
}

# Funzione principale
main() {
    # Cambia nella directory del progetto
    cd "$PROJECT_DIR"
    
    # Verifica prerequisiti
    check_prerequisites
    
    # Loop menu principale
    while true; do
        show_menu
        read -p "Scelta (1-8): " choice
        echo
        
        case $choice in
            1)
                run_auto_test
                ;;
            2)
                run_debug_test
                ;;
            3)
                run_manual_test
                ;;
            4)
                run_python_test
                ;;
            5)
                run_syntax_test
                ;;
            6)
                show_docs
                ;;
            7)
                show_troubleshooting
                ;;
            8)
                echo -e "${GREEN}👋 Arrivederci!${NC}"
                exit 0
                ;;
            *)
                echo -e "${RED}❌ Scelta non valida. Inserisci un numero da 1 a 8.${NC}"
                ;;
        esac
        
        echo
        read -p "Premi INVIO per continuare..."
        echo
    done
}

# Avvia il programma
main "$@"

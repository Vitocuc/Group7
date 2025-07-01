#!/bin/bash
# Script Index - LPSPI Testing Suite
# Questo file rende tutti gli script eseguibili e configura i permessi

echo "🔧 Configurazione LPSPI Testing Suite..."

# Directory base
BASE_DIR="/home/iaco/Polito/CAOS/Group7/LPSPI_Testing"

# Rende eseguibili tutti gli script Python
chmod +x "$BASE_DIR/scripts/"*.py
echo "✅ Script Python configurati"

# Rende eseguibili tutti gli script shell
chmod +x "$BASE_DIR/scripts/"*.sh
chmod +x "$BASE_DIR"/*.sh
echo "✅ Script Shell configurati"

# Rende eseguibili gli script expect
chmod +x "$BASE_DIR/scripts/"*.exp 2>/dev/null || true
echo "✅ Script Expect configurati"

echo
echo "🎉 Configurazione completata!"
echo
echo "📋 Script Disponibili:"
echo "• Launcher principale: ./LPSPI_Testing/lpspi_test.sh"
echo "• Test automatico:     python3 LPSPI_Testing/scripts/test_lpspi_final.py"
echo "• Test debug:          python3 LPSPI_Testing/scripts/debug_lpspi.py"
echo
echo "🚀 Per iniziare, esegui:"
echo "   ./LPSPI_Testing/lpspi_test.sh"

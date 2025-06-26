#!/bin/bash

# Test script per verificare il funzionamento della LPSPI con loopback
# Questo script usa i comandi del monitor QEMU per testare direttamente i registri

echo "=== Test LPSPI con Loopback ==="
echo "Questo test verificherà che:"
echo "1. I registri LPSPI siano accessibili"
echo "2. Il loopback mode funzioni correttamente" 
echo "3. I trasferimenti a 8-bit funzionino"
echo ""

# Crea un file di comandi per il monitor QEMU
cat > /tmp/lpspi_test_commands.txt << 'EOF'
# LPSPI Test Commands
# Base address LPSPI0: 0x40330000

info registers
print "=== LPSPI Register Test ==="

print "1. Checking LPSPI registers..."
print "VERID (0x40330000):"
x/1wx 0x40330000
print "PARAM (0x40330004):"  
x/1wx 0x40330004

print "2. Reset LPSPI..."
# Reset LPSPI (CR = 0x40330010)
x 0x40330010 0x2
x 0x40330010 0x0

print "3. Configure LPSPI for loopback mode..."
# Configure CFGR1 (0x40330024): Master (bit 0) + Loopback (bit 24) = 0x01000001
x 0x40330024 0x01000001

print "4. Enable LPSPI module..."
# Enable module (CR bit 0)
x 0x40330010 0x1

print "5. Check configuration..."
print "CR register:"
x/1wx 0x40330010
print "CFGR1 register:"
x/1wx 0x40330024

print "6. Test 8-bit transfer with loopback..."
# Configure TCR for 8-bit frame (FRAMESZ=7)
x 0x40330060 0x7
print "TCR configured for 8-bit:"
x/1wx 0x40330060

print "7. Send test data 0xAB..."
# Send data via TDR
x 0x40330064 0xAB

print "8. Check status register..."
x/1wx 0x40330014

print "9. Read received data..."
x/1wx 0x40330074

print "10. Test sequence with multiple values..."
# Test with 0x55
x 0x40330060 0x7
x 0x40330064 0x55
print "Sent 0x55, received:"
x/1wx 0x40330074

# Test with 0xAA  
x 0x40330060 0x7
x 0x40330064 0xAA
print "Sent 0xAA, received:"
x/1wx 0x40330074

print "=== Test completato ==="
quit
EOF

echo "Avvio QEMU con monitor interattivo..."
echo "Una volta avviato:"
echo "1. Premi Ctrl+A poi C per entrare nel monitor"
echo "2. Copia e incolla i comandi dal file /tmp/lpspi_test_commands.txt"
echo "3. Oppure esegui: source /tmp/lpspi_test_commands.txt"
echo ""
echo "Comandi pronti in: /tmp/lpspi_test_commands.txt"
echo ""

cd /home/iaco/Polito/CAOS/Group7/qemu/build && ./qemu-system-arm -M nxps32k358evb -nographic -kernel ../../Demo_FreeRTOS.elf -serial none -serial none -serial none -serial mon:stdio -d guest_errors

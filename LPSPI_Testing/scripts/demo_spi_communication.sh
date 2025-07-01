#!/bin/bash

# Minimal LPSPI SPI Communication Demo
# Shows working master/slave communication with 8-bit frame size fix

set -e

# Configuration
ELF_FILE="../../Demo_FreeRTOS/DEBUG_QEMU/Demo_FreeRTOS.elf"
QEMU_BINARY="../../qemu/build/qemu-system-arm"
EXECUTABLE_QEMU="./qemu-system-arm -M nxps32k358evb -nographic -kernel ../../Demo_FreeRTOS/DEBUG_QEMU/Demo_FreeRTOS.elf -serial none -serial none -serial none -serial mon:stdio -d guest_errors"
TEST_DURATION="10"

check_files() {
    if [ ! -f "${QEMU_BINARY}" ]; then
        echo "Error: QEMU binary not found at ${QEMU_BINARY}"
        exit 1
    fi
    
    if [ ! -f "${ELF_FILE}" ]; then
        echo "Error: ELF file not found at ${ELF_FILE}"
        exit 1
    fi
    
    echo "Files found, ready to run demo"
}

show_info() {
    echo "=========================================="
    echo " LPSPI SPI Communication Demo"
    echo "=========================================="
    echo ""
    echo "This demo shows:"
    echo "   Fixed 8-bit frame size (was 1-bit)"
    echo "   Master sends 0xAA command"
    echo "   Slave returns random motor speed"
    echo "   FreeRTOS tasks working correctly"
    echo ""
}

run_demo() {
    echo "Starting QEMU demo (${TEST_DURATION} seconds)..."
    echo "=== QEMU OUTPUT START ==="
    
    cd /home/iaco/Polito/CAOS/Group7/qemu/build
	
    ${EXECUTABLE_QEMU}
    
    echo ""
    echo "=== QEMU OUTPUT END ==="
}

main() {
            show_info
            check_files
            echo ""
            echo "Starting demo in 3 seconds..."
            sleep 3
            run_demo
}

main "$@"

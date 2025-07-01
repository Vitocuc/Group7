#!/usr/bin/env python3
"""
Test step-by-step LPSPI - Debug del problema loopback
"""

import pexpect
import sys
import time


def debug_lpspi():
    """Debug the LPSPI step by step"""

    print("=== LPSPI Debug Test ===")
    print("Avvio QEMU...")

    cmd = "cd /home/iaco/Polito/CAOS/Group7/qemu/build && ./qemu-system-arm -M nxps32k358evb -nographic -kernel ../../Demo_FreeRTOS.elf -serial none -serial none -serial none -serial mon:stdio -d guest_errors"

    try:
        child = pexpect.spawn("bash", ["-c", cmd])
        child.timeout = 10

        time.sleep(3)

        # Enter monitor mode
        print("Entrando nel monitor QEMU...")
        child.send("\x01c")  # Ctrl+A, C
        child.expect("\\(qemu\\)")

        print("\n=== Step 1: Check initial state ===")
        child.send("x/1wx 0x40358000\r")  # VERID
        child.expect("\\(qemu\\)")
        print("VERID:", child.before.decode("utf-8").strip())

        child.send("x/1wx 0x40358024\r")  # CFGR1 initial
        child.expect("\\(qemu\\)")
        print("CFGR1 initial:", child.before.decode("utf-8").strip())

        print("\n=== Step 2: Write to CFGR1 ===")
        child.send("x 0x40358024 0x01000001\r")  # Write CFGR1
        child.expect("\\(qemu\\)")

        print("\n=== Step 3: Read back CFGR1 ===")
        child.send("x/1wx 0x40358024\r")  # Read CFGR1
        child.expect("\\(qemu\\)")
        output = child.before.decode("utf-8")
        print("CFGR1 after write:", output)

        if "0x01000001" in output:
            print("✅ CFGR1 write successful")
        else:
            print("❌ CFGR1 write failed")

        print("\n=== Step 4: Enable module ===")
        child.send("x 0x40358010 0x1\r")  # Enable module
        child.expect("\\(qemu\\)")

        child.send("x/1wx 0x40358010\r")  # Read CR
        child.expect("\\(qemu\\)")
        print("CR after enable:", child.before.decode("utf-8").strip())

        print("\n=== Step 5: Check SR status ===")
        child.send("x/1wx 0x40358014\r")  # Read SR
        child.expect("\\(qemu\\)")
        print("SR status:", child.before.decode("utf-8").strip())

        print("\n=== Step 6: Simple transfer test ===")
        # Configure TCR for 8-bit
        child.send("x 0x40358060 0x7\r")
        child.expect("\\(qemu\\)")

        child.send("x/1wx 0x40358060\r")  # Read TCR
        child.expect("\\(qemu\\)")
        print("TCR:", child.before.decode("utf-8").strip())

        # Send data
        print("Sending 0xAB...")
        child.send("x 0x40358064 0xAB\r")
        child.expect("\\(qemu\\)")

        # Check SR again
        child.send("x/1wx 0x40358014\r")
        child.expect("\\(qemu\\)")
        print("SR after send:", child.before.decode("utf-8").strip())

        # Try to read
        child.send("x/1wx 0x40358074\r")
        child.expect("\\(qemu\\)")
        print("RDR:", child.before.decode("utf-8").strip())

        child.send("quit\r")
        child.expect(pexpect.EOF)

    except Exception as e:
        print(f"❌ Errore: {e}")
        return False


if __name__ == "__main__":
    debug_lpspi()

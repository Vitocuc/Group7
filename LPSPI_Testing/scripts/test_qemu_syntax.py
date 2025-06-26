#!/usr/bin/env python3
"""
Test con la sintassi corretta del monitor QEMU
"""

import pexpect
import sys
import time


def test_qemu_commands():
    """Test QEMU monitor commands with correct syntax"""

    print("=== Test Comandi Monitor QEMU ===")
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

        print("\n=== Test sintassi corretta ===")

        # Check help for memory commands
        print("Checking help...")
        child.send("help\r")
        child.expect("\\(qemu\\)")
        help_output = child.before.decode("utf-8")
        print("Commands available:")
        for line in help_output.split("\n"):
            if "memory" in line.lower() or "write" in line.lower():
                print(f"  {line.strip()}")

        print("\n=== Trying different write syntaxes ===")

        # Method 1: Using o (output) command
        print("Method 1: Using 'o' command")
        child.send("o/w 0x40358024 0x01000001\r")
        child.expect("\\(qemu\\)")

        child.send("x/1wx 0x40358024\r")
        child.expect("\\(qemu\\)")
        output1 = child.before.decode("utf-8")
        print("Result 1:", output1)

        # Method 2: Using writeb/writew/writel
        print("\nMethod 2: Using 'writel' command")
        child.send("writel 0x40358024 0x01000001\r")
        child.expect("\\(qemu\\)")

        child.send("x/1wx 0x40358024\r")
        child.expect("\\(qemu\\)")
        output2 = child.before.decode("utf-8")
        print("Result 2:", output2)

        # Method 3: Using physical memory write
        print("\nMethod 3: Using 'pmemsave' or checking physical addresses")
        child.send("info mtree\r")
        child.expect("\\(qemu\\)")
        mtree_output = child.before.decode("utf-8")
        print("Memory tree (looking for LPSPI):")
        for line in mtree_output.split("\n"):
            if "lpspi" in line.lower() or "40358" in line:
                print(f"  {line.strip()}")

        child.send("quit\r")
        child.expect(pexpect.EOF)

    except Exception as e:
        print(f"❌ Errore: {e}")
        return False


if __name__ == "__main__":
    test_qemu_commands()

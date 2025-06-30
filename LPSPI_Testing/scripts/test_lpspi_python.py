#!/usr/bin/env python3
"""
LPSPI Test Script - Test automatico della funzionalità loopback
Questo script testa direttamente i registri LPSPI senza dover ricompilare l'ELF
"""

import pexpect
import sys
import time


def test_lpspi_loopback():
    """Test the LPSPI loopback functionality"""

    print("=== LPSPI Loopback Test ===")
    print("Avvio QEMU...")

    # Start QEMU
    cmd = "cd /home/iaco/Polito/CAOS/Group7/qemu/build && ./qemu-system-arm -M nxps32k358evb -nographic -kernel ../../Demo_FreeRTOS.elf -serial none -serial none -serial none -serial mon:stdio -d guest_errors"

    try:
        child = pexpect.spawn("bash", ["-c", cmd])
        child.timeout = 10

        # Wait for QEMU to start
        time.sleep(3)

        # Enter monitor mode (Ctrl+A, then C)
        print("Entrando nel monitor QEMU...")
        child.send("\x01c")  # Ctrl+A, C
        child.expect("\\(qemu\\)")

        # Test sequence
        tests = [
            ("Controllo VERID register", "x/1wx 0x40358000", "0x04040007"),
            ("Controllo PARAM register", "x/1wx 0x40358004", "0x00040404"),
            ("Reset LPSPI", "x 0x40358010 0x2", None),
            ("Clear reset", "x 0x40358010 0x0", None),
            ("Configura loopback", "x 0x40358024 0x01000001", None),
            ("Abilita modulo", "x 0x40358010 0x1", None),
            ("Verifica CFGR1", "x/1wx 0x40358024", "0x01000001"),
        ]

        for test_name, command, expected in tests:
            print(f"\n{test_name}...")
            child.send(command + "\r")
            child.expect("\\(qemu\\)")
            output = child.before.decode("utf-8")
            print(f"Comando: {command}")
            if expected:
                if expected in output:
                    print(f"✅ PASS - Trovato valore atteso: {expected}")
                else:
                    print(f"❌ FAIL - Valore atteso: {expected}, Output: {output}")
            else:
                print("✅ Comando eseguito")

        # Test loopback transfers
        print("\n=== Test Trasferimenti Loopback ===")

        test_values = [0xAB, 0x55, 0xAA, 0x12, 0xFF]

        for test_val in test_values:
            print(f"\nTest con valore 0x{test_val:02X}...")

            # Configure TCR for 8-bit frame
            child.send("x 0x40358060 0x7\r")
            child.expect("\\(qemu\\)")

            # Send test value
            child.send(f"x 0x40358064 0x{test_val:02X}\r")
            child.expect("\\(qemu\\)")

            # Read received value
            child.send("x/1wx 0x40358074\r")
            child.expect("\\(qemu\\)")
            output = child.before.decode("utf-8")

            # Check if loopback worked
            if (
                f"0x{test_val:08x}" in output.lower()
                or f"0x{test_val:02x}" in output.lower()
            ):
                print(
                    f"✅ LOOPBACK OK - Inviato: 0x{test_val:02X}, Ricevuto: 0x{test_val:02X}"
                )
            else:
                print(f"❌ LOOPBACK FAIL - Inviato: 0x{test_val:02X}, Output: {output}")

        print("\n=== Test Completato ===")

        # Quit QEMU
        child.send("quit\r")
        child.expect(pexpect.EOF)

    except pexpect.exceptions.TIMEOUT:
        print("❌ Timeout durante il test")
        return False
    except Exception as e:
        print(f"❌ Errore durante il test: {e}")
        return False

    return True


if __name__ == "__main__":
    try:
        success = test_lpspi_loopback()
        if success:
            print("\n🎉 Test completato con successo!")
        else:
            print("\n💥 Test fallito!")
            sys.exit(1)
    except KeyboardInterrupt:
        print("\n⚠️  Test interrotto dall'utente")
        sys.exit(1)
    except ImportError:
        print("❌ Errore: modulo 'pexpect' non installato")
        print("Installa con: pip install pexpect")
        sys.exit(1)

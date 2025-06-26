#!/usr/bin/env python3
"""
Test finale per LPSPI - Verifica completa del loopback
"""
import subprocess
import time
import re
import sys


def test_lpspi_functionality():
    """Test completo del funzionamento LPSPI"""
    print("=== Test Funzionalità LPSPI Completa ===")

    # Test 1: Verifica che QEMU si avvii correttamente
    print("\n🔧 Test 1: Avvio QEMU e controllo basic functionality...")

    try:
        # Esegui QEMU per 10 secondi per catturare i log
        result = subprocess.run(
            [
                "./qemu/build/qemu-system-arm",
                "-machine",
                "nxps32k358evb",
                "-kernel",
                "Demo_FreeRTOS.elf",
                "-nographic",
            ],
            timeout=10,
            capture_output=True,
            text=True,
            cwd="/home/iaco/Polito/CAOS/Group7",
        )

    except subprocess.TimeoutExpired as e:
        output = ""
        if e.stdout:
            output += (
                e.stdout.decode("utf-8") if isinstance(e.stdout, bytes) else e.stdout
            )
        if e.stderr:
            output += (
                e.stderr.decode("utf-8") if isinstance(e.stderr, bytes) else e.stderr
            )

        print("Debug: Captured QEMU output successfully")

        # Analizza l'output per verificare il corretto funzionamento
        print("✅ QEMU eseguito con successo")

        # Test 2: Verifica configurazione LPSPI
        if "CFGR1 write: 0x01000001 - PINCFG=1, MASTER=1" in output:
            print("✅ Test 2: LPSPI configurato in modalità loopback")
        else:
            print("❌ Test 2: FALLITO - LPSPI non configurato correttamente")
            return False

        # Test 3: Verifica frame size
        if "TCR write: 0x00000007, calculated frame_size: 8" in output:
            print("✅ Test 3: Frame size configurato a 8-bit")
        else:
            print("❌ Test 3: FALLITO - Frame size non corretto")
            return False

        # Test 4: Verifica trasferimenti SPI
        transfers = re.findall(
            r"SPI transfer \(loopback\): tx=(0x[0-9a-fA-F]+) -> rx=(0x[0-9a-fA-F]+)",
            output,
        )

        if len(transfers) >= 12:
            print(f"✅ Test 4: Rilevati {len(transfers)} trasferimenti SPI")

            # Test 5: Verifica sequenza corretta
            expected_sequence = [f"0x{i:08x}" for i in range(0x10, 0x1C)]
            actual_tx = [tx for tx, rx in transfers[:12]]

            if actual_tx == expected_sequence:
                print("✅ Test 5: Sequenza TX corretta (0x10-0x1B)")
            else:
                print(f"❌ Test 5: FALLITO - Sequenza TX errata")
                print(f"   Atteso: {expected_sequence}")
                print(f"   Ricevuto: {actual_tx}")
                return False

            # Test 6: Verifica loopback perfetto
            loopback_ok = all(tx == rx for tx, rx in transfers[:12])
            if loopback_ok:
                print("✅ Test 6: Loopback perfetto - TX == RX per tutti i bytes")
            else:
                print("❌ Test 6: FALLITO - Loopback non funzionante")
                return False

            # Test 7: Verifica trasferimenti multipli
            if len(transfers) >= 24:  # Due cicli completi
                print("✅ Test 7: Trasferimenti multipli funzionanti")
            else:
                print(
                    "⚠️  Test 7: Solo un ciclo rilevato (potrebbero servire più cicli)"
                )

        else:
            print(
                f"❌ Test 4: FALLITO - Solo {len(transfers)} trasferimenti rilevati (attesi almeno 12)"
            )
            return False

        print("\n🎉 TUTTI I TEST PRINCIPALI SUPERATI!")
        print("✨ Il modulo LPSPI funziona correttamente in modalità loopback")
        print("✨ I trasferimenti SPI 8-bit sono operativi")
        print("✨ La sequenza di 12 bytes è trasferita correttamente")

        return True

    except Exception as e:
        print(f"❌ Errore durante l'esecuzione: {e}")
        return False


def test_interactive_mode():
    """Test modalità interattiva per debugging avanzato"""
    print("\n=== Modalità Test Interattiva ===")
    print(
        "Questa modalità permette di testare registri LPSPI specifici senza ricompilare"
    )

    # Avvia QEMU in background
    qemu_proc = subprocess.Popen(
        [
            "./qemu/build/qemu-system-arm",
            "-machine",
            "nxps32k358evb",
            "-kernel",
            "Demo_FreeRTOS.elf",
            "-nographic",
            "-monitor",
            "telnet:localhost:4444,server,nowait",
        ],
        cwd="/home/iaco/Polito/CAOS/Group7",
    )

    print("🚀 QEMU avviato con monitor su telnet:localhost:4444")
    print("💡 Puoi connetterti con: telnet localhost 4444")
    print("💡 Comandi utili:")
    print("   x/w 0x40358024  # Leggi CFGR1")
    print("   x/w 0x40358014  # Leggi SR")
    print("   x/w 0x40358060  # Leggi TCR")
    print("   x/w 0x40358074  # Leggi RDR")
    print("   x/w 0x40358064  # Leggi TDR")
    print("   info qtree      # Mostra albero dispositivi")
    print("   q               # Quit")

    try:
        # Aspetta che l'utente prema un tasto
        input("\n⏸️  Premi INVIO per terminare QEMU...")
    finally:
        qemu_proc.terminate()
        qemu_proc.wait()


if __name__ == "__main__":
    print("🔬 LPSPI Test Suite per NXP S32K358")
    print("=" * 50)

    # Test automatico
    success = test_lpspi_functionality()

    if success:
        print(f"\n🏆 RISULTATO: SUCCESS")
        print("Il modulo LPSPI è completamente funzionante!")

        # Opzione per test interattivo
        if sys.stdin.isatty():  # Solo se eseguito in terminale interattivo
            response = input("\nVuoi avviare la modalità test interattiva? (y/N): ")
            if response.lower() in ["y", "yes", "s", "si"]:
                test_interactive_mode()
        else:
            print("\n💡 Per test interattivi, esegui il comando da terminale")
    else:
        print(f"\n💥 RISULTATO: FAILED")
        print("Ci sono problemi nel modulo LPSPI che richiedono attenzione")
        sys.exit(1)

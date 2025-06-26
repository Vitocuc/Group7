#!/usr/bin/env python3
"""
Script to test the corrected LPSPI implementation in QEMU.
This script demonstrates the proper LPSPI usage with 8-bit frames and loopback mode.
"""

import subprocess
import time
import threading
import sys

def test_lpspi_with_gdb():
    """Test LPSPI using GDB to inject correct register values"""
    
    print("Starting QEMU with GDB server...")
    
    # Start QEMU with GDB server
    qemu_cmd = [
        './qemu-system-arm',
        '-M', 'nxps32k358evb',
        '-nographic',
        '-kernel', '../../Demo_FreeRTOS.elf',
        '-serial', 'none',
        '-serial', 'none', 
        '-serial', 'none',
        '-serial', 'mon:stdio',
        '-d', 'guest_errors',
        '-S',  # Start paused
        '-gdb', 'tcp::1234'  # GDB server on port 1234
    ]
    
    # Start QEMU process
    qemu_process = subprocess.Popen(
        qemu_cmd,
        cwd='/home/iaco/Polito/CAOS/Group7/qemu/build',
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True
    )
    
    # Give QEMU time to start
    time.sleep(2)
    
    print("Starting GDB...")
    
    # GDB commands to test LPSPI properly
    gdb_commands = """
set confirm off
target remote localhost:1234
continue

# Break when LPSPI is being used
break *0x40330000

# Let the program run until it hits LPSPI
continue

# Configure LPSPI for proper 8-bit operation with loopback
# LPSPI0 base address: 0x40330000

# Reset LPSPI
set *0x40330010 = 0x2
set *0x40330010 = 0x0

# Configure CFGR1: Master mode + Loopback (PINCFG=1)
set *0x40330024 = 0x01000001

# Enable module
set *0x40330010 = 0x1

# Test single 8-bit transfer
# Configure TCR for 8-bit frame (FRAMESZ=7, PCS=0)
set *0x40330060 = 0x00000007

# Send test data 0xAB
set *0x40330064 = 0xAB

# Check status register
x/1wx 0x40330014

# Read received data
x/1wx 0x40330074

continue
quit
"""
    
    # Write GDB commands to a file
    with open('/tmp/gdb_lpspi_test.gdb', 'w') as f:
        f.write(gdb_commands)
    
    # Run GDB with commands
    gdb_cmd = ['gdb', '-batch', '-x', '/tmp/gdb_lpspi_test.gdb']
    
    try:
        gdb_process = subprocess.run(
            gdb_cmd,
            capture_output=True,
            text=True,
            timeout=30
        )
        
        print("GDB Output:")
        print(gdb_process.stdout)
        if gdb_process.stderr:
            print("GDB Errors:")
            print(gdb_process.stderr)
            
    except subprocess.TimeoutExpired:
        print("GDB test timed out")
    finally:
        qemu_process.terminate()
        qemu_process.wait()

def test_lpspi_monitor():
    """Test LPSPI using QEMU monitor commands"""
    
    print("Testing LPSPI with QEMU monitor commands...")
    
    # Start QEMU with monitor
    qemu_cmd = [
        './qemu-system-arm',
        '-M', 'nxps32k358evb',
        '-nographic', 
        '-kernel', '../../Demo_FreeRTOS.elf',
        '-serial', 'none',
        '-serial', 'none',
        '-serial', 'none', 
        '-serial', 'mon:stdio',
        '-d', 'guest_errors'
    ]
    
    print("Starting QEMU...")
    print("Commands to test LPSPI manually:")
    print("1. Let the system boot")
    print("2. Press Ctrl+A, then C to enter QEMU monitor")
    print("3. Use these commands to test LPSPI:")
    print("")
    print("   # Reset and configure LPSPI")
    print("   xp /1w 0x40330000   # Check VERID")
    print("   xp /1w 0x40330004   # Check PARAM") 
    print("   x 0x40330010 2      # Reset LPSPI")
    print("   x 0x40330010 0      # Clear reset")
    print("   x 0x40330024 0x01000001  # Master + Loopback")
    print("   x 0x40330010 1      # Enable module")
    print("")
    print("   # Test 8-bit transfer")
    print("   x 0x40330060 7      # TCR: 8-bit frame")
    print("   x 0x40330064 0xAB   # Send 0xAB")
    print("   xp /1w 0x40330014   # Check status")
    print("   xp /1w 0x40330074   # Read received data (should be 0xAB)")
    print("")
    print("4. Type 'quit' to exit")
    print("")
    
    subprocess.run(qemu_cmd, cwd='/home/iaco/Polito/CAOS/Group7/qemu/build')

if __name__ == '__main__':
    print("LPSPI Test Script")
    print("================")
    print("")
    print("This script tests the corrected LPSPI implementation.")
    print("The LPSPI should now support:")
    print("- Proper 8-bit frame sizes")
    print("- Loopback mode for testing")
    print("- Correct register handling")
    print("")
    
    choice = input("Choose test method:\n1. Monitor commands (recommended)\n2. GDB automation\nEnter choice (1 or 2): ")
    
    if choice == '1':
        test_lpspi_monitor()
    elif choice == '2':
        test_lpspi_with_gdb()
    else:
        print("Invalid choice")
        sys.exit(1)

# Motor Speed Sensor Device Documentation

## Overview

The Motor Speed Sensor is a QEMU device model that emulates a simple SPI-based motor speed sensor. It responds to SPI commands by generating random speed values, simulating a real motor speed sensor for testing and development purposes. The device implements a simple command-response protocol where the master can request speed readings via SPI communication.

---

## Header File: `motor_speed.h`

### Key Definitions

-   **Type Definition**: `TYPE_MOTOR_SPEED_SENSOR` - QEMU object type string for the device.
-   **Command Protocol**: Defines the SPI command interface for communication.
-   **Device Structure**: Defines the internal state and data storage.

### Key Constants

-   **Device Type**:
    -   `TYPE_MOTOR_SPEED_SENSOR`: "motor-speed-sensor" - Device type identifier.
-   **Command Protocol**:
    -   `CMD_GET_SPEED`: 0xAA - Command to request motor speed reading.

### Key Structures

-   **`MotorSpeedSensor`**: Represents the state of the motor speed sensor device, including:
    -   **Parent Object**: `SSIPeripheral parent_obj` - Base SPI peripheral object.
    -   **State Variables**:
        -   `uint16_t last_speed`: Last generated speed value (in RPM).
        -   `uint16_t transfer_value`: Value that will be transferred to the SPI master.

---

## Source File: `motor_speed.c`

### Key Functions

#### `generate_random_speed()`

-   **Purpose**: Generates a random speed value to simulate motor speed readings.

-   **Functionality**:
    -   Generates a random speed value between 0 and 254 RPM.
    -   Uses standard C `rand()` function with modulo operation.
    -   Returns a 16-bit unsigned integer representing motor speed.

#### `motor_speed_sensor_transfer()`

-   **Purpose**: Handles SPI data transfer operations when the master device communicates with the sensor.

-   **Functionality**:

    -   Receives incoming SPI command from the master device.
    -   Processes the command using a switch statement:
        -   For `CMD_GET_SPEED` (0xAA): Generates new random speed and stores it.
        -   For any other command: No specific action, returns last transfer value.
    -   Updates internal state variables (`last_speed` and `transfer_value`).
    -   Returns the current transfer value to the SPI master.

-   **Parameters**:

    -   `SSIPeripheral *dev`: Pointer to the SPI peripheral device.
    -   `uint32_t val`: Command value received from the SPI master.

-   **Return Value**:
    -   The current `transfer_value` to be sent back to the master.

#### `motor_speed_sensor_realize()`

-   **Purpose**: Initializes the motor speed sensor device during its realization phase in the QEMU device model.

-   **Functionality**:

    -   Casts the generic SSI peripheral to the specific motor speed sensor type.
    -   Initializes device state variables:
        -   Sets `last_speed` to 0 (initial speed reading).
        -   Sets `transfer_value` to 0 (initial transfer value).
    -   Seeds the random number generator using current time for varied speed readings.

-   **Parameters**:
    -   `SSIPeripheral *dev`: Pointer to the SPI peripheral device being realized.
    -   `Error **errp`: Error reporting parameter (unused in this implementation).

#### `motor_speed_sensor_class_init()`

-   **Purpose**: Initializes the class-level properties of the motor speed sensor device type, preparing it for use within the QEMU device model infrastructure.

-   **Functionality**:
    -   **Device Class Casting**: Casts the provided `ObjectClass` to `SSIPeripheralClass` to access SPI-specific initialization hooks.
    -   **Transfer Function Assignment**: Sets the `transfer` function pointer to `motor_speed_sensor_transfer`, defining how SPI data transfers are handled.
    -   **Realize Function Assignment**: Sets the `realize` function pointer to `motor_speed_sensor_realize`, defining how the device is instantiated and initialized at runtime.
    -   **Chip Select Configuration**: Sets `cs_polarity` to `SSI_CS_NONE`, indicating the device doesn't require specific chip select polarity handling.

#### `motor_speed_sensor_register_types()`

-   **Purpose**: Registers the motor speed sensor device type with the QEMU type system so it can be instantiated and used during emulation.

-   **Functionality**:
    -   Calls `type_register_static()` with the `motor_speed_sensor_type_info` structure to make the device type available to the QEMU object model.

### Key Constants

#### Device Type Information

-   **Name**: The device type name, set to `TYPE_MOTOR_SPEED_SENSOR`.
-   **Parent**: The parent class, set to `TYPE_SSI_PERIPHERAL`.
-   **Instance Size**: The size of the device instance, set to `sizeof(MotorSpeedSensor)`.
-   **Class Initialization**: Points to `motor_speed_sensor_class_init` function.

---

## Communication Protocol

### SPI Command Interface

| Command       | Value | Description                 | Response                   |
| ------------- | ----- | --------------------------- | -------------------------- |
| CMD_GET_SPEED | 0xAA  | Request current motor speed | Random speed value (0-254) |
| Any other     | N/A   | Unknown command             | Last transfer value        |

### Data Flow

1. **Master sends command**: SPI master transmits command byte to sensor
2. **Command processing**: Sensor processes command in `motor_speed_sensor_transfer()`
3. **Response generation**:
    - For 0xAA: Generate new random speed value
    - For others: Use last transfer value
4. **Data return**: Sensor returns response value to master

---

## Key Features

### Simple Command Protocol

-   Single command interface for speed reading requests.
-   Command-response pattern with immediate data return.
-   Stateless operation - each speed request generates new random value.

### Random Speed Generation

-   Generates realistic speed values between 0-254 RPM.
-   Uses time-seeded random number generator for variation.
-   Simulates dynamic motor behavior for testing purposes.

### SPI Integration

-   Full integration with QEMU SPI subsystem.
-   Standard SSI peripheral interface implementation.
-   Compatible with any SPI master device in QEMU.

### Device Model Features

-   Proper QEMU device registration and type system integration.
-   Standard realize/class_init pattern following QEMU conventions.
-   Minimal resource usage with simple state management.

---

## Usage Example

```c
// In QEMU machine configuration
// The motor speed sensor can be attached to any SPI bus
// Master device sends 0xAA command to get speed reading
uint8_t command = CMD_GET_SPEED;  // 0xAA
uint8_t speed = spi_transfer(command);  // Returns random value 0-254
```

---

## Implementation Notes

### Random Number Generation

-   Uses standard C library `rand()` function
-   Seeded with `time(NULL)` during device initialization
-   Range limited to 0-254 through modulo operation

### State Management

-   Minimal state with only two 16-bit variables
-   No persistent storage or complex state transitions
-   Immediate response to commands without buffering

### Error Handling

-   No explicit error conditions defined
-   Unknown commands simply return last transfer value
-   Robust operation under all input conditions

---

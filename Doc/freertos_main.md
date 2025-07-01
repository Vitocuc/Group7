# FreeRTOS Main Application Documentation

## Overview

The FreeRTOS main application (`main.c`) is a comprehensive real-time operating system application designed for the NXP S32K358 microcontroller platform. It implements a multi-task motor speed monitoring system using FreeRTOS services including tasks, semaphores, mutexes, and timers. The application communicates with an external motor speed sensor via SPI and provides debugging output through UART. The system is designed to run on QEMU emulation of the S32K358 platform with full integration of hardware abstraction layers and FreeRTOS kernel services.

---

## Source File: `main.c`

### Key Definitions

-   **Motor Command**: `CMD_GET_SPEED` (0xAA) - SPI command to request motor speed data from sensor.
-   **Speed Threshold**: `HIGH_SPEED_THRESHOLD` (200) - RPM threshold for high-speed warning detection.
-   **UART Channel**: `UART_LPUART_INTERNAL_CHANNEL` (3) - Internal UART channel for debug output.
-   **SPI Device**: `MASTER_EXTERNAL_DEVICE` - External SPI device configuration for motor sensor.

### Key Constants

-   **Command Constants**:
    -   `CMD_GET_SPEED`: 0xAA - Command byte sent to motor sensor to request current speed.
-   **Threshold Constants**:
    -   `HIGH_SPEED_THRESHOLD`: 200 - RPM value above which high-speed warning is triggered.
-   **Hardware Constants**:
    -   `UART_LPUART_INTERNAL_CHANNEL`: 3 - LPUART instance used for debug communication.
-   **FreeRTOS Task Configuration**:
    -   `configMINIMAL_STACK_SIZE * 4`: Stack size for main tasks (ReadSpeedTask, CheckSpeedTask).
    -   `configMINIMAL_STACK_SIZE * 2`: Stack size for auxiliary task (TaskCodeC).
    -   `configMAX_PRIORITIES-2`: Priority level for main tasks.
    -   `configMAX_PRIORITIES-1`: Priority level for auxiliary task.
-   **Timer Configuration**:
    -   `pdMS_TO_TICKS(10000)`: Timer period of 10 seconds (10000ms).
    -   `pdTRUE`: Timer auto-reload enabled.

### Global Variables

-   **Motor Data**:
    -   `volatile uint8_t g_last_speed`: Last motor speed reading in RPM (shared between tasks).
-   **FreeRTOS Synchronization Objects**:
    -   `SemaphoreHandle_t xSemaphore`: Binary semaphore for Task A synchronization.
    -   `SemaphoreHandle_t ySemaphore`: Binary semaphore for Task B synchronization.
    -   `SemaphoreHandle_t zSemaphore`: Binary semaphore for Task C synchronization.
    -   `SemaphoreHandle_t xMutex`: Mutex for exclusive UART access.
    -   `TimerHandle_t xTimers`: Software timer for periodic Task C activation.
-   **Task Creation Status**:
    -   `BaseType_t xReturned`: Return status for Task A creation.
    -   `BaseType_t yReturned`: Return status for Task B and C creation.

### Functions

#### Utility Functions

-   **`uint8_to_str(uint8_t value, char *buffer)`**:

    -   Converts an 8-bit unsigned integer to its string representation.
    -   Parameters: `value` - integer to convert, `buffer` - output string buffer.
    -   Used for converting motor speed values to printable format.

-   **`UART_send_byte(uint8_t byte)`**:

    -   Sends a single byte via UART using synchronous transmission.
    -   Parameters: `byte` - data byte to transmit.
    -   Timeout: 100ms for transmission completion.

-   **`print(char* str)`**:
    -   Thread-safe string printing function using mutex protection.
    -   Parameters: `str` - null-terminated string to print.
    -   Acquires mutex before transmission and releases after completion.

#### Hardware Interface Functions

-   **`Motor_Sensor_ReadValue(uint8_t cmd, uint8_t* rx_buff)`**:
    -   Performs SPI transaction with motor speed sensor.
    -   Parameters: `cmd` - command byte to send, `rx_buff` - buffer for received data.
    -   Uses synchronous SPI transfer with 1000ms timeout.
    -   Single-transaction operation returns immediate sensor response.

#### FreeRTOS Task Functions

-   **`ReadSpeedTask(void * pvParameters)`**:

    -   **Purpose**: Task A - Periodic motor speed reading and display.
    -   **Period**: 1000ms (1 second intervals).
    -   **Synchronization**: Takes `xSemaphore`, gives `ySemaphore`.
    -   **Operation**: Reads motor speed via SPI, updates global variable, prints result.
    -   **Priority**: `configMAX_PRIORITIES-2`.

-   **`CheckSpeedTask(void * pvParameters)`**:

    -   **Purpose**: Task B - Motor speed analysis and status reporting.
    -   **Period**: 1000ms (1 second intervals).
    -   **Synchronization**: Takes `ySemaphore`, gives `xSemaphore`.
    -   **Operation**: Analyzes global speed value, prints warning or normal status.
    -   **Priority**: `configMAX_PRIORITIES-2`.

-   **`TaskCodeC(void * pvParameters)`**:
    -   **Purpose**: Task C - Timer-triggered auxiliary task.
    -   **Activation**: Timer-driven via semaphore (10-second intervals).
    -   **Synchronization**: Takes `zSemaphore` (given by timer callback).
    -   **Operation**: Prints timer activation message.
    -   **Priority**: `configMAX_PRIORITIES-1` (highest priority).

#### FreeRTOS Timer Handler

-   **`vTimerCallback(TimerHandle_t xTimer)`**:
    -   **Purpose**: Software timer callback function.
    -   **Period**: 10 seconds (auto-reload enabled).
    -   **Operation**: Releases `zSemaphore` to activate Task C.
    -   **Type**: Periodic timer with automatic restart.

#### Debug Functions

-   **`TestMPU()`**:
    -   **Purpose**: Memory Protection Unit testing function (currently commented out).
    -   **Operation**: Tests memory access permissions in different regions.
    -   **Test Cases**: SRAM write (allowed), Flash read (allowed), Flash write (should fault).

### Main Function Flow

1. **System Initialization**:

    - Clock system configuration
    - GPIO pin multiplexing setup
    - Interrupt controller initialization

2. **Peripheral Initialization**:

    - LPUART configuration for debug output
    - LPSPI configuration for motor sensor communication

3. **FreeRTOS Object Creation**:

    - Mutex creation for UART access protection
    - Semaphore creation for task synchronization
    - Task creation with appropriate stack sizes and priorities
    - Software timer creation with 10-second period

4. **Scheduler Activation**:
    - Start FreeRTOS scheduler
    - Transfer control to real-time kernel

---

## Features

### Real-Time Operating System Integration

-   **Multi-Task Architecture**: Three concurrent tasks with different priorities and synchronization patterns.
-   **Semaphore Synchronization**: Producer-consumer pattern between Task A and Task B using binary semaphores.
-   **Mutex Protection**: Thread-safe UART access using mutual exclusion.
-   **Software Timers**: Periodic timer-driven task activation with auto-reload capability.

### Hardware Abstraction Layer Integration

-   **Clock Management**: Full integration with NXP Clock IP driver for system clock configuration.
-   **GPIO Configuration**: Pin multiplexing setup using SIUL2 Port IP driver.
-   **Interrupt Handling**: Interrupt controller configuration for proper RTOS operation.
-   **UART Communication**: LPUART driver integration for debug output with configurable instances.
-   **SPI Communication**: LPSPI driver integration for motor sensor communication with timeout handling.

### Motor Speed Monitoring System

-   **Sensor Interface**: SPI-based communication with external motor speed sensor.
-   **Real-Time Monitoring**: Continuous 1-second interval speed readings.
-   **Threshold Detection**: Automatic high-speed warning system with configurable threshold.
-   **Status Reporting**: Real-time status messages via UART for system monitoring.

### Error Handling and Robustness

-   **Initialization Validation**: Comprehensive error checking for all FreeRTOS object creation.
-   **Timeout Protection**: SPI and UART operations with timeout mechanisms.
-   **Stack Overflow Protection**: Increased stack sizes to prevent overflow conditions.
-   **Synchronization Safety**: Proper semaphore and mutex usage to prevent race conditions.

### Development and Debug Features

-   **Debug Output**: Comprehensive UART-based logging with task identification.
-   **Memory Protection Testing**: MPU functionality testing for memory safety validation.
-   **Task Status Monitoring**: Built-in task creation status reporting.
-   **Real-Time Feedback**: Immediate system response indication through debug messages.

---

## Data Flow

1. **Task A**: Reads motor speed via SPI → Updates global variable → Signals Task B
2. **Task B**: Reads global variable → Analyzes speed → Reports status → Signals Task A
3. **Task C**: Activated by timer → Prints timer message → Waits for next timer event

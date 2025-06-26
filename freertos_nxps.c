/* Include dei file di sistema e dei driver */
#include "Clock_Ip.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "Lpspi_Ip.h"
#include "Lpspi_Ip_Sa_PBcfg.h"
#include "Siul2_Port_Ip.h"
#include "IntCtrl_Ip.h"
#include "Lpuart_Uart_Ip.h"
#include "Lpuart_Uart_Ip_Irq.h"
#include "string.h" // Per memset e memcmp
#include "stdio.h"  // Per i messaggi di debug (se si usa una LPUART)

/* Priorità dei Task */
#define main_TASK_PRIORITY (tskIDLE_PRIORITY + 2)

/* Definizioni per LPSPI */
#define LPSPI_INSTANCE (0)   // Usiamo LPSPI0
#define SPI_BUFFER_SIZE (12) // Dimensione del buffer di trasferimento

#define UART_LPUART_INTERNAL_CHANNEL 3

/* Direct register access for LPSPI configuration */
#define LPSPI0_BASE_ADDR 0x40358000

/* LPSPI Register offsets */
#define LPSPI_CR_OFFSET 0x10
#define LPSPI_SR_OFFSET 0x14
#define LPSPI_IER_OFFSET 0x18
#define LPSPI_CFGR0_OFFSET 0x20
#define LPSPI_CFGR1_OFFSET 0x24
#define LPSPI_CCR_OFFSET 0x40
#define LPSPI_FCR_OFFSET 0x58
#define LPSPI_FSR_OFFSET 0x5C
#define LPSPI_TCR_OFFSET 0x60
#define LPSPI_TDR_OFFSET 0x64
#define LPSPI_RSR_OFFSET 0x70
#define LPSPI_RDR_OFFSET 0x74

/* LPSPI Register bit definitions */
#define LPSPI_CR_MEN (1U << 0)
#define LPSPI_CR_RST (1U << 1)
#define LPSPI_CR_RTF (1U << 8)
#define LPSPI_CR_RRF (1U << 9)

#define LPSPI_SR_TDF (1U << 0)
#define LPSPI_SR_RDF (1U << 1)
#define LPSPI_SR_TCF (1U << 10)
#define LPSPI_SR_MBF (1U << 24)

#define LPSPI_CFGR1_MASTER (1U << 0)
#define LPSPI_CFGR1_PINCFG_LOOPBACK (1U << 24) // Loopback mode for testing

#define LPSPI_TCR_FRAMESZ(n) ((n - 1) << 0) // Frame size (n-1), so for 8-bit use n=8
#define LPSPI_TCR_PCS(n) ((n) << 24)        // Chip select
#define LPSPI_TCR_CPOL (1U << 31)           // Clock polarity
#define LPSPI_TCR_CPHA (1U << 30)           // Clock phase

/* Register access macros */
#define REG32(addr) (*(volatile uint32_t *)(addr))
#define LPSPI0_REG(offset) REG32(LPSPI0_BASE_ADDR + offset)

/* Messaggi di debug */
#define SEND_MSG "SendTask: Preparo e invio dati via SPI (8-bit frames)...\n"
#define RECV_SUCCESS_MSG "ReceiveTask: Trasferimento SPI completato con SUCCESSO!\n"
#define RECV_FAILURE_MSG "ReceiveTask: ERRORE nel trasferimento SPI!\n"
#define START_MSG "Inizializzazione completata! LPSPI configurato per 8-bit frames in loopback.\n"
#define CALLBACK_MSG "Sono nella callback! \n"
#define SEND_SUCCESS_MSG "SendTask: Trasferimento SPI completato con SUCCESSO!\n"

/* Semafori per la sincronizzazione */
SemaphoreHandle_t producer_go;
SemaphoreHandle_t transfer_complete_sem;

/* Buffer di trasmissione e ricezione */
uint8_t txBuffer[SPI_BUFFER_SIZE];
uint8_t rxBuffer[SPI_BUFFER_SIZE];

/* Contatore per vedere l'attività */
volatile uint32_t g_transfer_count = 0;

/**
 * @brief Configurazione diretta del registri LPSPI per test
 */
void configure_lpspi_direct(void)
{
    // Reset LPSPI
    LPSPI0_REG(LPSPI_CR_OFFSET) = LPSPI_CR_RST;

    // Simple delay loop instead of vTaskDelay (FreeRTOS not started yet)
    for (volatile int i = 0; i < 10000; i++)
    { /* wait */
    }

    // Clear reset
    LPSPI0_REG(LPSPI_CR_OFFSET) = 0;

    // Configure as master with loopback for testing
    LPSPI0_REG(LPSPI_CFGR1_OFFSET) = LPSPI_CFGR1_MASTER | LPSPI_CFGR1_PINCFG_LOOPBACK;

    // Configure clock divider (simple setup)
    LPSPI0_REG(LPSPI_CCR_OFFSET) = 0x04040404; // Set clock dividers

    // Configure FIFO watermarks
    LPSPI0_REG(LPSPI_FCR_OFFSET) = 0x00000000; // TX watermark=0, RX watermark=0

    // Enable module
    LPSPI0_REG(LPSPI_CR_OFFSET) = LPSPI_CR_MEN;

    // Skip complex sprintf/UART calls that might cause issues
    // The debug info will come from QEMU logs instead
}

/**
 * @brief Transfer SPI diretto usando i registri
 */
int direct_spi_transfer(uint8_t *tx_data, uint8_t *rx_data, size_t len)
{
    for (size_t i = 0; i < len; i++)
    {
        // Configure TCR for 8-bit frame
        uint32_t tcr_val = LPSPI_TCR_FRAMESZ(8) | LPSPI_TCR_PCS(0);
        LPSPI0_REG(LPSPI_TCR_OFFSET) = tcr_val;

        // Wait for TDF (TX FIFO ready) - with timeout
        int timeout = 10000; // Increase timeout
        while (!(LPSPI0_REG(LPSPI_SR_OFFSET) & LPSPI_SR_TDF) && timeout-- > 0)
        { /* wait */
        }
        if (timeout <= 0)
            return -1; // Timeout on TDF

        // Send data
        LPSPI0_REG(LPSPI_TDR_OFFSET) = tx_data[i];

        // Wait for TCF (Transfer Complete) - with timeout
        timeout = 10000; // Increase timeout
        while (!(LPSPI0_REG(LPSPI_SR_OFFSET) & LPSPI_SR_TCF) && timeout-- > 0)
        { /* wait */
        }
        if (timeout <= 0)
            return -2; // Timeout on TCF

        // Clear TCF flag
        LPSPI0_REG(LPSPI_SR_OFFSET) = LPSPI_SR_TCF;

        // Wait for RDF (RX data available) - with timeout
        timeout = 10000; // Increase timeout
        while (!(LPSPI0_REG(LPSPI_SR_OFFSET) & LPSPI_SR_RDF) && timeout-- > 0)
        { /* wait */
        }
        if (timeout <= 0)
            return -3; // Timeout on RDF

        // Read received data
        rx_data[i] = LPSPI0_REG(LPSPI_RDR_OFFSET) & 0xFF;

        // Clear RDF flag to prepare for next transfer
        LPSPI0_REG(LPSPI_SR_OFFSET) = LPSPI_SR_RDF;
    }

    return 0; // Success
}

/**
 * @brief Task Produttore: prepara i dati e avvia il trasferimento SPI.
 */
void SendTask(void *pvParameters)
{
    (void)pvParameters;

    for (;;)
    {
        // Attendi il via libera per iniziare a produrre
        xSemaphoreTake(producer_go, portMAX_DELAY);

        // Prepara i dati da inviare
        for (uint16_t i = 0; i < SPI_BUFFER_SIZE; i++)
        {
            txBuffer[i] = (uint8_t)(0x10 + i); // Dati di test: 0x10, 0x11, 0x12, ...
        }
        // Pulisci il buffer di ricezione
        memset(rxBuffer, 0, SPI_BUFFER_SIZE);

        // Esegui trasferimento SPI diretto
        int result = direct_spi_transfer(txBuffer, rxBuffer, SPI_BUFFER_SIZE);

        if (result == 0)
        {
            // Transfer successful - print some debug info
            char debug_msg[200];
            sprintf(debug_msg, "SendTask: Transfer complete. TX[0]=0x%02x, RX[0]=0x%02x, TX[11]=0x%02x, RX[11]=0x%02x\n",
                    txBuffer[0], rxBuffer[0], txBuffer[11], rxBuffer[11]);
            Lpuart_Uart_Ip_SyncSend(UART_LPUART_INTERNAL_CHANNEL, (uint8_t *)debug_msg, strlen(debug_msg), 200);
            xSemaphoreGive(transfer_complete_sem);
        }
        else
        {
            // Transfer failed - print error code
            char error_msg[100];
            sprintf(error_msg, "SendTask: Transfer FAILED with error code %d\n", result);
            Lpuart_Uart_Ip_SyncSend(UART_LPUART_INTERNAL_CHANNEL, (uint8_t *)error_msg, strlen(error_msg), 200);
            xSemaphoreGive(transfer_complete_sem);
        }

        vTaskDelay(pdMS_TO_TICKS(2000)); // Wait 2 seconds between transfers
    }
}

/**
 * @brief Task Consumatore: attende la fine del trasferimento e verifica i dati.
 */
void ReceiveTask(void *pvParameters)
{
    (void)pvParameters;
    char msg_buffer[100];

    for (;;)
    {
        // Attendi che il trasferimento sia completo
        xSemaphoreTake(transfer_complete_sem, portMAX_DELAY);

        g_transfer_count++; // Verifica i dati ricevuti (in loopback dovrebbero essere uguali)
        bool success = true;
        int first_error = -1;

        for (int i = 0; i < SPI_BUFFER_SIZE; i++)
        {
            if (txBuffer[i] != rxBuffer[i])
            {
                success = false;
                if (first_error == -1)
                    first_error = i;
                break; // Exit early if mismatch found
            }
        }

        if (success)
        {
            sprintf(msg_buffer, "Transfer #%lu: SUCCESS! All %d bytes match.\n", g_transfer_count, SPI_BUFFER_SIZE);
            Lpuart_Uart_Ip_SyncSend(UART_LPUART_INTERNAL_CHANNEL, (uint8_t *)msg_buffer, strlen(msg_buffer), 200);
        }
        else
        {
            sprintf(msg_buffer, "Transfer #%lu: FAILED at byte %d: TX=0x%02x, RX=0x%02x\n",
                    g_transfer_count, first_error, txBuffer[first_error], rxBuffer[first_error]);
            Lpuart_Uart_Ip_SyncSend(UART_LPUART_INTERNAL_CHANNEL, (uint8_t *)msg_buffer, strlen(msg_buffer), 200);
        }

        // Dai il via libera al produttore per iniziare un nuovo ciclo
        xSemaphoreGive(producer_go);
    }
}

/**
 * @brief Funzione Main
 */
int main(void)
{
    /* Inizializzazione di base del sistema */
    Clock_Ip_Init(&Clock_Ip_aClockConfig[0]);
    Siul2_Port_Ip_Init(NUM_OF_CONFIGURED_PINS0, g_pin_mux_InitConfigArr0);

    /* Inizializzazione del controllore degli interrupt */
    IntCtrl_Ip_Init(&IntCtrlConfig_0);
    Lpuart_Uart_Ip_Init(UART_LPUART_INTERNAL_CHANNEL, &Lpuart_Uart_Ip_xHwConfigPB_3);

    /* Simple message without sprintf to avoid issues */
    const char *init_msg = "System initialized. Configuring LPSPI...\n";
    Lpuart_Uart_Ip_SyncSend(UART_LPUART_INTERNAL_CHANNEL, (uint8_t *)init_msg, strlen(init_msg), 200);

    /* Configurazione diretta LPSPI */
    configure_lpspi_direct();

    /* Simple success message */
    const char *config_msg = "LPSPI configured. Starting FreeRTOS...\n";
    Lpuart_Uart_Ip_SyncSend(UART_LPUART_INTERNAL_CHANNEL, (uint8_t *)config_msg, strlen(config_msg), 200);

    /* Crea i semafori binari */
    producer_go = xSemaphoreCreateBinary();
    transfer_complete_sem = xSemaphoreCreateBinary();

    /* Crea i due task */
    xTaskCreate(SendTask, "SendTask", configMINIMAL_STACK_SIZE + 500, NULL, main_TASK_PRIORITY, NULL);
    xTaskCreate(ReceiveTask, "RecTask", configMINIMAL_STACK_SIZE + 500, NULL, main_TASK_PRIORITY, NULL);

    /* Dai il via al primo ciclo dando il semaforo al produttore */
    xSemaphoreGive(producer_go);

    /* Avvia lo scheduler di FreeRTOS */
    vTaskStartScheduler();

    for (;;)
        ;
    return 0;
}

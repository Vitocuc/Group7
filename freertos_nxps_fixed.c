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
#define LPSPI0_BASE_ADDR 0x40330000

/* LPSPI Register offsets */
#define LPSPI_CR_OFFSET     0x10
#define LPSPI_SR_OFFSET     0x14
#define LPSPI_IER_OFFSET    0x18
#define LPSPI_CFGR0_OFFSET  0x20
#define LPSPI_CFGR1_OFFSET  0x24
#define LPSPI_CCR_OFFSET    0x40
#define LPSPI_FCR_OFFSET    0x58
#define LPSPI_FSR_OFFSET    0x5C
#define LPSPI_TCR_OFFSET    0x60
#define LPSPI_TDR_OFFSET    0x64
#define LPSPI_RSR_OFFSET    0x70
#define LPSPI_RDR_OFFSET    0x74

/* LPSPI Register bit definitions */
#define LPSPI_CR_MEN        (1U << 0)
#define LPSPI_CR_RST        (1U << 1)
#define LPSPI_CR_RTF        (1U << 8)
#define LPSPI_CR_RRF        (1U << 9)

#define LPSPI_SR_TDF        (1U << 0)
#define LPSPI_SR_RDF        (1U << 1)
#define LPSPI_SR_TCF        (1U << 10)
#define LPSPI_SR_MBF        (1U << 24)

#define LPSPI_CFGR1_MASTER  (1U << 0)
#define LPSPI_CFGR1_PINCFG_LOOPBACK (1U << 24)  // Loopback mode for testing

#define LPSPI_TCR_FRAMESZ(n) ((n-1) << 0)  // Frame size (n-1), so for 8-bit use n=8
#define LPSPI_TCR_PCS(n)     ((n) << 24)    // Chip select
#define LPSPI_TCR_CPOL       (1U << 31)     // Clock polarity
#define LPSPI_TCR_CPHA       (1U << 30)     // Clock phase

/* Register access macros */
#define REG32(addr) (*(volatile uint32_t*)(addr))
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
    char msg_buffer[100];
    
    // Reset LPSPI
    LPSPI0_REG(LPSPI_CR_OFFSET) = LPSPI_CR_RST;
    vTaskDelay(pdMS_TO_TICKS(10));
    
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
    
    sprintf(msg_buffer, "LPSPI configurato: CR=0x%08lx, CFGR1=0x%08lx\n", 
            LPSPI0_REG(LPSPI_CR_OFFSET), LPSPI0_REG(LPSPI_CFGR1_OFFSET));
    Lpuart_Uart_Ip_SyncSend(UART_LPUART_INTERNAL_CHANNEL, (uint8_t *)msg_buffer, strlen(msg_buffer), 200);
}

/**
 * @brief Transfer SPI diretto usando i registri
 */
int direct_spi_transfer(uint8_t *tx_data, uint8_t *rx_data, size_t len)
{
    char msg_buffer[100];
    
    for (size_t i = 0; i < len; i++)
    {
        // Configure TCR for 8-bit frame
        uint32_t tcr_val = LPSPI_TCR_FRAMESZ(8) | LPSPI_TCR_PCS(0);
        LPSPI0_REG(LPSPI_TCR_OFFSET) = tcr_val;
        
        sprintf(msg_buffer, "TCR configurato: 0x%08lx (frame_size=8)\n", tcr_val);
        Lpuart_Uart_Ip_SyncSend(UART_LPUART_INTERNAL_CHANNEL, (uint8_t *)msg_buffer, strlen(msg_buffer), 200);
        
        // Wait for TDF (TX FIFO ready)
        while (!(LPSPI0_REG(LPSPI_SR_OFFSET) & LPSPI_SR_TDF))
        {
            vTaskDelay(pdMS_TO_TICKS(1));
        }
        
        // Send data
        LPSPI0_REG(LPSPI_TDR_OFFSET) = tx_data[i];
        
        sprintf(msg_buffer, "Inviato: 0x%02x\n", tx_data[i]);
        Lpuart_Uart_Ip_SyncSend(UART_LPUART_INTERNAL_CHANNEL, (uint8_t *)msg_buffer, strlen(msg_buffer), 200);
        
        // Wait for TCF (Transfer Complete)
        while (!(LPSPI0_REG(LPSPI_SR_OFFSET) & LPSPI_SR_TCF))
        {
            vTaskDelay(pdMS_TO_TICKS(1));
        }
        
        // Clear TCF flag
        LPSPI0_REG(LPSPI_SR_OFFSET) = LPSPI_SR_TCF;
        
        // Wait for RDF (RX data available)
        while (!(LPSPI0_REG(LPSPI_SR_OFFSET) & LPSPI_SR_RDF))
        {
            vTaskDelay(pdMS_TO_TICKS(1));
        }
        
        // Read received data
        rx_data[i] = LPSPI0_REG(LPSPI_RDR_OFFSET) & 0xFF;
        
        sprintf(msg_buffer, "Ricevuto: 0x%02x\n", rx_data[i]);
        Lpuart_Uart_Ip_SyncSend(UART_LPUART_INTERNAL_CHANNEL, (uint8_t *)msg_buffer, strlen(msg_buffer), 200);
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

        // Invia messaggio di debug
        Lpuart_Uart_Ip_SyncSend(UART_LPUART_INTERNAL_CHANNEL, (const uint8 *)SEND_MSG, strlen(SEND_MSG), 100);

        // Esegui trasferimento SPI diretto
        int result = direct_spi_transfer(txBuffer, rxBuffer, SPI_BUFFER_SIZE);
        
        if (result == 0)
        {
            Lpuart_Uart_Ip_SyncSend(UART_LPUART_INTERNAL_CHANNEL, (const uint8 *)SEND_SUCCESS_MSG, strlen(SEND_SUCCESS_MSG), 100);
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

        g_transfer_count++;
        
        sprintf(msg_buffer, "Transfer #%lu completato\n", g_transfer_count);
        Lpuart_Uart_Ip_SyncSend(UART_LPUART_INTERNAL_CHANNEL, (uint8_t *)msg_buffer, strlen(msg_buffer), 200);

        // Verifica i dati ricevuti (in loopback dovrebbero essere uguali)
        bool success = true;
        for (int i = 0; i < SPI_BUFFER_SIZE; i++)
        {
            sprintf(msg_buffer, "TX[%d]=0x%02x, RX[%d]=0x%02x\n", i, txBuffer[i], i, rxBuffer[i]);
            Lpuart_Uart_Ip_SyncSend(UART_LPUART_INTERNAL_CHANNEL, (uint8_t *)msg_buffer, strlen(msg_buffer), 200);
            
            if (txBuffer[i] != rxBuffer[i])
            {
                success = false;
            }
        }
        
        if (success)
        {
            Lpuart_Uart_Ip_SyncSend(UART_LPUART_INTERNAL_CHANNEL, (const uint8 *)RECV_SUCCESS_MSG, strlen(RECV_SUCCESS_MSG), 100);
        }
        else
        {
            Lpuart_Uart_Ip_SyncSend(UART_LPUART_INTERNAL_CHANNEL, (const uint8 *)RECV_FAILURE_MSG, strlen(RECV_FAILURE_MSG), 100);
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
    
    char msg_buffer[100];
    sprintf(msg_buffer, "Sistema inizializzato. Configurazione LPSPI diretta...\n");
    Lpuart_Uart_Ip_SyncSend(UART_LPUART_INTERNAL_CHANNEL, (uint8_t *)msg_buffer, strlen(msg_buffer), 200);

    /* Configurazione diretta LPSPI */
    configure_lpspi_direct();

    /* Crea i semafori binari */
    producer_go = xSemaphoreCreateBinary();
    transfer_complete_sem = xSemaphoreCreateBinary();

    Lpuart_Uart_Ip_SyncSend(UART_LPUART_INTERNAL_CHANNEL, (const uint8 *)START_MSG, strlen(START_MSG), 100);

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

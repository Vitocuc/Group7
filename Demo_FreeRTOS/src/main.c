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
#include "string.h" // Per memset e memcmp
#include "stdio.h"  // Per i messaggi di debug

/* Priorità dei Task */
#define main_TASK_PRIORITY      (tskIDLE_PRIORITY + 2)

/* Definizioni per LPSPI */
#define SPI_BUFFER_SIZE         (10)

/* Canale LPUART per i messaggi di debug */
#define UART_LPUART_INTERNAL_CHANNEL 3

/* Definizioni dei dispositivi Master e Slave */
#define SLAVE_EXTERNAL_DEVICE       (Lpspi_Ip_DeviceAttributes_SpiExternalDevice_1_Instance_2) // MODIFICATO: l'istanza 1 è LPSPI1
#define MASTER_EXTERNAL_DEVICE      (Lpspi_Ip_DeviceAttributes_SpiExternalDevice_0_Instance_2)

/* Semafori per la sincronizzazione */
SemaphoreHandle_t producer_go;
SemaphoreHandle_t transfer_complete_sem;
SemaphoreHandle_t slave_async_done_sem; // <-- MODIFICA: Nuovo semaforo per la callback

/* Buffer per il Master */
uint8_t masterTxBuffer[SPI_BUFFER_SIZE];
uint8_t masterRxBuffer[SPI_BUFFER_SIZE];

/* Buffer per lo Slave */
uint8_t slaveTxBuffer[SPI_BUFFER_SIZE];
uint8_t slaveRxBuffer[SPI_BUFFER_SIZE];

/* Contatore per vedere l'attività */
volatile uint32_t g_transfer_count = 0;

// =================================================================================
// --- NUOVA FUNZIONE DI CALLBACK ---
// =================================================================================
/**
 * @brief Callback chiamata dall'interrupt di LPSPI1 quando il suo trasferimento asincrono è terminato.
 */
void SpiSlave_Callback(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    // Rilascia il semaforo per segnalare che lo slave ha finito.
    // Usiamo la versione "FromISR" perché questa funzione è chiamata da un contesto di interrupt.
    xSemaphoreGiveFromISR(slave_async_done_sem, &xHigherPriorityTaskWoken);

    // Se un task con priorità più alta è stato sbloccato, forza un cambio di contesto alla fine dell'ISR.
    portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
}
// =================================================================================


/**
 * @brief Task del Master: attende il via, esegue il trasferimento e attende il completamento dello slave.
 */
void SendTask(void *pvParameters)
{
    (void)pvParameters;
    Lpspi_Ip_StatusType master_spi_status;

    for (;;)
    {
        // 1. Attendi che lo Slave sia pronto e ti dia il via libera
        xSemaphoreTake(producer_go, portMAX_DELAY);

        // 2. Prepara i dati di questo trasferimento
        for (uint16_t i = 0; i < SPI_BUFFER_SIZE; i++)
        {
            masterTxBuffer[i] = (uint8_t)(0xA0 + i + g_transfer_count);
        }
        memset(masterRxBuffer, 0, SPI_BUFFER_SIZE);

        // 3. Esegui il trasferimento bloccante del Master.
        master_spi_status = Lpspi_Ip_SyncTransmit(
            &MASTER_EXTERNAL_DEVICE,
            masterTxBuffer,
            masterRxBuffer,
            SPI_BUFFER_SIZE,
            1000
        );
        (void)master_spi_status;

        // 4. MODIFICA: Attendi che lo slave confermi (tramite callback) di aver finito la sua parte.
        // Questo sostituisce il ciclo di polling, è più efficiente.
        xSemaphoreTake(slave_async_done_sem, portMAX_DELAY);

        // 5. Ora il trasferimento è completo da entrambi i lati. Passa il testimone al ReceiveTask per la verifica.
        xSemaphoreGive(transfer_complete_sem);
    }
}

/**
 * @brief Task dello Slave: arma lo slave con la callback, segnala al master, attende, verifica e ricomincia.
 */
void ReceiveTask(void *pvParameters)
{
    (void)pvParameters;
    char msg_buffer[128];
    boolean master_received_ok = FALSE;
    boolean slave_received_ok = FALSE;

    for (;;)
    {
        // 1. Attendi il tuo turno per iniziare un nuovo ciclo
        xSemaphoreTake(transfer_complete_sem, portMAX_DELAY);

        // Se non è il primo giro, verifica i dati del ciclo PRECEDENTE
        if (g_transfer_count > 0)
        {
            master_received_ok = (0 == memcmp(masterRxBuffer, slaveTxBuffer, SPI_BUFFER_SIZE));
            slave_received_ok = (0 == memcmp(slaveRxBuffer, masterTxBuffer, SPI_BUFFER_SIZE));

            if (master_received_ok && slave_received_ok)
            {
                sprintf(msg_buffer, "Transfer #%lu: SUCCESS! Dati verificati.\n", g_transfer_count);
                Lpuart_Uart_Ip_SyncSend(UART_LPUART_INTERNAL_CHANNEL, (uint8_t *)msg_buffer, strlen(msg_buffer), 200);
            }
            else
            {
                sprintf(msg_buffer, "Transfer #%lu: FAILED! Errore nella verifica.\n", g_transfer_count);
                Lpuart_Uart_Ip_SyncSend(UART_LPUART_INTERNAL_CHANNEL, (uint8_t *)msg_buffer, strlen(msg_buffer), 200);
            }
             // Pausa per leggibilità dell'output
            vTaskDelay(pdMS_TO_TICKS(2000));
        }

        // Incrementa il contatore per il prossimo trasferimento
        g_transfer_count++;

        // 2. Prepara e ARMA lo SLAVE per il PROSSIMO trasferimento, fornendo la callback
        for (uint16_t i = 0; i < SPI_BUFFER_SIZE; i++)
        {
            slaveTxBuffer[i] = (uint8_t)(0xB0 + i + g_transfer_count);
        }
        memset(slaveRxBuffer, 0, SPI_BUFFER_SIZE);

        // MODIFICA: la callback "SpiSlave_Callback" è ora passata come ultimo argomento.
        Lpspi_Ip_AsyncTransmit(
            &SLAVE_EXTERNAL_DEVICE,
            slaveTxBuffer,
            slaveRxBuffer,
            SPI_BUFFER_SIZE,
            SpiSlave_Callback
        );

        // 3. Ora che lo slave è pronto, passa il testimone al Master
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
    IntCtrl_Ip_Init(&IntCtrlConfig_0);

    /* Inizializza LPUART per i messaggi di debug */
    Lpuart_Uart_Ip_Init(UART_LPUART_INTERNAL_CHANNEL, &Lpuart_Uart_Ip_xHwConfigPB_3);

    const char *init_msg = "System initialized. LPSPI Master/Slave test with callback.\n";
    Lpuart_Uart_Ip_SyncSend(UART_LPUART_INTERNAL_CHANNEL, (uint8_t *)init_msg, strlen(init_msg), 200);

    /* Inizializza i due driver LPSPI */
    Lpspi_Ip_Init(&Lpspi_Ip_PhyUnitConfig_SpiPhyUnit_0_Instance_2); // Master (LPSPI2)
    Lpspi_Ip_Init(&Lpspi_Ip_PhyUnitConfig_SpiPhyUnit_1_Instance_1); // Slave  (LPSPI1)

    /* Set transfer mode to interrupt per lo slave per abilitare la callback */
    Lpspi_Ip_UpdateTransferMode(SLAVE_EXTERNAL_DEVICE.Instance, LPSPI_IP_INTERRUPT);

    const char *config_msg = "LPSPI drivers configured. Starting FreeRTOS...\n";
    Lpuart_Uart_Ip_SyncSend(UART_LPUART_INTERNAL_CHANNEL, (uint8_t *)config_msg, strlen(config_msg), 200);

    /* Crea i semafori binari */
    producer_go = xSemaphoreCreateBinary();
    transfer_complete_sem = xSemaphoreCreateBinary();
    slave_async_done_sem = xSemaphoreCreateBinary(); // <-- MODIFICA: Crea il nuovo semaforo

    /* Crea i due task */
    xTaskCreate(SendTask, "SendTask", configMINIMAL_STACK_SIZE + 500, NULL, main_TASK_PRIORITY, NULL);
    xTaskCreate(ReceiveTask, "RecTask", configMINIMAL_STACK_SIZE + 500, NULL, main_TASK_PRIORITY, NULL);

    /* Dai il primo "via" al ReceiveTask per armare lo slave */
    xSemaphoreGive(transfer_complete_sem);

    /* Avvia lo scheduler di FreeRTOS */
    vTaskStartScheduler();

    /* Non dovrebbe mai arrivare qui */
    for (;;);
    return 0;
}

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

/* Priorit� dei Task */
#define main_TASK_PRIORITY (tskIDLE_PRIORITY + 2)

/* Definizioni per LPSPI */
#define LPSPI_INSTANCE (0)   // Usi2mo LPSPI0
#define SPI_BUFFER_SIZE (12) // Dimensione del buffer di trasferimento

#define UART_LPUART_INTERNAL_CHANNEL 3

/* Messaggi di debug (opzionali, richiedono una LPUART configurata) */
#define SEND_MSG "SendTask: Preparo e invio dati via SPI...\n"
#define RECV_SUCCESS_MSG "ReceiveTask: Trasferimento SPI completato con SUCCESSO!\n"
#define RECV_FAILURE_MSG "ReceiveTask: ERRORE nel trasferimento SPI!\n"
#define START_MSG "Inizializzazione completata!\n"
#define CALLBACK_MSG "Sono nella callback! \n"
#define SEND_SUCCESS_MSG "SendTask: Trasferimento SPI completato con SUCCESSO!\n"

/* Semafori per la sincronizzazione */
SemaphoreHandle_t producer_go;
SemaphoreHandle_t transfer_complete_sem;

/* Buffer di trasmissione e ricezione */
uint8_t txBuffer[SPI_BUFFER_SIZE];
uint8_t rxBuffer[SPI_BUFFER_SIZE];

/* Contatore per vedere l'attivit� */
volatile uint32_t g_transfer_count = 0;

/*
 * Dichiarazione delle configurazioni dei driver (devono essere definite altrove,
 * tipicamente generate da uno strumento come EB Tresos o S32 Design Studio)
 */

/**
 * @brief Questa � la funzione di callback che viene chiamata dalla ISR della LPSPI
 * quando il trasferimento asincrono � completato o fallito.
 */
void Lpspi_Callback(uint8 Instance, Lpspi_Ip_EventType Event)
{
    (void)Instance;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    Lpuart_Uart_Ip_SyncSend(UART_LPUART_INTERNAL_CHANNEL, (const uint8 *)CALLBACK_MSG, strlen(CALLBACK_MSG), 100);

    if (Event == LPSPI_IP_EVENT_END_TRANSFER)
    {
        // Il trasferimento � finito. Sblocca il ReceiveTask.
        xSemaphoreGiveFromISR(transfer_complete_sem, &xHigherPriorityTaskWoken);
    }
    else if (Event == LPSPI_IP_EVENT_FAULT)
    {
        // Si � verificato un errore. Sblocca comunque il task per gestire l'errore.
        xSemaphoreGiveFromISR(transfer_complete_sem, &xHigherPriorityTaskWoken);
    }

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
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
            txBuffer[i] = (uint8_t)(i + txBuffer[i]); // Dati variabili per ogni ciclo
        }
        // Pulisci il buffer di ricezione
        memset(rxBuffer, 0, SPI_BUFFER_SIZE);

        // (Opzionale) Invia messaggio di debug
        Lpuart_Uart_Ip_SyncSend(UART_LPUART_INTERNAL_CHANNEL, (const uint8 *)SEND_MSG, strlen(SEND_MSG), 100);

        // Avvia il trasferimento SPI in modo ASINCRONO
        // La funzione ritorna immediatamente. Il trasferimento avviene in background via interrupt.
        //        Lpspi_Ip_AsyncTransmit(
        //            &Lpspi_Ip_DeviceAttributes_SpiExternalDevice_0_Instance_0,
        //            txBuffer,
        //            rxBuffer,
        //            SPI_BUFFER_SIZE,
        //            Lpspi_Callback // La nostra callback per la notifica di fine trasferimento
        //        );
        // In SendTask, sostituisci Lpspi_Ip_AsyncTransmit con questo:
        Lpspi_Ip_StatusType spi_status = Lpspi_Ip_SyncTransmit(
            &Lpspi_Ip_DeviceAttributes_SpiExternalDevice_0_Instance_0,
            txBuffer,
            rxBuffer,
            SPI_BUFFER_SIZE,
            1000 // Timeout in millisecondi
        );
        vTaskDelay(pdMS_TO_TICKS(1000));

        // Dopo la chiamata, dai direttamente il semaforo al task ricevente
        if (spi_status == LPSPI_IP_STATUS_SUCCESS)
        {
            Lpuart_Uart_Ip_SyncSend(UART_LPUART_INTERNAL_CHANNEL, (const uint8 *)SEND_SUCCESS_MSG, strlen(SEND_SUCCESS_MSG), 100);
            xSemaphoreGive(transfer_complete_sem);
        }
    }
}

/**
 * @brief Task Consumatore: attende la fine del trasferimento e verifica i dati.
 */
void ReceiveTask(void *pvParameters)
{
    (void)pvParameters;

    for (;;)
    {
        // Attendi che il trasferimento sia completo (il semaforo viene dato dalla callback)
        xSemaphoreTake(transfer_complete_sem, portMAX_DELAY);

        g_transfer_count++;

        // Verifica i dati ricevuti. Per un test reale, questo richiede un loopback fisico
        // (collegare il pin MISO al pin MOSI) o uno slave SPI che rimandi indietro i dati.
        if (memcmp(txBuffer, rxBuffer, SPI_BUFFER_SIZE) == 0)
        {
            // (Opzionale) Invia messaggio di successo
            Lpuart_Uart_Ip_SyncSend(UART_LPUART_INTERNAL_CHANNEL, (const uint8 *)RECV_SUCCESS_MSG, strlen(RECV_SUCCESS_MSG), 100);
        }
        else
        {
            // (Opzionale) Invia messaggio di errore
            Lpuart_Uart_Ip_SyncSend(UART_LPUART_INTERNAL_CHANNEL, (const uint8 *)RECV_FAILURE_MSG, strlen(RECV_FAILURE_MSG), 100);
        }
        // vTaskDelay(pdMS_TO_TICKS(1000));

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

    /* Sostituisci 'LPSPI0_CLK' con il nome esatto del tuo clock se diverso.
       Questo nome si trova nel file Clock_Ip_Cfg.h generato dal tool. */

    /* Inizializza la periferica LPSPI usando la configurazione fornita */
    Lpspi_Ip_Init(&Lpspi_Ip_PhyUnitConfig_SpiPhyUnit_0_Instance_0);
    uint32_t lpspi_clk_freq = Clock_Ip_GetClockFrequency(LPSPI0_CLK);

    if (lpspi_clk_freq == 0)
    {
        Clock_Ip_EnableModuleClock(LPSPI0_CLK);
        lpspi_clk_freq = Clock_Ip_GetClockFrequency(LPSPI0_CLK);
    }

    uint32_t lpspi_clk_freq = 80000000; // Hardcode 80MHz for QEMU
    if (lpspi_clk_freq > 0)
    {
        sprintf(msg_buffer, "PROVA CLOCK: OK! Frequenza LPSPI0: %lu Hz\n", lpspi_clk_freq);
    }
    else
    {
        sprintf(msg_buffer, "PROVA CLOCK: FALLITA! Clock LPSPI0 e' SPENTO o il nome e' errato!\n");
    }
    Lpuart_Uart_Ip_SyncSend(UART_LPUART_INTERNAL_CHANNEL, (uint8_t *)msg_buffer, strlen(msg_buffer), 200);
    // vTaskDelay(pdMS_TO_TICKS(1000));

    /*
     * Installa e abilita l'handler per l'interrupt della LPSPI.
     * Questo � il passo FONDAMENTALE per la modalit� a interrupt.
     * Il nome dell'IRQ (es. LPSPI0_IRQn) dipende dal microcontrollore.
     */
    IntCtrl_Ip_InstallHandler(LPSPI0_IRQn, Lpspi_Ip_LPSPI_0_IRQHandler, NULL_PTR);
    IntCtrl_Ip_EnableIrq(LPSPI0_IRQn); // if enabled problem with clock

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

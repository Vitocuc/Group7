/* Include dei file di sistema e dei driver */
#include "Clock_Ip.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"
#include "Lpspi_Ip.h"
#include "Lpspi_Ip_Sa_PBcfg.h"
#include "Siul2_Port_Ip.h"
#include "IntCtrl_Ip.h"
#include "Lpuart_Uart_Ip.h"
#include "string.h" // Per memset e memcmp
#include "stdio.h"	// Per i messaggi di debug

/* Priorità dei Task */
#define MASTER_TASK_PRIORITY (tskIDLE_PRIORITY + 2)
#define SLAVE_TASK_PRIORITY (tskIDLE_PRIORITY + 2)

/* Definizioni per LPSPI */
#define SPI_BUFFER_SIZE (10)
#define SPI_TIMEOUT_MS (1000)

/* Stack size per i task */
#define MASTER_TASK_STACK_SIZE (configMINIMAL_STACK_SIZE )
#define SLAVE_TASK_STACK_SIZE (configMINIMAL_STACK_SIZE)

/* Canale LPUART per i messaggi di debug */
#define UART_LPUART_INTERNAL_CHANNEL 3

/* Task notification values */
#define NOTIFY_SLAVE_READY (1UL << 0)
#define NOTIFY_TRANSFER_DONE (1UL << 1)
#define NOTIFY_DATA_VERIFIED (1UL << 2)

/* Definizioni dei dispositivi Master e Slave */
#define SLAVE_EXTERNAL_DEVICE (Lpspi_Ip_DeviceAttributes_SpiExternalDevice_1_Instance_2)
#define MASTER_EXTERNAL_DEVICE (Lpspi_Ip_DeviceAttributes_SpiExternalDevice_0_Instance_2)

/* Task handles per le notifiche */
static TaskHandle_t xMasterTaskHandle = NULL;
static TaskHandle_t xSlaveTaskHandle = NULL;

/* Semafori per la sincronizzazione critica */
static SemaphoreHandle_t xSpiMutex = NULL;
static SemaphoreHandle_t xSlaveAsyncDoneSem = NULL;

/* Buffer per il Master */
static uint8_t masterTxBuffer[SPI_BUFFER_SIZE];
static uint8_t masterRxBuffer[SPI_BUFFER_SIZE];

/* Buffer per lo Slave */
static uint8_t slaveTxBuffer[SPI_BUFFER_SIZE];
static uint8_t slaveRxBuffer[SPI_BUFFER_SIZE];

/* Contatore per vedere l'attività */
static volatile uint32_t g_transfer_count = 0;

/* Flag di stato sistema */
static volatile BaseType_t xSystemInitialized = pdFALSE;

/* Struttura per i risultati di trasferimento */
typedef struct
{
	boolean master_received_ok;
	boolean slave_received_ok;
	uint32_t transfer_number;
} TransferResult_t;

// =================================================================================
// --- UTILITY FUNCTIONS ---
// =================================================================================

/**
 * @brief Invia un messaggio di debug via UART in modo sicuro
 */
static void SendDebugMessage(const char *message)
{
	if (message != NULL && xSystemInitialized == pdTRUE)
	{
		size_t len = strlen(message);
		if (len > 0)
		{
			Lpuart_Uart_Ip_SyncSend(UART_LPUART_INTERNAL_CHANNEL,
									(uint8_t *)message, len, 200);
		}
	}
}

/**
 * @brief Callback chiamata dall'interrupt di LPSPI quando il trasferimento asincrono dello slave è terminato
 */
void SpiSlave_Callback(void)
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	// Rilascia il semaforo per segnalare che lo slave ha finito
	if (xSlaveAsyncDoneSem != NULL)
	{
		xSemaphoreGiveFromISR(xSlaveAsyncDoneSem, &xHigherPriorityTaskWoken);
	}

	// Notifica anche il task slave che il trasferimento è completato
	if (xSlaveTaskHandle != NULL)
	{
		xTaskNotifyFromISR(xSlaveTaskHandle, NOTIFY_TRANSFER_DONE,
						   eSetBits, &xHigherPriorityTaskWoken);
	}

	// Se un task con priorità più alta è stato sbloccato, forza un cambio di contesto
	portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

// =================================================================================
// --- TASK FUNCTIONS ---
// =================================================================================

/**
 * @brief Task del Master: gestisce i trasferimenti SPI dal lato master
 */
static void MasterTask(void *pvParameters)
{
	(void)pvParameters;
	Lpspi_Ip_StatusType master_spi_status;
	uint32_t ulNotificationValue;
	char msg_buffer[128];

	SendDebugMessage("Master Task: Started\n");

	for (;;)
	{
		// 1. Attendi che lo Slave sia pronto
		if (xTaskNotifyWait(0, NOTIFY_SLAVE_READY, &ulNotificationValue,
							pdMS_TO_TICKS(5000)) != pdTRUE)
		{
			SendDebugMessage("Master Task: Timeout waiting for slave ready\n");
			continue;
		}

		if ((ulNotificationValue & NOTIFY_SLAVE_READY) == 0)
		{
			continue;
		}

		// 2. Acquisisci il mutex SPI per sicurezza
		if (xSemaphoreTake(xSpiMutex, pdMS_TO_TICKS(1000)) != pdTRUE)
		{
			SendDebugMessage("Master Task: Failed to acquire SPI mutex\n");
			continue;
		}

		// 3. Prepara i dati per questo trasferimento
		for (uint16_t i = 0; i < SPI_BUFFER_SIZE; i++)
		{
			masterTxBuffer[i] = (uint8_t)(0xA0 + i + g_transfer_count);
		}
		memset(masterRxBuffer, 0, SPI_BUFFER_SIZE);

		// 4. Esegui il trasferimento bloccante del Master
		master_spi_status = Lpspi_Ip_SyncTransmit(
			&MASTER_EXTERNAL_DEVICE,
			masterTxBuffer,
			masterRxBuffer,
			SPI_BUFFER_SIZE,
			SPI_TIMEOUT_MS);

		// 5. Rilascia il mutex SPI
		xSemaphoreGive(xSpiMutex);

		// 6. Controlla lo stato del trasferimento
		if (master_spi_status != LPSPI_IP_STATUS_SUCCESS)
		{
			sprintf(msg_buffer, "Master Task: SPI transfer failed with status %d\n",
					(int)master_spi_status);
			SendDebugMessage(msg_buffer);
			continue;
		}

		// 7. Attendi che lo slave confermi di aver finito
		if (xSemaphoreTake(xSlaveAsyncDoneSem, pdMS_TO_TICKS(2000)) != pdTRUE)
		{
			SendDebugMessage("Master Task: Timeout waiting for slave completion\n");
			continue;
		}

		// 8. Notifica al slave task che può verificare i dati
		xTaskNotify(xSlaveTaskHandle, NOTIFY_DATA_VERIFIED, eSetBits);

		sprintf(msg_buffer, "Master Task: Transfer #%lu completed\n", g_transfer_count);
		SendDebugMessage(msg_buffer);
	}
}

/**
 * @brief Task dello Slave: gestisce la ricezione e verifica dei dati
 */
static void SlaveTask(void *pvParameters)
{
	(void)pvParameters;
	char msg_buffer[128];
	uint32_t ulNotificationValue;
	TransferResult_t result;

	SendDebugMessage("Slave Task: Started\n");

	for (;;)
	{
		// 1. Incrementa il contatore per il prossimo trasferimento
		g_transfer_count++;

		// 2. Prepara e arma lo SLAVE per il prossimo trasferimento
		for (uint16_t i = 0; i < SPI_BUFFER_SIZE; i++)
		{
			slaveTxBuffer[i] = (uint8_t)(0xB0 + i + g_transfer_count);
		}
		memset(slaveRxBuffer, 0, SPI_BUFFER_SIZE);

		// 3. Acquisisci il mutex SPI
		if (xSemaphoreTake(xSpiMutex, pdMS_TO_TICKS(1000)) != pdTRUE)
		{
			SendDebugMessage("Slave Task: Failed to acquire SPI mutex\n");
			vTaskDelay(pdMS_TO_TICKS(100));
			continue;
		}

		// 4. Avvia il trasferimento asincrono dello slave con callback
		Lpspi_Ip_AsyncTransmit(
			&SLAVE_EXTERNAL_DEVICE,
			slaveTxBuffer,
			slaveRxBuffer,
			SPI_BUFFER_SIZE,
			SpiSlave_Callback);

		// 5. Rilascia il mutex SPI
		xSemaphoreGive(xSpiMutex);

		// 6. Notifica al Master che lo slave è pronto
		xTaskNotify(xMasterTaskHandle, NOTIFY_SLAVE_READY, eSetBits);

		// 7. Attendi che il trasferimento sia completato e che possiamo verificare i dati
		if (xTaskNotifyWait(0, NOTIFY_TRANSFER_DONE | NOTIFY_DATA_VERIFIED,
							&ulNotificationValue, pdMS_TO_TICKS(5000)) != pdTRUE)
		{
			SendDebugMessage("Slave Task: Timeout waiting for transfer completion\n");
			continue;
		}

		// 8. Verifica i dati solo se non è il primo trasferimento
		if (g_transfer_count > 1)
		{
			result.master_received_ok = (0 == memcmp(masterRxBuffer, slaveTxBuffer, SPI_BUFFER_SIZE));
			result.slave_received_ok = (0 == memcmp(slaveRxBuffer, masterTxBuffer, SPI_BUFFER_SIZE));
			result.transfer_number = g_transfer_count - 1;

			if (result.master_received_ok && result.slave_received_ok)
			{
				sprintf(msg_buffer, "Transfer #%lu: SUCCESS! Data verified.\n",
						result.transfer_number);
				SendDebugMessage(msg_buffer);
			}
			else
			{
				sprintf(msg_buffer, "Transfer #%lu: FAILED! Master OK: %s, Slave OK: %s\n",
						result.transfer_number,
						result.master_received_ok ? "YES" : "NO",
						result.slave_received_ok ? "YES" : "NO");
				SendDebugMessage(msg_buffer);
			}
		}

		// 9. Pausa per leggibilità dell'output
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}

// =================================================================================
// --- INITIALIZATION FUNCTIONS ---
// =================================================================================

/**
 * @brief Inizializza i semafori e le strutture dati FreeRTOS
 */
static BaseType_t InitializeFreeRTOSObjects(void)
{
	// Crea il mutex per l'accesso esclusivo al bus SPI
	xSpiMutex = xSemaphoreCreateMutex();
	if (xSpiMutex == NULL)
	{
		SendDebugMessage("ERROR: Failed to create SPI mutex\n");
		return pdFAIL;
	}

	// Crea il semaforo binario per la callback dello slave
	xSlaveAsyncDoneSem = xSemaphoreCreateBinary();
	if (xSlaveAsyncDoneSem == NULL)
	{
		SendDebugMessage("ERROR: Failed to create slave async done semaphore\n");
		return pdFAIL;
	}

	return pdPASS;
}

/**
 * @brief Inizializza l'hardware del sistema
 */
static BaseType_t InitializeHardware(void)
{
	// Inizializzazione di base del sistema
	Clock_Ip_Init(&Clock_Ip_aClockConfig[0]);
	Siul2_Port_Ip_Init(NUM_OF_CONFIGURED_PINS0, g_pin_mux_InitConfigArr0);
	IntCtrl_Ip_Init(&IntCtrlConfig_0);

	// Inizializza LPUART per i messaggi di debug
	Lpuart_Uart_Ip_Init(UART_LPUART_INTERNAL_CHANNEL, &Lpuart_Uart_Ip_xHwConfigPB_3);

	xSystemInitialized = pdTRUE;
	SendDebugMessage("Hardware initialization: UART initialized\n");

	// Inizializza i due driver LPSPI
	Lpspi_Ip_Init(&Lpspi_Ip_PhyUnitConfig_SpiPhyUnit_0_Instance_2); // Master (LPSPI2)
	Lpspi_Ip_Init(&Lpspi_Ip_PhyUnitConfig_SpiPhyUnit_1_Instance_1); // Slave  (LPSPI1)

	// Set transfer mode to interrupt per lo slave per abilitare la callback
	Lpspi_Ip_UpdateTransferMode(SLAVE_EXTERNAL_DEVICE.Instance, LPSPI_IP_INTERRUPT);

	SendDebugMessage("Hardware initialization: LPSPI drivers configured\n");

	return pdPASS;
}

/**
 * @brief Crea i task dell'applicazione
 */
static BaseType_t CreateApplicationTasks(void)
{
	BaseType_t xResult;

	// Crea il task Master
	xResult = xTaskCreate(MasterTask, "MasterTask", MASTER_TASK_STACK_SIZE,
						  NULL, MASTER_TASK_PRIORITY, &xMasterTaskHandle);
	if (xResult != pdPASS || xMasterTaskHandle == NULL)
	{
		SendDebugMessage("ERROR: Failed to create Master Task\n");
		return pdFAIL;
	}

	// Crea il task Slave
	xResult = xTaskCreate(SlaveTask, "SlaveTask", SLAVE_TASK_STACK_SIZE,
						  NULL, SLAVE_TASK_PRIORITY, &xSlaveTaskHandle);
	if (xResult != pdPASS || xSlaveTaskHandle == NULL)
	{
		SendDebugMessage("ERROR: Failed to create Slave Task\n");
		return pdFAIL;
	}

	SendDebugMessage("Application tasks created successfully\n");
	return pdPASS;
}

// =================================================================================
// --- MAIN FUNCTION ---
// =================================================================================

/**
 * @brief Funzione Main
 */
int main(void)
{
	BaseType_t xResult;

	// 1. Inizializza l'hardware
	xResult = InitializeHardware();
	if (xResult != pdPASS)
	{
		// Se non riusciamo ad inizializzare l'hardware, non possiamo continuare
		for (;;)
		{
			// Loop infinito in caso di errore
		}
	}

	SendDebugMessage("=== LPSPI Master/Slave Test with FreeRTOS ===\n");
	SendDebugMessage("System initialized. Starting FreeRTOS setup...\n");

	// 2. Inizializza gli oggetti FreeRTOS
	xResult = InitializeFreeRTOSObjects();
	if (xResult != pdPASS)
	{
		SendDebugMessage("ERROR: Failed to initialize FreeRTOS objects\n");
		for (;;)
		{
			// Loop infinito in caso di errore
		}
	}

	// 3. Crea i task dell'applicazione
	xResult = CreateApplicationTasks();
	if (xResult != pdPASS)
	{
		SendDebugMessage("ERROR: Failed to create application tasks\n");
		for (;;)
		{
			// Loop infinito in caso di errore
		}
	}

	SendDebugMessage("FreeRTOS configuration complete. Starting scheduler...\n");

	// 4. Avvia lo scheduler di FreeRTOS
	vTaskStartScheduler();

	// Non dovrebbe mai arrivare qui se lo scheduler è stato avviato correttamente
	SendDebugMessage("ERROR: FreeRTOS scheduler failed to start\n");
	for (;;)
	{
		// Loop infinito se lo scheduler fallisce
	}

	return 0;
}

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
#include "stdio.h"	// Per i messaggi di debug

/* Priorità dei Task */
#define main_TASK_PRIORITY (tskIDLE_PRIORITY + 2)

/* Definizioni per LPSPI */
#define SPI_BUFFER_SIZE (10)

/* Canale LPUART per i messaggi di debug */
#define UART_LPUART_INTERNAL_CHANNEL 3

/* Definizioni dei dispositivi Master e Slave */
#define SLAVE_EXTERNAL_DEVICE (Lpspi_Ip_DeviceAttributes_SpiExternalDevice_1_Instance_2)
#define MASTER_EXTERNAL_DEVICE (Lpspi_Ip_DeviceAttributes_SpiExternalDevice_0_Instance_2)

/* Semafori per la sincronizzazione - usando FreeRTOS best practices */
static SemaphoreHandle_t producer_go = NULL;
static SemaphoreHandle_t transfer_complete_sem = NULL;
static SemaphoreHandle_t slave_async_done_sem = NULL;

/* Buffer per il Master */
static uint8_t masterTxBuffer[SPI_BUFFER_SIZE];
static uint8_t masterRxBuffer[SPI_BUFFER_SIZE];

/* Buffer per lo Slave */
static uint8_t slaveTxBuffer[SPI_BUFFER_SIZE];
static uint8_t slaveRxBuffer[SPI_BUFFER_SIZE];

/* Contatore per vedere l'attività */
static volatile uint32_t g_transfer_count = 0;

// =================================================================================
// --- CALLBACK FUNCTION ---
// =================================================================================
/**
 * @brief Callback chiamata dall'interrupt di LPSPI1 quando il trasferimento asincrono dello slave è terminato.
 */
void SpiSlave_Callback(void)
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	// Rilascia il semaforo per segnalare che lo slave ha finito
	// Usa la versione FromISR perché questa funzione è chiamata da un contesto di interrupt
	if (slave_async_done_sem != NULL)
	{
		xSemaphoreGiveFromISR(slave_async_done_sem, &xHigherPriorityTaskWoken);
	}

	// Se un task con priorità più alta è stato sbloccato, forza un cambio di contesto
	portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

// =================================================================================
// --- TASK FUNCTIONS ---
// =================================================================================

/**
 * @brief Task del Master: attende il via, esegue il trasferimento e attende il completamento dello slave.
 */
static void SendTask(void *pvParameters)
{
	(void)pvParameters;
	Lpspi_Ip_StatusType master_spi_status;

	for (;;)
	{
		// 1. Attendi che lo Slave sia pronto e ti dia il via libera
		if (xSemaphoreTake(producer_go, pdMS_TO_TICKS(10000)) != pdTRUE)
		{
			// Timeout - continua e riprova
			continue;
		}

		// 2. Piccola pausa per assicurarsi che lo slave sia completamente pronto
		vTaskDelay(pdMS_TO_TICKS(20));

		// 3. Prepara i dati di questo trasferimento
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
			1000);

		// 5. Controlla se il trasferimento è riuscito
		if (master_spi_status != LPSPI_IP_STATUS_SUCCESS)
		{
			// Se fallisce, segnala comunque al ReceiveTask per continuare
			xSemaphoreGive(transfer_complete_sem);
			continue;
		}

		// 6. Attendi che lo slave confermi (tramite callback) di aver finito la sua parte
		if (xSemaphoreTake(slave_async_done_sem, pdMS_TO_TICKS(3000)) != pdTRUE)
		{
			// Timeout - segnala comunque al ReceiveTask per continuare
			xSemaphoreGive(transfer_complete_sem);
			continue;
		}

		// 7. Ora il trasferimento è completo da entrambi i lati
		// Passa il testimone al ReceiveTask per la verifica
		xSemaphoreGive(transfer_complete_sem);
	}
}

/**
 * @brief Task dello Slave: arma lo slave con la callback, segnala al master, attende, verifica e ricomincia.
 */
static void ReceiveTask(void *pvParameters)
{
	(void)pvParameters;
	char msg_buffer[128];
	boolean master_received_ok = FALSE;
	boolean slave_received_ok = FALSE;

	for (;;)
	{
		// 1. Attendi il tuo turno per iniziare un nuovo ciclo
		if (xSemaphoreTake(transfer_complete_sem, pdMS_TO_TICKS(15000)) != pdTRUE)
		{
			// Timeout - ricomincia il ciclo
			continue;
		}

		// 2. Se non è il primo giro, verifica i dati del ciclo PRECEDENTE
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

				// Debug extra: mostra alcuni byte per il debugging
				sprintf(msg_buffer, "Debug: Master expected[0-2]: %02X %02X %02X, got: %02X %02X %02X\n",
						slaveTxBuffer[0], slaveTxBuffer[1], slaveTxBuffer[2],
						masterRxBuffer[0], masterRxBuffer[1], masterRxBuffer[2]);
				Lpuart_Uart_Ip_SyncSend(UART_LPUART_INTERNAL_CHANNEL, (uint8_t *)msg_buffer, strlen(msg_buffer), 200);
			}

			// Pausa per leggibilità dell'output
			vTaskDelay(pdMS_TO_TICKS(1500));
		}

		// 3. Incrementa il contatore per il prossimo trasferimento
		g_transfer_count++;

		// 4. Prepara e ARMA lo SLAVE per il PROSSIMO trasferimento
		for (uint16_t i = 0; i < SPI_BUFFER_SIZE; i++)
		{
			slaveTxBuffer[i] = (uint8_t)(0xB0 + i + g_transfer_count);
		}
		memset(slaveRxBuffer, 0, SPI_BUFFER_SIZE);

		// 5. Avvia il trasferimento asincrono dello slave con callback
		// IMPORTANTE: Questo deve essere fatto PRIMA di segnalare al master
		Lpspi_Ip_AsyncTransmit(
			&SLAVE_EXTERNAL_DEVICE,
			slaveTxBuffer,
			slaveRxBuffer,
			SPI_BUFFER_SIZE,
			SpiSlave_Callback);

		// 6. Piccola pausa per assicurarsi che lo slave sia completamente armato
		vTaskDelay(pdMS_TO_TICKS(50));

		// 7. Ora che lo slave è pronto, passa il testimone al Master
		xSemaphoreGive(producer_go);
	}
}

// =================================================================================
// --- MAIN FUNCTION ---
// =================================================================================

/**
 * @brief Funzione Main con migliore gestione degli errori
 */
int main(void)
{
	BaseType_t xResult;

	/* Inizializzazione di base del sistema */
	Clock_Ip_Init(&Clock_Ip_aClockConfig[0]);
	Siul2_Port_Ip_Init(NUM_OF_CONFIGURED_PINS0, g_pin_mux_InitConfigArr0);
	IntCtrl_Ip_Init(&IntCtrlConfig_0);

	/* Inizializza LPUART per i messaggi di debug */
	Lpuart_Uart_Ip_Init(UART_LPUART_INTERNAL_CHANNEL, &Lpuart_Uart_Ip_xHwConfigPB_3);

	const char *init_msg = "=== System initialized. LPSPI Master/Slave test with improved FreeRTOS ===\n";
	Lpuart_Uart_Ip_SyncSend(UART_LPUART_INTERNAL_CHANNEL, (uint8_t *)init_msg, strlen(init_msg), 200);

	/* Inizializza i due driver LPSPI */
	Lpspi_Ip_Init(&Lpspi_Ip_PhyUnitConfig_SpiPhyUnit_0_Instance_2); // Master (LPSPI2)
	Lpspi_Ip_Init(&Lpspi_Ip_PhyUnitConfig_SpiPhyUnit_1_Instance_1); // Slave  (LPSPI1)

	/* Set transfer mode to interrupt per lo slave per abilitare la callback */
	Lpspi_Ip_UpdateTransferMode(SLAVE_EXTERNAL_DEVICE.Instance, LPSPI_IP_INTERRUPT);

	const char *config_msg = "LPSPI drivers configured. Creating FreeRTOS objects...\n";
	Lpuart_Uart_Ip_SyncSend(UART_LPUART_INTERNAL_CHANNEL, (uint8_t *)config_msg, strlen(config_msg), 200);

	/* Crea i semafori binari con controllo errori */
	producer_go = xSemaphoreCreateBinary();
	if (producer_go == NULL)
	{
		const char *error_msg = "ERROR: Failed to create producer_go semaphore\n";
		Lpuart_Uart_Ip_SyncSend(UART_LPUART_INTERNAL_CHANNEL, (uint8_t *)error_msg, strlen(error_msg), 200);
		for (;;)
			; // Blocca qui in caso di errore
	}

	transfer_complete_sem = xSemaphoreCreateBinary();
	if (transfer_complete_sem == NULL)
	{
		const char *error_msg = "ERROR: Failed to create transfer_complete_sem semaphore\n";
		Lpuart_Uart_Ip_SyncSend(UART_LPUART_INTERNAL_CHANNEL, (uint8_t *)error_msg, strlen(error_msg), 200);
		for (;;)
			; // Blocca qui in caso di errore
	}

	slave_async_done_sem = xSemaphoreCreateBinary();
	if (slave_async_done_sem == NULL)
	{
		const char *error_msg = "ERROR: Failed to create slave_async_done_sem semaphore\n";
		Lpuart_Uart_Ip_SyncSend(UART_LPUART_INTERNAL_CHANNEL, (uint8_t *)error_msg, strlen(error_msg), 200);
		for (;;)
			; // Blocca qui in caso di errore
	}

	/* Crea i due task con controllo errori */
	xResult = xTaskCreate(SendTask, "SendTask", configMINIMAL_STACK_SIZE + 600, NULL, main_TASK_PRIORITY, NULL);
	if (xResult != pdPASS)
	{
		const char *error_msg = "ERROR: Failed to create SendTask\n";
		Lpuart_Uart_Ip_SyncSend(UART_LPUART_INTERNAL_CHANNEL, (uint8_t *)error_msg, strlen(error_msg), 200);
		for (;;)
			; // Blocca qui in caso di errore
	}

	xResult = xTaskCreate(ReceiveTask, "RecTask", configMINIMAL_STACK_SIZE + 600, NULL, main_TASK_PRIORITY, NULL);
	if (xResult != pdPASS)
	{
		const char *error_msg = "ERROR: Failed to create ReceiveTask\n";
		Lpuart_Uart_Ip_SyncSend(UART_LPUART_INTERNAL_CHANNEL, (uint8_t *)error_msg, strlen(error_msg), 200);
		for (;;)
			; // Blocca qui in caso di errore
	}

	const char *success_msg = "FreeRTOS objects and tasks created successfully!\n";
	Lpuart_Uart_Ip_SyncSend(UART_LPUART_INTERNAL_CHANNEL, (uint8_t *)success_msg, strlen(success_msg), 200);

	/* Dai il primo "via" al ReceiveTask per armare lo slave */
	xSemaphoreGive(transfer_complete_sem);

	const char *start_msg = "Starting FreeRTOS scheduler...\n";
	Lpuart_Uart_Ip_SyncSend(UART_LPUART_INTERNAL_CHANNEL, (uint8_t *)start_msg, strlen(start_msg), 200);

	/* Avvia lo scheduler di FreeRTOS */
	vTaskStartScheduler();

	/* Non dovrebbe mai arrivare qui se lo scheduler è stato avviato correttamente */
	const char *fatal_msg = "FATAL ERROR: FreeRTOS scheduler failed to start!\n";
	Lpuart_Uart_Ip_SyncSend(UART_LPUART_INTERNAL_CHANNEL, (uint8_t *)fatal_msg, strlen(fatal_msg), 200);

	for (;;)
	{
		// Loop infinito in caso di errore fatale
	}

	return 0;
}

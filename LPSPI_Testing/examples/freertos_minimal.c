/*
 * Esempio FreeRTOS Minimale - LPSPI NXP S32K358
 *
 * Questo esempio mostra la configurazione minimale per il testing LPSPI
 * in modalità loopback con frame da 8-bit.
 */

#include "FreeRTOS.h"
#include "task.h"
#include "string.h"

/* Base address per LPSPI0 */
#define LPSPI0_BASE_ADDR 0x40358000

/* Register offsets */
#define LPSPI_CR_OFFSET 0x10
#define LPSPI_SR_OFFSET 0x14
#define LPSPI_CFGR1_OFFSET 0x24
#define LPSPI_CCR_OFFSET 0x40
#define LPSPI_TCR_OFFSET 0x60
#define LPSPI_TDR_OFFSET 0x64
#define LPSPI_RDR_OFFSET 0x74

/* Register bit definitions */
#define LPSPI_CR_MEN (1U << 0)
#define LPSPI_CR_RST (1U << 1)
#define LPSPI_SR_TDF (1U << 0)
#define LPSPI_SR_RDF (1U << 1)
#define LPSPI_SR_TCF (1U << 10)
#define LPSPI_CFGR1_MASTER (1U << 0)
#define LPSPI_CFGR1_PINCFG_LOOPBACK (1U << 24)
#define LPSPI_TCR_FRAMESZ(n) ((n - 1) << 0)

/* Register access macro */
#define REG32(addr) (*(volatile uint32_t *)(addr))
#define LPSPI0_REG(offset) REG32(LPSPI0_BASE_ADDR + offset)

/**
 * @brief Configurazione minimale LPSPI per test
 */
void lpspi_minimal_init(void)
{
	// 1. Reset LPSPI
	LPSPI0_REG(LPSPI_CR_OFFSET) = LPSPI_CR_RST;

	// 2. Delay minimo
	for (volatile int i = 0; i < 1000; i++)
		;

	// 3. Clear reset
	LPSPI0_REG(LPSPI_CR_OFFSET) = 0;

	// 4. Configure Master + Loopback
	LPSPI0_REG(LPSPI_CFGR1_OFFSET) = LPSPI_CFGR1_MASTER | LPSPI_CFGR1_PINCFG_LOOPBACK;

	// 5. Set clock divider
	LPSPI0_REG(LPSPI_CCR_OFFSET) = 0x04040404;

	// 6. Enable module
	LPSPI0_REG(LPSPI_CR_OFFSET) = LPSPI_CR_MEN;
}

/**
 * @brief Trasferimento SPI singolo
 * @param data Byte da trasferire
 * @return Byte ricevuto
 */
uint8_t lpspi_transfer_byte(uint8_t data)
{
	// 1. Configure 8-bit frame
	LPSPI0_REG(LPSPI_TCR_OFFSET) = LPSPI_TCR_FRAMESZ(8);

	// 2. Wait for TDF (TX ready)
	while (!(LPSPI0_REG(LPSPI_SR_OFFSET) & LPSPI_SR_TDF))
		;

	// 3. Send data
	LPSPI0_REG(LPSPI_TDR_OFFSET) = data;

	// 4. Wait for TCF (Transfer complete)
	while (!(LPSPI0_REG(LPSPI_SR_OFFSET) & LPSPI_SR_TCF))
		;

	// 5. Clear TCF
	LPSPI0_REG(LPSPI_SR_OFFSET) = LPSPI_SR_TCF;

	// 6. Wait for RDF (RX data available)
	while (!(LPSPI0_REG(LPSPI_SR_OFFSET) & LPSPI_SR_RDF))
		;

	// 7. Read received data
	uint8_t received = LPSPI0_REG(LPSPI_RDR_OFFSET) & 0xFF;

	// 8. Clear RDF
	LPSPI0_REG(LPSPI_SR_OFFSET) = LPSPI_SR_RDF;

	return received;
}

/**
 * @brief Test task per LPSPI
 */
void test_task(void *pvParameters)
{
	(void)pvParameters;

	// Inizializza LPSPI
	lpspi_minimal_init();

	// Test buffer
	uint8_t test_data[] = {0x10, 0x11, 0x12, 0x13};
	uint8_t received[4];

	for (;;)
	{
		// Esegui test di loopback
		for (int i = 0; i < 4; i++)
		{
			received[i] = lpspi_transfer_byte(test_data[i]);
		}

		// Verifica risultati (in loopback: TX == RX)
		bool success = true;
		for (int i = 0; i < 4; i++)
		{
			if (test_data[i] != received[i])
			{
				success = false;
				break;
			}
		}

		// Il risultato può essere verificato con debugger
		// o con output UART se configurato

		vTaskDelay(pdMS_TO_TICKS(1000)); // Wait 1 second
	}
}

/**
 * @brief Main function minimale
 */
int main(void)
{
	// Crea task di test
	xTaskCreate(test_task, "LPSPITest", 512, NULL, 1, NULL);

	// Avvia scheduler
	vTaskStartScheduler();

	// Non dovrebbe mai arrivare qui
	for (;;)
		;
	return 0;
}

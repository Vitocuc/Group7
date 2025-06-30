/*
 * Esempio Accesso Diretto Registri - LPSPI NXP S32K358
 *
 * Questo esempio mostra come accedere direttamente ai registri LPSPI
 * senza l'overhead di FreeRTOS, utile per test e debugging.
 */

#include <stdint.h>
#include <stdbool.h>

/* Base address per LPSPI0 */
#define LPSPI0_BASE 0x40358000UL

/* Register structure */
typedef struct
{
	volatile uint32_t VERID; // 0x00 - Version ID
	volatile uint32_t PARAM; // 0x04 - Parameter
	uint32_t RESERVED0[2];	 // 0x08-0x0C
	volatile uint32_t CR;	 // 0x10 - Control
	volatile uint32_t SR;	 // 0x14 - Status
	volatile uint32_t IER;	 // 0x18 - Interrupt Enable
	volatile uint32_t DER;	 // 0x1C - DMA Enable
	volatile uint32_t CFGR0; // 0x20 - Configuration 0
	volatile uint32_t CFGR1; // 0x24 - Configuration 1
	uint32_t RESERVED1[6];	 // 0x28-0x3C
	volatile uint32_t CCR;	 // 0x40 - Clock Configuration
	uint32_t RESERVED2[5];	 // 0x44-0x54
	volatile uint32_t FCR;	 // 0x58 - FIFO Control
	volatile uint32_t FSR;	 // 0x5C - FIFO Status
	volatile uint32_t TCR;	 // 0x60 - Transmit Command
	volatile uint32_t TDR;	 // 0x64 - Transmit Data
	uint32_t RESERVED3[2];	 // 0x68-0x6C
	volatile uint32_t RSR;	 // 0x70 - Receive Status
	volatile uint32_t RDR;	 // 0x74 - Receive Data
} LPSPI_Type;

/* LPSPI instance */
#define LPSPI0 ((LPSPI_Type *)LPSPI0_BASE)

/* Register bit masks */
#define LPSPI_CR_MEN_MASK (1UL << 0)
#define LPSPI_CR_RST_MASK (1UL << 1)
#define LPSPI_SR_TDF_MASK (1UL << 0)
#define LPSPI_SR_RDF_MASK (1UL << 1)
#define LPSPI_SR_TCF_MASK (1UL << 10)
#define LPSPI_CFGR1_MASTER_MASK (1UL << 0)
#define LPSPI_CFGR1_PINCFG_MASK (3UL << 24)
#define LPSPI_TCR_FRAMESZ_MASK (0xFFFUL << 0)

/* PINCFG values */
#define LPSPI_PINCFG_NORMAL (0UL << 24)
#define LPSPI_PINCFG_LOOPBACK (1UL << 24)

/**
 * @brief Delay function (simple loop)
 * @param cycles Number of cycles to wait
 */
static void delay_cycles(uint32_t cycles)
{
	for (volatile uint32_t i = 0; i < cycles; i++)
	{
		// Empty loop
	}
}

/**
 * @brief Initialize LPSPI in loopback mode
 */
void lpspi_init_loopback(void)
{
	// 1. Reset LPSPI module
	LPSPI0->CR = LPSPI_CR_RST_MASK;
	delay_cycles(1000);

	// 2. Clear reset
	LPSPI0->CR = 0;

	// 3. Configure as Master with Loopback
	LPSPI0->CFGR1 = LPSPI_CFGR1_MASTER_MASK | LPSPI_PINCFG_LOOPBACK;

	// 4. Set clock configuration
	LPSPI0->CCR = 0x04040404; // Prescaler settings

	// 5. Configure FIFO watermarks (optional)
	LPSPI0->FCR = 0x00000000; // TX=0, RX=0

	// 6. Enable module
	LPSPI0->CR = LPSPI_CR_MEN_MASK;
}

/**
 * @brief Set frame size for transfers
 * @param bits Number of bits per frame (1-4096)
 */
void lpspi_set_frame_size(uint16_t bits)
{
	if (bits > 0 && bits <= 4096)
	{
		uint32_t tcr = LPSPI0->TCR;
		tcr &= ~LPSPI_TCR_FRAMESZ_MASK;
		tcr |= ((bits - 1) & 0xFFF);
		LPSPI0->TCR = tcr;
	}
}

/**
 * @brief Transfer a single word via SPI
 * @param tx_data Data to transmit
 * @param frame_bits Number of bits in frame
 * @return Received data
 */
uint32_t lpspi_transfer_word(uint32_t tx_data, uint16_t frame_bits)
{
	// 1. Set frame size
	lpspi_set_frame_size(frame_bits);

	// 2. Wait for TX FIFO ready
	while (!(LPSPI0->SR & LPSPI_SR_TDF_MASK))
	{
		// Wait for transmit data flag
	}

	// 3. Write data to transmit
	LPSPI0->TDR = tx_data;

	// 4. Wait for transfer complete
	while (!(LPSPI0->SR & LPSPI_SR_TCF_MASK))
	{
		// Wait for transfer complete flag
	}

	// 5. Clear transfer complete flag
	LPSPI0->SR = LPSPI_SR_TCF_MASK;

	// 6. Wait for receive data available
	while (!(LPSPI0->SR & LPSPI_SR_RDF_MASK))
	{
		// Wait for receive data flag
	}

	// 7. Read received data
	uint32_t rx_data = LPSPI0->RDR;

	// 8. Clear receive data flag
	LPSPI0->SR = LPSPI_SR_RDF_MASK;

	return rx_data;
}

/**
 * @brief Transfer multiple bytes
 * @param tx_buffer Transmit buffer
 * @param rx_buffer Receive buffer
 * @param count Number of bytes to transfer
 * @return true if successful, false if error
 */
bool lpspi_transfer_bytes(const uint8_t *tx_buffer, uint8_t *rx_buffer, uint16_t count)
{
	if (!tx_buffer || !rx_buffer || count == 0)
	{
		return false;
	}

	for (uint16_t i = 0; i < count; i++)
	{
		uint32_t received = lpspi_transfer_word(tx_buffer[i], 8);
		rx_buffer[i] = (uint8_t)(received & 0xFF);
	}

	return true;
}

/**
 * @brief Read LPSPI register values (for debugging)
 */
typedef struct
{
	uint32_t verid;
	uint32_t param;
	uint32_t cr;
	uint32_t sr;
	uint32_t cfgr1;
	uint32_t tcr;
} lpspi_regs_t;

void lpspi_read_registers(lpspi_regs_t *regs)
{
	if (regs)
	{
		regs->verid = LPSPI0->VERID;
		regs->param = LPSPI0->PARAM;
		regs->cr = LPSPI0->CR;
		regs->sr = LPSPI0->SR;
		regs->cfgr1 = LPSPI0->CFGR1;
		regs->tcr = LPSPI0->TCR;
	}
}

/**
 * @brief Simple test function
 */
void lpspi_simple_test(void)
{
	// Initialize LPSPI
	lpspi_init_loopback();

	// Test data
	uint8_t tx_data[4] = {0x10, 0x11, 0x12, 0x13};
	uint8_t rx_data[4] = {0};

	// Transfer data
	bool result = lpspi_transfer_bytes(tx_data, rx_data, 4);

	// Verify loopback (in loopback mode: tx == rx)
	bool success = result;
	if (success)
	{
		for (int i = 0; i < 4; i++)
		{
			if (tx_data[i] != rx_data[i])
			{
				success = false;
				break;
			}
		}
	}

	// Result can be checked with debugger
	// success = true means loopback is working
	(void)success; // Suppress unused variable warning
}

/**
 * @brief Main function for standalone test
 */
int main(void)
{
	// Run simple test
	lpspi_simple_test();

	// Infinite loop
	while (1)
	{
		delay_cycles(1000000);
	}

	return 0;
}

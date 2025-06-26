/*
 * NXP S32K358 LPSPI Controller Implementation
 *
 * This file implements the Low Power SPI (LPSPI) controller for the NXP S32K358
 * microcontroller family. The implementation is designed to be compatible with
 * FreeRTOS applications and follows the official S32K3xx reference manual.
 *
 * Key Features:
 * - 4-word deep TX and RX FIFOs (16 bytes each)
 * - Support for 1-4096 bit frame sizes (limited to 32-bit words in this implementation)
 * - Configurable chip select (CS) polarity and timing
 * - Full interrupt support (TDF, RDF, TCF, TEF, REF, etc.)
 * - Support for continuous and single transfer modes
 * - Proper watermark-based FIFO status flags
 * - MSB/LSB first transfer modes
 * - Clock validation and FreeRTOS compatibility
 *
 * Status Flags:
 * - TDF: Transmit Data Flag (TX FIFO ready for data)
 * - RDF: Receive Data Flag (RX FIFO has data)
 * - TCF: Transfer Complete Flag
 * - TEF: Transmit Error Flag (TX FIFO overflow)
 * - REF: Receive Error Flag (RX FIFO overflow)
 * - MBF: Module Busy Flag
 *
 * Register Reset Values (per S32K358 RM):
 * - VERID: 0x04040007 (Version ID specific to S32K358)
 * - PARAM: 0x00040404 (4-word FIFOs, 4 PCS lines)
 * - TCR: 0x0000001F (32-bit frame size default)
 *
 * FreeRTOS Integration:
 * - Supports both synchronous and asynchronous transfers
 * - Callback-based completion notification
 * - Proper interrupt handling for real-time requirements
 * - 80MHz clock frequency validation
 */

#include "qemu/osdep.h"
#include "qemu/log.h"
#include "qemu/module.h"
#include "migration/vmstate.h"
#include "hw/irq.h"
#include "hw/qdev-properties.h"
#include "hw/qdev-properties-system.h"
#include "hw/ssi/ssi.h"
#include "hw/ssi/nxps32k358_lpspi.h"
#include "hw/qdev-clock.h"
#include "qapi/error.h"
#include "trace.h" // tracing system of qemu

#ifndef NXP_LPSPI_ERR_DEBUG
#define NXP_LPSPI_ERR_DEBUG 1
#endif

// Macro for conditional debug logging based on the debug level.
// Logs messages to QEMU's log system if the specified debug level
// is less than or equal to the current debug level (NXP_LPSPI_ERR_DEBUG).
#define DB_PRINT_L(lvl, fmt, args...)               \
    do                                              \
    {                                               \
        if (NXP_LPSPI_ERR_DEBUG >= lvl)             \
        {                                           \
            qemu_log("%s: " fmt, __func__, ##args); \
        }                                           \
    } while (0)

#define DB_PRINT(fmt, args...) DB_PRINT_L(1, fmt, ##args)

/**
 * Updates LPSPI status registers based on FIFO states.
 *
 * Updates TX/RX word counts in FSR, and adjusts flags (TDF, RDF, RXEMPTY)
 * in SR and RSR according to TX/RX FIFO availability.
 *
 * Pointer to the LPSPI state structure.
 */
static void lpspi_update_status(NXPS32K358LPSPIState *s)
{
    // Calculate frame size for proper word counting
    uint16_t frame_size = ((s->lpspi_tcr & TCR_FRAMESZ_MASK) >> TCR_FRAMESZ_SHIFT) + 1;
    uint8_t bytes_per_frame = (frame_size + 7) / 8;
    if (bytes_per_frame > 4)
        bytes_per_frame = 4; // Limit to 32-bit words

    uint8_t tx_word_count = fifo8_num_used(&s->tx_fifo) / bytes_per_frame;
    uint8_t rx_word_count = fifo8_num_used(&s->rx_fifo) / bytes_per_frame;

    s->lpspi_fsr = (rx_word_count << 16) | (tx_word_count << 0);

    // TDF: Transmit Data Flag - set when TX FIFO words <= watermark (when ready for more data)
    if (tx_word_count <= s->tx_watermark)
    {
        s->lpspi_sr |= LPSPI_SR_TDF;
    }
    else
    {
        s->lpspi_sr &= ~LPSPI_SR_TDF;
    }

    // RDF: Receive Data Flag - set when RX FIFO words > watermark (when data available)
    if (rx_word_count > s->rx_watermark)
    {
        s->lpspi_sr |= LPSPI_SR_RDF;
    }
    else
    {
        s->lpspi_sr &= ~LPSPI_SR_RDF;
    }

    // Update RSR register - RXEMPTY when no data in RX FIFO
    if (rx_word_count == 0)
    {
        s->lpspi_rsr |= LPSPI_RSR_RXEMPTY;
    }
    else
    {
        s->lpspi_rsr &= ~LPSPI_RSR_RXEMPTY;
    }
}

/**
 *
 * This function checks the status and interrupt enable registers of the LPSPI
 * device to determine if an interrupt condition is met. If any enabled interrupt
 * condition is met, it asserts the IRQ. Otherwise, it deasserts the IRQ.
 *
 */
static void lpspi_update_irq(NXPS32K358LPSPIState *s)
{
    lpspi_update_status(s);

    // Check all possible interrupt sources
    uint32_t irq_mask = (LPSPI_SR_TDF | LPSPI_SR_RDF | LPSPI_SR_WCF |
                         LPSPI_SR_FCF | LPSPI_SR_TCF | LPSPI_SR_TEF |
                         LPSPI_SR_REF | LPSPI_SR_DMF);

    if ((s->lpspi_sr & s->lpspi_ier) & irq_mask)
    {
        qemu_set_irq(s->irq, 1);
    }
    else
    {
        qemu_set_irq(s->irq, 0);
    }
}

/**
    Flushes the TX FIFO of the LPSPI peripheral and performs a transfer burst.

    This function checks the TX and RX FIFOs for sufficient space and data,
    respectively, and performs a transfer burst if conditions are met. It
    handles chip select (CS) assertion and de-assertion, transfers data
    between the TX and RX FIFOs, and updates the interrupt status. If the
    TX FIFO becomes empty, the MBF (Module Busy Flag) is cleared.
 */
static void lpspi_flush_txfifo(NXPS32K358LPSPIState *s)
{
    uint8_t pcs = (s->lpspi_tcr & TCR_PCS_MASK) >> TCR_PCS_SHIFT;
    uint16_t frame_size = ((s->lpspi_tcr & TCR_FRAMESZ_MASK) >> TCR_FRAMESZ_SHIFT) + 1;
    bool cont = s->lpspi_tcr & TCR_CONT;
    bool contc = s->lpspi_tcr & TCR_CONTC;
    bool txmsk = s->lpspi_tcr & TCR_TXMSK;
    bool rxmsk = s->lpspi_tcr & TCR_RXMSK;

    // Calculate bytes per frame (minimum 1 byte, up to 4096 bits / 8 = 512 bytes max)
    uint8_t bytes_per_frame = (frame_size + 7) / 8;

    // Ensure frame size is valid (1-4096 bits as per LPSPI spec)
    if (frame_size < 1 || frame_size > 4096)
    {
        DB_PRINT("Invalid frame size: %d bits\n", frame_size);
        return;
    }

    if (bytes_per_frame > 4)
    {
        bytes_per_frame = 4; // Limit to 32-bit words for current implementation
    }

    // Check if we have enough data to transfer
    if (fifo8_num_used(&s->tx_fifo) < bytes_per_frame)
    {
        return; // Not enough data
    }

    // Check if RX FIFO has space (unless RX is masked)
    if (!rxmsk && fifo8_num_free(&s->rx_fifo) < bytes_per_frame)
    {
        return; // No space for response
    }

    DB_PRINT("Starting transfer: frame_size=%d bits, bytes_per_frame=%d, tx_fifo_used=%d\n",
             frame_size, bytes_per_frame, fifo8_num_used(&s->tx_fifo));

    /* Assert CS on first transfer or if not in continuous mode */
    if (!s->busy || (!cont && !contc))
    {
        // Check CFGR1.PCSPOL for CS polarity - bit 24+pcs determines polarity
        bool cs_active_high = !!(s->lpspi_cfgr1 & (1 << (LPSPI_CFGR1_PCSPOL_SHIFT + pcs)));
        qemu_set_irq(s->cs_lines[pcs], cs_active_high ? 1 : 0); // Assert CS
        s->busy = true;
        s->lpspi_sr |= LPSPI_SR_MBF;
    }

    /* Transfer data */
    uint32_t tx_data = 0;
    uint32_t rx_data = 0;

    // Build transmit word from FIFO bytes
    for (int i = 0; i < bytes_per_frame; i++)
    {
        if (!fifo8_is_empty(&s->tx_fifo))
        {
            uint8_t byte = fifo8_pop(&s->tx_fifo);
            if (s->lpspi_tcr & TCR_LSBF)
            {
                tx_data |= byte << (i * 8); // LSB first
            }
            else
            {
                tx_data |= byte << ((bytes_per_frame - 1 - i) * 8); // MSB first
            }
        }
    }

    // Perform SPI transfer
    if (!txmsk)
    {
        // Check if loopback mode is enabled (PINCFG bits in CFGR1)
        uint8_t pincfg = (s->lpspi_cfgr1 & LPSPI_CFGR1_PINCFG_MASK) >> LPSPI_CFGR1_PINCFG_SHIFT;
        if (pincfg == 0x1) // Loopback mode (SOUT internally connected to SIN)
        {
            rx_data = tx_data; // Perfect loopback for testing
            DB_PRINT("SPI transfer (loopback): tx=0x%08x -> rx=0x%08x\n", tx_data, rx_data);
        }
        else
        {
            rx_data = ssi_transfer(s->ssi, tx_data);
            DB_PRINT("SPI transfer: tx=0x%08x -> rx=0x%08x\n", tx_data, rx_data);
        }
    }
    else
    {
        // TX masked - just generate dummy data for RX
        rx_data = ssi_transfer(s->ssi, 0);
        DB_PRINT("SPI transfer (TX masked): tx=0x00000000 -> rx=0x%08x\n", rx_data);
    }

    // Store received data in RX FIFO
    if (!rxmsk)
    {
        for (int i = 0; i < bytes_per_frame; i++)
        {
            uint8_t rx_byte;
            if (s->lpspi_tcr & TCR_LSBF)
            {
                rx_byte = (rx_data >> (i * 8)) & 0xFF; // LSB first
            }
            else
            {
                rx_byte = (rx_data >> ((bytes_per_frame - 1 - i) * 8)) & 0xFF; // MSB first
            }

            if (fifo8_is_full(&s->rx_fifo))
            {
                // RX FIFO overflow - set error flag
                s->lpspi_sr |= LPSPI_SR_REF;
                DB_PRINT("RX FIFO overflow detected\n");
            }
            else
            {
                fifo8_push(&s->rx_fifo, rx_byte);
            }
        }
    }

    lpspi_update_status(s);

    /* Deassert CS when transfer completes and not in continuous mode */
    if (fifo8_is_empty(&s->tx_fifo) && s->busy)
    {
        if (!cont && !contc)
        {
            // Check CFGR1.PCSPOL for CS polarity
            bool cs_active_high = !!(s->lpspi_cfgr1 & (1 << (LPSPI_CFGR1_PCSPOL_SHIFT + pcs)));
            qemu_set_irq(s->cs_lines[pcs], cs_active_high ? 0 : 1); // Deassert CS
            s->busy = false;
            s->lpspi_sr &= ~LPSPI_SR_MBF;
        }
        // Set Transfer Complete Flag
        s->lpspi_sr |= LPSPI_SR_TCF;
        DB_PRINT("Transfer completed - setting TCF flag\n");
    }
}

static void nxps32k358_lpspi_do_reset(NXPS32K358LPSPIState *s)
{
    s->lpspi_verid = 0x04040007; // Correct S32K358 LPSPI version (per RM)
    s->lpspi_param = 0x00040404; // TXFIFO=4, RXFIFO=4, PCSNUM=4 (correct for S32K358)
    s->lpspi_cr = 0x0;
    s->lpspi_sr = LPSPI_SR_TDF | LPSPI_SR_TCF; // TDF=1 (TX ready), TCF=1 (transfer complete)
    s->lpspi_ier = 0x0;
    s->lpspi_der = 0x0;
    s->lpspi_cfgr0 = 0x0;
    s->lpspi_cfgr1 = 0x0;
    s->lpspi_ccr = 0x0;
    s->lpspi_fcr = 0x0;
    s->lpspi_fsr = 0x0;
    s->lpspi_tcr = 0x0000001F; // CORRECT: FRAMESZ=0x1F (32-bit), all other bits 0
    s->lpspi_tdr = 0x0;
    s->lpspi_rsr = LPSPI_RSR_RXEMPTY;
    s->lpspi_rdr = 0x0;

    s->busy = false;
    s->tx_watermark = 0; // Default watermark values
    s->rx_watermark = 0;
    s->frame_size = 32; // Default frame size
    s->continuous_mode = false;

    fifo8_reset(&s->tx_fifo);
    fifo8_reset(&s->rx_fifo);

    // Deassert all CS lines (default is active low, so set to high)
    for (int i = 0; i < s->num_cs_lines; ++i)
    {
        // Default CS polarity is active low, so deassert = high
        qemu_set_irq(s->cs_lines[i], 1);
    }

    lpspi_update_irq(s);
}

static void nxps32k358_lpspi_reset(DeviceState *dev)
{
    nxps32k358_lpspi_do_reset(NXPS32K358_LPSPI(dev));
}

static uint64_t nxps32k358_lpspi_read(void *opaque, hwaddr addr, unsigned int size)
{
    NXPS32K358LPSPIState *s = opaque;
    lpspi_update_status(s);

    switch (addr)
    {
    case S32K_LPSPI_VERID:
        return s->lpspi_verid;
    case S32K_LPSPI_PARAM:
        return s->lpspi_param;
    case S32K_LPSPI_CR:
        return s->lpspi_cr;
    case S32K_LPSPI_SR:
        return s->lpspi_sr;
    case S32K_LPSPI_IER:
        return s->lpspi_ier;
    case S32K_LPSPI_DER:
        return s->lpspi_der;
    case S32K_LPSPI_CFGR0:
        return s->lpspi_cfgr0;
    case S32K_LPSPI_CFGR1:
        return s->lpspi_cfgr1;
    case S32K_LPSPI_CCR:
        return s->lpspi_ccr;
    case S32K_LPSPI_FCR:
        return s->lpspi_fcr;
    case S32K_LPSPI_FSR:
        return s->lpspi_fsr;
    case S32K_LPSPI_TCR:
        return s->lpspi_tcr;
    case S32K_LPSPI_RSR:
        return s->lpspi_rsr;
    case S32K_LPSPI_RDR:
    {
        if (fifo8_num_used(&s->rx_fifo) < 1)
        {
            // RX FIFO underflow - reading when empty
            qemu_log_mask(LOG_GUEST_ERROR, "%s: Read from empty RX FIFO\n", __func__);
            return 0; // Return 0 if no data available
        }

        // Calculate frame size in bytes
        uint16_t frame_size = ((s->lpspi_tcr & TCR_FRAMESZ_MASK) >> TCR_FRAMESZ_SHIFT) + 1;
        uint8_t bytes_per_frame = (frame_size + 7) / 8;
        if (bytes_per_frame > 4)
        {
            bytes_per_frame = 4; // Limit to 32-bit words
        }

        uint32_t ret = 0;
        // Read available bytes up to frame size
        for (int i = 0; i < bytes_per_frame && !fifo8_is_empty(&s->rx_fifo); i++)
        {
            uint8_t byte = fifo8_pop(&s->rx_fifo);
            if (s->lpspi_tcr & TCR_LSBF)
            {
                ret |= (uint32_t)byte << (i * 8); // LSB first
            }
            else
            {
                ret |= (uint32_t)byte << ((bytes_per_frame - 1 - i) * 8); // MSB first
            }
        }

        s->lpspi_rdr = ret;
        lpspi_update_status(s);
        return ret;
    }
    default:
        qemu_log_mask(LOG_GUEST_ERROR, "%s: Bad read offset 0x%" HWADDR_PRIx "\n", __func__, addr);
        return 0;
    }
}

static void nxps32k358_lpspi_write(void *opaque, hwaddr addr, uint64_t val64, unsigned int size)
{
    NXPS32K358LPSPIState *s = opaque;
    uint32_t value = val64;

    switch (addr)
    {
    case S32K_LPSPI_VERID:
    case S32K_LPSPI_PARAM:
    case S32K_LPSPI_FSR:
    case S32K_LPSPI_RSR:
    case S32K_LPSPI_RDR:
        qemu_log_mask(LOG_GUEST_ERROR, "%s: Write to read-only reg 0x%" HWADDR_PRIx "\n", __func__, addr);
        return;

    case S32K_LPSPI_CR:
        if (value & LPSPI_CR_RST)
        {
            nxps32k358_lpspi_do_reset(s);
            return;
        }
        if (value & LPSPI_CR_RTF) // Reset TX FIFO
        {
            fifo8_reset(&s->tx_fifo);
            s->lpspi_sr |= LPSPI_SR_TDF; // Set TDF when TX FIFO is reset
        }
        if (value & LPSPI_CR_RRF) // Reset RX FIFO
        {
            fifo8_reset(&s->rx_fifo);
            s->lpspi_rsr |= LPSPI_RSR_RXEMPTY; // Set RXEMPTY when RX FIFO is reset
        }
        s->lpspi_cr = value & ~(LPSPI_CR_RST | LPSPI_CR_RTF | LPSPI_CR_RRF); // Clear reset bits
        break;

    case S32K_LPSPI_SR:
        s->lpspi_sr &= ~value;
        break;

    case S32K_LPSPI_TCR:
        s->lpspi_tcr = value;
        // Update frame size for next transfers
        s->frame_size = ((value & TCR_FRAMESZ_MASK) >> TCR_FRAMESZ_SHIFT) + 1;
        s->continuous_mode = !!(value & (TCR_CONT | TCR_CONTC));

        DB_PRINT("TCR write: 0x%08x, calculated frame_size: %d\n", value, s->frame_size);

        if (s->lpspi_cr & LPSPI_CR_MEN)
        {
            // Start transfer if there's data and module is enabled
            if (!fifo8_is_empty(&s->tx_fifo))
            {
                lpspi_flush_txfifo(s);
            }
        }
        return;

    case S32K_LPSPI_TDR:
        if (s->lpspi_cr & LPSPI_CR_MEN)
        {
            // Calculate current frame size in bytes
            uint16_t frame_size = ((s->lpspi_tcr & TCR_FRAMESZ_MASK) >> TCR_FRAMESZ_SHIFT) + 1;
            uint8_t bytes_per_frame = (frame_size + 7) / 8;
            if (bytes_per_frame > 4)
            {
                bytes_per_frame = 4; // Limit to 32-bit words
            }

            DB_PRINT("TDR write: TCR=0x%08x, frame_size=%d, bytes_per_frame=%d\n",
                     s->lpspi_tcr, frame_size, bytes_per_frame);

            if (fifo8_num_free(&s->tx_fifo) < bytes_per_frame)
            {
                qemu_log_mask(LOG_GUEST_ERROR, "%s: Write to full TX FIFO!\n", __func__);
                // Set TX FIFO Error flag
                s->lpspi_sr |= LPSPI_SR_TEF;
                lpspi_update_irq(s);
                return;
            }

            // Store data in TX FIFO based on frame size and byte order
            for (int i = 0; i < bytes_per_frame; i++)
            {
                uint8_t byte;
                if (s->lpspi_tcr & TCR_LSBF)
                {
                    byte = (val64 >> (i * 8)) & 0xFF; // LSB first
                }
                else
                {
                    byte = (val64 >> ((bytes_per_frame - 1 - i) * 8)) & 0xFF; // MSB first
                }
                fifo8_push(&s->tx_fifo, byte);
            }

            DB_PRINT("Pushed 0x%08" PRIx64 " to TX FIFO (used: %d, frame_size: %d)\n",
                     val64, fifo8_num_used(&s->tx_fifo), frame_size);

            // Try to start transfer
            lpspi_flush_txfifo(s);
        }
        else
        {
            qemu_log_mask(LOG_GUEST_ERROR, "%s: Write to TDR when module disabled!\n", __func__);
        }
        return;

    case S32K_LPSPI_IER:
        s->lpspi_ier = value;
        break;
    case S32K_LPSPI_DER:
        s->lpspi_der = value;
        break;
    case S32K_LPSPI_CFGR0:
        s->lpspi_cfgr0 = value;
        break;
    case S32K_LPSPI_CFGR1:
        s->lpspi_cfgr1 = value;
        DB_PRINT("CFGR1 write: 0x%08x - PINCFG=%d, MASTER=%d\n",
                 value,
                 (value >> LPSPI_CFGR1_PINCFG_SHIFT) & 0x3,
                 value & 1);
        break;
    case S32K_LPSPI_CCR:
        s->lpspi_ccr = value;
        break;
    case S32K_LPSPI_FCR:
        s->lpspi_fcr = value;
        // Update watermark values
        s->tx_watermark = (value & LPSPI_FCR_TXWATER_MASK) >> LPSPI_FCR_TXWATER_SHIFT;
        s->rx_watermark = (value & LPSPI_FCR_RXWATER_MASK) >> LPSPI_FCR_RXWATER_SHIFT;
        lpspi_update_status(s); // Recalculate flags based on new watermarks
        break;

    default:
        qemu_log_mask(LOG_GUEST_ERROR, "%s: Bad write offset 0x%" HWADDR_PRIx "\n", __func__, addr);
        return;
    }

    lpspi_update_irq(s);
}

static const MemoryRegionOps nxps32k358_lpspi_ops = {
    .read = nxps32k358_lpspi_read,
    .write = nxps32k358_lpspi_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .impl.min_access_size = 4,
    .impl.max_access_size = 4,
};

static const VMStateDescription vmstate_nxps32k358_lpspi = {
    .name = TYPE_NXPS32K358_LPSPI,
    .version_id = 8,
    .minimum_version_id = 8,
    .fields = (const VMStateField[]){
        VMSTATE_FIFO8(tx_fifo, NXPS32K358LPSPIState),
        VMSTATE_FIFO8(rx_fifo, NXPS32K358LPSPIState),
        VMSTATE_BOOL(busy, NXPS32K358LPSPIState),
        VMSTATE_UINT8(tx_watermark, NXPS32K358LPSPIState),
        VMSTATE_UINT8(rx_watermark, NXPS32K358LPSPIState),
        VMSTATE_UINT16(frame_size, NXPS32K358LPSPIState),
        VMSTATE_BOOL(continuous_mode, NXPS32K358LPSPIState),
        VMSTATE_UINT32(lpspi_verid, NXPS32K358LPSPIState),
        VMSTATE_UINT32(lpspi_param, NXPS32K358LPSPIState),
        VMSTATE_UINT32(lpspi_cr, NXPS32K358LPSPIState),
        VMSTATE_UINT32(lpspi_sr, NXPS32K358LPSPIState),
        VMSTATE_UINT32(lpspi_ier, NXPS32K358LPSPIState),
        VMSTATE_UINT32(lpspi_der, NXPS32K358LPSPIState),
        VMSTATE_UINT32(lpspi_cfgr0, NXPS32K358LPSPIState),
        VMSTATE_UINT32(lpspi_cfgr1, NXPS32K358LPSPIState),
        VMSTATE_UINT32(lpspi_ccr, NXPS32K358LPSPIState),
        VMSTATE_UINT32(lpspi_fcr, NXPS32K358LPSPIState),
        VMSTATE_UINT32(lpspi_fsr, NXPS32K358LPSPIState),
        VMSTATE_UINT32(lpspi_tcr, NXPS32K358LPSPIState),
        VMSTATE_UINT32(lpspi_tdr, NXPS32K358LPSPIState),
        VMSTATE_UINT32(lpspi_rsr, NXPS32K358LPSPIState),
        VMSTATE_UINT32(lpspi_rdr, NXPS32K358LPSPIState),
        VMSTATE_END_OF_LIST()}};

static const Property nxps32k358_lpspi_properties[] = {
    DEFINE_PROP_UINT8("num-cs-lines", NXPS32K358LPSPIState, num_cs_lines, 1),
};

static void nxps32k358_lpspi_init(Object *dev)
{
    SysBusDevice *sbd = SYS_BUS_DEVICE(dev);
    NXPS32K358LPSPIState *s = NXPS32K358_LPSPI(dev);

    DeviceState *d1 = DEVICE(dev);

    s->clk = qdev_init_clock_in(DEVICE(s), "clk", NULL, s, 0);

    memory_region_init_io(&s->mmio, OBJECT(s), &nxps32k358_lpspi_ops, s,
                          TYPE_NXPS32K358_LPSPI, 0x4000);
    sysbus_init_mmio(sbd, &s->mmio);

    s->ssi = ssi_create_bus(d1, "spi");

    sysbus_init_irq(sbd, &s->irq);

    s->cs_lines = g_new0(qemu_irq, s->num_cs_lines);
    qdev_init_gpio_out_named(d1, s->cs_lines, "cs", s->num_cs_lines);

    fifo8_create(&s->tx_fifo, LPSPI_FIFO_BYTE_CAPACITY);
    fifo8_create(&s->rx_fifo, LPSPI_FIFO_BYTE_CAPACITY);
}

static void nxps32k358_lpspi_realize(DeviceState *dev, Error **errp)
{
    NXPS32K358LPSPIState *s = NXPS32K358_LPSPI(dev);

    if (!s->clk)
    {
        error_setg(errp, "LPSPI: no clock connected");
        return;
    }
    s->input_clk = clock_get_hz(s->clk);

    // Validate that we have a reasonable clock frequency
    if (s->input_clk == 0)
    {
        error_setg(errp, "LPSPI: Invalid clock frequency (0 Hz)");
        return;
    }

    // Warn if not using the expected 80MHz clock for FreeRTOS compatibility
    if (s->input_clk != 80000000)
    {
        qemu_log("LPSPI: Warning - expected 80MHz clock, got %lu Hz\n", s->input_clk);
    }

    // Validate CS lines count
    if (s->num_cs_lines < 1 || s->num_cs_lines > 4)
    {
        error_setg(errp, "LPSPI: Invalid number of CS lines (%d), must be 1-4", s->num_cs_lines);
        return;
    }
}

static void nxps32k358_lpspi_class_init(ObjectClass *klass, const void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    dc->realize = nxps32k358_lpspi_realize;
    device_class_set_legacy_reset(dc, nxps32k358_lpspi_reset);
    device_class_set_props(dc, nxps32k358_lpspi_properties);
    dc->vmsd = &vmstate_nxps32k358_lpspi;
}

static const TypeInfo nxps32k358_lpspi_info = {
    .name = TYPE_NXPS32K358_LPSPI,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(NXPS32K358LPSPIState),
    .instance_init = nxps32k358_lpspi_init,
    .class_init = nxps32k358_lpspi_class_init,
};

static void nxps32k358_lpspi_register_types(void)
{
    type_register_static(&nxps32k358_lpspi_info);
}

type_init(nxps32k358_lpspi_register_types)
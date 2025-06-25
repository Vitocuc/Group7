#include "qemu/osdep.h"
#include "qemu/log.h"
#include "qemu/module.h"
#include "migration/vmstate.h"
#include "hw/irq.h"
#include "hw/qdev-properties.h"
#include "hw/ssi/ssi.h"
#include "hw/ssi/nxps32k358_lpspi.h"
#include "hw/qdev-clock.h"
#include "hw/qdev-properties-system.h"
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
    uint8_t tx_word_count = fifo8_num_used(&s->tx_fifo) / 4;
    uint8_t rx_word_count = fifo8_num_used(&s->rx_fifo) / 4;

    s->lpspi_fsr = (rx_word_count << 16) | (tx_word_count << 0);

    if (fifo8_num_free(&s->tx_fifo) >= 4)
    {
        s->lpspi_sr |= LPSPI_SR_TDF;
    }
    else
    {
        s->lpspi_sr &= ~LPSPI_SR_TDF;
    }

    if (fifo8_num_used(&s->rx_fifo) >= 4)
    {
        s->lpspi_sr |= LPSPI_SR_RDF;
    }
    else
    {
        s->lpspi_sr &= ~LPSPI_SR_RDF;
    }

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
 * device to determine if an interrupt condition is met. If the transmit data
 * flag (TDF) or receive data flag (RDF) is set and enabled, it asserts the IRQ.
 * Otherwise, it deasserts the IRQ.
 *
 */
static void lpspi_update_irq(NXPS32K358LPSPIState *s)
{
    lpspi_update_status(s);
    if ((s->lpspi_sr & s->lpspi_ier) & (LPSPI_SR_TDF | LPSPI_SR_RDF))
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

    /* 1) on first data word, assert CS (active low) */
    if (!s->busy)
    {
        qemu_set_irq(s->cs_lines[pcs], 0);
        s->busy = true;
        s->lpspi_sr |= LPSPI_SR_MBF;
    }

    /* 2) shift data until TX empty or RX full */
    while (!fifo8_is_empty(&s->tx_fifo) && /* rx space */)
    {
        uint32_t tx = fifo8_get(&s->tx_fifo);
        uint32_t rx = ssi_transfer(s->ssi, tx);
        fifo8_put(&s->rx_fifo, rx);
    }
    lpspi_update_status(s);

    /* 3) when TX is empty, deassert CS and clear busy/MBF */
    if (fifo8_is_empty(&s->tx_fifo) && s->busy)
    {
        qemu_set_irq(s->cs_lines[pcs], 1); // ← deassert CS
        s->busy = false;
        s->lpspi_sr &= ~LPSPI_SR_MBF;
    }
}

static void nxps32k358_lpspi_do_reset(NXPS32K358LPSPIState *s)
{
    s->lpspi_verid = 0x02000004;
    s->lpspi_param = 0x00080202;
    s->lpspi_cr = 0x0;
    s->lpspi_sr = LPSPI_SR_TDF;
    s->lpspi_ier = 0x0;
    s->lpspi_der = 0x0;
    s->lpspi_cfgr0 = 0x0;
    s->lpspi_cfgr1 = 0x0;
    s->lpspi_ccr = 0x0;
    s->lpspi_fcr = 0x0;
    s->lpspi_fsr = 0x0;
    s->lpspi_tcr = 0xFFFFFFFF;
    s->lpspi_tdr = 0x0;
    s->lpspi_rsr = LPSPI_RSR_RXEMPTY;
    s->lpspi_rdr = 0x0;

    fifo8_reset(&s->tx_fifo);
    fifo8_reset(&s->rx_fifo);

    for (int i = 0; i < s->num_cs_lines; ++i)
    {
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
        if (fifo8_num_used(&s->rx_fifo) < 4)
        {
            return 0;
        }
        uint32_t ret = 0;
        ret |= (uint32_t)fifo8_pop(&s->rx_fifo);
        ret |= (uint32_t)fifo8_pop(&s->rx_fifo) << 8;
        ret |= (uint32_t)fifo8_pop(&s->rx_fifo) << 16;
        ret |= (uint32_t)fifo8_pop(&s->rx_fifo) << 24;
        s->lpspi_rdr = ret;
        lpspi_flush_txfifo(s);
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
        if (value & LPSPI_CR_RSTF)
        {
            fifo8_reset(&s->tx_fifo);
            fifo8_reset(&s->rx_fifo);
        }
        s->lpspi_cr = value;
        break;

    case S32K_LPSPI_SR:
        s->lpspi_sr &= ~value;
        break;

    case S32K_LPSPI_TCR:
        s->lpspi_tcr = value;
        if (s->lpspi_cr & LPSPI_CR_MEN)
        {
            if (!(s->lpspi_sr & LPSPI_SR_MBF) && !fifo8_is_empty(&s->tx_fifo))
            {
                s->lpspi_sr |= LPSPI_SR_MBF;
            }
            lpspi_flush_txfifo(s);
        }
        return;

    case S32K_LPSPI_TDR:
        if (s->lpspi_cr & LPSPI_CR_MEN)
        {
            if (fifo8_num_free(&s->tx_fifo) < 4)
            {
                qemu_log_mask(LOG_GUEST_ERROR, "%s: Write to full TX FIFO!\n", __func__);
            }
            else
            {
                // Push data in little-endian order
                fifo8_push(&s->tx_fifo, val64 & 0xFF);
                fifo8_push(&s->tx_fifo, (val64 >> 8) & 0xFF);
                fifo8_push(&s->tx_fifo, (val64 >> 16) & 0xFF);
                fifo8_push(&s->tx_fifo, (val64 >> 24) & 0xFF);

                DB_PRINT("Pushed 0x%08" PRIx64 " to TX FIFO (used: %d)\n",
                         val64, fifo8_num_used(&s->tx_fifo));

                // Only flush if we have a full word (4 bytes)
                if (fifo8_num_used(&s->tx_fifo) >= 4)
                {
                    lpspi_flush_txfifo(s);
                }
            }
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
        break;
    case S32K_LPSPI_CCR:
        s->lpspi_ccr = value;
        break;
    case S32K_LPSPI_FCR:
        s->lpspi_fcr = value;
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
    .version_id = 7,
    .minimum_version_id = 7,
    .fields = (const VMStateField[]){
        VMSTATE_FIFO8(tx_fifo, NXPS32K358LPSPIState),
        VMSTATE_FIFO8(rx_fifo, NXPS32K358LPSPIState),
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
    DEFINE_PROP_CLOCK("clk", NXPS32K358LPSPIState, clk), // ← enable clock binding
    DEFINE_PROP_END_OF_LIST(),
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
    s->input_clk = clock_get_rate(s->clk); // ← grab 80 MHz
    // …then compute prescalers etc before MEN=1…
}

static void nxps32k358_lpspi_class_init(ObjectClass *klass, const void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    dc->realize = nxps32k358_lpspi_realize;
    device_class_set_legacy_reset(dc, nxps32k358_lpspi_reset);
    device_class_set_props(dc, nxps32k358_lpspi_properties);
    dc->vmsd = &vmstate_nxps32k358_lpspi;
    // device_class_add_clock_in(dc, "clk", NULL, NULL, 0);
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
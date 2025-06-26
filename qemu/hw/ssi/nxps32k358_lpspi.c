#include "qemu/osdep.h"
#include "qemu/log.h"
#include "qemu/module.h"
#include "migration/vmstate.h"
#include "hw/irq.h"
#include "hw/qdev-properties.h"
#include "hw/ssi/ssi.h"
#include "hw/ssi/nxps32k358_lpspi.h"
#include "hw/qdev-clock.h"
#include "qapi/error.h"

#ifndef NXP_LPSPI_ERR_DEBUG
#define NXP_LPSPI_ERR_DEBUG 1
#endif

// Macro per il logging di debug condizionale.
#define DB_PRINT_L(lvl, fmt, args...)                                   \
    do {                                                                \
        if (NXP_LPSPI_ERR_DEBUG >= lvl) {                               \
            qemu_log("%s: " fmt, __func__, ##args);                      \
        }                                                               \
    } while (0)

#define DB_PRINT(fmt, args...) DB_PRINT_L(1, fmt, ##args)


// --- INIZIO DEFINIZIONI REGISTRI (Idealmente da mettere in nxps32k358_lpspi.h) ---
#define S32K_LPSPI_CCR1 0x44
#define FSR_RXCOUNT_SHIFT 16
#define FSR_TXCOUNT_SHIFT 0
#define TCR_CPOL              (1U << 31)
#define TCR_CPHA              (1U << 30)
#define TCR_PRESCALE_MASK     (0x7 << 27)
#define TCR_PRESCALE_SHIFT    27
#define CCR1_SCKSET_MASK      (0xFF << 0)
#define CCR1_SCKSET_SHIFT     0
#define CCR1_SCKHLD_MASK      (0xFF << 8)
#define CCR1_SCKHLD_SHIFT     8
#define CFGR1_PCSPOL_SHIFT 8
// --- FINE DEFINIZIONI REGISTRI ---

static void lpspi_update_status(NXPS32K358LPSPIState *s)
{
    uint8_t tx_word_count = fifo8_num_used(&s->tx_fifo) / 4;
    uint8_t rx_word_count = fifo8_num_used(&s->rx_fifo) / 4;

    s->lpspi_fsr = (rx_word_count << FSR_RXCOUNT_SHIFT) | (tx_word_count << FSR_TXCOUNT_SHIFT);

    if (fifo8_num_free(&s->tx_fifo) >= 4) {
        s->lpspi_sr |= LPSPI_SR_TDF;
    } else {
        s->lpspi_sr &= ~LPSPI_SR_TDF;
    }

    if (fifo8_num_used(&s->rx_fifo) >= 4) {
        s->lpspi_sr |= LPSPI_SR_RDF;
    } else {
        s->lpspi_sr &= ~LPSPI_SR_RDF;
    }

    if (rx_word_count == 0) {
        s->lpspi_rsr |= LPSPI_RSR_RXEMPTY;
    } else {
        s->lpspi_rsr &= ~LPSPI_RSR_RXEMPTY;
    }
}

static void lpspi_update_irq(NXPS32K358LPSPIState *s)
{
    lpspi_update_status(s);
    if ((s->lpspi_sr & s->lpspi_ier) & (LPSPI_SR_TDF | LPSPI_SR_RDF)) {
        qemu_set_irq(s->irq, 1);
    } else {
        qemu_set_irq(s->irq, 0);
    }
}

static void lpspi_update_clock_config(NXPS32K358LPSPIState *s)
{
    uint64_t input_clk = clock_get_hz(s->clk);
    if (input_clk == 0) {
        return;
    }

    uint32_t prescale_val = 1 << ((s->lpspi_tcr & TCR_PRESCALE_MASK) >> TCR_PRESCALE_SHIFT);
    uint32_t sckset = ((s->lpspi_ccr1 & CCR1_SCKSET_MASK) >> CCR1_SCKSET_SHIFT) + 1;
    uint32_t sckhld = ((s->lpspi_ccr1 & CCR1_SCKHLD_MASK) >> CCR1_SCKHLD_SHIFT) + 1;
    if (sckset + sckhld + 2 == 0) return; // Evita divisione per zero
    uint32_t sck_period_divisor = sckset + sckhld + 2;
    uint64_t sck_freq = input_clk / (prescale_val * sck_period_divisor);

    uint8_t cpol = (s->lpspi_tcr & TCR_CPOL) ? 1 : 0;
    uint8_t cpha = (s->lpspi_tcr & TCR_CPHA) ? 1 : 0;

    DB_PRINT("Clock config updated: CPOL=%d, CPHA=%d, PRESCALE_DIV=%u, SCK_FREQ=%" PRIu64 " Hz\n",
             cpol, cpha, prescale_val, sck_freq);
}

static void lpspi_flush_txfifo(NXPS32K358LPSPIState *s)
{
    if (fifo8_num_used(&s->tx_fifo) < 4) {
        return;
    }

    uint8_t pcs = (s->lpspi_tcr & TCR_PCS_MASK) >> TCR_PCS_SHIFT;
    if (pcs >= s->num_cs_lines) {
        qemu_log_mask(LOG_GUEST_ERROR, "%s: Invalid Chip Select %d\n", __func__, pcs);
        return;
    }

    bool is_active_high = (s->lpspi_cfgr1 >> (CFGR1_PCSPOL_SHIFT + pcs)) & 1;
    uint8_t active_level = is_active_high ? 1 : 0;
    uint8_t inactive_level = is_active_high ? 0 : 1;

    DB_PRINT("Asserting CS%d with level %d (active-high: %d).\n", pcs, active_level, is_active_high);
    qemu_set_irq(s->cs_lines[pcs], active_level);

    while ((fifo8_num_used(&s->tx_fifo) >= 4) && (fifo8_num_free(&s->rx_fifo) >= 4)) {
        uint32_t tx_word = 0;
        tx_word |= (uint32_t)fifo8_pop(&s->tx_fifo);
        tx_word |= (uint32_t)fifo8_pop(&s->tx_fifo) << 8;
        tx_word |= (uint32_t)fifo8_pop(&s->tx_fifo) << 16;
        tx_word |= (uint32_t)fifo8_pop(&s->tx_fifo) << 24;

        uint32_t rx_word = ssi_transfer(s->ssi, tx_word);

        fifo8_push(&s->rx_fifo, rx_word & 0xFF);
        fifo8_push(&s->rx_fifo, (rx_word >> 8) & 0xFF);
        fifo8_push(&s->rx_fifo, (rx_word >> 16) & 0xFF);
        fifo8_push(&s->rx_fifo, (rx_word >> 24) & 0xFF);
    }

    DB_PRINT("De-asserting CS%d with level %d after transfer burst.\n", pcs, inactive_level);
    qemu_set_irq(s->cs_lines[pcs], inactive_level);

    if (fifo8_is_empty(&s->tx_fifo)) {
        s->lpspi_sr &= ~LPSPI_SR_MBF;
        DB_PRINT("TX FIFO is now empty, MBF cleared.\n");
    }

    lpspi_update_irq(s);
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
    s->lpspi_ccr1 = 0x0;
    s->lpspi_fcr = 0x0;
    s->lpspi_fsr = 0x0;
    s->lpspi_tcr = 0xFFFFFFFF;
    s->lpspi_tdr = 0x0;
    s->lpspi_rsr = LPSPI_RSR_RXEMPTY;
    s->lpspi_rdr = 0x0;

    fifo8_reset(&s->tx_fifo);
    fifo8_reset(&s->rx_fifo);

    for (int i = 0; i < s->num_cs_lines; ++i) {
        qemu_set_irq(s->cs_lines[i], 1);
    }

    lpspi_update_clock_config(s);
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

    switch (addr) {
    case S32K_LPSPI_VERID:   return s->lpspi_verid;
    case S32K_LPSPI_PARAM:   return s->lpspi_param;
    case S32K_LPSPI_CR:      return s->lpspi_cr;
    case S32K_LPSPI_SR:      return s->lpspi_sr;
    case S32K_LPSPI_IER:     return s->lpspi_ier;
    case S32K_LPSPI_DER:     return s->lpspi_der;
    case S32K_LPSPI_CFGR0:   return s->lpspi_cfgr0;
    case S32K_LPSPI_CFGR1:   return s->lpspi_cfgr1;
    case S32K_LPSPI_CCR:     return s->lpspi_ccr;
    case S32K_LPSPI_CCR1:    return s->lpspi_ccr1;
    case S32K_LPSPI_FCR:     return s->lpspi_fcr;
    case S32K_LPSPI_FSR:     return s->lpspi_fsr;
    case S32K_LPSPI_TCR:     return s->lpspi_tcr;
    case S32K_LPSPI_RSR:     return s->lpspi_rsr;
    case S32K_LPSPI_RDR:
    {
        if (fifo8_num_used(&s->rx_fifo) < 4) {
            qemu_log_mask(LOG_GUEST_ERROR, "%s: Read from empty RX FIFO!\n", __func__);
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

    switch (addr) {
    case S32K_LPSPI_VERID:
    case S32K_LPSPI_PARAM:
    case S32K_LPSPI_FSR:
    case S32K_LPSPI_RSR:
    case S32K_LPSPI_RDR:
        qemu_log_mask(LOG_GUEST_ERROR, "%s: Write to read-only reg 0x%" HWADDR_PRIx "\n", __func__, addr);
        return;

    case S32K_LPSPI_CR:
        if (value & LPSPI_CR_RST) {
            nxps32k358_lpspi_do_reset(s);
            return;
        }
        if (value & LPSPI_CR_RSTF) {
            fifo8_reset(&s->tx_fifo);
            fifo8_reset(&s->rx_fifo);
        }
        s->lpspi_cr = value & ~(LPSPI_CR_RSTF);
        break;

    case S32K_LPSPI_SR:
        s->lpspi_sr &= ~value;
        break;

    case S32K_LPSPI_TCR:
        s->lpspi_tcr = value;
        lpspi_update_clock_config(s);
        if (s->lpspi_cr & LPSPI_CR_MEN) {
            if (!(s->lpspi_sr & LPSPI_SR_MBF) && !fifo8_is_empty(&s->tx_fifo)) {
                s->lpspi_sr |= LPSPI_SR_MBF;
            }
            lpspi_flush_txfifo(s);
        }
        return;

    case S32K_LPSPI_TDR:
        if (s->lpspi_cr & LPSPI_CR_MEN) {
            if (fifo8_num_free(&s->tx_fifo) < 4) {
                qemu_log_mask(LOG_GUEST_ERROR, "%s: Write to full TX FIFO!\n", __func__);
            } else {
                s->lpspi_sr |= LPSPI_SR_MBF;
                s->lpspi_tdr = value;
                fifo8_push(&s->tx_fifo, value & 0xFF);
                fifo8_push(&s->tx_fifo, (value >> 8) & 0xFF);
                fifo8_push(&s->tx_fifo, (value >> 16) & 0xFF);
                fifo8_push(&s->tx_fifo, (value >> 24) & 0xFF);
            }
            lpspi_flush_txfifo(s);
        } else {
            qemu_log_mask(LOG_GUEST_ERROR, "LPSPI is not enabled, cannot write to TDR\n");
        }
        return;

    case S32K_LPSPI_IER:    s->lpspi_ier = value; break;
    case S32K_LPSPI_DER:    s->lpspi_der = value; break;
    case S32K_LPSPI_CFGR0:  s->lpspi_cfgr0 = value; break;
    case S32K_LPSPI_CFGR1:
        s->lpspi_cfgr1 = value;
        lpspi_update_clock_config(s);
        break;
    case S32K_LPSPI_CCR:
        s->lpspi_ccr = value;
        lpspi_update_clock_config(s);
        break;
    case S32K_LPSPI_CCR1:
        s->lpspi_ccr1 = value;
        lpspi_update_clock_config(s);
        break;
    case S32K_LPSPI_FCR:    s->lpspi_fcr = value; break;

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
    .version_id = 9,
    .minimum_version_id = 9,
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
        VMSTATE_UINT32(lpspi_ccr1, NXPS32K358LPSPIState),
        VMSTATE_UINT32(lpspi_fcr, NXPS32K358LPSPIState),
        VMSTATE_UINT32(lpspi_fsr, NXPS32K358LPSPIState),
        VMSTATE_UINT32(lpspi_tcr, NXPS32K358LPSPIState),
        VMSTATE_UINT32(lpspi_tdr, NXPS32K358LPSPIState),
        VMSTATE_UINT32(lpspi_rsr, NXPS32K358LPSPIState),
        VMSTATE_UINT32(lpspi_rdr, NXPS32K358LPSPIState),
        VMSTATE_UINT8(num_cs_lines, NXPS32K358LPSPIState),
        VMSTATE_END_OF_LIST()
    }
};

static void nxps32k358_lpspi_init(Object *dev)
{
    NXPS32K358LPSPIState *s = NXPS32K358_LPSPI(dev);
    SysBusDevice *sbd = SYS_BUS_DEVICE(dev);

    qdev_init_clock_in(DEVICE(s), "clk", NULL, NULL, 0);

    memory_region_init_io(&s->mmio, OBJECT(s), &nxps32k358_lpspi_ops, s,
                          TYPE_NXPS32K358_LPSPI, 0x4000);
    sysbus_init_mmio(sbd, &s->mmio);

    s->ssi = ssi_create_bus(DEVICE(s), "spi");

    sysbus_init_irq(sbd, &s->irq);
}

static void nxps32k358_lpspi_realize(DeviceState *dev, Error **errp)
{
    NXPS32K358LPSPIState *s = NXPS32K358_LPSPI(dev);

    if (!s->clk) {
        error_setg(errp, "LPSPI: clock not connected");
        return;
    }
    if (clock_get_hz(s->clk) == 0) {
        qemu_log_mask(LOG_GUEST_ERROR, "LPSPI: input clock frequency is zero.\n");
    }

    // Imposta il numero di linee CS qui, dato che abbiamo rimosso le proprietà
    s->num_cs_lines = 8;
    
    s->cs_lines = g_new0(qemu_irq, s->num_cs_lines);
    qdev_init_gpio_out_named(dev, s->cs_lines, "cs", s->num_cs_lines);

    fifo8_create(&s->tx_fifo, LPSPI_FIFO_BYTE_CAPACITY);
    fifo8_create(&s->rx_fifo, LPSPI_FIFO_BYTE_CAPACITY);
}

static void nxps32k358_lpspi_class_init(ObjectClass *klass, const void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    dc->realize = nxps32k358_lpspi_realize;
    device_class_set_legacy_reset(dc, nxps32k358_lpspi_reset);
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

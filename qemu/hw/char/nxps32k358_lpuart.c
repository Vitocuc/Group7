#include "qemu/osdep.h"
#include "qemu/log.h"
#include "qemu/module.h"
#include "hw/char/nxps32k358_lpuart.h"
#include "hw/qdev-clock.h"
#include "hw/qdev-properties.h"
#include "hw/qdev-properties-system.h"
#include "hw/irq.h"
#include "trace.h" // tracing system of qemu
#include "chardev/char-serial.h"
#include "qapi/error.h"

#ifndef NXP_LPUART_DEBUG
#define NXP_LPUART_DEBUG 0
#endif

#define DB_PRINT_L(lvl, fmt, args...)               \
    do                                              \
    {                                               \
        if (NXP_LPUART_DEBUG >= lvl)                \
        {                                           \
            qemu_log("%s: " fmt, __func__, ##args); \
        }                                           \
    } while (0)

#define DB_PRINT(fmt, args...) DB_PRINT_L(1, fmt, ##args)
#define DB_PRINT_READ(fmt, args...) DB_PRINT_L(2, fmt, ##args)

static uint32_t nxps32k358_lpuart_calculate_baud_rate(NXPS32K358LPUARTState *s)
{
    uint32_t sbr;
    uint32_t osr_val_in_reg; // Value of OSR field read from register
    uint64_t lpuart_module_clk_freq;

    // 1. Extract SBR (Baud Rate Modulo Divisor)
    sbr = (s->baud_rate_config & LPUART_BAUD_SBR_MASK) >> LPUART_BAUD_SBR_SHIFT;

    // 2. Extract OSR (Over Sampling Ratio)
    osr_val_in_reg = (s->baud_rate_config & LPUART_BAUD_OSR_MASK) >> LPUART_BAUD_OSR_SHIFT;

    lpuart_module_clk_freq = clock_get_hz(s->clk);

    // 3. Calculate and return baud rate using direct formula (osr + 1)
    // WARNING: This does not consider that if osr_val_in_reg < 3, effective oversampling is 16x.
    uint64_t divisor = (uint64_t)(osr_val_in_reg + 1) * sbr;
    if (divisor == 0)
    {
        return 0;
    }

    return lpuart_module_clk_freq / divisor;
}
static void nxps32k358_lpuart_update_params(NXPS32K358LPUARTState *s)
{
    QEMUSerialSetParams ssp;
    ssp.speed = nxps32k358_lpuart_calculate_baud_rate(s);
    DB_PRINT("Baud rate: %d\n", ssp.speed);

    qemu_chr_fe_ioctl(&s->chr, CHR_IOCTL_SERIAL_SET_PARAMS, &ssp);
}

static void nxps32k358_lpuart_update_irq(NXPS32K358LPUARTState *s)
{
    uint32_t mask = s->lpuart_sr & s->lpuart_cr;

    if (mask &
        (LPUART_CTRL_TIE | LPUART_CTRL_TCIE | LPUART_CTRL_RIE))
    {
        qemu_set_irq(s->irq, 1);
    }
    else
    {
        qemu_set_irq(s->irq, 0);
    }
}

// Function called when QEMU can send a character to the guest
static int nxps32k358_lpuart_can_receive(void *opaque)
{
    NXPS32K358LPUARTState *s = NXPS32K358_LPUART(opaque);

    if (s->lpuart_sr & LPUART_STAT_RDRF)
    {
        return 0; // Cannot receive
    }
    return 1; // Can receive
}

// Function called when QEMU has a character to send to the guest
static void nxps32k358_lpuart_receive(void *opaque, const uint8_t *buf, int size)
{
    NXPS32K358LPUARTState *s = NXPS32K358_LPUART(opaque);

    // return when the size is 0(so no data to be sent) or when the RE is not enabled
    if (!(s->lpuart_cr & LPUART_CTRL_RE) || size == 0)
    {
        DB_PRINT("Dropping the chars, read is disabled\n");

        return;
    }
    s->lpuart_dr = *buf;
    s->lpuart_sr |= LPUART_STAT_RDRF; // set the status register on the flag receive data register full

    // at the end need to be done to send the Interrupt :)
    nxps32k358_lpuart_update_irq(s);
    DB_PRINT("Receiving: %c\n", s->lpuart_dr);
}

static void nxps32k358_lpuart_reset(DeviceState *dev)
{
    NXPS32K358LPUARTState *s = NXPS32K358_LPUART(dev);

    s->lpuart_cr = LPUART_CONTROL_RESET; // Reset value from manual
    s->lpuart_sr = LPUART_STAT_RESET;    // TDRE is usually 1 at reset, TBD
    s->lpuart_dr = LPUART_DATA_RESET;
    s->lpuart_gb = LPUART_GLOBAL_RESET;

    s->baud_rate_config = LPUART_BAUD_RESET; // Reset value from manual (example)

    nxps32k358_lpuart_update_irq(s);
}

// hwaddr is called offset because it the offset to access to a given register. In this case is possible because MemoryRegionOps is
// the offset relative to that memory region defined. Opaque will define the lpuart

static uint64_t nxps32k358_lpuart_read(void *opaque, hwaddr offset, unsigned size)
{
    NXPS32K358LPUARTState *s = NXPS32K358_LPUART(opaque);
    DB_PRINT_READ("Read 0x%" HWADDR_PRIx "\n", offset);

    // qemu_log_mask(LOG_GUEST_ERROR, "LPUART read offset=0x%02x size=%u\n", (int)offset, size);

    switch (offset)
    {
    case LPUART_GLOBAL:
        return s->lpuart_gb;
    case LPUART_BAUD:
        return s->baud_rate_config;
    case LPUART_STAT:
        return s->lpuart_sr;
    case LPUART_CTRL:
        return s->lpuart_cr;

    case LPUART_DATA:
        DB_PRINT_READ("Value: 0x%" PRIx32 ", %c\n", s->lpuart_dr,
                      (char)s->lpuart_dr);

        s->lpuart_sr &= ~LPUART_STAT_RDRF;
        qemu_chr_fe_accept_input(&s->chr);
        // Reading DATA often clears RDRF and associated interrupt
        nxps32k358_lpuart_update_irq(s);
        return s->lpuart_dr;
    default:
        qemu_log_mask(LOG_GUEST_ERROR, "NXP S32K3 LPUART: Unimplemented read offset 0x%" HWADDR_PRIx "\n", offset);
        return 0;
    }

    return 0;
}

static void nxps32k358_lpuart_write(void *opaque, hwaddr offset, uint64_t val64, unsigned size)
{
    NXPS32K358LPUARTState *s = NXPS32K358_LPUART(opaque);
    uint32_t value = val64;
    unsigned char ch;
    // qemu_log_mask(LOG_GUEST_ERROR, "LPUART write offset=0x%02x val=0x%08x size=%u\n", (int)offset, (uint32_t)value, size);

    switch (offset)
    {
    case LPUART_GLOBAL:
        s->lpuart_gb = value;
        if (value & LPUART_GLOBAL_RST_MASK)
        {
            nxps32k358_lpuart_reset(DEVICE(s));
        }
        return;
    case LPUART_BAUD:
        s->baud_rate_config = value;
        nxps32k358_lpuart_update_params(s);
        return;
    case LPUART_STAT:
        if (value <= 0x3FF)
        {
            /* I/O being synchronous, TXE is always set. In addition, it may
            only be set by hardware, so keep it set here. */
            s->lpuart_sr = value | LPUART_STAT_TDRE;
        }
        else
            s->lpuart_sr &= value;
        return;
    case LPUART_CTRL:
        s->lpuart_cr = value;
        nxps32k358_lpuart_update_irq(s);

        // qemu_log_mask(LOG_GUEST_ERROR, "LPUART CTRL set to 0x%08x\n", (uint32_t)value);
        return;
    case LPUART_DATA:
        if (value < 0xF000)
        {
            ch = value;
            /* XXX this blocks entire thread. Rewrite to use
             * qemu_chr_fe_write and background I/O callbacks */
            qemu_chr_fe_write_all(&s->chr, &ch, 1);
            /* XXX I/O are currently synchronous, making it impossible for
               software to observe transient states where TXE or TC aren't
               set. Unlike TXE however, which is read-only, software may
               clear TC by writing 0 to the SR register, so set it again
               on each write. */
            s->lpuart_sr |= LPUART_STAT_TDRE;
            nxps32k358_lpuart_update_irq(s);
        }
        return;
    default:
        qemu_log_mask(LOG_GUEST_ERROR, "%s: NXP S32K3 LPUART: Unimplemented write to offset 0x%" HWADDR_PRIx " with value 0x%" PRIx32 "\n",
                      __func__, offset, value);
    }
}

// FINAL FUNCTION FOR EACH VIRTUALIZED DEVICE
static const MemoryRegionOps nxps32k358_lpuart_ops = {
    .read = nxps32k358_lpuart_read,
    .write = nxps32k358_lpuart_write,
    .endianness = DEVICE_NATIVE_ENDIAN, // Verify S32K3 peripherals endianness
};

static const Property nxps32k358_lpuart_properties[] = {
    DEFINE_PROP_CHR("chardev", NXPS32K358LPUARTState, chr),
};

static void nxps32k358_lpuart_init(Object *obj)
{
    NXPS32K358LPUARTState *s = NXPS32K358_LPUART(obj);

    // Initialize the IRQ line
    sysbus_init_irq(SYS_BUS_DEVICE(obj), &s->irq);

    // Initialize the MemoryRegion for LPUART registers
    // The size (e.g. 0x1000 or 4KB) must cover all LPUART registers
    memory_region_init_io(&s->iomem, obj, &nxps32k358_lpuart_ops, s,
                          "nxps32k358-lpuart", 0x4000); // Example size

    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->iomem);
    s->clk = qdev_init_clock_in(DEVICE(s), "clk", NULL, s, 0);

    // Initialize the character backend
    // qdev_prop_set_chr(DEVICE(obj), "chardev", qemu_chr_fe_get_driver(&s->chr)); // Allows specifying -serial ...
    // or another chardev
}

static void nxps32k358_lpuart_realize(DeviceState *dev, Error **errp)
{
    NXPS32K358LPUARTState *s = NXPS32K358_LPUART(dev);
    if (!clock_has_source(s->clk))
    {
        error_setg(errp, "LPUART clock must be wired up by SoC code");
        return;
    }
    // Connect callback functions for receiving characters
    // from QEMU host to emulated device.
    qemu_chr_fe_set_handlers(&s->chr, nxps32k358_lpuart_can_receive,
                             nxps32k358_lpuart_receive, NULL, NULL, s, NULL, true);

    // Here no need to connect clock because clock is passed from SoC
    // at connection time in nxps32k358_soc_realize
    // qdev_connect_clock_in(dev, "clk", some_clock_source);
}

static void nxps32k358_lpuart_class_init(ObjectClass *klass, const void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    device_class_set_legacy_reset(dc, nxps32k358_lpuart_reset);
    device_class_set_props(dc, nxps32k358_lpuart_properties);
    dc->realize = nxps32k358_lpuart_realize;
}

static const TypeInfo nxps32k358_lpuart_info = {
    .name = TYPE_NXPS32K358_LPUART,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(NXPS32K358LPUARTState),
    .instance_init = nxps32k358_lpuart_init,
    .class_init = nxps32k358_lpuart_class_init,
};

static void nxps32k358_lpuart_register_types(void)
{
    type_register_static(&nxps32k358_lpuart_info);
}

type_init(nxps32k358_lpuart_register_types);

#ifndef HW_NXP_S32K358_LPSPI_H
#define HW_NXP_S32K358_LPSPI_H

#include "hw/sysbus.h"
#include "qemu/fifo8.h"
#include "qom/object.h"
#include "hw/ssi/ssi.h"
#include "hw/qdev-clock.h"

#define TYPE_NXPS32K358_LPSPI "nxps32k358-lpspi"
OBJECT_DECLARE_SIMPLE_TYPE(NXPS32K358LPSPIState, NXPS32K358_LPSPI)

// Register offsets for the LPSPI (Low Power Serial Peripheral Interface)
#define S32K_LPSPI_VERID 0x00
#define S32K_LPSPI_PARAM 0x04
#define S32K_LPSPI_CR 0x10
#define S32K_LPSPI_SR 0x14
#define S32K_LPSPI_IER 0x18
#define S32K_LPSPI_DER 0x1C
#define S32K_LPSPI_CFGR0 0x20
#define S32K_LPSPI_CFGR1 0x24
#define S32K_LPSPI_CCR 0x40
#define S32K_LPSPI_FCR 0x58
#define S32K_LPSPI_FSR 0x5C
#define S32K_LPSPI_TCR 0x60
#define S32K_LPSPI_TDR 0x64
#define S32K_LPSPI_RSR 0x70
#define S32K_LPSPI_RDR 0x74
#define S32K_LPSPI_REG_MAX_OFFSET 0x78

// Bitmask definitions for the LPSPI registers
#define LPSPI_CR_MEN (1U << 0)
#define LPSPI_CR_RST (1U << 1)
#define LPSPI_CR_RSTF (1U << 9)

// Status Register (SR) bitmasks
#define LPSPI_SR_TDF (1U << 0)
#define LPSPI_SR_RDF (1U << 1)
#define LPSPI_SR_MBF (1U << 24)

// Receive Status Register (RSR) bitmasks
#define LPSPI_RSR_RXEMPTY (1U << 1)

// Transmit Command Register (TCR) bits and masks
#define TCR_PCS_SHIFT 24
#define TCR_PCS_MASK (0x3 << TCR_PCS_SHIFT)

// FIFO depth and capacity definitions
#define LPSPI_FIFO_WORD_DEPTH 4
#define LPSPI_FIFO_BYTE_CAPACITY (LPSPI_FIFO_WORD_DEPTH * 4)

struct NXPS32K358LPSPIState
{
    SysBusDevice parent_obj;
    Clock *clk;

    MemoryRegion mmio;
    SSIBus *ssi;
    qemu_irq irq;

    uint8_t num_cs_lines;
    qemu_irq *cs_lines;

    // FIFO software
    Fifo8 tx_fifo;
    Fifo8 rx_fifo;

    uint32_t lpspi_verid;
    uint32_t lpspi_param;
    uint32_t lpspi_cr;
    uint32_t lpspi_sr;
    uint32_t lpspi_ier;
    uint32_t lpspi_der;
    uint32_t lpspi_cfgr0;
    uint32_t lpspi_cfgr1;
    uint32_t lpspi_ccr;
    uint32_t lpspi_ccr1;

    uint32_t lpspi_fcr;
    uint32_t lpspi_fsr;
    uint32_t lpspi_tcr;
    uint32_t lpspi_tdr;
    uint32_t lpspi_rsr;
    uint32_t lpspi_rdr;
};

#endif // HW_NXP_S32K358_LPSPI_H
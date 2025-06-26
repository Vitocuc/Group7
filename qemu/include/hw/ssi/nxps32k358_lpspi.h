#ifndef HW_NXP_S32K358_LPSPI_H
#define HW_NXP_S32K358_LPSPI_H

#include "hw/sysbus.h"
#include "qemu/fifo8.h"
#include "qom/object.h"
#include "hw/ssi/ssi.h"
#include "hw/qdev-clock.h"

#define TYPE_NXPS32K358_LPSPI "nxps32k358-lpspi"
OBJECT_DECLARE_SIMPLE_TYPE(NXPS32K358LPSPIState, NXPS32K358_LPSPI)

// Register definitions
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

// Bitmask definitions

// Control Register (CR) bits
#define LPSPI_CR_MEN (1U << 0)
#define LPSPI_CR_RST (1U << 1)
#define LPSPI_CR_DOZEN (1U << 2)
#define LPSPI_CR_DBGEN (1U << 3)
#define LPSPI_CR_RTF (1U << 8)
#define LPSPI_CR_RRF (1U << 9)
#define LPSPI_CR_RSTF (1U << 8) // Fixed bit position

// Status Register (SR) bits
#define LPSPI_SR_TDF (1U << 0)
#define LPSPI_SR_RDF (1U << 1)
#define LPSPI_SR_WCF (1U << 8)
#define LPSPI_SR_FCF (1U << 9)
#define LPSPI_SR_TCF (1U << 10)
#define LPSPI_SR_TEF (1U << 11)
#define LPSPI_SR_REF (1U << 12)
#define LPSPI_SR_DMF (1U << 13)
#define LPSPI_SR_MBF (1U << 24)

// Receive Status Register (RSR) bits
#define LPSPI_RSR_SOF (1U << 0)
#define LPSPI_RSR_RXEMPTY (1U << 1)

// Configuration Register 1 (CFGR1) bits
#define LPSPI_CFGR1_MASTER (1U << 0)
#define LPSPI_CFGR1_SAMPLE (1U << 1)
#define LPSPI_CFGR1_AUTOPCS (1U << 2)
#define LPSPI_CFGR1_NOSTALL (1U << 3)
#define LPSPI_CFGR1_PCSPOL_SHIFT 8
#define LPSPI_CFGR1_PCSPOL_MASK (0xF << LPSPI_CFGR1_PCSPOL_SHIFT)
#define LPSPI_CFGR1_MATCFG_SHIFT 16
#define LPSPI_CFGR1_MATCFG_MASK (0x7 << LPSPI_CFGR1_MATCFG_SHIFT)
#define LPSPI_CFGR1_PINCFG_SHIFT 24
#define LPSPI_CFGR1_PINCFG_MASK (0x3 << LPSPI_CFGR1_PINCFG_SHIFT)

// FIFO Control Register (FCR) bits
#define LPSPI_FCR_TXWATER_SHIFT 0
#define LPSPI_FCR_TXWATER_MASK (0x3 << LPSPI_FCR_TXWATER_SHIFT)
#define LPSPI_FCR_RXWATER_SHIFT 16
#define LPSPI_FCR_RXWATER_MASK (0x3 << LPSPI_FCR_RXWATER_SHIFT)

// Transmit Command Register (TCR) bits
#define TCR_FRAMESZ_SHIFT 0
#define TCR_FRAMESZ_MASK (0xFFF << TCR_FRAMESZ_SHIFT)
#define TCR_WIDTH_SHIFT 16
#define TCR_WIDTH_MASK (0x3 << TCR_WIDTH_SHIFT)
#define TCR_TXMSK (1U << 18)
#define TCR_RXMSK (1U << 19)
#define TCR_CONTC (1U << 20)
#define TCR_CONT (1U << 21)
#define TCR_BYSW (1U << 22)
#define TCR_LSBF (1U << 23)
#define TCR_PCS_SHIFT 24
#define TCR_PCS_MASK (0x3 << TCR_PCS_SHIFT)
#define TCR_PRESCALE_SHIFT 27
#define TCR_PRESCALE_MASK (0x7 << TCR_PRESCALE_SHIFT)
#define TCR_CPHA (1U << 30)
#define TCR_CPOL (1U << 31)

#define LPSPI_FIFO_WORD_DEPTH 4
#define LPSPI_FIFO_BYTE_CAPACITY (LPSPI_FIFO_WORD_DEPTH * 4) // 16 bytes total - CORRECT for S32K358

struct NXPS32K358LPSPIState
{
    SysBusDevice parent_obj;

    MemoryRegion mmio;

    Clock *clk;
    uint64_t input_clk;

    qemu_irq irq;
    SSIBus *ssi;

    qemu_irq *cs_lines;
    uint8_t num_cs_lines;

    bool busy;
    uint8_t tx_watermark;
    uint8_t rx_watermark;
    uint16_t frame_size;  // Current frame size in bits
    bool continuous_mode; // Continuous transfer mode

    Fifo8 tx_fifo;
    Fifo8 rx_fifo;

    // LPSPI registers
    uint32_t lpspi_verid;
    uint32_t lpspi_param;
    uint32_t lpspi_cr;
    uint32_t lpspi_sr;
    uint32_t lpspi_ier;
    uint32_t lpspi_der;
    uint32_t lpspi_cfgr0;
    uint32_t lpspi_cfgr1;
    uint32_t lpspi_ccr;
    uint32_t lpspi_fcr;
    uint32_t lpspi_fsr;
    uint32_t lpspi_tcr;
    uint32_t lpspi_tdr;
    uint32_t lpspi_rsr;
    uint32_t lpspi_rdr;
};

#endif // HW_NXP_S32K358_LPSPI_H
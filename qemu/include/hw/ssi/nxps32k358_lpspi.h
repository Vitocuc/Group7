#ifndef HW_NXP_S32K358_LPSPI_H
#define HW_NXP_S32K358_LPSPI_H

#include "hw/sysbus.h"
#include "qemu/fifo8.h"
#include "qom/object.h"
#include "hw/ssi/ssi.h"
#include "hw/qdev-clock.h"

#define TYPE_NXPS32K358_LPSPI "nxps32k358-lpspi"
OBJECT_DECLARE_SIMPLE_TYPE(NXPS32K358LPSPIState, NXPS32K358_LPSPI)

//==============================================================================
// DEFINIZIONI DEI REGISTRI (Le tue definizioni originali)
//==============================================================================
#define S32K_LPSPI_VERID 0x00
#define S32K_LPSPI_PARAM 0x04
#define S32K_LPSPI_CR 0x10
#define S32K_LPSPI_SR 0x14
#define S32K_LPSPI_IER 0x18
#define S32K_LPSPI_DER 0x1C
#define S32K_LPSPI_CFGR0 0x20
#define S32K_LPSPI_CFGR1 0x24
#define S32K_LPSPI_CCR 0x40
#define S32K_LPSPI_CCR1 0x44
#define S32K_LPSPI_FCR 0x58
#define S32K_LPSPI_FSR 0x5C
#define S32K_LPSPI_TCR 0x60
#define S32K_LPSPI_TDR 0x64
#define S32K_LPSPI_RSR 0x70
#define S32K_LPSPI_RDR 0x74

//==============================================================================
// DEFINIZIONI DELLE MASCHERE DI BIT (Combinazione di esistenti e mancanti)
//==============================================================================

/* Control Register (CR) */
#define LPSPI_CR_MEN  (1U << 0)
#define LPSPI_CR_RST  (1U << 1)
#define LPSPI_CR_RTF  (1U << 8)
#define LPSPI_CR_RRF  (1U << 9)
// NOTA: Il tuo file aveva LPSPI_CR_RSTF, ma il .c usa RTF e RRF. Ho usato questi.

/* Status Register (SR) */
#define LPSPI_SR_TDF  (1U << 0)
#define LPSPI_SR_RDF  (1U << 1)
#define LPSPI_SR_WCF  (1U << 8)
#define LPSPI_SR_FCF  (1U << 9)
#define LPSPI_SR_TCF  (1U << 10)
#define LPSPI_SR_TEF  (1U << 11)
#define LPSPI_SR_REF  (1U << 12)
#define LPSPI_SR_DMF  (1U << 13)
#define LPSPI_SR_MBF  (1U << 24)

/* Receive Status Register (RSR) */
#define LPSPI_RSR_RXEMPTY (1U << 1)

/* Transmit Command Register (TCR) */
#define TCR_FRAMESZ_SHIFT 0
#define TCR_FRAMESZ_MASK  (0xFFF << TCR_FRAMESZ_SHIFT)
#define TCR_PCS_SHIFT     24
#define TCR_PCS_MASK      (0x3 << TCR_PCS_SHIFT)
#define TCR_TXMSK         (1U << 23)
#define TCR_RXMSK         (1U << 22)
#define TCR_LSBF          (1U << 28)
#define TCR_CONT          (1U << 30)
#define TCR_CONTC         (1U << 31)

/* FIFO Status Register (FSR) */
#define FSR_TXCOUNT_SHIFT 0
#define FSR_RXCOUNT_SHIFT 16

/* FIFO Control Register (FCR) */
#define LPSPI_FCR_TXWATER_SHIFT 0
#define LPSPI_FCR_TXWATER_MASK  (0x3 << LPSPI_FCR_TXWATER_SHIFT)
#define LPSPI_FCR_RXWATER_SHIFT 16
#define LPSPI_FCR_RXWATER_MASK  (0x3 << LPSPI_FCR_RXWATER_SHIFT)

/* Config Register 1 (CFGR1) */
#define LPSPI_CFGR1_PINCFG_SHIFT 8
#define LPSPI_CFGR1_PINCFG_MASK  (0x3 << LPSPI_CFGR1_PINCFG_SHIFT)
#define LPSPI_CFGR1_PCSPOL_SHIFT 24 // Unificato il nome con prefisso LPSPI_

// FIFO depth and capacity definitions
#define LPSPI_FIFO_WORD_DEPTH 4
#define LPSPI_FIFO_BYTE_CAPACITY (LPSPI_FIFO_WORD_DEPTH * 4)

//==============================================================================
// STRUTTURA DI STATO COMPLETA
//==============================================================================

struct NXPS32K358LPSPIState
{
    SysBusDevice parent_obj;
    Clock *clk;

    MemoryRegion mmio;
    SSIBus *ssi;
    qemu_irq irq;
    qemu_irq *cs_lines;

    // FIFO software
    Fifo8 tx_fifo;
    Fifo8 rx_fifo;

    // Registri
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

    /* --- MEMBRI DI STATO AGGIUNTI --- */
    bool busy;
    uint8_t tx_watermark;
    uint8_t rx_watermark;
    uint16_t frame_size;
    bool continuous_mode;
    uint64_t input_clk;
    uint8_t num_cs_lines;
};

#endif // HW_NXP_S32K358_LPSPI_H
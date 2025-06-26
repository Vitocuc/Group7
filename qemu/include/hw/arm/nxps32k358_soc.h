#ifndef HW_ARM_NXPS32K358_SOC_H
#define HW_ARM_NXPS32K358_SOC_H

//#include "hw/misc/stm32f2xx_syscfg.h"
#include "hw/char/nxps32k358_lpuart.h"
#include "hw/or-irq.h"
#include "hw/ssi/nxps32k358_lpspi.h"
#include "hw/arm/armv7m.h"
#include "hw/clock.h"
#include "qom/object.h"
#include "hw/misc/nxps32k358_syscfg.h"


#define TYPE_NXPS32K358_SOC "nxps32k358-soc"
OBJECT_DECLARE_SIMPLE_TYPE(NXPS32K358State, NXPS32K358_SOC)
#define NXP_NUM_LPUARTS 16
#define NXP_NUM_LPSPIS 6  // Add this line - S32K358 has 6 LPSPI instances

#define CODE_FLASH_BASE_ADDRESS 0x00400000
#define CODE_FLASH_BLOCK_SIZE (2 * 1024 * 1024)
#define DATA_FLASH_BASE_ADDRESS 0x10000000
#define DATA_FLASH_SIZE (128 * 1024)
#define SRAM_BASE_ADDRESS 0x20400000
#define SRAM_BLOCK_SIZE (256 * 1024)
#define SRAM_BLOCK_SIZE (256 * 1024)
#define DTCM_BASE_ADDRESS 0x20000000
#define DTCM_SIZE (128 * 1024) + 1
#define ITCM_BASE_ADDRESS 0x00000000
#define ITCM_SIZE (64 * 1024)




struct NXPS32K358State {
    SysBusDevice parent_obj;

    ARMv7MState armv7m;

    NXPS32K358SYSCFGState syscfg;
    NXPS32K358LPUARTState lpuarts[NXP_NUM_LPUARTS];
    NXPS32K358LPSPIState lpspis[NXP_NUM_LPSPIS];

    OrIRQState *adc_irqs;

    MemoryRegion code_flash_0;
    MemoryRegion code_flash_1;
    MemoryRegion code_flash_2;
    MemoryRegion code_flash_3;

    MemoryRegion data_flash;

    MemoryRegion sram_0;
    MemoryRegion sram_1;
    MemoryRegion sram_2;

    MemoryRegion dtcm;
    MemoryRegion itcm;
    Clock *sysclk;
    Clock *refclk;

    Clock *aips_plat_clk; 
    Clock *aips_slow_clk; 
};

#endif
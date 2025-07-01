
#include "qemu/osdep.h"
#include "qapi/error.h"
#include "qemu/module.h"
#include "hw/arm/boot.h"

#include "system/address-spaces.h"

// -- Project dependant: LOCATED ON /INCLUDE/HW/ARM
#include "hw/arm/nxps32k358_soc.h"

#include "hw/qdev-properties.h"
#include "hw/qdev-clock.h"
#include "hw/misc/unimp.h"
#include "system/system.h"

#include "hw/ssi/motor_speed.h"

/* stm32f100_soc implementation is derived from stm32f205_soc */

// // The variables represent addresses on our nxp_s32k and need to be changed(is also present the number of pins that we have for usart and spi)
static const uint32_t lpuart_addr[NXP_NUM_LPUARTS] = {0x40328000, 0x4032C000, 0x40330000, 0x40334000, 0x40338000, 0x4033C000, 0x40340000, 0x40344000, 0x4048C000, 0x40490000, 0x40494000, 0x40498000, 0x4049C000, 0x404A0000, 0x404A4000, 0x404A8000};
static const uint32_t lpspi_addr[NXP_NUM_LPSPIS] = {0x40358000, 0x4035C000, 0x40360000, 0x40364000, 0x404BC000, 0x404C0000};

static const int lpuart_irq[NXP_NUM_LPUARTS] = {141, 142, 143, 144, 145, 146, 147};
static const int lpspi_irq[NXP_NUM_LPSPIS] = {165, 166, 167, 168, 169, 170};

// -------------------------------------

static void create_unimplemented_devices(void)
{

    create_unimplemented_device("hse_xbic", 0x40008000, 0x4000);
    create_unimplemented_device("erm1", 0x4000C000, 0x4000);
    create_unimplemented_device("pfc1", 0x40068000, 0x4000);
    create_unimplemented_device("pfc1_alt", 0x4006C000, 0x4000);
    create_unimplemented_device("swt_3", 0x40070000, 0x4000);
    create_unimplemented_device("trgmux", 0x40080000, 0x4000);
    create_unimplemented_device("bctu", 0x40084000, 0x4000);
    create_unimplemented_device("emios0", 0x40088000, 0x4000);
    create_unimplemented_device("emios1", 0x4008C000, 0x4000);
    create_unimplemented_device("emios2", 0x40090000, 0x4000);
    create_unimplemented_device("lcu0", 0x40098000, 0x4000);
    create_unimplemented_device("lcu1", 0x4009C000, 0x4000);
    create_unimplemented_device("adc_0", 0x400A0000, 0x4000);
    create_unimplemented_device("adc_1", 0x400A4000, 0x4000);
    create_unimplemented_device("adc_2", 0x400A8000, 0x4000);
    create_unimplemented_device("pit0", 0x400B0000, 0x4000);
    create_unimplemented_device("pit1", 0x400B4000, 0x4000);
    create_unimplemented_device("mu_2_mua", 0x400B8000, 0x4000); // Note: CSV had two lines for MU_2, distinguished as MUA/MUB in description
    create_unimplemented_device("mu_2_mub", 0x400BC000, 0x4000); // Used _mua/_mub to distinguish them in name
    create_unimplemented_device("mu_3_mua", 0x400C4000, 0x4000); // Same as above for MU_3
    create_unimplemented_device("mu_3_mub", 0x400C8000, 0x4000);
    create_unimplemented_device("mu_4_mua", 0x400CC000, 0x4000); // Same as above for MU_4
    create_unimplemented_device("mu_4_mub", 0x400D0000, 0x4000);
    create_unimplemented_device("axbs", 0x40200000, 0x4000);
    create_unimplemented_device("system_xbic", 0x40204000, 0x4000);
    create_unimplemented_device("periph_xbic", 0x40208000, 0x4000);
    create_unimplemented_device("edma", 0x4020C000, 0x4000);
    create_unimplemented_device("edma_tcd_0", 0x40210000, 0x4000);
    create_unimplemented_device("edma_tcd_1", 0x40214000, 0x4000);
    create_unimplemented_device("edma_tcd_2", 0x40218000, 0x4000);
    create_unimplemented_device("edma_tcd_3", 0x4021C000, 0x4000);
    create_unimplemented_device("edma_tcd_4", 0x40220000, 0x4000);
    create_unimplemented_device("edma_tcd_5", 0x40224000, 0x4000);
    create_unimplemented_device("edma_tcd_6", 0x40228000, 0x4000);
    create_unimplemented_device("edma_tcd_7", 0x4022C000, 0x4000);
    create_unimplemented_device("edma_tcd_8", 0x40230000, 0x4000);
    create_unimplemented_device("edma_tcd_9", 0x40234000, 0x4000);
    create_unimplemented_device("edma_tcd_10", 0x40238000, 0x4000);
    create_unimplemented_device("edma_tcd_11", 0x4023C000, 0x4000);
    create_unimplemented_device("debug_apb_page0", 0x40240000, 0x4000);
    create_unimplemented_device("debug_apb_page1", 0x40244000, 0x4000);
    create_unimplemented_device("debug_apb_page2", 0x40248000, 0x4000);
    create_unimplemented_device("debug_apb_page3", 0x4024C000, 0x4000);
    create_unimplemented_device("debug_apb_paged_area", 0x40250000, 0x4000);
    create_unimplemented_device("sda-ap", 0x40254000, 0x4000);
    create_unimplemented_device("eim0", 0x40258000, 0x4000);
    create_unimplemented_device("erm0", 0x4025C000, 0x4000);
    create_unimplemented_device("mscm", 0x40260000, 0x4000);
    create_unimplemented_device("pram_0", 0x40264000, 0x4000);
    create_unimplemented_device("pfc", 0x40268000, 0x4000);
    create_unimplemented_device("pfc_alt", 0x4026C000, 0x4000);
    create_unimplemented_device("swt_0", 0x40270000, 0x4000);
    create_unimplemented_device("stm_0", 0x40274000, 0x4000);
    create_unimplemented_device("xrdc", 0x40278000, 0x4000);
    create_unimplemented_device("intm", 0x4027C000, 0x4000);
    create_unimplemented_device("dmamux_0", 0x40280000, 0x4000);
    create_unimplemented_device("dmamux_1", 0x40284000, 0x4000);
    create_unimplemented_device("rtc", 0x40288000, 0x4000);
    create_unimplemented_device("mc_rgm", 0x4028C000, 0x4000);
    create_unimplemented_device("siul_virtwrapper_pdac0_hse", 0x40290000, 0x4000); // Long name, could be abbreviated if preferred
    // create_unimplemented_device("siul_virtwrapper_pdac0_hse_alt", 0x40294000, 0x4000); // Duplicate address in name, using _alt
    create_unimplemented_device("siul_virtwrapper_pdac1_m7_0", 0x40298000, 0x4000);
    // create_unimplemented_device("siul_virtwrapper_pdac1_m7_0_alt", 0x4029C000, 0x4000); // Duplicate address in name, using _alt
    create_unimplemented_device("siul_virtwrapper_pdac2_m7_1", 0x402A0000, 0x4000);
    // create_unimplemented_device("siul_virtwrapper_pdac2_m7_1_alt", 0x402A4000, 0x4000); // Duplicate address in name, using _alt
    create_unimplemented_device("siul_virtwrapper_pdac3", 0x402A8000, 0x4000);
    create_unimplemented_device("dcm", 0x402AC000, 0x4000);
    create_unimplemented_device("wkpu", 0x402B4000, 0x4000);
    create_unimplemented_device("cmu", 0x402BC000, 0x4000);
    create_unimplemented_device("tspc", 0x402C4000, 0x4000);
    create_unimplemented_device("sirc", 0x402C8000, 0x4000);
    create_unimplemented_device("sxosc", 0x402CC000, 0x4000);
    create_unimplemented_device("firc", 0x402D0000, 0x4000);
    create_unimplemented_device("fxosc", 0x402D4000, 0x4000);
    create_unimplemented_device("mc_cgm", 0x402D8000, 0x4000);
    create_unimplemented_device("mc_me", 0x402DC000, 0x4000); // Already handled separately in SoC code, but present in list
    create_unimplemented_device("pll", 0x402E0000, 0x4000);
    create_unimplemented_device("pll2", 0x402E4000, 0x4000);
    create_unimplemented_device("pmc", 0x402E8000, 0x4000);
    create_unimplemented_device("fmu", 0x402EC000, 0x4000);
    create_unimplemented_device("fmu_alt", 0x402F0000, 0x4000);
    create_unimplemented_device("siul_virtwrapper_pdac4_m7_2", 0x402F4000, 0x4000);
    // create_unimplemented_device("siul_virtwrapper_pdac4_m7_2_alt", 0x402F8000, 0x4000); // Duplicate address in name, using _alt
    create_unimplemented_device("pit2", 0x402FC000, 0x4000);
    create_unimplemented_device("pit3", 0x40300000, 0x4000);
    create_unimplemented_device("flexcan_0", 0x40304000, 0x4000);
    create_unimplemented_device("flexcan_1", 0x40308000, 0x4000);
    create_unimplemented_device("flexcan_2", 0x4030C000, 0x4000);
    create_unimplemented_device("flexcan_3", 0x40310000, 0x4000);
    create_unimplemented_device("flexcan_4", 0x40314000, 0x4000);
    create_unimplemented_device("flexcan_5", 0x40318000, 0x4000);
    create_unimplemented_device("flexcan_6", 0x4031C000, 0x4000);
    create_unimplemented_device("flexcan_7", 0x40320000, 0x4000);
    create_unimplemented_device("flexio", 0x40324000, 0x4000);
    create_unimplemented_device("lpuart_0", 0x40328000, 0x4000);
    create_unimplemented_device("lpuart_1", 0x4032C000, 0x4000);
    create_unimplemented_device("lpuart_2", 0x40330000, 0x4000);
    create_unimplemented_device("lpuart_3", 0x40334000, 0x4000);
    create_unimplemented_device("lpuart_4", 0x40338000, 0x4000);
    create_unimplemented_device("lpuart_5", 0x4033C000, 0x4000);
    create_unimplemented_device("lpuart_6", 0x40340000, 0x4000);
    create_unimplemented_device("lpuart_7", 0x40344000, 0x4000);
    create_unimplemented_device("siul_virtwrapper_pdac5_m7_3", 0x40348000, 0x4000);
    // create_unimplemented_device("siul_virtwrapper_pdac5_m7_3_alt", 0x4034C000, 0x4000); // Duplicate address in name, using _alt
    create_unimplemented_device("lpi2c_0", 0x40350000, 0x4000);
    create_unimplemented_device("lpi2c_1", 0x40354000, 0x4000);
    create_unimplemented_device("lpspi_0", 0x40358000, 0x4000);
    create_unimplemented_device("lpspi_1", 0x4035C000, 0x4000);
    create_unimplemented_device("lpspi_2", 0x40360000, 0x4000);
    create_unimplemented_device("lpspi_3", 0x40364000, 0x4000);
    create_unimplemented_device("sai0", 0x4036C000, 0x4000);
    create_unimplemented_device("lpcmp_0", 0x40370000, 0x4000);
    create_unimplemented_device("lpcmp_1", 0x40374000, 0x4000);
    create_unimplemented_device("tmu", 0x4037C000, 0x4000);
    create_unimplemented_device("crc", 0x40380000, 0x4000);
    create_unimplemented_device("fccu_", 0x40384000, 0x4000);    // Note: name ends with underscore in CSV
    create_unimplemented_device("mu_0_mub", 0x4038C000, 0x4000); // MU_0 exists only as MUB
    create_unimplemented_device("mu_1_mub", 0x40390000, 0x4000); // MU_1 exists only as MUB
    create_unimplemented_device("jdc", 0x40394000, 0x4000);
    create_unimplemented_device("configuration_gpr", 0x4039C000, 0x4000);
    create_unimplemented_device("stcu", 0x403A0000, 0x4000);
    create_unimplemented_device("selftest_gpr", 0x403B0000, 0x4000);
    create_unimplemented_device("aes_accel", 0x403C0000, 0x10000); // Size 64KB
    create_unimplemented_device("aes_app0", 0x403D0000, 0x10000);  // Size 64KB
    create_unimplemented_device("aes_app1", 0x403E0000, 0x10000);  // Size 64KB
    create_unimplemented_device("aes_app2", 0x403F0000, 0x10000);  // Size 64KB
    create_unimplemented_device("tcm_xbic", 0x40400000, 0x4000);
    create_unimplemented_device("edma_xbic", 0x40404000, 0x4000);
    create_unimplemented_device("pram2_tcm_xbic", 0x40408000, 0x4000);
    create_unimplemented_device("aes_mux_xbic", 0x4040C000, 0x4000);
    create_unimplemented_device("edma_tcd_12", 0x40410000, 0x4000);
    create_unimplemented_device("edma_tcd_13", 0x40414000, 0x4000);
    create_unimplemented_device("edma_tcd_14", 0x40418000, 0x4000);
    create_unimplemented_device("edma_tcd_15", 0x4041C000, 0x4000);
    create_unimplemented_device("edma_tcd_16", 0x40420000, 0x4000);
    create_unimplemented_device("edma_tcd_17", 0x40424000, 0x4000);
    create_unimplemented_device("edma_tcd_18", 0x40428000, 0x4000);
    create_unimplemented_device("edma_tcd_19", 0x4042C000, 0x4000);
    create_unimplemented_device("edma_tcd_20", 0x40430000, 0x4000);
    create_unimplemented_device("edma_tcd_21", 0x40434000, 0x4000);
    create_unimplemented_device("edma_tcd_22", 0x40438000, 0x4000);
    create_unimplemented_device("edma_tcd_23", 0x4043C000, 0x4000);
    create_unimplemented_device("edma_tcd_24", 0x40440000, 0x4000);
    create_unimplemented_device("edma_tcd_25", 0x40444000, 0x4000);
    create_unimplemented_device("edma_tcd_26", 0x40448000, 0x4000);
    create_unimplemented_device("edma_tcd_27", 0x4044C000, 0x4000);
    create_unimplemented_device("edma_tcd_28", 0x40450000, 0x4000);
    create_unimplemented_device("edma_tcd_29", 0x40454000, 0x4000);
    create_unimplemented_device("edma_tcd_30", 0x40458000, 0x4000);
    create_unimplemented_device("edma_tcd_31", 0x4045C000, 0x4000);
    create_unimplemented_device("sema42", 0x40460000, 0x4000);
    create_unimplemented_device("pram_1", 0x40464000, 0x4000);
    create_unimplemented_device("pram_2", 0x40468000, 0x4000);
    create_unimplemented_device("swt_1", 0x4046C000, 0x4000);
    create_unimplemented_device("swt_2", 0x40470000, 0x4000);
    create_unimplemented_device("stm_1", 0x40474000, 0x4000);
    create_unimplemented_device("stm_2", 0x40478000, 0x4000);
    create_unimplemented_device("stm_3", 0x4047C000, 0x4000);
    create_unimplemented_device("emac", 0x40480000, 0x4000);
    create_unimplemented_device("gmac0", 0x40484000, 0x4000);
    create_unimplemented_device("gmac1", 0x40488000, 0x4000);
    create_unimplemented_device("lpuart_8", 0x4048C000, 0x4000);
    create_unimplemented_device("lpuart_9", 0x40490000, 0x4000);
    create_unimplemented_device("lpuart_10", 0x40494000, 0x4000);
    create_unimplemented_device("lpuart_11", 0x40498000, 0x4000);
    create_unimplemented_device("lpuart_12", 0x4049C000, 0x4000);
    create_unimplemented_device("lpuart_13", 0x404A0000, 0x4000);
    create_unimplemented_device("lpuart_14", 0x404A4000, 0x4000);
    create_unimplemented_device("lpuart_15", 0x404A8000, 0x4000);
    create_unimplemented_device("lpspi_4", 0x404BC000, 0x4000);
    create_unimplemented_device("lpspi_5", 0x404C0000, 0x4000);
    create_unimplemented_device("quadspi", 0x404CC000, 0x4000);
    create_unimplemented_device("sai1", 0x404DC000, 0x4000);
    create_unimplemented_device("usdhc", 0x404E4000, 0x4000);
    create_unimplemented_device("lpcmp_2", 0x404E8000, 0x4000);
    // create_unimplemented_device("mu_1_mub_dup", 0x404EC000, 0x4000); // MU_1_MUB is duplicated here, commenting it
    create_unimplemented_device("eim0_dup", 0x4050C000, 0x4000); // Also EIM0 is duplicated, adding _dup
    create_unimplemented_device("eim1", 0x40510000, 0x4000);
    create_unimplemented_device("eim2", 0x40514000, 0x4000);
    create_unimplemented_device("eim3", 0x40518000, 0x4000);
    create_unimplemented_device("aes_app3", 0x40520000, 0x10000); // Size 64KB
    create_unimplemented_device("aes_app4", 0x40530000, 0x10000); // Size 64KB
    create_unimplemented_device("aes_app5", 0x40540000, 0x10000); // Size 64KB
    create_unimplemented_device("aes_app6", 0x40550000, 0x10000); // Size 64KB
    create_unimplemented_device("aes_app7", 0x40560000, 0x10000); // Size 64KB
    create_unimplemented_device("flexcan_8", 0x40570000, 0x4000);
    create_unimplemented_device("flexcan_9", 0x40574000, 0x4000);
    create_unimplemented_device("flexcan_10", 0x40578000, 0x4000);
    create_unimplemented_device("flexcan_11", 0x4057C000, 0x4000);
    create_unimplemented_device("fmu1", 0x40580000, 0x4000);
    create_unimplemented_device("fmu1_alt", 0x40584000, 0x4000);
    create_unimplemented_device("pram_3", 0x40588000, 0x4000);
}

// Definition of the soc class init
// in this function we will initialize the peripherals that i need to emulare
// So we have the ARM-M7- LPUART - SPI
//  * - Initializes additional clocks required for LPUARTs, aips_plat_clk and aips_slow_clk.

static void nxps32k358_soc_initfn(Object *obj)
{
    NXPS32K358State *s = NXPS32K358_SOC(obj);

    object_initialize_child(obj, "armv7m", &s->armv7m, TYPE_ARMV7M);
    object_initialize_child(obj, "syscfg", &s->syscfg, TYPE_NXPS32K358_SYSCFG);

    s->sysclk = qdev_init_clock_in(DEVICE(s), "sysclk", NULL, NULL, 0);
    s->refclk = qdev_init_clock_in(DEVICE(s), "refclk", NULL, NULL, 0);

    // intern bus used by microcontroller to check peripherals
    s->aips_plat_clk =
        qdev_init_clock_in(DEVICE(s), "aips_plat_clk", NULL, NULL, 0);
    s->aips_slow_clk =
        qdev_init_clock_in(DEVICE(s), "aips_slow_clk", NULL, NULL, 0);

    // NXP_NUM_LPUARTS AND NXP_NUM_SPIS ARE DEFINED IN .H
    for (int i = 0; i < NXP_NUM_LPUARTS; i++)
    {
        object_initialize_child(obj, "lpuart[*]", &s->lpuarts[i],
                                TYPE_NXPS32K358_LPUART);
    }

    for (int i = 0; i < NXP_NUM_LPSPIS; i++)
    {
        object_initialize_child(obj, "lpspi[*]", &s->lpspis[i], TYPE_NXPS32K358_LPSPI);
    }
}

static void nxps32k358_soc_realize(DeviceState *dev_soc, Error **errp)
{

    NXPS32K358State *s = NXPS32K358_SOC(dev_soc);
    DeviceState *dev, *armv7m;
    SysBusDevice *busdev;
    int i;

    MemoryRegion *system_memory = get_system_memory();

    /*
     * We use s->refclk internally and only define it with qdev_init_clock_in()
     * so it is correctly parented and not leaked on an init/deinit; it is not
     * intended as an externally exposed clock.
     */

    // Add fixed clock sources for AIPS clocks
    Clock *aips_plat_fixed = clock_new(OBJECT(s), "aips_plat_fixed");
    clock_set_hz(aips_plat_fixed, 80000000);
    clock_set_source(s->aips_plat_clk, aips_plat_fixed);

    Clock *aips_slow_fixed = clock_new(OBJECT(s), "aips_slow_fixed");
    clock_set_hz(aips_slow_fixed, 40000000);
    clock_set_source(s->aips_slow_clk, aips_slow_fixed);

    if (clock_has_source(s->refclk))
    {
        error_setg(errp, "refclk clock must not be wired up by the board code");
        return;
    }

    if (!clock_has_source(s->sysclk))
    {
        error_setg(errp, "sysclk clock must be wired up by the board code");
        return;
    }

    // set up the source and the frequency of the source
    /* The refclk always runs at frequency HCLK / 8 */
    clock_set_mul_div(s->refclk, 8, 1);
    clock_set_source(s->refclk, s->sysclk);
    // clock_set_hz(s->aips_plat_clk, 80000000);
    // clock_set_hz(s->aips_slow_clk, 40000000);

    // FLASH SIZE DEFINED IN .H

    // Set up the memory region for our board
    /*
     * Init code flash region
     */
    memory_region_init_rom(&s->code_flash_0, OBJECT(dev_soc),
                           "NXPS32K358.code_flash_0", CODE_FLASH_BLOCK_SIZE,
                           &error_fatal);
    memory_region_add_subregion(system_memory, CODE_FLASH_BASE_ADDRESS,
                                &s->code_flash_0);

    memory_region_init_rom(&s->code_flash_1, OBJECT(dev_soc),
                           "NXPS32K358.code_flash_1", CODE_FLASH_BLOCK_SIZE,
                           &error_fatal);
    memory_region_add_subregion(system_memory,
                                CODE_FLASH_BASE_ADDRESS + CODE_FLASH_BLOCK_SIZE,
                                &s->code_flash_1);

    memory_region_init_rom(&s->code_flash_2, OBJECT(dev_soc),
                           "NXPS32K358.code_flash_2", CODE_FLASH_BLOCK_SIZE,
                           &error_fatal);
    memory_region_add_subregion(
        system_memory, CODE_FLASH_BASE_ADDRESS + 2 * CODE_FLASH_BLOCK_SIZE,
        &s->code_flash_2);

    memory_region_init_rom(&s->code_flash_3, OBJECT(dev_soc),
                           "NXPS32K358.code_flash_3", CODE_FLASH_BLOCK_SIZE,
                           &error_fatal);
    memory_region_add_subregion(
        system_memory, CODE_FLASH_BASE_ADDRESS + 3 * CODE_FLASH_BLOCK_SIZE,
        &s->code_flash_3);

    /* Init data flash region */
    memory_region_init_rom(&s->data_flash, OBJECT(dev_soc),
                           "NXPS32K358.data_flash", DATA_FLASH_SIZE,
                           &error_fatal);
    memory_region_add_subregion(system_memory, DATA_FLASH_BASE_ADDRESS,
                                &s->data_flash);

    /* Init SRAM region */
    memory_region_init_ram(&s->sram_0, OBJECT(dev_soc), "NXPS32K358.sram_0",
                           SRAM_BLOCK_SIZE, &error_fatal);
    memory_region_add_subregion(system_memory, SRAM_BASE_ADDRESS, &s->sram_0);

    memory_region_init_ram(&s->sram_1, OBJECT(dev_soc), "NXPS32K358.sram_1",
                           SRAM_BLOCK_SIZE, &error_fatal);
    memory_region_add_subregion(
        system_memory, SRAM_BASE_ADDRESS + SRAM_BLOCK_SIZE, &s->sram_1);

    memory_region_init_ram(&s->sram_2, OBJECT(dev_soc), "NXPS32K358.sram_2",
                           SRAM_BLOCK_SIZE, &error_fatal);
    memory_region_add_subregion(
        system_memory, (SRAM_BASE_ADDRESS + (2 * SRAM_BLOCK_SIZE)), &s->sram_2);
    // ----------------------------------
    /* Init DTCM */
    memory_region_init_ram(&s->dtcm, OBJECT(dev_soc), "NXPS32K358.dtcm",
                           DTCM_SIZE, &error_fatal);
    memory_region_add_subregion(system_memory, DTCM_BASE_ADDRESS, &s->dtcm);

    /* Init ITCM */
    memory_region_init_ram(&s->itcm, OBJECT(dev_soc), "NXPS32K358.itcm",
                           ITCM_SIZE, &error_fatal);
    memory_region_add_subregion(system_memory, ITCM_BASE_ADDRESS, &s->itcm);

    // Set up the CPU -> CONNECTING TO PINS
    armv7m = DEVICE(&s->armv7m);
    qdev_prop_set_uint32(armv7m, "num-irq", 240); // defines the IRQ numbers
    qdev_prop_set_uint8(armv7m, "num-prio-bits", 4);
    qdev_prop_set_string(armv7m, "cpu-type", ARM_CPU_TYPE_NAME("cortex-m7"));
    qdev_prop_set_bit(armv7m, "enable-bitband", true);
    qdev_prop_set_uint32(armv7m, "init-svtor", CODE_FLASH_BASE_ADDRESS + 2048);
    qdev_prop_set_uint32(armv7m, "init-nsvtor", CODE_FLASH_BASE_ADDRESS + 2048);
    qdev_prop_set_uint32(armv7m, "mpu-ns-regions", 16);
    qdev_prop_set_uint32(armv7m, "mpu-s-regions", 16);
    qdev_connect_clock_in(armv7m, "cpuclk", s->sysclk);
    qdev_connect_clock_in(armv7m, "refclk", s->refclk);
    object_property_set_link(OBJECT(&s->armv7m), "memory",
                             OBJECT(get_system_memory()), &error_abort);

    if (!sysbus_realize(SYS_BUS_DEVICE(&s->armv7m), errp))
    {
        return;
    }

    // Set up the BUS
    /* System configuration controller */
    dev = DEVICE(&s->syscfg);
    qdev_connect_clock_in(dev, "clk", s->sysclk);
    if (!sysbus_realize(SYS_BUS_DEVICE(&s->syscfg), errp))
    {
        return;
    }
    busdev = SYS_BUS_DEVICE(dev);
    sysbus_mmio_map(busdev, 0, 0x40013800);

    // REALIZING LPUART registers and controllers
    for (i = 0; i < NXP_NUM_LPUARTS; i++)
    {
        dev = DEVICE(&(s->lpuarts[i]));
        qdev_prop_set_chr(dev, "chardev", serial_hd(i));
        // LPUART 0, 1 and 8 use AIPS_PLAT_CLK (MUX_0_DC_1)
        // LPUART 2 to 7 and 9 to 15 use AIPS_SLOW_CLK (MUX_0_DC_2)
        if (i < 2 || i == 8)
        {
            qdev_connect_clock_in(dev, "clk", s->aips_plat_clk);
        }
        else
        {
            qdev_connect_clock_in(dev, "clk", s->aips_slow_clk);
        }
        if (!sysbus_realize(SYS_BUS_DEVICE(&s->lpuarts[i]), errp))
        {
            return;
        }
        busdev = SYS_BUS_DEVICE(dev);
        sysbus_mmio_map(busdev, 0, lpuart_addr[i]);
        sysbus_connect_irq(busdev, 0, qdev_get_gpio_in(armv7m, lpuart_irq[i]));
    }

    // REALIZING LPSPI
    for (i = 0; i < NXP_NUM_LPSPIS; i++)
    {
        dev = DEVICE(&(s->lpspis[i]));

        qdev_connect_clock_in(dev, "clk", s->aips_plat_clk);

        if (!sysbus_realize(SYS_BUS_DEVICE(&s->lpspis[i]), errp))
        {
            return;
        }

        busdev = SYS_BUS_DEVICE(dev);
        sysbus_mmio_map(busdev, 0, lpspi_addr[i]);
        sysbus_connect_irq(busdev, 0, qdev_get_gpio_in(armv7m, lpspi_irq[i]));
    }

    for (i = 0; i < NXP_NUM_LPSPIS; i++)
    {
        ssi_create_peripheral(s->lpspis[i].ssi, TYPE_MOTOR_SPEED_SENSOR);
    }

    create_unimplemented_devices();
}

static void nxps32k358_soc_class_init(ObjectClass *klass, const void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = nxps32k358_soc_realize;
    /* No vmstate or reset required: device has no internal state */
}

static const TypeInfo nxps32k358_soc_info = {
    .name = TYPE_NXPS32K358_SOC,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(NXPS32K358State),
    .instance_init = nxps32k358_soc_initfn,
    .class_init = nxps32k358_soc_class_init,
};

static void nxps32k358_soc_types(void)
{
    type_register_static(&nxps32k358_soc_info);
}

type_init(nxps32k358_soc_types)
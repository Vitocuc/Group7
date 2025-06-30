/*==================================================================================================
*   Project              : RTD AUTOSAR 4.7
*   Platform             : CORTEXM
*   Peripheral           : LPSPI
*   Dependencies         : 
*
*   Autosar Version      : 4.7.0
*   Autosar Revision     : ASR_REL_4_7_REV_0000
*   Autosar Conf.Variant :
*   SW Version           : 3.0.0
*   Build Version        : S32K3_RTD_3_0_0_D2303_ASR_REL_4_7_REV_0000_20230331
*
*   Copyright 2020 - 2023 NXP Semiconductors NXP
*
*   NXP Confidential. This software is owned or controlled by NXP and may only be
*   used strictly in accordance with the applicable license terms. By expressly
*   accepting such terms or by downloading, installing, activating and/or otherwise
*   using the software, you are agreeing that you have read, and that you agree to
*   comply with and are bound by, such license terms. If you do not agree to be
*   bound by the applicable license terms, then you may not retain, install,
*   activate or otherwise use the software.
==================================================================================================*/

/**   
*   @file    Flexio_Spi_Ip_PBcfg.c
*   @version 3.0.0
*
*   @brief   AUTOSAR Spi - Post-Build(PB) configuration file code template.
*   @details Code template for Post-Build(PB) configuration file generation.
*
*   @addtogroup FLEXIO_SPI_IP_DRIVER_CONFIGURATION Flexio_Spi Driver Configuration
*   @{
*/

#ifdef __cplusplus
extern "C"
{
#endif


/*==================================================================================================
                                         INCLUDE FILES
 1) system and project includes
 2) needed interfaces from external units
 3) internal and external interfaces from this unit
==================================================================================================*/
#include "Flexio_Spi_Ip.h"

#if (FLEXIO_SPI_IP_ENABLE == STD_ON)
#if (FLEXIO_SPI_IP_DMA_USED == STD_ON)
#include "Dma_Ip.h"
#endif
#endif

/*==================================================================================================
*                              SOURCE FILE VERSION INFORMATION
==================================================================================================*/

#define FLEXIO_SPI_IP_SA_VENDOR_ID_PBCFG_C                        43
#define FLEXIO_SPI_IP_SA_AR_RELEASE_MAJOR_VERSION_PBCFG_C         4
#define FLEXIO_SPI_IP_SA_AR_RELEASE_MINOR_VERSION_PBCFG_C         7
#define FLEXIO_SPI_IP_SA_AR_RELEASE_REVISION_VERSION_PBCFG_C      0
#define FLEXIO_SPI_IP_SA_SW_MAJOR_VERSION_PBCFG_C                 3
#define FLEXIO_SPI_IP_SA_SW_MINOR_VERSION_PBCFG_C                 0
#define FLEXIO_SPI_IP_SA_SW_PATCH_VERSION_PBCFG_C                 0

/*==================================================================================================
*                                     FILE VERSION CHECKS
==================================================================================================*/
/* Check if Flexio_Spi_Ip_Sa_PBcfg source file and Spi configuration header file are of the same vendor */
#if (FLEXIO_SPI_IP_SA_VENDOR_ID_PBCFG_C != FLEXIO_SPI_IP_VENDOR_ID)
    #error "Flexio_Spi_Ip_Sa_PBcfg.c and Flexio_Spi_Ip.h have different vendor IDs"
#endif

#if ((FLEXIO_SPI_IP_SA_AR_RELEASE_MAJOR_VERSION_PBCFG_C    != FLEXIO_SPI_IP_AR_RELEASE_MAJOR_VERSION) || \
     (FLEXIO_SPI_IP_SA_AR_RELEASE_MINOR_VERSION_PBCFG_C    != FLEXIO_SPI_IP_AR_RELEASE_MINOR_VERSION) || \
     (FLEXIO_SPI_IP_SA_AR_RELEASE_REVISION_VERSION_PBCFG_C != FLEXIO_SPI_IP_AR_RELEASE_REVISION_VERSION))
    #error "AutoSar Version Numbers of Flexio_Spi_Ip__PBcfg.h and Flexio_Spi_Ip.h are different"
#endif
/* Check if Flexio_Spi_Ip_Sa_PBcfg header file and Spi configuration header file are of the same software version */
#if ((FLEXIO_SPI_IP_SA_SW_MAJOR_VERSION_PBCFG_C != FLEXIO_SPI_IP_SW_MAJOR_VERSION) || \
     (FLEXIO_SPI_IP_SA_SW_MINOR_VERSION_PBCFG_C != FLEXIO_SPI_IP_SW_MINOR_VERSION) || \
     (FLEXIO_SPI_IP_SA_SW_PATCH_VERSION_PBCFG_C != FLEXIO_SPI_IP_SW_PATCH_VERSION))
    #error "Software Version Numbers of Flexio_Spi_Ip_Sa_PBcfg.h and Flexio_Spi_Ip.h are different"
#endif

#ifndef DISABLE_MCAL_INTERMODULE_ASR_CHECK
    #if (FLEXIO_SPI_IP_ENABLE == STD_ON)
    #if (FLEXIO_SPI_IP_DMA_USED == STD_ON)
        #if ((FLEXIO_SPI_IP_SA_AR_RELEASE_MAJOR_VERSION_PBCFG_C != DMA_IP_AR_RELEASE_MAJOR_VERSION) || \
             (FLEXIO_SPI_IP_SA_AR_RELEASE_MINOR_VERSION_PBCFG_C != DMA_IP_AR_RELEASE_MINOR_VERSION) \
            )
            #error "AutoSar Version Numbers of Flexio_Spi_Ip_Sa_PBcfg.h and Dma_Ip.h are different"
        #endif
    #endif /* (FLEXIO_SPI_IP_DMA_USED == STD_ON) */
    #endif /* (FLEXIO_SPI_IP_ENABLE == STD_ON) */
#endif /* DISABLE_MCAL_INTERMODULE_ASR_CHECK */


/*==================================================================================================
*                                        LOCAL MACROS
==================================================================================================*/

/*==================================================================================================
                          LOCAL TYPEDEFS (STRUCTURES, UNIONS, ENUMS)
==================================================================================================*/


/*==================================================================================================
                                       LOCAL CONSTANTS
==================================================================================================*/


/*==================================================================================================
                                       LOCAL VARIABLES
==================================================================================================*/


/*==================================================================================================
                                       GLOBAL CONSTANTS
==================================================================================================*/


/*==================================================================================================
                                       GLOBAL VARIABLES
==================================================================================================*/
#if (FLEXIO_SPI_IP_ENABLE == STD_ON)
#if (FLEXIO_SPI_IP_DMA_USED == STD_ON)
    #define SPI_START_SEC_VAR_INIT_UNSPECIFIED_NO_CACHEABLE
#else
    #define SPI_START_SEC_VAR_INIT_UNSPECIFIED
#endif
#include "Spi_MemMap.h"
/* Flexio_Spi_Ip_DeviceParamsCfg Device Attribute Configuration of Spi*/
static Flexio_Spi_Ip_DeviceParamsType Flexio_Spi_Ip_DeviceParamsCfg[1U] =
{
    {
        (uint8)1U, /* Frame size */
        (boolean)TRUE, /* Lsb */
        (uint32)0U  /* Default Data */
    }
};
#if (FLEXIO_SPI_IP_DMA_USED == STD_ON)
    #define SPI_STOP_SEC_VAR_INIT_UNSPECIFIED_NO_CACHEABLE
#else
    #define SPI_STOP_SEC_VAR_INIT_UNSPECIFIED
#endif
#include "Spi_MemMap.h"
#define SPI_START_SEC_CONFIG_DATA_UNSPECIFIED
#include "Spi_MemMap.h"
/* Flexio_Spi_Ip_DeviceAttributes_SpiExternalDevice_0 Device Attribute Configuration of Spi*/
const Flexio_Spi_Ip_ExternalDeviceType Flexio_Spi_Ip_DeviceAttributes_SpiExternalDevice_0_Instance_0 =
{
    0U,  /* Instance */
    (uint8)FLEXIO_SPI_IP_CPOL_LOW_U8,
    (uint8)FLEXIO_SPI_IP_CPHA_LEADING_U8,
    /* Shifter control (SHIFTCTL) for TX */
    (uint32)0u, /* ClkTimeCmpBaudRate is not used in salve mode - Normal clock */
    (uint32)0u, /* ClkTimeCfgTimDec is not used in slave mode - Normal clock */
      FLEXIO_SPI_IP_SHIFTCTL_TIMSEL(0) /* Select TIMER */
    | FLEXIO_SPI_IP_SHIFTCTL_PINSEL(0) /* Select PIN */
    | FLEXIO_SPI_IP_SHIFTCTL_PINCFG(3) /* Set is output pin */
    | FLEXIO_SPI_IP_SHIFTCTL_SMOD(2) /* Transmit mode */
    | FLEXIO_SPI_IP_SHIFTCTL_TIMPOL(0) /* If CPHA = 0, set "Trigger active low" */
    /* Shifter control (SHIFTCFG) for TX */
    ,FLEXIO_SPI_IP_SHIFTCFG_SSTART(1)
    /* Shifter control (SHIFTCTL) for RX */
      ,FLEXIO_SPI_IP_SHIFTCTL_TIMSEL(0) /* Select TIMER */
    | FLEXIO_SPI_IP_SHIFTCTL_PINSEL(1) /* Select PIN */
    | FLEXIO_SPI_IP_SHIFTCTL_PINCFG(0) /* Set is input pin */
    | FLEXIO_SPI_IP_SHIFTCTL_SMOD(1) /* Receive mode */
    | FLEXIO_SPI_IP_SHIFTCTL_TIMPOL(1) /* If CPHA = 0, set "Trigger active low" */
    /* SHIFTCFG for RX - will be updated in runtime => dont care */
    ,(uint32)0u
    /* Timer compare (TIMCMP) for CLK - will be updated in runtime */
    ,(uint32)0u
    ,FLEXIO_SPI_IP_TIMCFG_DEFAULT_SLAVE_CPHA1_VALUE_U32 /* If CPHA = 0, set "Trigger active low" */
    /* Timer control (TIMCTL) for CLK - Only 1 timer need to configure */
    , FLEXIO_SPI_IP_TIMCTL_TRGSEL(28) /* Select PIN for CS input -> 2*N - Pin N input */
    | FLEXIO_SPI_IP_TIMCTL_PINSEL(12) /* Select PIN for CLK input*/
    | FLEXIO_SPI_IP_TIMCTL_TRGSRC(1) /* Internal trigger selected */
    | FLEXIO_SPI_IP_TIMCTL_TIMOD(3) /* Single 16-bit counter mode */
    | FLEXIO_SPI_IP_TIMCTL_TRGPOL(1) /* Pin is active low */
    | FLEXIO_SPI_IP_TIMCTL_PINCFG(0) /* Set is input pin */
    /* In slave mode, only 1 timer is selected. */
    ,(uint32)0u
    ,(uint32)0u
    ,(uint32)0u
    ,&Flexio_Spi_Ip_DeviceParamsCfg[0]
};
/* SPI controller SpiPhyUnit_0 configuration. */
const Flexio_Spi_Ip_ConfigType Flexio_Spi_Ip_PhyUnitConfig_SpiPhyUnit_0_Instance_0 = 
{
    0U,  /* Instance */
    #if (FLEXIO_SPI_IP_SLAVE_SUPPORT == STD_ON)
    (boolean)TRUE,
    #endif
    #if (FLEXIO_SPI_IP_DMA_USED == STD_ON)
    (boolean)FALSE,
    (uint8)0, /* txDmaChannel */
    (uint8)0, /* rxDmaChannel */
    #endif
    FLEXIO_SPI_IP_POLLING, /* Transfer mode */
    (uint32)8UL, /* Frame size */
    (boolean)TRUE, /* Lsb */ 
    (uint32)1U,  /* Default Data */
    FLEXIO_SPI_IP_SHIFTER_0_U8, /* Shifter index of TX */
    FLEXIO_SPI_IP_SHIFTER_1_U8, /* Shifter index of RX */
    FLEXIO_SPI_IP_TIMER_0_U8, /* Timer index of CLK */
    FLEXIO_SPI_IP_TIMER_1_U8, /* Timer index of CS */
    (uint8)0U /* State structure element from the array */
};

#define SPI_STOP_SEC_CONFIG_DATA_UNSPECIFIED
#include "Spi_MemMap.h"

#endif /*#if (FLEXIO_SPI_IP_ENABLE == STD_ON)*/
/*==================================================================================================
                                   LOCAL FUNCTION PROTOTYPES
==================================================================================================*/


/*==================================================================================================
                                       LOCAL FUNCTIONS
==================================================================================================*/


/*==================================================================================================
                                       GLOBAL FUNCTIONS
==================================================================================================*/

#ifdef __cplusplus
}
#endif

/** @} */


#include <stm32h7xx_hal.h>
#include "system.h"

static void Error_Handler(void);

// System Level C functions and IRQ Handlers
extern "C"
{
    void SysTick_Handler(void)
    {
        HAL_IncTick();
        HAL_SYSTICK_IRQHandler();
    }
}

namespace plaits_daisy
{

void System::Init()
{
    System::Config cfg;
    cfg.Defaults();
    Init(cfg);
}

// TODO: DONT FORGET TO IMPLEMENT CLOCK CONFIG
void System::Init(const System::Config& config)
{
    cfg_ = config;
    HAL_Init();
    ConfigureClocks();
    ConfigureMpu();

    __HAL_RCC_DMA1_CLK_ENABLE();
    HAL_NVIC_SetPriority(DMA1_Stream0_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream0_IRQn);

    SCB_EnableDCache();
    SCB_EnableICache();
}

void System::ConfigureClocks()
{
    RCC_OscInitTypeDef       RCC_OscInitStruct   = {0};
    RCC_ClkInitTypeDef       RCC_ClkInitStruct   = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

    /** Supply configuration update enable 
  */
    HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);

    /** Configure the main internal regulator output voltage 
     ** and set PLLN value, and flash-latency.
     **
     ** See page 159 of Reference manual for VOS/Freq relationship 
     ** and table for flash latency.
     */

    uint32_t plln_val, flash_latency;
    switch(cfg_.cpu_freq)
    {
        case Config::SysClkFreq::FREQ_480MHZ:
            __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);
            plln_val      = 240;
            flash_latency = FLASH_LATENCY_4;
            break;
        case Config::SysClkFreq::FREQ_400MHZ:
        default:
            __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
            plln_val      = 200;
            flash_latency = FLASH_LATENCY_2;
            break;
    }

    while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}
    /** Macro to configure the PLL clock source 
  */
    __HAL_RCC_PLL_PLLSOURCE_CONFIG(RCC_PLLSOURCE_HSE);
    /** Initializes the CPU, AHB and APB busses clocks 
  */
    RCC_OscInitStruct.OscillatorType
        = RCC_OSCILLATORTYPE_HSI48 | RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState      = RCC_HSE_ON;
    RCC_OscInitStruct.HSI48State    = RCC_HSI48_ON;
    RCC_OscInitStruct.PLL.PLLState  = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM      = 4;
    //
    RCC_OscInitStruct.PLL.PLLN      = plln_val;
    RCC_OscInitStruct.PLL.PLLP      = 2;
    RCC_OscInitStruct.PLL.PLLQ      = 5; // was 4 in cube
    RCC_OscInitStruct.PLL.PLLR      = 2;
    RCC_OscInitStruct.PLL.PLLRGE    = RCC_PLL1VCIRANGE_2;
    RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
    RCC_OscInitStruct.PLL.PLLFRACN  = 0;
    if(HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }
    /** Initializes the CPU, AHB and APB busses clocks 
  */
    RCC_ClkInitStruct.ClockType
        = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1
          | RCC_CLOCKTYPE_PCLK2 | RCC_CLOCKTYPE_D3PCLK1 | RCC_CLOCKTYPE_D1PCLK1;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.SYSCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
    RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

    if(HAL_RCC_ClockConfig(&RCC_ClkInitStruct, flash_latency) != HAL_OK)
    {
        Error_Handler();
    }
    PeriphClkInitStruct.PeriphClockSelection
        = RCC_PERIPHCLK_USART1 | RCC_PERIPHCLK_RNG | RCC_PERIPHCLK_SPI1
          | RCC_PERIPHCLK_SAI2 | RCC_PERIPHCLK_SAI1 | RCC_PERIPHCLK_SDMMC
          | RCC_PERIPHCLK_I2C2 | RCC_PERIPHCLK_ADC | RCC_PERIPHCLK_I2C1
          | RCC_PERIPHCLK_USB | RCC_PERIPHCLK_QSPI | RCC_PERIPHCLK_FMC;
    // PLL 2
    //  PeriphClkInitStruct.PLL2.PLL2N = 115; // Max Freq @ 3v3
    //PeriphClkInitStruct.PLL2.PLL2N      = 84; // Max Freq @ 1V9
    PeriphClkInitStruct.PLL2.PLL2N      = 100; // Max supported freq of FMC;
    PeriphClkInitStruct.PLL2.PLL2M      = 4;
    PeriphClkInitStruct.PLL2.PLL2P      = 8;  // 57.5
    PeriphClkInitStruct.PLL2.PLL2Q      = 10; // 46
    PeriphClkInitStruct.PLL2.PLL2R      = 2;  // 115Mhz
    PeriphClkInitStruct.PLL2.PLL2RGE    = RCC_PLL2VCIRANGE_2;
    PeriphClkInitStruct.PLL2.PLL2VCOSEL = RCC_PLL2VCOWIDE;
    PeriphClkInitStruct.PLL2.PLL2FRACN  = 0;
    // PLL 3
    PeriphClkInitStruct.PLL3.PLL3M           = 6;
    PeriphClkInitStruct.PLL3.PLL3N           = 295;
    PeriphClkInitStruct.PLL3.PLL3P           = 16; // 49.xMhz
    PeriphClkInitStruct.PLL3.PLL3Q           = 4;
    PeriphClkInitStruct.PLL3.PLL3R           = 32; // 24.xMhz
    PeriphClkInitStruct.PLL3.PLL3RGE         = RCC_PLL3VCIRANGE_1;
    PeriphClkInitStruct.PLL3.PLL3VCOSEL      = RCC_PLL3VCOWIDE;
    PeriphClkInitStruct.PLL3.PLL3FRACN       = 0;
    PeriphClkInitStruct.FmcClockSelection    = RCC_FMCCLKSOURCE_PLL2;
    PeriphClkInitStruct.QspiClockSelection   = RCC_QSPICLKSOURCE_D1HCLK;
    PeriphClkInitStruct.SdmmcClockSelection  = RCC_SDMMCCLKSOURCE_PLL;
    PeriphClkInitStruct.Sai1ClockSelection   = RCC_SAI1CLKSOURCE_PLL3;
    PeriphClkInitStruct.Sai23ClockSelection  = RCC_SAI23CLKSOURCE_PLL3;
    PeriphClkInitStruct.Spi123ClockSelection = RCC_SPI123CLKSOURCE_PLL2;
    PeriphClkInitStruct.Usart234578ClockSelection
        = RCC_USART234578CLKSOURCE_D2PCLK1;
    PeriphClkInitStruct.Usart16ClockSelection = RCC_USART16CLKSOURCE_D2PCLK2;
    PeriphClkInitStruct.I2c123ClockSelection  = RCC_I2C123CLKSOURCE_D2PCLK1;
    PeriphClkInitStruct.I2c4ClockSelection    = RCC_I2C4CLKSOURCE_PLL3;
    PeriphClkInitStruct.UsbClockSelection     = RCC_USBCLKSOURCE_HSI48;
    PeriphClkInitStruct.AdcClockSelection     = RCC_ADCCLKSOURCE_PLL3;
    if(HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
    {
        Error_Handler();
    }
    /** Enable USB Voltage detector 
  */
    HAL_PWREx_EnableUSBVoltageDetector();
}

void System::ConfigureMpu()
{
    MPU_Region_InitTypeDef MPU_InitStruct;
    HAL_MPU_Disable();
    // Configure RAM D2 (SRAM1) as non cacheable
    MPU_InitStruct.Enable           = MPU_REGION_ENABLE;
    MPU_InitStruct.BaseAddress      = 0x30000000;
    MPU_InitStruct.Size             = MPU_REGION_SIZE_32KB;
    MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
    MPU_InitStruct.IsBufferable     = MPU_ACCESS_NOT_BUFFERABLE;
    MPU_InitStruct.IsCacheable      = MPU_ACCESS_NOT_CACHEABLE;
    MPU_InitStruct.IsShareable      = MPU_ACCESS_SHAREABLE;
    MPU_InitStruct.Number           = MPU_REGION_NUMBER0;
    MPU_InitStruct.TypeExtField     = MPU_TEX_LEVEL1;
    MPU_InitStruct.SubRegionDisable = 0x00;
    MPU_InitStruct.DisableExec      = MPU_INSTRUCTION_ACCESS_ENABLE;
    HAL_MPU_ConfigRegion(&MPU_InitStruct);

    MPU_InitStruct.IsCacheable  = MPU_ACCESS_CACHEABLE;
    MPU_InitStruct.IsBufferable = MPU_ACCESS_BUFFERABLE;
    MPU_InitStruct.IsShareable  = MPU_ACCESS_NOT_SHAREABLE;
    MPU_InitStruct.Number       = MPU_REGION_NUMBER1;
    MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
    MPU_InitStruct.Size         = MPU_REGION_SIZE_64MB;
    MPU_InitStruct.BaseAddress  = 0xC0000000;
    HAL_MPU_ConfigRegion(&MPU_InitStruct);

    HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

} // namespace daisy

static void Error_Handler()
{
    // Insert code to handle errors here.
    while(1) {}
}

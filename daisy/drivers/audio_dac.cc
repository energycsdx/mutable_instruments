#include <stm32h7xx_hal.h>
#include "audio_dac.h"

static const size_t kAudioMaxBufferSize = 1024;
static int32_t __attribute__((section(".sram1_bss"))) audio_tx_buffer[kAudioMaxBufferSize];
static const size_t kAudioBufferSize = 48 * 2 * 2;

namespace plaits_daisy
{
AudioDac* AudioDac::instance_;
  
static uint16_t SAI_pins [] = {GPIO_PIN_2 , GPIO_PIN_4, GPIO_PIN_5, GPIO_PIN_6};
  
extern "C" void HAL_SAI_MspInit(SAI_HandleTypeDef* hsai)
{
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_SAI1_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct;

    GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull  = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
    GPIO_InitStruct.Alternate = GPIO_AF6_SAI1;
                
    for(size_t i = 0; i < 4; i++)
    {
        GPIO_InitStruct.Pin = SAI_pins[i];
        HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
    }

    __HAL_RCC_DMA1_CLK_ENABLE();
    AudioDac::GetInstance()->InitDma();
}
extern "C" void HAL_SAI_MspDeInit(SAI_HandleTypeDef* hsai)
{
    __HAL_RCC_SAI1_CLK_DISABLE();
    for(size_t i = 0; i < 4; i++)
    {
        HAL_GPIO_DeInit(GPIOE, SAI_pins[i]);
    }
}

extern "C" void DMA1_Stream0_IRQHandler(void)
{
    AudioDac::GetInstance()->DMAIRQHandler();  
}

extern "C" void HAL_SAI_TxHalfCpltCallback(SAI_HandleTypeDef* hsai)
{
    AudioDac::GetInstance()->Fill(0);
}

extern "C" void HAL_SAI_TxCpltCallback(SAI_HandleTypeDef* hsai)
{
    AudioDac::GetInstance()->Fill(kAudioBufferSize / 2);
}

void AudioDac::Init()
{
    instance_ = this;
    sai_handle_.Instance = SAI1_Block_A;
    sai_handle_.Init.AudioFrequency = SAI_AUDIO_FREQUENCY_48K;
    sai_handle_.Init.AudioMode = SAI_MODEMASTER_TX;
    sai_handle_.Init.Synchro = SAI_ASYNCHRONOUS;
    sai_handle_.Init.OutputDrive    = SAI_OUTPUTDRIVE_DISABLE;
    sai_handle_.Init.NoDivider      = SAI_MASTERDIVIDER_ENABLE;
    sai_handle_.Init.FIFOThreshold  = SAI_FIFOTHRESHOLD_EMPTY;
    sai_handle_.Init.SynchroExt     = SAI_SYNCEXT_DISABLE;
    sai_handle_.Init.MonoStereoMode = SAI_STEREOMODE;
    sai_handle_.Init.CompandingMode = SAI_NOCOMPANDING;
    sai_handle_.Init.TriState       = SAI_OUTPUT_NOTRELEASED;
    HAL_SAI_InitProtocol(&sai_handle_, SAI_I2S_MSBJUSTIFIED, SAI_PROTOCOL_DATASIZE_24BIT, 2);

    GPIO_InitTypeDef ginit;
    ginit.Mode = GPIO_MODE_OUTPUT_PP;
    ginit.Pull = GPIO_NOPULL;
    ginit.Speed = GPIO_SPEED_LOW;
    ginit.Pin   = GPIO_PIN_11;
    __HAL_RCC_GPIOB_CLK_ENABLE();
    HAL_GPIO_Init(GPIOB, &ginit);
    
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, GPIO_PIN_SET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, GPIO_PIN_RESET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, GPIO_PIN_SET);
}

void AudioDac::Start(FillBufferCallback callback)
{
    callback_  = callback;
    HAL_SAI_Transmit_DMA(&sai_handle_, (uint8_t*)audio_tx_buffer, kAudioBufferSize);
}

void AudioDac::InitDma()
{
    sai_dma_handle_.Instance                 = DMA1_Stream0;
    sai_dma_handle_.Init.Request             = DMA_REQUEST_SAI1_A;
    sai_dma_handle_.Init.Direction           = DMA_MEMORY_TO_PERIPH;
    sai_dma_handle_.Init.PeriphInc           = DMA_PINC_DISABLE;
    sai_dma_handle_.Init.MemInc              = DMA_MINC_ENABLE;
    sai_dma_handle_.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    sai_dma_handle_.Init.MemDataAlignment    = DMA_MDATAALIGN_WORD;
    sai_dma_handle_.Init.Mode                = DMA_CIRCULAR;
    sai_dma_handle_.Init.Priority            = DMA_PRIORITY_HIGH;
    sai_dma_handle_.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;
    HAL_DMA_Init(&sai_dma_handle_);
    __HAL_LINKDMA(&sai_handle_, hdmarx, sai_dma_handle_);
    __HAL_LINKDMA(&sai_handle_, hdmatx, sai_dma_handle_);
}

void AudioDac::DMAIRQHandler(void)
{
    HAL_DMA_IRQHandler(&sai_dma_handle_);
}
  
void AudioDac::Fill(size_t offset)
{
    int32_t *out;
    out = audio_tx_buffer + offset;
    if(callback_)
        callback_(reinterpret_cast<Frame*>(out), kAudioBufferSize / 2 / 2);
}

} // namespace daisy

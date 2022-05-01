#pragma once
#ifndef DSY_SAI_H
#define DSY_SAI_H
#include <stdint.h>
#include <stdlib.h>

namespace plaits_daisy
{
class AudioDac
{
  public:
    typedef struct {
      int32_t l;
      int32_t r;
    } Frame;
  
    typedef void (*FillBufferCallback)(Frame* tx, size_t size);
  
    void Init();
    void InitDma();
    void DMAIRQHandler();
  
    static AudioDac* GetInstance() { return instance_; }

    void Start(FillBufferCallback callback);
    void Fill(size_t offset);

  private:
    static AudioDac* instance_;  
    SAI_HandleTypeDef sai_handle_;
    DMA_HandleTypeDef sai_dma_handle_;
    FillBufferCallback callback_;
};

} // namespace daisy

#endif

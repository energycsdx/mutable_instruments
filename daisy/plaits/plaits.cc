// Copyright 2016 Emilie Gillet.
// 
// Author: Emilie Gillet (emilie.o.gillet@gmail.com)
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
// 
// See http://creativecommons.org/licenses/MIT/ for more information.

#include <stm32h7xx_hal_conf.h>
#include <stm32h7xx_hal.h>

#include "daisy/drivers/audio_dac.h"
#include "daisy/drivers/system.h"

#include "plaits/dsp/dsp.h"
#include "plaits/dsp/voice.h"


using namespace plaits;
using namespace plaits_daisy;
using namespace stmlib;

// #define PROFILE_INTERRUPT 1

const bool test_adc_noise = false;

Modulations modulations;
Patch patch;
Voice voice;

AudioDac audio_dac;
System sys;

char shared_buffer[16384];
uint32_t test_ramp;

// Default interrupt handlers.
extern "C" {

void NMI_Handler() { }
void HardFault_Handler() { while (1); }
void MemManage_Handler() { while (1); }
void BusFault_Handler() { while (1); }
void UsageFault_Handler() { while (1); }
void SVC_Handler() { }
void DebugMon_Handler() { }
void PendSV_Handler() { }
void __cxa_pure_virtual() { while (1); }

}

void FillBuffer(AudioDac::Frame* output, size_t size) {
#ifdef PROFILE_INTERRUPT
  TIC
#endif  // PROFILE_INTERRUPT
  
  if (test_adc_noise) {
    static float note_lp = 0.0f;
    float note = modulations.note;
    ONE_POLE(note_lp, note, 0.0001f);
    float cents = (note - note_lp) * 100.0f;
    CONSTRAIN(cents, -8.0f, +8.0f);
    while (size--) {
      output->r = output->l = static_cast<int16_t>(cents * 4040.0f);
      ++output;
    }
  } else if (/*ui.test_mode()*/0) {
    // 100 Hz ascending and descending ramps.
    while (size--) {
      output->l = ~test_ramp >> 8;
      output->r = test_ramp >> 8;
      test_ramp += 8947848;
      ++output;
    }
  } else {
    voice.Render(patch, modulations, (Voice::Frame*)(output), size);
  }
    
#ifdef PROFILE_INTERRUPT
  TOC
#endif  // PROFILE_INTERRUPT
}

int main(void) {

  sys.Init();
  audio_dac.Init();
  
  BufferAllocator allocator(shared_buffer, 16384);
  voice.Init(&allocator);
  
  patch.note = 69.0f;
  patch.engine = 0;
  patch.harmonics = 0.5f;
  patch.timbre = 0.3f;
  audio_dac.Start(FillBuffer);

  // Theme from Ernest Borgnine
  float notes[] = {80.0, 79.0, 78.0, 65.0, 80.0, 63.0, 63.0, 63.0, 75.0, 75.0, 77.0, 73.0, 80.0, 80.0, 82.0, 60.0, 72.0, 72.0, 73.0, 65.0, 65.0, 80.0, 79.0, 63.0, 63.0, 77.0, 75.0, 73.0, 84.0, 80.0, 82.0, 96.0};
  // Fur Elise 
  //float notes[] = {76.0, 75.0, 76.0, 75.0, 76.0, 71.0, 74.0, 72.0, 45.0, 52.0, 57.0, 60.0, 64.0, 69.0, 40.0, 52.0, 56.0, 64.0, 68.0, 71.0, 45.0, 52.0, 57.0, 64.0, 76.0, 75.0, 76.0, 75.0, 76.0, 71.0, 74.0, 72.0, 45.0, 52.0, 57.0, 60.0, 64.0, 69.0, 40.0, 52.0, 56.0, 62.0, 72.0, 71.0, 69.0, 52.0, 57.0, 64.0};

  int i=0;
  while (1) {
    patch.note = notes[i++];
    i %= sizeof(notes)/sizeof(float);
    HAL_Delay(187);
  }
}

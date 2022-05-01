F_CRYSTAL      = 16000000
F_CPU          = 72000000L
SYSCLOCK       = SYSCLK_FREQ_72MHz
FAMILY         = h7xx

TARGET         = plaits_daisy
PACKAGES       = daisy/plaits \
		plaits/dsp \
		plaits/dsp/fx \
		plaits/dsp/drum_modelling \
		plaits/dsp/engine \
		plaits/dsp/physical_modelling \
		plaits/dsp/speech \
                daisy/drivers \
		stmlib/dsp \
		stmlib/utils \
		stmlib/system

include stmlib/makefile.inc

VPATH += plaits
CC_FILES += resources.cc
$(TARGET_ELF): $(BUILD_DIR)resources.o
$(DEP_FILE): $(BUILD_DIR)resources.d
LINKER_SCRIPT = daisy/STM32H750IB_flash.lds
INCFLAGS := -Idaisy $(INCFLAGS)
#ifndef DSY_SYSTEM_H
#define DSY_SYSTEM_H

#include <cstdint>

namespace plaits_daisy
{
class System
{
  public:
    /** Contains settings for initializing the System */
    struct Config
    {
        /** Specifies the system clock frequency that feeds APB/AHB clocks, etc. */
        enum class SysClkFreq
        {
            FREQ_400MHZ,
            FREQ_480MHZ,
        };

        /** Method to call on the struct to set to defaults
         ** CPU Freq set to 400MHz
         ** Cache Enabled 
         ** */
        void Defaults()
        {
            cpu_freq   = SysClkFreq::FREQ_400MHZ;
            //            use_dcache = true;
            //            use_icache = true;
        }

        SysClkFreq cpu_freq;
    };

    System() {}
    ~System() {}

    /** Default Initializer with no input will create an internal config, 
     ** and set everything to Defaults
     */
    void Init();

    /** Configurable Initializer
     ** Initializes clock tree, DMA initializaiton and 
     ** any necessary global inits.
     */
    void Init(const Config& config);

    /**
     ** Returns a const reference to the Systems Configuration struct
     */
    const Config& GetConfig() const { return cfg_; }

  private:
    void   ConfigureClocks();
    void   ConfigureMpu();
    Config cfg_;

};

} // namespace daisy

#endif

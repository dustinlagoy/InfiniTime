#pragma once

#include "drivers/TwiMaster.h"

namespace Pinetime {
  namespace Drivers {
    class Hrs3300 {
    public:
      enum class Registers : uint8_t {
        Id = 0x00,
        Enable = 0x01,
        EnableHen = 0x80,
        C1dataM = 0x08,
        C0DataM = 0x09,
        C0DataH = 0x0a,
        PDriver = 0x0c,
        C1dataH = 0x0d,
        C1dataL = 0x0e,
        C0dataL = 0x0f,
        Res = 0x16,
        Hgain = 0x17
      };

      enum class WaitTime : uint8_t {
        ms_800 = 0x00,
        ms_400 = 0x01,
        ms_200 = 0x02,
        ms_100 = 0x03,
        ms_75 = 0x04,
        ms_50 = 0x05,
        ms_12_5 = 0x06,
        ms_0 = 0x07,
      };

      // this is current in milliamps but the register calls it power
      enum class PowerDrive : uint8_t {
        mA_12_5 = 0x00,
        mA_20 = 0x01,
        mA_30 = 0x02,
        mA_40 = 0x03,
      };

      enum class Resolution : uint8_t {
        bits_8 = 0x00,
        bits_9 = 0x01,
        bits_10 = 0x02,
        bits_11 = 0x03,
        bits_12 = 0x04,
        bits_13 = 0x05,
        bits_14 = 0x06,
        bits_15 = 0x07,
        bits_16 = 0x08,
        bits_17 = 0x09,
        bits_18 = 0x0a,
      };

      enum class Gain : uint8_t {
        x_1 = 0x00,
        x_2 = 0x01,
        x_4 = 0x02,
        x_8 = 0x03,
        x_64 = 0x04,
      };

      // set most significant reserved bits to 0111
      static constexpr uint8_t resolutionMask = 0x70;
      // set least significant reserved bits to 0xF and power on bit (0x20) high
      // Note: Setting low nibble to 0x8 per the datasheet results in
      // modulated LED driver output. Setting to 0xF results in clean,
      // steady output during the ADC conversion period.
      static constexpr uint8_t driverMask = 0x2f;
      Hrs3300(TwiMaster& twiMaster, uint8_t twiAddress);
      Hrs3300(const Hrs3300&) = delete;
      Hrs3300& operator=(const Hrs3300&) = delete;
      Hrs3300(Hrs3300&&) = delete;
      Hrs3300& operator=(Hrs3300&&) = delete;

      void Init();
      void Enable();
      void Disable();
      uint32_t ReadHrs();
      uint32_t ReadAls();
      void SetWaitTime(WaitTime wait);
      void SetPowerDrive(PowerDrive power);
      void SetResolution(Resolution resolution);
      void SetGain(Gain gain);

    private:
      TwiMaster& twiMaster;
      uint8_t twiAddress;

      void WriteRegister(uint8_t reg, uint8_t data);
      uint8_t ReadRegister(uint8_t reg);
    };
  }
}

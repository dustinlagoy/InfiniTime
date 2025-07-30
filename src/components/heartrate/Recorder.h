#pragma once
#include <FreeRTOS.h>
#include <task.h>
#include "components/fs/FS.h"

namespace Pinetime {
  namespace Controllers {
    class HeartRecorder {
    public:
      HeartRecorder(Pinetime::Controllers::FS& fs);
      int8_t Preprocess(uint16_t hrs, uint16_t als, int16_t x, int16_t y, int16_t z);
      int HeartRate();
      void Reset(bool resetDaqBuffer);
      static constexpr int deltaTms = 40;
      // Daq dataLength: Must be power of 2
      static constexpr uint16_t dataLength = 64;
      static constexpr uint16_t spectrumLength = dataLength >> 1;
    private:
      Pinetime::Controllers::FS& fs;
      uint32_t offset = 0;
      const char* filename = "/heart-dump.dat";
      // limit file size to 1 MB
      uint32_t max_bytes = 1000000;
    };
  }
}

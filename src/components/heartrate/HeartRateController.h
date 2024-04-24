#pragma once

#include <cstdint>
#include <components/ble/HeartRateService.h>
#include <drivers/Hrs3300.h>

namespace Pinetime {
  namespace Applications {
    class HeartRateTask;
  }

  namespace System {
    class SystemTask;
  }

  namespace Controllers {
    class HeartRateController {
    public:
      enum class States { Stopped, NotEnoughData, NoTouch, Running };
      Drivers::Hrs3300& heartRateSensor;

      explicit HeartRateController(Drivers::Hrs3300& heartRateSensor);
      void Start();
      void Stop();
      void Update(States newState, int heartRates[20]);

      void SetHeartRateTask(Applications::HeartRateTask* task);

      States State() const {
        return state;
      }

      uint8_t HeartRate() const {
        return heartRate;
      }

      void SetService(Pinetime::Controllers::HeartRateService* service);

    private:
      Applications::HeartRateTask* task = nullptr;
      States state = States::Stopped;
      uint8_t heartRate = 0;
      Pinetime::Controllers::HeartRateService* service = nullptr;
    };
  }
}

#include "heartratetask/HeartRateTask.h"
#include <drivers/Hrs3300.h>
#include <components/heartrate/HeartRateController.h>
#include <components/motion/MotionController.h>
#include <nrf_log.h>

using namespace Pinetime::Applications;

HeartRateTask::HeartRateTask(
  Drivers::Hrs3300& heartRateSensor,
  Controllers::HeartRateController& controller,
  Controllers::MotionController& motion,
  Pinetime::Controllers::FS& fs
) :
  heartRateSensor {heartRateSensor},
  controller {controller},
  motion {motion},
  processor(fs)
{
}

void HeartRateTask::Start() {
  messageQueue = xQueueCreate(10, 1);
  controller.SetHeartRateTask(this);

  if (pdPASS != xTaskCreate(HeartRateTask::Process, "Heartrate", 500, this, 0, &taskHandle)) {
    APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
  }
}

void HeartRateTask::Process(void* instance) {
  auto* app = static_cast<HeartRateTask*>(instance);
  app->Work();
}

void HeartRateTask::Work() {
  int lastBpm = 0;
  while (true) {
    Messages msg;
    uint32_t delay;
    if (state == States::Running) {
      if (measurementStarted) {
        delay = processor.deltaTms;
      } else {
        delay = 100;
      }
    } else {
      delay = portMAX_DELAY;
    }

    if (xQueueReceive(messageQueue, &msg, delay)) {
      switch (msg) {
        case Messages::GoToSleep:
          StopMeasurement();
          state = States::Idle;
          break;
        case Messages::WakeUp:
          state = States::Running;
          if (measurementStarted) {
            lastBpm = 0;
            StartMeasurement();
          }
          break;
        case Messages::StartMeasurement:
          if (measurementStarted) {
            break;
          }
          lastBpm = 0;
          StartMeasurement();
          measurementStarted = true;
          break;
        case Messages::StopMeasurement:
          if (!measurementStarted) {
            break;
          }
          StopMeasurement();
          measurementStarted = false;
          break;
      }
    }

    if (measurementStarted) {
      auto sensorData = heartRateSensor.ReadHrsAls();
      int8_t error = processor.Preprocess(
        sensorData.hrs, sensorData.als, motion.X(), motion.Y(), motion.Z()
      );
      int bpm = processor.HeartRate();
      // for debugging recorder
      // if (bpm <= 0) {
      //   bpm = error;
      // }

      // If ambient light detected or a reset requested (bpm < 0)
      if (error > 0) {
        // Reset all DAQ buffers
        processor.Reset(true);
        // Force state to NotEnoughData (below)
        lastBpm = 0;
        bpm = 0;
      } else if (bpm < 0) {
        // Reset all DAQ buffers except HRS buffer
        processor.Reset(false);
        // Set HR to zero and update
        bpm = 0;
        controller.Update(Controllers::HeartRateController::States::Running, bpm);
      }

      if (lastBpm == 0 && bpm == 0) {
        controller.Update(Controllers::HeartRateController::States::NotEnoughData, bpm);
      }

      if (bpm != 0) {
        lastBpm = bpm;
        controller.Update(Controllers::HeartRateController::States::Running, lastBpm);
      }
    }
  }
}

void HeartRateTask::PushMessage(HeartRateTask::Messages msg) {
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  xQueueSendFromISR(messageQueue, &msg, &xHigherPriorityTaskWoken);
  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void HeartRateTask::StartMeasurement() {
  heartRateSensor.Enable();
  processor.Reset(true);
  vTaskDelay(100);
}

void HeartRateTask::StopMeasurement() {
  heartRateSensor.Disable();
  processor.Reset(true);
  vTaskDelay(100);
}

#include "components/heartrate/Recorder.h"

using namespace Pinetime::Controllers;

namespace {
  struct HeartDump {
    TickType_t count;
    uint16_t hrs;
    uint16_t als;
    int16_t x;
    int16_t y;
    int16_t z;
  };
}

HeartRecorder::HeartRecorder(Pinetime::Controllers::FS& fs) : fs {fs} {
}

int8_t HeartRecorder::Preprocess(uint16_t hrs, uint16_t als, int16_t x, int16_t y, int16_t z) {
  HeartDump to_write = {xTaskGetTickCount(), hrs, als, x, y, z};
  int result;

  if (offset == 0) {
    // reset the file when starting a new measurement, continue even if not found
    fs.FileDelete(filename);
    result = fs.FileOpen(&file, filename, LFS_O_RDWR | LFS_O_CREAT);
    if (result == LFS_ERR_OK) {
      return result * -1;
    }
  }

  if (offset >= max_bytes) {
    return 0;
  }

  // next thing to try is to buffer here and call write occasionally
  // but we could be facing a limit in the flash speed
  // didn't work even setting littlefs cache_size to 256
  result = fs.FileSeek(&file, offset);
  if (result >= 0) {
    result = fs.FileWrite(&file, reinterpret_cast<uint8_t*>(&to_write), sizeof(to_write));
  }
  if (result < 0) {
    fs.FileClose(&file);
    // should we reset offset on failure?
    return result * -1;
  }

  offset += sizeof(to_write);
  return 0;
}

int HeartRecorder::HeartRate() {
  uint32_t entries = offset / sizeof(HeartDump);
  if (entries % 64 == 0) {
    return entries/64;
  }
  return 0;
}

void HeartRecorder::Reset(bool hardReset) {
  if (hardReset) {
    offset = 0;
    fs.FileClose(&file);
  }
}

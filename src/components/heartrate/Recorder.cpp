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
  lfs_file_t file;
  HeartDump to_write = {xTaskGetTickCount(), hrs, als, x, y, z};
  int result;

  if (offset == 0) {
    // reset the file when starting a new measurement, continue even if not found
    fs.FileDelete(filename);
  }

  if (offset >= max_bytes) {
    return 0;
  }

  result = fs.FileOpen(&file, filename, LFS_O_WRONLY);
  if (result != LFS_ERR_OK) {
    return 0;
  }

  result = fs.FileSeek(&file, offset);
  if (result != LFS_ERR_OK) {
    return 0;
  }

  result = fs.FileWrite(&file, reinterpret_cast<uint8_t*>(&to_write), sizeof(HeartDump));
  if (result != LFS_ERR_OK) {
    return 0;
  }

  offset += sizeof(HeartDump);
  return 0;
}

int HeartRecorder::HeartRate() {
  uint32_t entries = offset / sizeof(HeartDump);
  if (entries % 64 == 0) {
    return entries;
  }
  return 0;
}

void HeartRecorder::Reset(bool hardReset) {
  if (hardReset) {
    offset = 0;
  }
}

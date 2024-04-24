/*
  SPDX-License-Identifier: LGPL-3.0-or-later
  Original work Copyright (C) 2020 Daniel Thompson
  C++ port Copyright (C) 2021 Jean-François Milants
*/

#include "drivers/Hrs3300.h"
#include <algorithm>
#include <nrf_gpio.h>

#include <FreeRTOS.h>
#include <task.h>
#include <nrf_log.h>

using namespace Pinetime::Drivers;

/** Driver for the HRS3300 heart rate sensor.
 * Original implementation from wasp-os : https://github.com/wasp-os/wasp-os/blob/master/wasp/drivers/hrs3300.py
 *
 * Experimentaly derived changes to improve signal/noise (see comments below) - Ceimour
 */
Hrs3300::Hrs3300(TwiMaster& twiMaster, uint8_t twiAddress, Pinetime::Controllers::FS& fs) : twiMaster {twiMaster}, twiAddress {twiAddress}, fs {fs} {
}

void Hrs3300::Init() {
  nrf_gpio_cfg_input(30, NRF_GPIO_PIN_NOPULL);
  LoadSettingsFromFile();

  Disable();
  vTaskDelay(100);
  Enable();
}

void Hrs3300::Enable() {
  NRF_LOG_INFO("ENABLE");
  LoadSettingsFromFile();
  // set all settings registers now
  SetWaitTime(settings.waitTime);
  SetPowerDrive(settings.powerDrive);
  SetResolution(settings.resolution);
  SetGain(settings.gain);

  auto value = ReadRegister(static_cast<uint8_t>(Registers::Enable));
  value |= 0x80;
  WriteRegister(static_cast<uint8_t>(Registers::Enable), value);
}

void Hrs3300::Disable() {
  NRF_LOG_INFO("DISABLE");
  auto value = ReadRegister(static_cast<uint8_t>(Registers::Enable));
  value &= ~0x80;
  WriteRegister(static_cast<uint8_t>(Registers::Enable), value);

  // turn power off completely
  WriteRegister(static_cast<uint8_t>(Registers::PDriver), 0);
}

uint32_t Hrs3300::ReadHrs() {
  auto m = ReadRegister(static_cast<uint8_t>(Registers::C0DataM));
  auto h = ReadRegister(static_cast<uint8_t>(Registers::C0DataH));
  auto l = ReadRegister(static_cast<uint8_t>(Registers::C0dataL));
  return ((l & 0x30) << 12) | (m << 8) | ((h & 0x0f) << 4) | (l & 0x0f);
}

uint32_t Hrs3300::ReadAls() {
  auto m = ReadRegister(static_cast<uint8_t>(Registers::C1dataM));
  auto h = ReadRegister(static_cast<uint8_t>(Registers::C1dataH));
  auto l = ReadRegister(static_cast<uint8_t>(Registers::C1dataL));
  return ((h & 0x3f) << 11) | (m << 3) | (l & 0x07);
}

void Hrs3300::SetWaitTime(WaitTime wait) {
  auto value = ReadRegister(static_cast<uint8_t>(Registers::Enable));
  value |= static_cast<uint8_t>(wait) << 4;
  WriteRegister(static_cast<uint8_t>(Registers::Enable), value);
}

void Hrs3300::SetPowerDrive(PowerDrive power) {
  auto enable = ReadRegister(static_cast<uint8_t>(Registers::Enable));
  uint8_t value = static_cast<uint8_t>(power);

  // third enable bit is the second power bit
  enable |= (value & 0x02) << 2;

  uint8_t driver = settings.driverMask;
  // sixth driver bit is the first power bit
  driver |= (value & 0x01) << 6;

  WriteRegister(static_cast<uint8_t>(Registers::Enable), enable);
  WriteRegister(static_cast<uint8_t>(Registers::PDriver), driver);
}

void Hrs3300::SetResolution(Resolution resolution) {
  WriteRegister(
    static_cast<uint8_t>(Registers::Res),
    static_cast<uint8_t>(resolution) | settings.resolutionMask
  );
}

void Hrs3300::SetGain(Gain gain) {
  WriteRegister(
    static_cast<uint8_t>(Registers::Hgain), static_cast<uint8_t>(gain) << 2
  );
}

void Hrs3300::WriteRegister(uint8_t reg, uint8_t data) {
  auto ret = twiMaster.Write(twiAddress, reg, &data, 1);
  if (ret != TwiMaster::ErrorCodes::NoError)
    NRF_LOG_INFO("WRITE ERROR");
}

uint8_t Hrs3300::ReadRegister(uint8_t reg) {
  uint8_t value;
  auto ret = twiMaster.Read(twiAddress, reg, &value, 1);
  if (ret != TwiMaster::ErrorCodes::NoError)
    NRF_LOG_INFO("READ ERROR");
  return value;
}

void Hrs3300::LoadSettingsFromFile() {
  Settings bufferSettings;
  lfs_file_t settingsFile;

  if (fs.FileOpen(&settingsFile, "/hrs_settings.dat", LFS_O_RDONLY) != LFS_ERR_OK) {
    return;
  }
  fs.FileRead(&settingsFile, reinterpret_cast<uint8_t*>(&bufferSettings), sizeof(settings));
  fs.FileClose(&settingsFile);
  if (bufferSettings.version == settingsVersion) {
    settings = bufferSettings;
  }
}

void Hrs3300::SaveSettingsToFile() {
  lfs_file_t settingsFile;

  if (fs.FileOpen(&settingsFile, "/hrs_settings.dat", LFS_O_WRONLY | LFS_O_CREAT) != LFS_ERR_OK) {
    return;
  }
  fs.FileWrite(&settingsFile, reinterpret_cast<uint8_t*>(&settings), sizeof(settings));
  fs.FileClose(&settingsFile);
}

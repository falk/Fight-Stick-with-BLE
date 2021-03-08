#include "BleConnectionStatus.h"

BleConnectionStatus::BleConnectionStatus(void) {
  sem = xSemaphoreCreateBinary();
}

void BleConnectionStatus::onConnect(BLEServer* pServer)
{
  this->connected = true;
  BLE2902* desc = (BLE2902*)this->inputGamepad->getDescriptorByUUID(BLEUUID((uint16_t)0x2902));
  desc->setNotifications(true);
  xSemaphoreGive(sem);
}

void BleConnectionStatus::onDisconnect(BLEServer* pServer)
{
  this->connected = false;
  BLE2902* desc = (BLE2902*)this->inputGamepad->getDescriptorByUUID(BLEUUID((uint16_t)0x2902));
  desc->setNotifications(false);
  xSemaphoreTake(sem, portMAX_DELAY);
}

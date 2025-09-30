#include "esphome/core/hal.h"
#include "esphome/core/log.h"
#include "esphome/components/i2c/i2c.h"
#include "vl53l1_platform.h"

{
#include "VL53L1X_api.h"
}

namespace esphome {
namespace vl53l1 {
// Map device address to an ESPHome I2CDevice; since we only have one per component
// instance, we temporarily store a pointer set during setup.
static esphome::i2c::I2CDevice *g_dev = nullptr;

void set_current_device(esphome::i2c::I2CDevice *dev) { g_dev = dev; }

int8_t VL53L1_WriteMulti(uint16_t dev, uint16_t index, uint8_t *pdata, uint32_t count) {
  (void) dev;
  if (g_dev == nullptr) return -1;
  uint8_t addr[2] = {static_cast<uint8_t>((index >> 8) & 0xFF), static_cast<uint8_t>(index & 0xFF)};
  if (!g_dev->write(addr, 2, true)) return -1;
  return g_dev->write(pdata, count) ? 0 : -1;
}

int8_t VL53L1_ReadMulti(uint16_t dev, uint16_t index, uint8_t *pdata, uint32_t count) {
  (void) dev;
  if (g_dev == nullptr) return -1;
  uint8_t addr[2] = {static_cast<uint8_t>((index >> 8) & 0xFF), static_cast<uint8_t>(index & 0xFF)};
  if (!g_dev->write(addr, 2, true)) return -1;
  return g_dev->read(pdata, count) ? 0 : -1;
}

int8_t VL53L1_WrByte(uint16_t dev, uint16_t index, uint8_t data) {
  return VL53L1_WriteMulti(dev, index, &data, 1);
}

int8_t VL53L1_WrWord(uint16_t dev, uint16_t index, uint16_t data) {
  uint8_t buf[2] = {static_cast<uint8_t>((data >> 8) & 0xFF), static_cast<uint8_t>(data & 0xFF)};
  return VL53L1_WriteMulti(dev, index, buf, 2);
}

int8_t VL53L1_WrDWord(uint16_t dev, uint16_t index, uint32_t data) {
  uint8_t buf[4] = {static_cast<uint8_t>((data >> 24) & 0xFF), static_cast<uint8_t>((data >> 16) & 0xFF),
                    static_cast<uint8_t>((data >> 8) & 0xFF), static_cast<uint8_t>(data & 0xFF)};
  return VL53L1_WriteMulti(dev, index, buf, 4);
}

int8_t VL53L1_RdByte(uint16_t dev, uint16_t index, uint8_t *pdata) {
  return VL53L1_ReadMulti(dev, index, pdata, 1);
}

int8_t VL53L1_RdWord(uint16_t dev, uint16_t index, uint16_t *pdata) {
  uint8_t buf[2] = {0};
  int8_t rc = VL53L1_ReadMulti(dev, index, buf, 2);
  if (rc == 0) *pdata = (static_cast<uint16_t>(buf[0]) << 8) | static_cast<uint16_t>(buf[1]);
  return rc;
}

int8_t VL53L1_RdDWord(uint16_t dev, uint16_t index, uint32_t *pdata) {
  uint8_t buf[4] = {0};
  int8_t rc = VL53L1_ReadMulti(dev, index, buf, 4);
  if (rc == 0)
    *pdata = (static_cast<uint32_t>(buf[0]) << 24) | (static_cast<uint32_t>(buf[1]) << 16) |
             (static_cast<uint32_t>(buf[2]) << 8) | static_cast<uint32_t>(buf[3]);
  return rc;
}

int8_t VL53L1_WaitMs(uint16_t dev, int32_t wait_ms) {
  (void) dev;
  esphome::delay(wait_ms);
  return 0;
}

} // namespace vl53l1
} // namespace esphome

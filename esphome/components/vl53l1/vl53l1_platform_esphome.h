#pragma once

#include "esphome/components/i2c/i2c.h"

namespace esphome {
namespace vl53l1_platform_bridge {

// Set the current I2C device instance for the vendor VL53L1 ULD C API bridge
void set_current_device(esphome::i2c::I2CDevice *dev);

}  // namespace vl53l1_platform_bridge
}  // namespace esphome



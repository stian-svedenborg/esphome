#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/i2c/i2c.h"

#include <list>

namespace esphome {
  namespace i2c {
    class I2CBus;
  }
namespace vl53l1 {
    void register_sensor(::esphome::i2c::I2CDevice* dev);
    void set_bootstrap_device(::esphome::i2c::I2CDevice* dev);
    void clear_bootstrap_device();

enum DistanceMode : uint8_t { SHORT = 0, LONG = 2 };

class VL53L1Sensor : public sensor::Sensor, public PollingComponent, public i2c::I2CDevice {
 public:
  VL53L1Sensor();
  void setup() override;
  void dump_config() override;
  void update() override;

  void set_timeout_ms(uint32_t timeout_ms) { this->timeout_ms_ = timeout_ms; }
  void set_enable_pin(GPIOPin *enable) { this->enable_pin_ = enable; }
  void set_timing_budget(uint32_t timing_budget) { this->measurement_timing_budget_ms_ = timing_budget; }
  void set_distance_mode(DistanceMode mode) { this->distance_mode_ = mode; }

  /** Calibrate the sensor and store the result in the firmware.
   *  The calibration should be done using a 17% grey reflective surface at 10cm.
   */
  int16_t calibrate();

 protected:
  
  /** Enable the device. Will pull the enable pin high, if configured. */
  bool enable();
  /** Disable the device. Will pull the enable pin low, if configured. */
  void disable();

  bool init_sensor_();
  void enable_pin_setup();
  bool read_distance_mm_(uint16_t &distance_mm);
  bool set_distance_mode_(DistanceMode mode);
  bool set_timing_budget_(uint16_t timing_budget_us);

  GPIOPin *enable_pin_{nullptr};
  uint32_t timeout_ms_{50};
  uint32_t measurement_timing_budget_ms_{50};
  DistanceMode distance_mode_{DistanceMode::SHORT};
  bool initialized_{false};

  static std::list<VL53L1Sensor*> all_sensors;
  static bool pin_setup_complete;
};

}  // namespace vl53l1
}  // namespace esphome



#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/i2c/i2c.h"




namespace esphome {
namespace i2c {
    class I2CDevice;
}

namespace vl53l1 {

    void register_sensor(I2CDevice* dev);

enum DistanceMode : uint8_t { SHORT = 0, LONG = 2 };

class VL53L1Sensor : public sensor::Sensor, public PollingComponent, public i2c::I2CDevice {
 public:
  void setup() override;
  void dump_config() override;
  void update() override;

  void set_timeout_us(uint32_t timeout_us) { this->timeout_us_ = timeout_us; }
  void set_enable_pin(GPIOPin *enable) { this->enable_pin_ = enable; }
  void set_timing_budget(uint32_t timing_budget) { this->measurement_timing_budget_us_ = timing_budget; }
  void set_distance_mode(DistanceMode mode) { this->distance_mode_ = mode; }

  float get_setup_priority() const override {
    // Return the setup priority of this component
    // Higher values mean this component will be set up later
    return setup_priority::LATE;
  }

 protected:
  bool init_sensor_();
  bool read_distance_mm_(uint16_t &distance_mm);
  bool set_distance_mode_(DistanceMode mode);
  bool set_timing_budget_(uint32_t timing_budget_us);

  GPIOPin *enable_pin_{nullptr};
  uint32_t timeout_us_{50000};
  uint32_t measurement_timing_budget_us_{50000};
  DistanceMode distance_mode_{DistanceMode::SHORT};
  bool initialized_{false};
};

}  // namespace vl53l1
}  // namespace esphome



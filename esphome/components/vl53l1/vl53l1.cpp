#include "vl53l1.h"

#include "esphome/core/log.h"

namespace esphome {
namespace vl53l1 {

static const char *const TAG = "vl53l1";

void VL53L1Sensor::setup() {
  ESP_LOGCONFIG(TAG, "Setting up VL53L1...");
  if (this->enable_pin_ != nullptr) {
    this->enable_pin_->setup();
    this->enable_pin_->digital_write(true);
  }

  this->initialized_ = this->init_sensor_();
  if (!this->initialized_) {
    this->mark_failed();
    return;
  }

  // Apply configuration
  this->set_distance_mode_(this->distance_mode_);
  this->set_timing_budget_(this->measurement_timing_budget_us_);
}

void VL53L1Sensor::dump_config() {
  ESP_LOGCONFIG(TAG, "VL53L1:");
  LOG_I2C_DEVICE(this);
  LOG_UPDATE_INTERVAL(this);
  ESP_LOGCONFIG(TAG, "  Timeout: %u us", (unsigned) this->timeout_us_);
  ESP_LOGCONFIG(TAG, "  Timing Budget: %u us", (unsigned) this->measurement_timing_budget_us_);
  ESP_LOGCONFIG(TAG, "  Distance Mode: %u", (unsigned) this->distance_mode_);
  if (this->is_failed()) {
    ESP_LOGE(TAG, "  Communication failed!");
  }
}

void VL53L1Sensor::update() {
  if (!this->initialized_) {
    this->publish_state(NAN);
    return;
  }

  uint16_t distance_mm = 0;
  if (!this->read_distance_mm_(distance_mm)) {
    this->publish_state(NAN);
    this->status_momentary_warning("read", 5000);
    return;
  }

  const float distance_m = distance_mm / 1000.0f;
  ESP_LOGD(TAG, "Distance: %.3f m", distance_m);
  this->publish_state(distance_m);
}

// The following methods are minimal placeholders to allow component scaffolding.
// They should be replaced with proper VL53L1 ULD register setup, taken from
// the VL53L1-ULD-ESP reference implementation.

bool VL53L1Sensor::init_sensor_() {
  // Basic check: try reading a known register if available. For now, just return true.
  // Proper implementation should reset sensor, boot state, load default config.
  return true;
}

bool VL53L1Sensor::read_distance_mm_(uint16_t &distance_mm) {
  // Placeholder: return dummy value to ensure end-to-end plumbing works
  distance_mm = 0;
  // TODO: Implement single ranging using ULD sequences and I2C transactions
  return false;
}

bool VL53L1Sensor::set_distance_mode_(DistanceMode mode) {
  // TODO: Configure sensor distance mode via ULD sequences
  (void) mode;
  return true;
}

bool VL53L1Sensor::set_timing_budget_(uint32_t timing_budget_us) {
  // TODO: Apply timing budget via ULD sequences
  (void) timing_budget_us;
  return true;
}

}  // namespace vl53l1
}  // namespace esphome



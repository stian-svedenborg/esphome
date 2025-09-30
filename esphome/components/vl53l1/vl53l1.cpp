#include "vl53l1.h"
#include "esphome/components/i2c/i2c.h"
#include "esphome/core/log.h"
#include "VL53L1X_api.h"





namespace esphome {
namespace vl53l1 {

using namespace st_vl53l1x_uld;

// Set the current I2C device instance for the vendor VL53L1 ULD C API bridge
void set_current_device(esphome::i2c::I2CDevice *dev);

static const char *const TAG = "vl53l1";



void VL53L1Sensor::setup() {
  ESP_LOGE(TAG, "  Starting setup!");
  ESP_LOGCONFIG(TAG, "Setting up VL53L1...");
  if (this->enable_pin_ != nullptr) {
    this->enable_pin_->setup();
    this->enable_pin_->digital_write(true);
  }

  // Bridge I2C for vendor API
  set_current_device(this);

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
  if (this->enable_pin_ != nullptr) {
    LOG_PIN("  Enable Pin: ", this->enable_pin_);
  }
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

bool VL53L1Sensor::init_sensor_() {
  uint8_t boot = 0;
  uint8_t err = 0;
  const uint32_t start_us = micros();
  while ((micros() - start_us) < 10000000) {
    if ((err = VL53L1X_BootState(this->address_, &boot)) == 0 && boot) break;
    delay(2);
  }
  
  if (!boot) {
    ESP_LOGE(TAG, "Boot not completed: %d", err);
    return false;
  }

  if ((err = VL53L1X_SensorInit(this->address_)) != 0) {
    ESP_LOGE(TAG, "SensorInit failed: %d", err);
    return false;
  }
  return true;
}

bool VL53L1Sensor::read_distance_mm_(uint16_t &distance_mm) {
  uint8_t err = 0;
  if ((err = VL53L1X_StartRanging(this->address_)) != 0) {
    ESP_LOGW(TAG, "StartRanging failed: %d", err);
    return false;
  }

  const uint32_t start_us = micros();
  uint8_t ready = 0;
  while ((micros() - start_us) < this->timeout_us_) {
    if ((err = VL53L1X_CheckForDataReady(this->address_, &ready)) == 0 && ready) break;
    delay(1);
  }
  if (!ready) {
    VL53L1X_StopRanging(this->address_);
    ESP_LOGW(TAG, "Data not ready within timeout: %d", err);
    return false;
  }

  if ((err = VL53L1X_GetDistance(this->address_, &distance_mm)) != 0) {
    VL53L1X_StopRanging(this->address_);
    ESP_LOGW(TAG, "GetDistance failed: %d", err);
    return false;
  }

  VL53L1X_ClearInterrupt(this->address_);
  VL53L1X_StopRanging(this->address_);
  return true;
}

bool VL53L1Sensor::set_distance_mode_(DistanceMode mode) {
  this->distance_mode_ = mode;
  uint8_t err = 0;
  uint16_t vendor_mode = (mode == DistanceMode::SHORT) ? 1 : 2;  // ULD supports short/long
  if ((err = VL53L1X_SetDistanceMode(this->address_, vendor_mode)) != 0) {
    ESP_LOGW(TAG, "SetDistanceMode failed: %d", err);
    return false;
  }
  return true;
}

bool VL53L1Sensor::set_timing_budget_(uint32_t timing_budget_us) {
  this->measurement_timing_budget_us_ = timing_budget_us;
  uint8_t err = 0;
  uint16_t ms = static_cast<uint16_t>((timing_budget_us + 500) / 1000);
  if (ms == 0) ms = 1;
  if ((err = VL53L1X_SetTimingBudgetInMs(this->address_, ms)) != 0) {
    ESP_LOGW(TAG, "SetTimingBudgetInMs failed: %d", err);
    return false;
  }
  return true;
}

}  // namespace vl53l1
}  // namespace esphome



#include "vl53l1.h"
#include "esphome/components/i2c/i2c.h"
#include "esphome/core/log.h"
#include "VL53L1X_api.h"


namespace esphome {
namespace vl53l1 {

using namespace st_vl53l1x_uld;

static const char *const TAG = "vl53l1";
constexpr ::uint8_t DEFAULT_I2C_ADDRESS = 0x29;


void VL53L1Sensor::setup() {
  VL53L1X_ERROR err = 0;
  ESP_LOGCONFIG(TAG, "Setting up VL53L1...");
  if (this->enable_pin_ != nullptr) {
    this->enable_pin_->setup();
    this->enable_pin_->digital_write(true);
  }

  // Setup I2C Address
  esphome::delay(3);
  register_sensor(this); // Bridge I2C for vendor API
  VL53L1X_SetI2CAddress(DEFAULT_I2C_ADDRESS, this->get_i2c_address());
  
  this->initialized_ = this->init_sensor_();
  if (!this->initialized_) {
    this->mark_failed();
    return;
  }

  // Apply configuration
  this->set_distance_mode_(this->distance_mode_);
  this->set_timing_budget_(this->measurement_timing_budget_ms_);

  // Enable measurements

  if ((err = VL53L1X_StartRanging(this->address_)) != VL53L1X_ERROR_NONE) {
    ESP_LOGW(TAG, "StartRanging failed: %d", err);
    this->mark_failed();
  }
}

void VL53L1Sensor::dump_config() {
  ESP_LOGCONFIG(TAG, "VL53L1:");
  LOG_I2C_DEVICE(this);
  LOG_UPDATE_INTERVAL(this);
  ESP_LOGCONFIG(TAG, "  Timeout: %u us", (unsigned) this->timeout_ms_);
  ESP_LOGCONFIG(TAG, "  Timing Budget: %u us", (unsigned) this->measurement_timing_budget_ms_);
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
  VL53L1X_ERROR err = 0;
  const char * failing_call = "";

  const uint32_t start_us = micros();
  while ((micros() - start_us) < 200000) {
    if ((err = VL53L1X_BootState(this->address_, &boot)) == VL53L1X_ERROR_NONE) {
      ESP_LOGW(TAG, "BootState failed %d", err);
    } 
    if (boot) break;
    ESP_LOGVV(TAG, "BootState: %d", boot);
    delay(2);
  }
  
  if (!boot) {
    ESP_LOGE(TAG, "Boot not completed: %d", err);
    return false;
  }

  if ((err = VL53L1X_SensorInit(this->address_)) != 0) {
    failing_call = "SensorInit";
    goto setup_error;
  }

  if ((err = VL53L1X_StartTemperatureUpdate(this->address_)) != 0) {
    failing_call = "StartTemperatureUpdate";
    goto setup_error;
  }

  return true;

setup_error:
  ESP_LOGE(TAG, "%s failed: %d", failing_call, err);
  return false;
}

bool VL53L1Sensor::read_distance_mm_(uint16_t &distance_mm) {
  const char* failing_call = "";
  uint8_t err = 0;
  
  const uint32_t start_us = micros();
  uint8_t ready = 0;
  uint8_t rangeStatus = 0;
  uint16_t tmp_distance = 0;

  while ((micros() - start_us) < this->timeout_ms_*1000) {
    if (err = VL53L1X_CheckForDataReady(this->address_, &ready) != VL53L1X_ERROR_NONE) {
      failing_call = "CheckForDataReady";
      goto read_distance_error;
    }
    if (ready) break;
    delay(2);
  }

  if (!ready) {
    ESP_LOGW(TAG, "Data not ready within timeout");
    return false;
  }
  
  if ((err = VL53L1X_GetRangeStatus(this->address_, &rangeStatus)) != VL53L1X_ERROR_NONE) {
      failing_call = "GetRangeStatus";
      goto read_distance_error;
  }
  switch(rangeStatus) {
    case 0: break;
    case 1:
    case 2:
      ESP_LOGW(TAG, "Range failure: %d", rangeStatus);
      return false;
    default:
      ESP_LOGE(TAG, "Critical range failure: %d", rangeStatus);
      return false;
  }
  
  if ((err = VL53L1X_GetDistance(this->address_, &tmp_distance)) != VL53L1X_ERROR_NONE) {
    failing_call = "GetDistance";
    goto read_distance_error;
  }

  distance_mm = tmp_distance;
  ESP_LOGVV(TAG, "read_distance_mm successfull\n  distance_mm: %d", distance_mm);

  return true;
read_distance_error:
  ESP_LOGE(TAG, "%s failed: %d", failing_call, err);
  return false;
}

bool VL53L1Sensor::set_distance_mode_(DistanceMode mode) {
  uint8_t err = 0;
  uint16_t vendor_mode = (mode == DistanceMode::SHORT) ? 1 : 2;  // ULD supports short/long
  if ((err = VL53L1X_SetDistanceMode(this->address_, vendor_mode)) != VL53L1X_ERROR_NONE) {
    ESP_LOGW(TAG, "SetDistanceMode failed: %d", err);
    return false;
  }
  return true;
}

bool VL53L1Sensor::set_timing_budget_(uint16_t timing_budget_ms) {
  uint8_t err = 0;
  if ((err = VL53L1X_SetTimingBudgetInMs(this->address_, timing_budget_ms)) != VL53L1X_ERROR_NONE) {
    ESP_LOGW(TAG, "SetTimingBudgetInMs failed: %d", err);
    return false;
  }
  return true;
}

}  // namespace vl53l1
}  // namespace esphome



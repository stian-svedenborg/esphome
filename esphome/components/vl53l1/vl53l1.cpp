#include "vl53l1.h"
#include "esphome/components/i2c/i2c.h"
#include "esphome/core/log.h"
#include "VL53L1X_api.h"
#include "VL53L1X_calibration.h"
#include <map>
#include <cassert>


namespace esphome {
namespace vl53l1 {

using namespace st_vl53l1x_uld;

static const char *const TAG = "vl53l1";

// When using multiple VL53L1 sensors on the board, we require them to all
// have different addresses, and if multiple boards are on the same bus, 
// they all need the enable_pin set.
// During setup we will disable all sensors, and bring them online
// one after another as we are bringing them online and register the updated
// I2C address with the sensor firmware.
constexpr ::uint8_t DEFAULT_I2C_ADDRESS = 0x29; 



bool VL53L1Sensor::pin_setup_complete = false;
std::list<VL53L1Sensor*> VL53L1Sensor::all_sensors;

VL53L1Sensor::VL53L1Sensor() {
  all_sensors.push_back(this);
}

void VL53L1Sensor::setup() {
  VL53L1X_ERROR err = 0;
  if (!this->pin_setup_complete) {
    ESP_LOGCONFIG(TAG, "Bootstrapping VL53L1x enable-pins...");
    // Disable all sensors that have enable_pins set.
    for (auto sensor : this->all_sensors) {
        sensor->enable_pin_setup();
        sensor->disable();
    }
    this->pin_setup_complete = true;
  }
  ESP_LOGCONFIG(TAG, "Setting up VL53L1...");

  ::esphome::delay(2);
  
  // Powercycle this sensor to reset firmware address to DEFAULT_I2C_ADDRESS
  this->enable();
  ::esphome::delay(2);
  
  uint16_t final_i2c_address = this->get_i2c_address();
  
  set_bootstrap_device(this);
  if (final_i2c_address != DEFAULT_I2C_ADDRESS) {
    this->set_i2c_address(DEFAULT_I2C_ADDRESS);
  }
  
  this->initialized_ = this->init_sensor_();
  if (!this->initialized_) {
    this->mark_failed();
    return;
  }

  if (this->address_ != final_i2c_address) {
    // The first address argument is used to hook the currently configured the I2CDevice in the platform bridge. 
    if ((err = VL53L1X_SetI2CAddress(this->address_, final_i2c_address << 1)) != VL53L1X_ERROR_NONE) {
      ESP_LOGE(TAG, "SetI2CAddress failed: %d", err);
      this->mark_failed();
      return;
    }
    
    this->set_i2c_address(final_i2c_address);
  }
  register_sensor(this); // Bridge I2C for vendor API
  clear_bootstrap_device();

  // Apply configuration
  this->set_distance_mode_(this->distance_mode_);
  this->set_timing_budget_(this->measurement_timing_budget_ms_);


  //this->calibrate();

  // Enable measurements
  if ((err = VL53L1X_StartRanging(this->address_)) != VL53L1X_ERROR_NONE) {
    ESP_LOGE(TAG, "StartRanging failed: %d", err);
    this->mark_failed();
  }

}

void VL53L1Sensor::dump_config() {
  ESP_LOGCONFIG(TAG, "VL53L1:");
  LOG_I2C_DEVICE(this);
  LOG_UPDATE_INTERVAL(this);
  ESP_LOGCONFIG(TAG, "  Timeout: %u ms", (unsigned) this->timeout_ms_);
  ESP_LOGCONFIG(TAG, "  Timing Budget: %u ms", (unsigned) this->measurement_timing_budget_ms_);
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

void VL53L1Sensor::enable_pin_setup() { 
  if (this->enable_pin_ != nullptr) {
    this->enable_pin_->setup();
  }
 }

int16_t VL53L1Sensor::calibrate() {
  VL53L1X_ERROR err = 0;
  int16_t offset = 0;
  if ((err = VL53L1X_CalibrateOffset(this->address_, 100, &offset)) == VL53L1X_ERROR_NONE) {
      ESP_LOGE(TAG, "CalibrateOffset failed %d", err);
      return 0xefff; 
  } 
  ESP_LOGI(TAG, "Calibration Successful. New offset: %d mm", offset);
  return offset;
}

 bool VL53L1Sensor::enable() {
   if (this->enable_pin_ != nullptr) {
     this->enable_pin_->digital_write(true);
     return true;
   }
   return false;
 }

 void VL53L1Sensor::disable() { 
  if (this->enable_pin_ != nullptr) {
    this->enable_pin_->digital_write(false);
  }
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

static const char * range_status_to_str(uint8_t range_status) {
  switch (range_status) {
    case 0: return "ok";
    case 1: return "sigma failure";
    case 2: return "signal failure";
    case 4: return "too far away";
    case 7: return "wraparound";
    default: return "unknown" 
  }
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
  if (rangeStatus != 0) {
    ESP_LOGW(TAG, "Range failure: %d", range_status_to_str(rangeStatus));
    return false;
  }
    
  
  if ((err = VL53L1X_GetDistance(this->address_, &tmp_distance)) != VL53L1X_ERROR_NONE) {
    failing_call = "GetDistance";
    goto read_distance_error;
  }
  distance_mm = tmp_distance;
  
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



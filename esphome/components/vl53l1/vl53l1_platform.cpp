
/**
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

  #include "vl53l1_platform.h"
  #include "VL53L1X_api.h"
  #include "esphome/core/hal.h"
  #include "esphome/core/log.h"
  #include "esphome/components/i2c/i2c.h"
  
  #include <map>

  namespace esphome {
    namespace vl53l1 {
      static std::map<uint16_t, ::esphome::i2c::I2CDevice*> devices;
      
      void register_sensor(::esphome::i2c::I2CDevice* dev) {
        devices[dev->get_i2c_address()] = dev;
      }
      
      ::esphome::i2c::I2CDevice* lookup(uint16_t devAddr) {
        return devices.at(devAddr);
      }

    int8_t VL53L1_WriteMulti( uint16_t dev, uint16_t index, uint8_t *pdata, uint32_t count) {
      uint8_t status = VL53L1X_ERROR_TIMEOUT;

      if(lookup(dev)->write_bytes(index, pdata, count)) {
        status = VL53L1X_ERROR_NONE;
      }
      
      return status;
    }
  
  int8_t VL53L1_ReadMulti(uint16_t dev, uint16_t index, uint8_t *pdata, uint32_t count){
      uint8_t status = VL53L1X_ERROR_TIMEOUT;

      if(lookup(dev)->read_bytes(index, pdata, count)) {
        status = VL53L1X_ERROR_NONE;
      }
      
      return status;
  }
  
  int8_t VL53L1_WrByte(uint16_t dev, uint16_t index, uint8_t data) {
      return VL53L1_WriteMulti(dev, index, std::reinterpret_cast<uint8_t*>(&data), sizeof(data));
  }
  
  int8_t VL53L1_WrWord(uint16_t dev, uint16_t index, uint16_t data) {
      return VL53L1_WriteMulti(dev, index, std::reinterpret_cast<uint8_t*>(&data), sizeof(data));
  }
  
  int8_t VL53L1_WrDWord(uint16_t dev, uint16_t index, uint32_t data) {
    return VL53L1_WriteMulti(dev, index, std::reinterpret_cast<uint8_t*>(&data), sizeof(data));
  }
  
  int8_t VL53L1_RdByte(uint16_t dev, uint16_t index, uint8_t *data) {
    return VL53L1_ReadMulti(dev, index, std::reinterpret_cast<uint8_t*>(&data), sizeof(*data));
  }
  
  int8_t VL53L1_RdWord(uint16_t dev, uint16_t index, uint16_t *data) {
    return VL53L1_ReadMulti(dev, index, std::reinterpret_cast<uint8_t*>(&data), sizeof(*data));
  }
  
  int8_t VL53L1_RdDWord(uint16_t dev, uint16_t index, uint32_t *data) {
    return VL53L1_ReadMulti(dev, index, std::reinterpret_cast<uint8_t*>(&data), sizeof(*data));
  }
  
  int8_t VL53L1_WaitMs(uint16_t dev, int32_t wait_ms){
      ::esphome::delay(wait_ms);
  }
  


} // namespace vl53l1
} // namespace esphome

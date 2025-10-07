from esphome import pins
import esphome.codegen as cg
from esphome.components import i2c, sensor
import esphome.config_validation as cv
from esphome.const import (
    CONF_ADDRESS,
    CONF_ENABLE_PIN,
    CONF_INTERRUPT_PIN,
    CONF_TIMEOUT,
    DEVICE_CLASS_DISTANCE,
    ICON_ARROW_EXPAND_VERTICAL,
    STATE_CLASS_MEASUREMENT,
    UNIT_METER,

)

DEPENDENCIES = ["i2c"]
CODEOWNERS = ["@stian-svedenborg"]

vl53l1_ns = cg.esphome_ns.namespace("vl53l1")
VL53L1Sensor = vl53l1_ns.class_(
    "VL53L1Sensor", sensor.Sensor, cg.PollingComponent, i2c.I2CDevice
)

CONF_TIMING_BUDGET = "timing_budget"
CONF_DISTANCE_MODE = "distance_mode"
CONF_INTERRUPT_POLARITY = "interrupt_polarity"
CONF_INTER_MEASUREMENT_INTERVAL = "inter_measurement_interval"
CONF_OFFSET = "offset"
CONF_XTALK_CORRECTION = "xtalk_correction"
CONF_DISTANCE_THRESHOLD = "distance_threshold"
CONF_MIN = "min"
CONF_MAX = "max"
CONF_INTERRUPT_WHEN = "interrupt_when"
CONF_ROI = "region_of_interest"
CONF_ROI_X = "x"
CONF_ROI_Y = "y"
CONF_ROI_W = "w"
CONF_ROI_H = "h"
CONF_SIGNAL_THRESHOLD = "signal_threshold"
CONF_SIGMA_THRESHOLD = "sigma_threshold"

DISTANCE_MODE_ENUM = vl53l1_ns.enum("DistanceMode")
DISTANCE_MODE = {
    "short": DISTANCE_MODE_ENUM.SHORT,
    "long": DISTANCE_MODE_ENUM.LONG,
}

TIMING_BUDGET = {
    "15ms": 15,
    "20ms": 20,
    "33ms": 33,
    "50ms": 50,
    "100ms": 100,
    "200ms": 200,
    "500ms": 500,
}

INTERRUPT_WHEN = {
    "below_min": 0,
    "above_max": 1,
    "outside_window": 2,
    "inside_window": 3
}

def check_keys(obj):
    if obj[CONF_ADDRESS] != 0x29 and CONF_ENABLE_PIN not in obj:
        msg = (
            "Address other then 0x29 requires enable_pin definition to allow sensor\r"
        )
        msg += (
            "re-addressing. Also if you have more then one VL53L1x device on the same\r"
        )
        msg += (
            "i2c bus, then all VL53 devices must have enable_pin defined."
        )
        raise cv.Invalid(msg)
    
    if obj[CONF_DISTANCE_MODE] == "long" and obj[CONF_TIMING_BUDGET] not in ("200ms", "500ms"):
        msg = "When (distance_mode == long) the sensor requires a timing budget of at least 200ms"
        raise cv.Invalid(msg)
    
    if CONF_ROI in obj:
        if ( obj[CONF_ROI][CONF_ROI_X] + obj[CONF_ROI][CONF_ROI_W] > 16 
            or obj[CONF_ROI][CONF_ROI_Y] + obj[CONF_ROI][CONF_ROI_H] > 16):
            msg = "Region of interest coordinates cannot exceed 16 in either axis."
            raise cv.Invalid(msg)

    return obj


CONFIG_SCHEMA = cv.All(
    sensor.sensor_schema(
        VL53L1Sensor,
        unit_of_measurement=UNIT_METER,
        icon=ICON_ARROW_EXPAND_VERTICAL,
        accuracy_decimals=3,
        device_class=DEVICE_CLASS_DISTANCE,
        state_class=STATE_CLASS_MEASUREMENT,
    )
    .extend(
        {
            cv.Optional(CONF_TIMEOUT, default="50ms"):  cv.All(
                cv.positive_time_period_milliseconds,
                cv.Range(
                    min=cv.TimePeriod(milliseconds=1),
                    max=cv.TimePeriod(milliseconds=1000),
                ),
            ),
            cv.Optional(CONF_ENABLE_PIN): pins.gpio_output_pin_schema,
            cv.Optional(CONF_TIMING_BUDGET, default="50ms"): cv.enum(
                TIMING_BUDGET, lower=True
            ),
            cv.Optional(CONF_DISTANCE_MODE, default="short"): cv.enum(
                DISTANCE_MODE, lower=True
            ),
            cv.Optional(CONF_OFFSET) : cv.All(
                cv.distance(),
                cv.float_range(-4.0, 12.0)
            ),
            cv.Optional(CONF_XTALK_CORRECTION): cv.uint16_t,
            cv.Optional(CONF_ROI): cv.Schema({
                    cv.Required(CONF_ROI_X): cv.int_range(min=0, max=16),
                    cv.Required(CONF_ROI_Y): cv.int_range(min=0, max=10),
                    cv.Required(CONF_ROI_W): cv.int_range(min=4, max=16),
                    cv.Required(CONF_ROI_H): cv.int_range(min=4, max=16),
            }),
            cv.Optional(CONF_SIGNAL_THRESHOLD): cv.uint16_t,
            cv.Optional(CONF_SIGMA_THRESHOLD): cv.uint16_t,

            # Interrupt config
            cv.Optional(CONF_INTERRUPT_PIN): pins.gpio_input_pin_schema,
            cv.Optional(CONF_DISTANCE_THRESHOLD): cv.Schema({
                cv.Optional(CONF_MIN): cv.All(
                    cv.distance(),
                    cv.float_range(0.0, 4.0)
                ),
                cv.Optional(CONF_MAX): cv.All(
                    cv.distance(),
                    cv.float_range(0.0, 4.0)
                ),
                cv.Required(CONF_INTERRUPT_WHEN): cv.enum(INTERRUPT_WHEN, lower=True)
            })
        }
    )
    .extend(cv.polling_component_schema("60s"))
    .extend(i2c.i2c_device_schema(0x29)),
    check_keys,
)


async def to_code(config):
    var = await sensor.new_sensor(config)
    await cg.register_component(var, config)

    cg.add(var.set_timeout_ms(config[CONF_TIMEOUT]))

    if CONF_ENABLE_PIN in config:
        enable = await cg.gpio_pin_expression(config[CONF_ENABLE_PIN])
        cg.add(var.set_enable_pin(enable))

    cg.add(var.set_timing_budget(config[CONF_TIMING_BUDGET]))
    cg.add(var.set_distance_mode(config[CONF_DISTANCE_MODE]))

    await i2c.register_i2c_device(var, config)



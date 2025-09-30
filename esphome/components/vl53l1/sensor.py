from esphome import pins
import esphome.codegen as cg
from esphome.components import i2c, sensor
import esphome.config_validation as cv
from esphome.const import (
    CONF_ADDRESS,
    CONF_ENABLE_PIN,
    CONF_TIMEOUT,
    DEVICE_CLASS_DISTANCE,
    ICON_ARROW_EXPAND_VERTICAL,
    STATE_CLASS_MEASUREMENT,
    UNIT_METER,
)

DEPENDENCIES = ["i2c"]
CODEOWNERS = ["@your-username"]

vl53l1_ns = cg.esphome_ns.namespace("vl53l1")
VL53L1Sensor = vl53l1_ns.class_(
    "VL53L1Sensor", sensor.Sensor, cg.PollingComponent, i2c.I2CDevice
)

CONF_TIMING_BUDGET = "timing_budget"
CONF_DISTANCE_MODE = "distance_mode"

DISTANCE_MODE_ENUM = vl53l1_ns.enum("DistanceMode")
DISTANCE_MODE = {
    "short": DISTANCE_MODE_ENUM.SHORT,
    "medium": DISTANCE_MODE_ENUM.MEDIUM,
    "long": DISTANCE_MODE_ENUM.LONG,
}


def check_keys(obj):
    if obj[CONF_ADDRESS] != 0x29 and CONF_ENABLE_PIN not in obj:
        msg = (
            "Address other then 0x29 requires enable_pin definition to allow sensor\r"
        )
        msg += (
            "re-addressing. Also if you have more then one VL53 device on the same\r"
        )
        msg += (
            "i2c bus, then all VL53 devices must have enable_pin defined."
        )
        raise cv.Invalid(msg)
    return obj


def check_timeout(value):
    value = cv.positive_time_period_microseconds(value)
    if value.total_microseconds > 60 * 1000 * 1000:
        raise cv.Invalid("Maximum timeout can not be greater then 60 seconds")
    return value


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
            cv.Optional(CONF_TIMEOUT, default="50ms"): check_timeout,
            cv.Optional(CONF_ENABLE_PIN): pins.gpio_output_pin_schema,
            cv.Optional(CONF_TIMING_BUDGET): cv.All(
                cv.positive_time_period_microseconds,
                cv.Range(
                    min=cv.TimePeriod(microseconds=20000),
                    max=cv.TimePeriod(microseconds=200000),
                ),
            ),
            cv.Optional(CONF_DISTANCE_MODE, default="short"): cv.enum(
                DISTANCE_MODE, lower=True
            ),
        }
    )
    .extend(cv.polling_component_schema("60s"))
    .extend(i2c.i2c_device_schema(0x29)),
    check_keys,
)


async def to_code(config):
    var = await sensor.new_sensor(config)
    await cg.register_component(var, config)

    cg.add(var.set_timeout_us(config[CONF_TIMEOUT]))

    if CONF_ENABLE_PIN in config:
        enable = await cg.gpio_pin_expression(config[CONF_ENABLE_PIN])
        cg.add(var.set_enable_pin(enable))

    if timing_budget := config.get(CONF_TIMING_BUDGET):
        cg.add(var.set_timing_budget(timing_budget))

    cg.add(var.set_distance_mode(config[CONF_DISTANCE_MODE]))

    await i2c.register_i2c_device(var, config)



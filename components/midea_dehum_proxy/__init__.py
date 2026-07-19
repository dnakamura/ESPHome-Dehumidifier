import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart, udp
from esphome.const import CONF_ID

DEPENDENCIES = ["udp"]

CONF_UDP_ID = "udp_id"

midea_dehum_proxy_ns = cg.esphome_ns.namespace("midea_dehum_proxy")
MideaDehumProxy = midea_dehum_proxy_ns.class_(
    "MideaDehumProxy", cg.Component, uart.UARTDevice
)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(MideaDehumProxy),
            cv.Required(CONF_UDP_ID): cv.use_id(udp.UDPComponent),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(uart.UART_DEVICE_SCHEMA)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)

    udp_component = await cg.get_variable(config[CONF_UDP_ID])
    cg.add(var.set_udp(udp_component))

    # The UDP component only creates the sockets that a configured consumer needs.
    cg.add(udp_component.set_should_broadcast())
    cg.add(udp_component.set_should_listen())

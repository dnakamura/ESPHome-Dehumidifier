import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart, udp
from esphome.const import CONF_ID, CONF_RX_BUFFER_SIZE

AUTO_LOAD = ["uart"]
DEPENDENCIES = ["udp"]

CONF_UDP_ID = "udp_id"

udp_uart_ns = cg.esphome_ns.namespace("udp_uart")
UDPUARTComponent = udp_uart_ns.class_(
    "UDPUARTComponent", cg.Component, uart.UARTComponent
)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(UDPUARTComponent),
        cv.Required(CONF_UDP_ID): cv.use_id(udp.UDPComponent),
        # ESPHome's UDP receive buffer is 508 bytes. Keeping at least one full
        # datagram ensures that a valid packet is never truncated by design.
        cv.Optional(CONF_RX_BUFFER_SIZE, default=1024): cv.int_range(
            min=508, max=65535
        ),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    udp_component = await cg.get_variable(config[CONF_UDP_ID])
    cg.add(var.set_udp(udp_component))
    cg.add(var.set_rx_buffer_size(config[CONF_RX_BUFFER_SIZE]))

    # UDP allocates only the sockets requested by its consumers.
    cg.add(udp_component.set_should_broadcast())
    cg.add(udp_component.set_should_listen())

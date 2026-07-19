# Midea dehumidifier UART/UDP proxy

This development component forwards complete Midea UART frames to a local
protocol handler over UDP and writes received UDP datagrams back to the UART.
It lets the protocol handler change locally without reflashing the ESP.

UART receive framing follows `MideaDehumComponent::handleUart()` for normal
Midea packets, while preserving malformed data for diagnostics:

- The first byte is `0xAA`.
- The second byte is the total frame length minus one.
- Each complete, valid UART frame becomes exactly one UDP datagram.
- Bytes before a start byte are forwarded too, grouped into datagrams of at
  most 256 bytes.
- A frame header with a length below 3 is forwarded as observed rather than
  discarded.
- A 256-byte receive buffer is sent immediately instead of dropping data.
- Each non-empty UDP datagram is written to UART verbatim.

Use different listen and broadcast ports. That prevents a datagram sent by the
ESP from being received by the same ESP and echoed back to the appliance.

```yaml
external_components:
  - source:
      type: local
      path: components
    components: [midea_dehum_proxy]

uart:
  id: uart_midea_proxy
  tx_pin: GPIO16
  rx_pin: GPIO17
  baud_rate: 9600

udp:
  id: udp_midea_proxy
  port:
    # Local handler -> ESP
    listen_port: 48899
    # ESP -> local handler
    broadcast_port: 48900
  addresses:
    - 192.168.1.100 # Local development machine

midea_dehum_proxy:
  uart_id: uart_midea_proxy
  udp_id: udp_midea_proxy
```
+
The local handler should bind UDP port `48900` and send complete Midea frames
to the ESP's IP address on port `48899`.

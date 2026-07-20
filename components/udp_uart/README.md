# UDP-backed virtual UART

`udp_uart` implements ESPHome's `uart::UARTComponent` interface without using
physical UART pins. Incoming UDP datagrams become bytes available on UART RX,
and bytes written to UART TX are sent over UDP. Existing UART consumers can use
it through their normal `uart_id` option.

This is the inverse endpoint for
[`midea_dehum_proxy`](../midea_dehum_proxy/README.md): the proxy connects UDP to
a physical UART, while `udp_uart` connects a virtual UART to UDP.

- Incoming datagrams are concatenated into the UART RX byte stream.
- Each UART write is sent as one UDP datagram when it is at most 508 bytes.
- Larger UART writes are split into 508-byte datagrams without changing the
  resulting byte stream.
- Empty datagrams and empty writes are ignored.
- If the RX buffer cannot hold an entire datagram, that datagram is dropped
  rather than partially inserting it into the UART stream.
- `flush()` reports assumed success because UDP cannot acknowledge delivery.

Use opposite listen and broadcast ports at the two endpoints. For example, if
the physical proxy listens on `48899` and broadcasts to `48900`, configure the
virtual endpoint like this:

```yaml
external_components:
  - source:
      type: local
      path: components
    components: [midea_dehum, udp_uart]

udp:
  id: udp_midea_virtual
  port:
    # Frames emitted by midea_dehum_proxy arrive here.
    listen_port: 48900
    # Commands sent by the virtual UART go to the proxy's listen port.
    broadcast_port: 48899
  addresses:
    - 192.168.1.50 # IP address of the ESP running midea_dehum_proxy

udp_uart:
  id: uart_midea_virtual
  udp_id: udp_midea_virtual
  rx_buffer_size: 1024

midea_dehum:
  id: dehumidifier
  uart_id: uart_midea_virtual
  protocol_version: 0
```

The `udp` component controls network addresses and ports. `rx_buffer_size`
defaults to 1024 bytes and must be at least 508 bytes, the largest datagram the
ESPHome UDP component receives.

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "esphome/components/uart/uart.h"
#include "esphome/components/udp/udp_component.h"
#include "esphome/core/component.h"

namespace esphome {
namespace midea_dehum_proxy {

class MideaDehumProxy : public Component, public uart::UARTDevice {
public:
  void set_udp(udp::UDPComponent* udp) { this->udp_ = udp; }

  void setup() override;
  void loop() override;
  void dump_config() override;

protected:
  static constexpr size_t MAX_MIDEA_FRAME_SIZE   = 256;
  static constexpr uint8_t MIDEA_START_BYTE      = 0xAA;
  static constexpr uint8_t MIN_MIDEA_LENGTH_BYTE = 3;

  void handle_uart_byte_(uint8_t byte);
  void handle_udp_packet_(const uint8_t* data, size_t size);
  void forward_uart_buffer_(const char* description);

  udp::UDPComponent* udp_{nullptr};
  std::array<uint8_t, MAX_MIDEA_FRAME_SIZE> uart_rx_buffer_{};
  size_t uart_rx_length_{0};
  size_t expected_frame_length_{0};
  bool assembling_midea_frame_{false};
};

}  // namespace midea_dehum_proxy
}  // namespace esphome

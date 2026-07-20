#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "esphome/components/uart/uart_component.h"
#include "esphome/components/udp/udp_component.h"
#include "esphome/core/component.h"

namespace esphome {
namespace udp_uart {

class UDPUARTComponent : public Component, public uart::UARTComponent {
public:
  void set_udp(udp::UDPComponent* udp) { this->udp_ = udp; }

  void setup() override;
  void dump_config() override;

  void write_array(const uint8_t* data, size_t len) override;
  bool peek_byte(uint8_t* data) override;
  bool read_array(uint8_t* data, size_t len) override;
  size_t available() override { return this->rx_count_; }
  uart::UARTFlushResult flush() override;
  bool is_connected() override { return this->udp_ != nullptr && !this->is_failed(); }

#if defined(USE_ESP8266) || defined(USE_ESP32)
  void load_settings(bool dump_config) override;
#endif

protected:
  void check_logger_conflict() override {}
  void handle_udp_packet_(const uint8_t* data, size_t size);
  void push_rx_(const uint8_t* data, size_t size);
  uint8_t pop_rx_();

  udp::UDPComponent* udp_{nullptr};
  std::vector<uint8_t> rx_buffer_{};
  size_t rx_head_{0};
  size_t rx_tail_{0};
  size_t rx_count_{0};
};

}  // namespace udp_uart
}  // namespace esphome

#include "udp_uart.h"

#include <algorithm>

#include "esphome/core/log.h"

namespace esphome {
namespace udp_uart {

static const char* const TAG = "udp_uart";

void UDPUARTComponent::setup() {
  if (this->udp_ == nullptr) {
    ESP_LOGE(TAG, "UDP component is not configured");
    this->mark_failed();
    return;
  }

  if (this->rx_buffer_size_ == 0) {
    ESP_LOGE(TAG, "RX buffer size must be greater than zero");
    this->mark_failed();
    return;
  }

  this->rx_buffer_.resize(this->rx_buffer_size_);

  // The callback is invoked synchronously from UDPComponent::loop(), so the
  // received span remains valid for the duration of this copy.
  this->udp_->add_listener(
      [this](const auto& data) { this->handle_udp_packet_(data.data(), data.size()); });
}

void UDPUARTComponent::dump_config() {
  ESP_LOGCONFIG(TAG,
                "UDP-backed UART:\n"
                "  RX buffer size: %u bytes\n"
                "  Maximum UDP packet size: %u bytes",
                static_cast<unsigned>(this->rx_buffer_size_),
                static_cast<unsigned>(udp::MAX_PACKET_SIZE));
}

void UDPUARTComponent::write_array(const uint8_t* data, size_t len) {
  if (this->udp_ == nullptr || len == 0) {
    return;
  }

  // Preserve one UART write as one datagram when possible. Larger writes are
  // split because UDPComponent receives at most MAX_PACKET_SIZE per datagram.
  while (len > 0) {
    const size_t packet_size = std::min(len, udp::MAX_PACKET_SIZE);
    ESP_LOGV(TAG, "Forwarding %u UART TX bytes over UDP", static_cast<unsigned>(packet_size));
    this->udp_->send_packet(data, packet_size);
    data += packet_size;
    len -= packet_size;
  }
}

bool UDPUARTComponent::peek_byte(uint8_t* data) {
  if (this->rx_count_ == 0) {
    return false;
  }
  *data = this->rx_buffer_[this->rx_tail_];
  return true;
}

bool UDPUARTComponent::read_array(uint8_t* data, size_t len) {
  if (len == 0 || len > this->rx_count_) {
    return false;
  }

  for (size_t i = 0; i < len; i++) {
    data[i] = this->pop_rx_();
  }
  return true;
}

uart::UARTFlushResult UDPUARTComponent::flush() {
  if (this->udp_ == nullptr || this->is_failed()) {
    return uart::UARTFlushResult::UART_FLUSH_RESULT_FAILED;
  }

  // send_packet() hands every datagram to the socket synchronously, but UDP
  // has no acknowledgement that could confirm delivery to the peer.
  return uart::UARTFlushResult::UART_FLUSH_RESULT_ASSUMED_SUCCESS;
}

#if defined(USE_ESP8266) || defined(USE_ESP32)
void UDPUARTComponent::load_settings(bool dump_config) {
  // Baud rate, parity, and framing have no meaning for the UDP transport.
  if (dump_config) {
    this->dump_config();
  }
}
#endif

void UDPUARTComponent::handle_udp_packet_(const uint8_t* data, size_t size) {
  if (size == 0) {
    ESP_LOGV(TAG, "Ignoring empty UDP datagram");
    return;
  }

  const size_t free_space = this->rx_buffer_.size() - this->rx_count_;
  if (size > free_space) {
    ESP_LOGW(TAG, "Dropping %u-byte UDP datagram: only %u bytes free in the UART RX buffer",
             static_cast<unsigned>(size), static_cast<unsigned>(free_space));
    return;
  }

  ESP_LOGV(TAG, "Adding %u UDP bytes to UART RX", static_cast<unsigned>(size));
  this->push_rx_(data, size);
}

void UDPUARTComponent::push_rx_(const uint8_t* data, size_t size) {
  for (size_t i = 0; i < size; i++) {
    this->rx_buffer_[this->rx_head_] = data[i];
    this->rx_head_                   = (this->rx_head_ + 1) % this->rx_buffer_.size();
  }
  this->rx_count_ += size;
}

uint8_t UDPUARTComponent::pop_rx_() {
  const uint8_t value = this->rx_buffer_[this->rx_tail_];
  this->rx_tail_      = (this->rx_tail_ + 1) % this->rx_buffer_.size();
  this->rx_count_--;
  return value;
}

}  // namespace udp_uart
}  // namespace esphome

#include "midea_dehum_proxy.h"

#include "esphome/core/log.h"

namespace esphome {
namespace midea_dehum_proxy {

static const char* const TAG = "midea_dehum_proxy";

void MideaDehumProxy::setup() {
  if (this->udp_ == nullptr) {
    ESP_LOGE(TAG, "UDP component is not configured");
    this->mark_failed();
    return;
  }

  // A generic lambda keeps this compatible with both vector- and span-based
  // UDP listener APIs. The data is consumed synchronously by write_array().
  this->udp_->add_listener(
      [this](const auto& data) { this->handle_udp_packet_(data.data(), data.size()); });
}

void MideaDehumProxy::loop() {
  while (this->available() > 0) {
    uint8_t byte;
    if (!this->read_byte(&byte)) {
      break;
    }
    this->handle_uart_byte_(byte);
  }

  // Prefix/noise data has no length field to wait for. Forward what arrived in
  // this loop so protocol tools can inspect it without waiting for an 0xAA.
  if (!this->assembling_midea_frame_) {
    this->forward_uart_buffer_("UART data without a Midea start byte");
  }
}

void MideaDehumProxy::dump_config() {
  ESP_LOGCONFIG(TAG,
                "Midea dehumidifier UART/UDP proxy:\n"
                "  Maximum Midea frame size: %u bytes",
                static_cast<unsigned>(MAX_MIDEA_FRAME_SIZE));
}

void MideaDehumProxy::handle_uart_byte_(uint8_t byte) {
  if (!this->assembling_midea_frame_) {
    if (byte == MIDEA_START_BYTE) {
      this->forward_uart_buffer_("UART data before a Midea frame");
      this->uart_rx_buffer_[this->uart_rx_length_++] = byte;
      this->assembling_midea_frame_                  = true;
      return;
    }

    this->uart_rx_buffer_[this->uart_rx_length_++] = byte;
    if (this->uart_rx_length_ == this->uart_rx_buffer_.size()) {
      this->forward_uart_buffer_("UART data at the frame buffer limit");
    }
    return;
  }

  if (this->uart_rx_length_ >= this->uart_rx_buffer_.size()) {
    this->forward_uart_buffer_("Midea frame at the frame buffer limit");
    this->handle_uart_byte_(byte);
    return;
  }

  this->uart_rx_buffer_[this->uart_rx_length_++] = byte;

  if (this->uart_rx_length_ == 2) {
    const uint8_t length_byte    = this->uart_rx_buffer_[1];
    this->expected_frame_length_ = static_cast<size_t>(length_byte) + 1;

    // Match the normal framing rule for valid packets, but preserve malformed
    // headers too. A short length cannot identify any following frame bytes,
    // so emit the observed header immediately and resume raw collection.
    if (length_byte < MIN_MIDEA_LENGTH_BYTE ||
        this->expected_frame_length_ > this->uart_rx_buffer_.size()) {
      ESP_LOGW(TAG, "Forwarding Midea UART data with invalid length byte: %u", length_byte);
      this->forward_uart_buffer_("Midea frame with an invalid length byte");
      return;
    }
  }

  if (this->expected_frame_length_ != 0 && this->uart_rx_length_ >= this->expected_frame_length_) {
    this->forward_uart_buffer_("complete Midea frame");
  }
}

void MideaDehumProxy::handle_udp_packet_(const uint8_t* data, size_t size) {
  if (size == 0) {
    ESP_LOGV(TAG, "Ignoring empty UDP datagram");
    return;
  }

  ESP_LOGV(TAG, "Forwarding %u-byte UDP datagram over UART", static_cast<unsigned>(size));
  this->write_array(data, size);
}

void MideaDehumProxy::forward_uart_buffer_(const char* description) {
  if (this->uart_rx_length_ == 0) {
    return;
  }

  ESP_LOGV(TAG, "Forwarding %u-byte %s over UDP", static_cast<unsigned>(this->uart_rx_length_),
           description);
  this->udp_->send_packet(this->uart_rx_buffer_.data(), this->uart_rx_length_);
  this->uart_rx_length_         = 0;
  this->expected_frame_length_  = 0;
  this->assembling_midea_frame_ = false;
}

}  // namespace midea_dehum_proxy
}  // namespace esphome

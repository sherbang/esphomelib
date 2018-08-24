//
//  uart_component.cpp
//  esphomelib
//
//  Created by Otto Winter on 24.06.18.
//  Copyright © 2018 Otto Winter. All rights reserved.
//

#include "esphomelib/defines.h"

#ifdef USE_UART

#include "esphomelib/uart_component.h"
#include "esphomelib/helpers.h"
#include "esphomelib/log.h"

#ifdef ARDUINO_ARCH_ESP8266
  #include "FunctionalInterrupt.h"
#endif

ESPHOMELIB_NAMESPACE_BEGIN

static const char *TAG = "uart";

#ifdef ARDUINO_ARCH_ESP32
uint8_t next_uart_num = 1;
#endif

UARTComponent::UARTComponent(int8_t tx_pin, int8_t rx_pin, uint32_t baud_rate)
    : tx_pin_(tx_pin), rx_pin_(rx_pin), baud_rate_(baud_rate) {
#ifdef ARDUINO_ARCH_ESP32
  if (this->rx_pin_ == 3 && this->tx_pin_ == 1) {
    // Default UART for logging
    this->hw_serial_ = &Serial;
  } else {
    this->hw_serial_ = new HardwareSerial(next_uart_num++);
  }
#endif
#ifdef ARDUINO_ARCH_ESP8266
  if (this->rx_pin_ == 3 && this->tx_pin_ == 1) {
    this->hw_serial_ = &Serial;
  } else {
    this->sw_serial_ = new ESP8266SoftwareSerial();
  }
#endif
}

float UARTComponent::get_setup_priority() const {
  return setup_priority::PRE_HARDWARE;
}

#ifdef ARDUINO_ARCH_ESP32
void UARTComponent::setup() {
  ESP_LOGCONFIG(TAG, "Setting up UART...");
  ESP_LOGCONFIG(TAG, "    TX Pin: %d", this->tx_pin_);
  ESP_LOGCONFIG(TAG, "    RX Pin: %d", this->rx_pin_);
  ESP_LOGCONFIG(TAG, "    Baud Rate: %u", this->baud_rate_);
  this->hw_serial_->begin(this->baud_rate_, SERIAL_8N1, this->rx_pin_, this->tx_pin_);
}

void UARTComponent::write_byte(uint8_t data) {
  this->hw_serial_->write(data);
  ESP_LOGVV(TAG, "    Wrote 0b" BYTE_TO_BINARY_PATTERN " (0x%02X)", BYTE_TO_BINARY(data), data);
}
void UARTComponent::write_array(const uint8_t *data, size_t len) {
  this->hw_serial_->write(data, len);
  for (size_t i = 0; i < len; i++) {
    ESP_LOGVV(TAG, "    Wrote 0b" BYTE_TO_BINARY_PATTERN " (0x%02X)", BYTE_TO_BINARY(data[i]), data[i]);
  }
}
void UARTComponent::write_str(const char *str) {
  this->hw_serial_->write(str);
  ESP_LOGVV(TAG, "    Wrote \"%s\"", str);
}
bool UARTComponent::read_byte(uint8_t *data) {
  if (!this->check_read_timeout_())
    return false;
  *data = this->hw_serial_->read();
  ESP_LOGVV(TAG, "    Read 0b" BYTE_TO_BINARY_PATTERN " (0x%02X)", BYTE_TO_BINARY(*data), *data);
  return true;
}
bool UARTComponent::peek_byte(uint8_t *data) {
  if (!this->check_read_timeout_())
    return false;
  *data = this->hw_serial_->peek();
  return true;
}
bool UARTComponent::read_array(uint8_t *data, size_t len) {
  if (!this->check_read_timeout_(len))
    return false;
  this->hw_serial_->readBytes(data, len);
  for (size_t i = 0; i < len; i++) {
    ESP_LOGVV(TAG, "    Read 0b" BYTE_TO_BINARY_PATTERN " (0x%02X)", BYTE_TO_BINARY(data[i]), data[i]);
  }

  return true;
}
bool UARTComponent::check_read_timeout_(size_t len) {
  if (this->available() >= len)
    return true;

  uint32_t start_time = millis();
  while (this->available() < len) {
    if (millis() - start_time > 1000) {
      ESP_LOGE(TAG, "Reading from UART timed out at byte %u!", this->available());
      return false;
    }
  }
  return true;
}
size_t UARTComponent::available() {
  return static_cast<size_t>(this->hw_serial_->available());
}
void UARTComponent::flush() {
  ESP_LOGVV(TAG, "    Flushing...");
  this->hw_serial_->flush();
}
#endif //ESP32

#ifdef ARDUINO_ARCH_ESP8266
void UARTComponent::setup() {
  ESP_LOGCONFIG(TAG, "Setting up UART...");
  ESP_LOGCONFIG(TAG, "    TX Pin: %d", this->tx_pin_);
  ESP_LOGCONFIG(TAG, "    RX Pin: %d", this->rx_pin_);
  ESP_LOGCONFIG(TAG, "    Baud Rate: %u", this->baud_rate_);
  if (this->hw_serial_ != nullptr) {
    ESP_LOGCONFIG(TAG, "    Using default serial interface.");
    this->hw_serial_->begin(this->baud_rate_);
  } else {
    ESP_LOGCONFIG(TAG, "    Using software serial");
    this->sw_serial_->setup(this->tx_pin_, this->rx_pin_, this->baud_rate_);
  }
}

void UARTComponent::write_byte(uint8_t data) {
  if (this->hw_serial_ != nullptr) {
    this->hw_serial_->write(data);
  } else {
    this->sw_serial_->write_byte(data);
  }
  ESP_LOGVV(TAG, "    Wrote 0b" BYTE_TO_BINARY_PATTERN " (0x%02X)", BYTE_TO_BINARY(data), data);
}
void UARTComponent::write_array(const uint8_t *data, size_t len) {
  if (this->hw_serial_ != nullptr) {
    this->hw_serial_->write(data, len);
  } else {
    for (size_t i = 0; i < len; i++)
      this->sw_serial_->write_byte(data[i]);
  }
  for (size_t i = 0; i < len; i++) {
    ESP_LOGVV(TAG, "    Wrote 0b" BYTE_TO_BINARY_PATTERN " (0x%02X)", BYTE_TO_BINARY(data[i]), data[i]);
  }
}
void UARTComponent::write_str(const char *str) {
  if (this->hw_serial_ != nullptr) {
    this->hw_serial_->write(str);
  } else {
    const auto *data = reinterpret_cast<const uint8_t *>(str);
    for (size_t i = 0; data[i] != 0; i++)
      this->sw_serial_->write_byte(data[i]);
  }
  ESP_LOGVV(TAG, "    Wrote \"%s\"", str);
}
bool UARTComponent::read_byte(uint8_t *data) {
  if (!this->check_read_timeout_())
    return false;
  if (this->hw_serial_ != nullptr) {
    *data = this->hw_serial_->read();
  } else {
    *data = this->sw_serial_->read_byte();
  }
  ESP_LOGVV(TAG, "    Read 0b" BYTE_TO_BINARY_PATTERN " (0x%02X)", BYTE_TO_BINARY(*data), *data);
  return true;
}
bool UARTComponent::peek_byte(uint8_t *data) {
  if (!this->check_read_timeout_())
    return false;
  if (this->hw_serial_ != nullptr) {
    *data = this->hw_serial_->peek();
  } else {
    *data = this->sw_serial_->peek_byte();
  }
  return true;
}
bool UARTComponent::read_array(uint8_t *data, size_t len) {
  if (!this->check_read_timeout_(len))
    return false;
  if (this->hw_serial_ != nullptr) {
    this->hw_serial_->readBytes(data, len);
  } else {
    for (size_t i = 0; i < len; i++)
      data[i] = this->sw_serial_->read_byte();
  }
  for (size_t i = 0; i < len; i++) {
    ESP_LOGVV(TAG, "    Read 0b" BYTE_TO_BINARY_PATTERN " (0x%02X)", BYTE_TO_BINARY(data[i]), data[i]);
  }

  return true;
}
bool UARTComponent::check_read_timeout_(size_t len) {
  if (this->available() >= len)
    return true;

  uint32_t start_time = millis();
  while (this->available() < len) {
    if (millis() - start_time > 100) {
      ESP_LOGE(TAG, "Reading from UART timed out at byte %u!", this->available());
      return false;
    }
    yield();
  }
  return true;
}
size_t UARTComponent::available() {
  if (this->hw_serial_ != nullptr) {
    return static_cast<size_t>(this->hw_serial_->available());
  } else {
    return this->sw_serial_->available();
  }
}
void UARTComponent::flush() {
  ESP_LOGVV(TAG, "    Flushing...");
  if (this->hw_serial_ != nullptr) {
    this->hw_serial_->flush();
  } else {
    this->sw_serial_->flush();
  }
}

void ESP8266SoftwareSerial::setup(int8_t tx_pin, int8_t rx_pin, uint32_t baud_rate) {
  this->bit_time_ = F_CPU / baud_rate;
  if (tx_pin != -1) {
    this->tx_mask_ = (1U << tx_pin);
    pinMode(tx_pin, OUTPUT);
    this->tx_high_();
  }
  if (rx_pin != -1) {
    this->rx_mask_ = (1U << rx_pin);
    pinMode(rx_pin, INPUT);
    this->rx_buffer_ = new uint8_t[this->rx_buffer_size_];
    auto f = std::bind(&ESP8266SoftwareSerial::gpio_intr_, this);
    attachInterrupt(rx_pin, f, FALLING);
  }
}

void ICACHE_RAM_ATTR ESP8266SoftwareSerial::gpio_intr_() {
  uint32_t wait = this->bit_time_ + this->bit_time_/3 - 500;
  const uint32_t start = ESP.getCycleCount();
  uint8_t rec = 0;
  // Manually unroll the loop
  rec |= this->read_bit_(wait, start) << 0;
  rec |= this->read_bit_(wait, start) << 1;
  rec |= this->read_bit_(wait, start) << 2;
  rec |= this->read_bit_(wait, start) << 3;
  rec |= this->read_bit_(wait, start) << 4;
  rec |= this->read_bit_(wait, start) << 5;
  rec |= this->read_bit_(wait, start) << 6;
  rec |= this->read_bit_(wait, start) << 7;
  // Stop bit
  this->wait_(wait, start);

  this->rx_buffer_[this->rx_in_pos_] = rec;
  this->rx_in_pos_ = (this->rx_in_pos_ + 1) % this->rx_buffer_size_;
  // Clear RX pin so that the interrupt doesn't re-trigger right away again.
  GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, this->rx_mask_);
}
void ESP8266SoftwareSerial::write_byte(uint8_t data) {
  if (this->tx_mask_ == 0) {
    ESP_LOGE(TAG, "UART doesn't have TX pins set!");
    return;
  }

  disable_interrupts();
  uint32_t wait = this->bit_time_;
  const uint32_t start = ESP.getCycleCount();
  // Start bit
  this->write_bit_(false, wait, start);
  this->write_bit_(data & (1 << 0), wait, start);
  this->write_bit_(data & (1 << 1), wait, start);
  this->write_bit_(data & (1 << 2), wait, start);
  this->write_bit_(data & (1 << 3), wait, start);
  this->write_bit_(data & (1 << 4), wait, start);
  this->write_bit_(data & (1 << 5), wait, start);
  this->write_bit_(data & (1 << 6), wait, start);
  this->write_bit_(data & (1 << 7), wait, start);
  // Stop bit
  this->write_bit_(true, wait, start);
  enable_interrupts();
}
void ESP8266SoftwareSerial::wait_(uint32_t &wait, const uint32_t &start) {
  while (ESP.getCycleCount() - start < wait);
  wait += this->bit_time_;
}
uint8_t ESP8266SoftwareSerial::read_bit_(uint32_t &wait, const uint32_t &start) {
  this->wait_(wait, start);
  return uint8_t((GPI & this->rx_mask_) != 0);
}
void ESP8266SoftwareSerial::write_bit_(bool bit, uint32_t &wait, const uint32_t &start) {
  if (bit) {
    this->tx_high_();
  } else {
    this->tx_low_();
  }
  this->wait_(wait, start);
}
void ESP8266SoftwareSerial::tx_high_() {
  GPOS = this->tx_mask_;
}
void ESP8266SoftwareSerial::tx_low_() {
  GPOC = this->tx_mask_;
}
uint8_t ESP8266SoftwareSerial::read_byte() {
  if (this->rx_in_pos_ == this->rx_out_pos_)
    return 0;
  uint8_t data = this->rx_buffer_[this->rx_out_pos_];
  this->rx_out_pos_ = (this->rx_out_pos_ + 1) % this->rx_buffer_size_;
  return data;
}
uint8_t ESP8266SoftwareSerial::peek_byte() {
  if (this->rx_in_pos_ == this->rx_out_pos_)
    return 0;
  return this->rx_buffer_[this->rx_out_pos_];
}
void ESP8266SoftwareSerial::flush() {
  this->rx_in_pos_ = this->rx_out_pos_ = 0;
}
size_t ESP8266SoftwareSerial::available() {
  int avail = int(this->rx_in_pos_) - int(this->rx_out_pos_);
  if (avail < 0)
    return avail + this->rx_buffer_size_;
  return static_cast<size_t>(avail);
}
#endif //ESP8266

void UARTDevice::write_byte(uint8_t data) {
  this->parent_->write_byte(data);
}
void UARTDevice::write_array(const uint8_t *data, size_t len) {
  this->parent_->write_array(data, len);
}
void UARTDevice::write_str(const char *str) {
  this->parent_->write_str(str);
}
bool UARTDevice::read_byte(uint8_t *data) {
  return this->parent_->read_byte(data);
}
bool UARTDevice::peek_byte(uint8_t *data) {
  return this->parent_->peek_byte(data);
}
bool UARTDevice::read_array(uint8_t *data, size_t len) {
  return this->parent_->read_array(data, len);
}
size_t UARTDevice::available() {
  return this->parent_->available();
}
void UARTDevice::flush() {
  return this->parent_->flush();
}
UARTDevice::UARTDevice(UARTComponent *parent) : parent_(parent) {}

ESPHOMELIB_NAMESPACE_END

#endif //USE_UART
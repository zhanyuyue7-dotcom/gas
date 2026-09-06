#include "Ssd1306.h"

#include <algorithm>
#include <array>

#include "esp_err.h"
#include "esp_rom_sys.h"

namespace gasmonitor {

bool Ssd1306::beginSpi(const gpio_num_t mosiPin, const gpio_num_t clockPin,
                       const gpio_num_t resetPin, const gpio_num_t dcPin) {
  resetPin_ = resetPin;
  dcPin_ = dcPin;

  spi_bus_config_t busConfig{};
  busConfig.mosi_io_num = mosiPin;
  busConfig.miso_io_num = GPIO_NUM_NC;
  busConfig.sclk_io_num = clockPin;
  busConfig.quadwp_io_num = GPIO_NUM_NC;
  busConfig.quadhd_io_num = GPIO_NUM_NC;
  busConfig.max_transfer_sz = 64;
  if (spi_bus_initialize(SPI2_HOST, &busConfig, SPI_DMA_DISABLED) != ESP_OK) {
    return false;
  }

  spi_device_interface_config_t deviceConfig{};
  deviceConfig.clock_speed_hz = 8000000;
  deviceConfig.mode = 0;
  deviceConfig.spics_io_num = GPIO_NUM_NC;
  deviceConfig.queue_size = 1;
  if (spi_bus_add_device(SPI2_HOST, &deviceConfig, &device_) != ESP_OK) {
    return false;
  }

  if (gpio_set_direction(dcPin_, GPIO_MODE_OUTPUT) != ESP_OK ||
      gpio_set_direction(resetPin_, GPIO_MODE_OUTPUT) != ESP_OK) {
    return false;
  }
  gpio_set_level(dcPin_, 0);
  gpio_set_level(resetPin_, 0);
  esp_rom_delay_us(10000);
  gpio_set_level(resetPin_, 1);
  esp_rom_delay_us(10000);

  constexpr std::array<std::uint8_t, 25> init{
      0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00, 0x40, 0x8D,
      0x14, 0x20, 0x02, 0xA1, 0xC8, 0xDA, 0x12, 0x81, 0x7F,
      0xD9, 0xF1, 0xDB, 0x40, 0xA4, 0xA6, 0xAF};
  for (const auto value : init) {
    if (!command(value)) {
      return false;
    }
  }
  clear();
  return present();
}

void Ssd1306::clear() { buffer_.fill(0); }

void Ssd1306::drawPixel(const int x, const int y, const bool on) {
  if (x < 0 || x >= 128 || y < 0 || y >= 64) {
    return;
  }
  auto& value = buffer_[static_cast<std::size_t>(x + (y / 8) * 128)];
  const auto mask = static_cast<std::uint8_t>(1U << (y % 8));
  if (on) {
    value = static_cast<std::uint8_t>(value | mask);
  } else {
    value = static_cast<std::uint8_t>(value & ~mask);
  }
}

void Ssd1306::drawText(const int x, const int y, const char* text,
                       const OledFont font) {
  renderOledTextPixels(
      font, x, y, text, this,
      [](void* context, const int pixelX, const int pixelY) {
        static_cast<Ssd1306*>(context)->drawPixel(pixelX, pixelY, true);
      });
}

void Ssd1306::drawHorizontalLine(const int y) {
  for (int x = 0; x < 128; ++x) {
    drawPixel(x, y, true);
  }
}

bool Ssd1306::command(const std::uint8_t value) {
  return write(&value, 1, false);
}

bool Ssd1306::write(const std::uint8_t* data, const std::size_t size,
                    const bool isData) {
  if (device_ == nullptr || data == nullptr || size == 0) {
    return false;
  }
  gpio_set_level(dcPin_, isData ? 1 : 0);
  constexpr std::size_t kMaxPollingTransfer = 64;
  std::size_t offset = 0;
  while (offset < size) {
    const std::size_t chunkSize =
        std::min(kMaxPollingTransfer, size - offset);
    spi_transaction_t transaction{};
    transaction.length = chunkSize * 8;
    transaction.tx_buffer = data + offset;
    if (spi_device_polling_transmit(device_, &transaction) != ESP_OK) {
      return false;
    }
    offset += chunkSize;
  }
  return true;
}

bool Ssd1306::present() {
  if (device_ == nullptr) {
    return false;
  }
  for (int page = 0; page < 8; ++page) {
    if (!command(static_cast<std::uint8_t>(0xB0 + page)) ||
        !command(0x00) || !command(0x10)) {
      return false;
    }
    if (!write(buffer_.data() + page * 128, 128, true)) {
      return false;
    }
  }
  return true;
}

}  // namespace gasmonitor

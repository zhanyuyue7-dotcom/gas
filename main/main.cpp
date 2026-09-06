#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

#include "GasFrameParser.h"
#include "GasState.h"
#include "OledLayout.h"
#include "Ssd1306.h"
#include "StatusJson.h"
#include "driver/uart.h"
#include "driver/rmt_tx.h"
#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"

namespace {

extern const char webPageStart[] asm("_binary_web_page_html_start");
extern const char webPageEnd[] asm("_binary_web_page_html_end");

constexpr char kLogTag[] = "GasMonitor";
constexpr uart_port_t kGasUart = UART_NUM_1;
constexpr gpio_num_t kGasRxPin = GPIO_NUM_5;
constexpr gpio_num_t kOledMosiPin = GPIO_NUM_11;
constexpr gpio_num_t kOledClockPin = GPIO_NUM_12;
constexpr gpio_num_t kOledResetPin = GPIO_NUM_10;
constexpr gpio_num_t kOledDcPin = GPIO_NUM_9;
constexpr std::uint32_t kGasBaud = 9600;
constexpr char kAccessPointPassword[] = "gasmonitor";
constexpr gpio_num_t kBoardRgbPin = GPIO_NUM_48;
constexpr std::uint32_t kRgbResolutionHz = 10000000;

constexpr rmt_symbol_word_t kRgbZero{.duration0 = 3,
                                     .level0 = 1,
                                     .duration1 = 9,
                                     .level1 = 0};
constexpr rmt_symbol_word_t kRgbOne{.duration0 = 9,
                                    .level0 = 1,
                                    .duration1 = 3,
                                    .level1 = 0};
constexpr rmt_symbol_word_t kRgbReset{.duration0 = 250,
                                      .level0 = 0,
                                      .duration1 = 250,
                                      .level1 = 0};

gasmonitor::GasFrameParser gParser;
gasmonitor::GasState gState;
gasmonitor::Ssd1306 gOled;
portMUX_TYPE gStateMux = portMUX_INITIALIZER_UNLOCKED;
httpd_handle_t gHttpServer = nullptr;

std::size_t encodeRgbOff(const void* data, const std::size_t dataSize,
                         const std::size_t symbolsWritten,
                         const std::size_t symbolsFree,
                         rmt_symbol_word_t* symbols, bool* done, void*) {
  if (symbolsFree < 8) {
    return 0;
  }
  const auto dataPosition = symbolsWritten / 8;
  if (dataPosition < dataSize) {
    const auto* bytes = static_cast<const std::uint8_t*>(data);
    std::size_t symbolPosition = 0;
    for (int mask = 0x80; mask != 0; mask >>= 1) {
      symbols[symbolPosition++] =
          (bytes[dataPosition] & mask) != 0 ? kRgbOne : kRgbZero;
    }
    return symbolPosition;
  }
  symbols[0] = kRgbReset;
  *done = true;
  return 1;
}

void turnOffBoardRgb() {
  rmt_channel_handle_t channel = nullptr;
  rmt_tx_channel_config_t channelConfig{};
  channelConfig.clk_src = RMT_CLK_SRC_DEFAULT;
  channelConfig.gpio_num = kBoardRgbPin;
  channelConfig.mem_block_symbols = 64;
  channelConfig.resolution_hz = kRgbResolutionHz;
  channelConfig.trans_queue_depth = 1;
  ESP_ERROR_CHECK(rmt_new_tx_channel(&channelConfig, &channel));

  rmt_encoder_handle_t encoder = nullptr;
  rmt_simple_encoder_config_t encoderConfig{};
  encoderConfig.callback = encodeRgbOff;
  ESP_ERROR_CHECK(rmt_new_simple_encoder(&encoderConfig, &encoder));
  ESP_ERROR_CHECK(rmt_enable(channel));

  constexpr std::array<std::uint8_t, 3> off{0, 0, 0};
  rmt_transmit_config_t transmitConfig{};
  ESP_ERROR_CHECK(rmt_transmit(channel, encoder, off.data(), off.size(),
                               &transmitConfig));
  ESP_ERROR_CHECK(rmt_tx_wait_all_done(channel, portMAX_DELAY));
  ESP_ERROR_CHECK(rmt_disable(channel));
  ESP_ERROR_CHECK(rmt_del_encoder(encoder));
  ESP_ERROR_CHECK(rmt_del_channel(channel));
  gpio_reset_pin(kBoardRgbPin);
  ESP_ERROR_CHECK(gpio_set_direction(kBoardRgbPin, GPIO_MODE_OUTPUT));
  ESP_ERROR_CHECK(gpio_set_level(kBoardRgbPin, 0));
}

std::uint64_t nowMs() {
  return static_cast<std::uint64_t>(esp_timer_get_time() / 1000ULL);
}

gasmonitor::GasSnapshot snapshotState(const std::uint64_t now) {
  gasmonitor::GasSnapshot snapshot;
  portENTER_CRITICAL(&gStateMux);
  snapshot = gState.snapshot(now);
  portEXIT_CRITICAL(&gStateMux);
  return snapshot;
}

esp_err_t rootHandler(httpd_req_t* request) {
  httpd_resp_set_type(request, "text/html; charset=utf-8");
  httpd_resp_set_hdr(request, "Cache-Control", "no-store");
  return httpd_resp_send(request, webPageStart,
                         static_cast<ssize_t>(webPageEnd - webPageStart));
}

esp_err_t statusHandler(httpd_req_t* request) {
  const auto now = nowMs();
  const auto snapshot = snapshotState(now);
  const std::string json = gasmonitor::makeStatusJson(snapshot, now);
  httpd_resp_set_type(request, "application/json");
  httpd_resp_set_hdr(request, "Cache-Control", "no-store");
  return httpd_resp_send(request, json.c_str(), json.size());
}

esp_err_t healthHandler(httpd_req_t* request) {
  const bool online = snapshotState(nowMs()).sensorOnline;
  httpd_resp_set_type(request, "application/json");
  httpd_resp_set_hdr(request, "Cache-Control", "no-store");
  if (!online) {
    httpd_resp_set_status(request, "503 Service Unavailable");
    return httpd_resp_sendstr(request, "{\"ok\":false}");
  }
  return httpd_resp_sendstr(request, "{\"ok\":true}");
}

void startHttpServer() {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.max_uri_handlers = 4;
  ESP_ERROR_CHECK(httpd_start(&gHttpServer, &config));

  const httpd_uri_t root{.uri = "/", .method = HTTP_GET,
                         .handler = rootHandler, .user_ctx = nullptr};
  const httpd_uri_t status{.uri = "/api/status", .method = HTTP_GET,
                           .handler = statusHandler, .user_ctx = nullptr};
  const httpd_uri_t health{.uri = "/api/health", .method = HTTP_GET,
                           .handler = healthHandler, .user_ctx = nullptr};
  ESP_ERROR_CHECK(httpd_register_uri_handler(gHttpServer, &root));
  ESP_ERROR_CHECK(httpd_register_uri_handler(gHttpServer, &status));
  ESP_ERROR_CHECK(httpd_register_uri_handler(gHttpServer, &health));
}

void initNvs() {
  esp_err_t result = nvs_flash_init();
  if (result == ESP_ERR_NVS_NO_FREE_PAGES ||
      result == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    result = nvs_flash_init();
  }
  ESP_ERROR_CHECK(result);
}

void startAccessPoint() {
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_netif_create_default_wifi_ap();

  wifi_init_config_t initConfig = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&initConfig));
  ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));

  std::array<std::uint8_t, 6> mac{};
  ESP_ERROR_CHECK(esp_read_mac(mac.data(), ESP_MAC_WIFI_SOFTAP));
  char ssid[33]{};
  std::snprintf(ssid, sizeof(ssid), "GasMonitor-%02X%02X", mac[4], mac[5]);

  wifi_config_t accessPoint{};
  std::strncpy(reinterpret_cast<char*>(accessPoint.ap.ssid), ssid,
               sizeof(accessPoint.ap.ssid) - 1);
  accessPoint.ap.ssid_len = std::strlen(ssid);
  std::strncpy(reinterpret_cast<char*>(accessPoint.ap.password),
               kAccessPointPassword, sizeof(accessPoint.ap.password) - 1);
  accessPoint.ap.channel = 1;
  accessPoint.ap.max_connection = 4;
  accessPoint.ap.authmode = WIFI_AUTH_WPA2_PSK;
  accessPoint.ap.pmf_cfg.capable = true;
  accessPoint.ap.pmf_cfg.required = false;
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &accessPoint));
  ESP_ERROR_CHECK(esp_wifi_start());
  ESP_LOGI(kLogTag, "Wi-Fi ready: SSID=%s password=%s", ssid,
           kAccessPointPassword);
  ESP_LOGI(kLogTag, "Dashboard: http://192.168.4.1");
}

void initGasSensor() {
  const uart_config_t config{.baud_rate = static_cast<int>(kGasBaud),
                             .data_bits = UART_DATA_8_BITS,
                             .parity = UART_PARITY_DISABLE,
                             .stop_bits = UART_STOP_BITS_1,
                             .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
                             .rx_flow_ctrl_thresh = 0,
                             .source_clk = UART_SCLK_DEFAULT,
                             .flags = {}};
  ESP_ERROR_CHECK(uart_driver_install(kGasUart, 1024, 0, 0, nullptr, 0));
  ESP_ERROR_CHECK(uart_param_config(kGasUart, &config));
  ESP_ERROR_CHECK(uart_set_pin(kGasUart, UART_PIN_NO_CHANGE, kGasRxPin,
                               UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
  ESP_LOGI(kLogTag, "TVOC-301 UART ready: RX=GPIO%d baud=%u", kGasRxPin,
           kGasBaud);
}

void gasTask(void*) {
  std::array<std::uint8_t, 64> bytes{};
  while (true) {
    const int count = uart_read_bytes(kGasUart, bytes.data(), bytes.size(),
                                      pdMS_TO_TICKS(200));
    for (int index = 0; index < count; ++index) {
      gasmonitor::GasReading reading;
      const auto result = gParser.push(bytes[static_cast<std::size_t>(index)],
                                       reading);
      if (result == gasmonitor::ParseResult::Frame) {
        portENTER_CRITICAL(&gStateMux);
        gState.observe(reading, nowMs());
        portEXIT_CRITICAL(&gStateMux);
        ESP_LOGI(kLogTag, "TVOC=%u HCHO=%u eCO2=%u", reading.tvocUgM3,
                 reading.hchoUgM3, reading.eco2Ppm);
      } else if (result == gasmonitor::ParseResult::ChecksumError) {
        portENTER_CRITICAL(&gStateMux);
        gState.noteChecksumError();
        portEXIT_CRITICAL(&gStateMux);
      }
    }
  }
}

void drawOled(const gasmonitor::GasSnapshot& snapshot) {
  using gasmonitor::OledFont;

  gOled.clear();
  if (!snapshot.sensorOnline) {
    const int titleX =
        (128 - gasmonitor::oledTextWidth(OledFont::Small, "GAS MONITOR")) /
        2;
    const int waitingX =
        (128 - gasmonitor::oledTextWidth(OledFont::Medium, "WAITING")) / 2;
    const int sensorX =
        (128 - gasmonitor::oledTextWidth(OledFont::Small, "CHECK SENSOR")) /
        2;
    gOled.drawText(titleX, 2, "GAS MONITOR", OledFont::Small);
    gOled.drawHorizontalLine(12);
    gOled.drawText(waitingX, 20, "WAITING", OledFont::Medium);
    gOled.drawText(sensorX, 51, "CHECK SENSOR", OledFont::Small);
    gOled.present();
    return;
  }

  const auto layout = gasmonitor::makeOledLayout(snapshot);
  gOled.drawText(layout.hchoX, 8, layout.hcho.data(), OledFont::Medium);
  gOled.drawText(layout.hchoUnitX, 10, gasmonitor::kHchoUnitLabel,
                 OledFont::Small);
  gOled.drawText(layout.tvocLabelX, 34, gasmonitor::kTvocLabel,
                 OledFont::Small);
  gOled.drawText(layout.eco2LabelX, 34, gasmonitor::kEco2Label,
                 OledFont::Small);
  gOled.drawText(layout.tvocX, 47, layout.tvoc.data(), OledFont::Medium);
  gOled.drawText(layout.eco2X, 47, layout.eco2.data(), OledFont::Medium);
  gOled.present();
}

void oledTask(void*) {
  while (true) {
    if (gOled.ready()) {
      drawOled(snapshotState(nowMs()));
    }
    vTaskDelay(pdMS_TO_TICKS(250));
  }
}

}  // namespace

extern "C" void app_main() {
  turnOffBoardRgb();
  initNvs();
  initGasSensor();
  if (gOled.beginSpi(kOledMosiPin, kOledClockPin, kOledResetPin, kOledDcPin)) {
    ESP_LOGI(kLogTag,
             "SSD1306 SPI ready: MOSI=GPIO%d SCLK=GPIO%d RES=GPIO%d DC=GPIO%d",
             kOledMosiPin, kOledClockPin, kOledResetPin, kOledDcPin);
  } else {
    ESP_LOGE(kLogTag, "SSD1306 SPI initialization failed");
  }
  startAccessPoint();
  startHttpServer();
  xTaskCreate(gasTask, "gas_uart", 4096, nullptr, 6, nullptr);
  xTaskCreate(oledTask, "oled", 4096, nullptr, 4, nullptr);
}

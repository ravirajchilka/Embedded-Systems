#include <cstdint>
#include "esp_log.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "LED_CONTROL";

class Led {
private:
    gpio_num_t pin_;
    bool state_ = false;

    // Helper for consistent error logging
    void log_error(esp_err_t err, const char* operation) const {
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "GPIO %d: %s failed: %s (0x%x)", 
                     pin_, operation, esp_err_to_name(err), err);
        }
    }

public:
    explicit Led(gpio_num_t pin) : pin_(pin) {
        gpio_config_t config = {
            .pin_bit_mask = (1ULL << static_cast<uint32_t>(pin)),
            .mode         = GPIO_MODE_OUTPUT,
            .pull_up_en   = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type    = GPIO_INTR_DISABLE
        };

        esp_err_t ret = gpio_config(&config);
        if (ret != ESP_OK) {
            log_error(ret, "configuration");
            return;
        }

        // Start OFF
        gpio_set_level(pin_, 0);
        ESP_LOGI(TAG, "LED initialized on GPIO %d → OFF", pin_);
    }

    void toggle() {
        state_ = !state_;

        esp_err_t ret = gpio_set_level(pin_, state_ ? 1 : 0);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "LED %d → %s", pin_, state_ ? "ON" : "OFF");
        } else {
            log_error(ret, "set level");
        }
    }
};

extern "C" void app_main(void)
{
    constexpr gpio_num_t LED_PIN = GPIO_NUM_23;

    ESP_LOGI(TAG, "Starting LED blink demo");

    Led led(LED_PIN);

    // Optional quick startup confirmation blink
    for (int i = 0; i < 3; i++) {
        led.toggle();
        vTaskDelay(pdMS_TO_TICKS(200));
    }

    ESP_LOGI(TAG, "Main blink loop started (500ms)");

    while (true) {
        led.toggle();
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
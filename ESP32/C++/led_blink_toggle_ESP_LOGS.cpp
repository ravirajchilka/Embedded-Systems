#include <cstdint>
#include "esp_log.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


static const char *TAG = "LED_BLINK";

class LEDBlinkController {
    private:
        gpio_num_t led_pin;
        bool led_state = false;

        void log_error_helper(esp_err_t err, const char* operation) const {
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "GPIO %d: %s failed: %s (0x%x)", 
                     led_pin, operation, esp_err_to_name(err), err);
            }
        }

        void setup_LED_config() {
            gpio_config_t config = {
                .pin_bit_mask = 1ULL << static_cast<std::uint32_t>(led_pin),
                .mode = GPIO_MODE_OUTPUT,
                .pull_up_en = GPIO_PULLUP_DISABLE,
                .pull_down_en = GPIO_PULLDOWN_DISABLE,
                .intr_type = GPIO_INTR_DISABLE
            };
            esp_err_t ret = gpio_config(&config);
            if(ret != ESP_OK) {
                log_error_helper(ret,"LED setup");
            }

            esp_err_t ret1 = gpio_set_level(led_pin,0);
            log_error_helper(ret1,"initial led value setup");
            ESP_LOGI(TAG, "LED state -=-=-==- %d → %s", led_pin, led_state ? "ON" : "OFF");
        }

    public:

        LEDBlinkController(gpio_num_t led_pin, bool led_state) : led_pin(led_pin),led_state(led_state) {
            setup_LED_config();
        }

        void toggle_LED_states() {
            led_state = !led_state;
            esp_err_t ret = gpio_set_level(led_pin,led_state?1:0);
            if(ret == ESP_OK) {
                ESP_LOGI(TAG, "LED %d → %s", led_pin, led_state ? "ON" : "OFF");
            } else {
                log_error_helper(ret,"set LED level");
            }
        }
};

extern "C" void app_main(void) 
{
    constexpr gpio_num_t led_pin = GPIO_NUM_23;
    bool led_state = false;
    LEDBlinkController controller(led_pin,led_state);
    while(true) {
        controller.toggle_LED_states();
        vTaskDelay(pdMS_TO_TICKS(300));
    }
}



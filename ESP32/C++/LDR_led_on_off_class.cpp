#include <stdio.h>
#include "esp_adc/adc_oneshot.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_check.h"
#include "esp_log.h"

static const char *TAG = "LDR_LED";

class LdrLedController {
public:
    LdrLedController(adc_channel_t ldr_channel, gpio_num_t led_pin, int dark_threshold_raw)
        : ldr_channel_(ldr_channel),
          led_pin_(led_pin),
          dark_threshold_raw_(dark_threshold_raw)
    {
        initADC();
        initLED();
    }

    ~LdrLedController() {
        if (adc_handle_) {
            adc_oneshot_del_unit(adc_handle_);
        }
    }

    void run() {
        int raw;
        while (true) {
            ESP_ERROR_CHECK_WITHOUT_ABORT(adc_oneshot_read(adc_handle_, ldr_channel_, &raw));

            // Optional: very simple voltage estimation (still not calibrated!)
            float voltage = raw * (3.3f / 4095.0f);

            // Print using ESP_LOG - much safer!
            ESP_LOGI(TAG, "LDR raw: %d   ≈ %.2fV   threshold: %d   LED: %s",
                     raw, voltage, dark_threshold_raw_,
                     (raw < dark_threshold_raw_) ? "ON (dark)" : "OFF (bright)");

            // Dark → LED ON, Bright → LED OFF
            gpio_set_level(led_pin_, (raw < dark_threshold_raw_) ? 1 : 0);

            vTaskDelay(pdMS_TO_TICKS(200));
        }
    }

private:
    adc_channel_t ldr_channel_;
    gpio_num_t led_pin_;
    int dark_threshold_raw_;
    adc_oneshot_unit_handle_t adc_handle_ = nullptr;

    void initADC() {
        adc_oneshot_unit_init_cfg_t init_cfg = {};
        init_cfg.unit_id = ADC_UNIT_1;
        init_cfg.ulp_mode = ADC_ULP_MODE_DISABLE;
        ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_cfg, &adc_handle_));

        adc_oneshot_chan_cfg_t chan_cfg = {
            .atten = ADC_ATTEN_DB_12,
            .bitwidth = ADC_BITWIDTH_12,
        };
        ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle_, ldr_channel_, &chan_cfg));
    }

    void initLED() {
        gpio_config_t io_conf = {};
        io_conf.pin_bit_mask = (1ULL << led_pin_);
        io_conf.mode = GPIO_MODE_OUTPUT;
        io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
        io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
        io_conf.intr_type = GPIO_INTR_DISABLE;
        ESP_ERROR_CHECK(gpio_config(&io_conf));

        // Optional: start with LED off
        gpio_set_level(led_pin_, 0);
    }

    // Deleted copy/move to prevent accidental multiple ADC instances
    LdrLedController(const LdrLedController&) = delete;
    LdrLedController& operator=(const LdrLedController&) = delete;
};

// ────────────────────────────────────────────────────────────────
extern "C" void app_main(void)
{
    // Very common pins combinations:
    // ADC1_CH0 = GPIO 36 (VP)   or   ADC1_CH3 = GPIO 39 (VN)
    // LED: almost any GPIO, popular choices: 2, 4, 5, 16, 17, 18, 19, 21, 22, 23, 25, 26, 27...

    constexpr adc_channel_t LDR_CHANNEL = ADC_CHANNEL_0;     // GPIO36
    constexpr gpio_num_t    LED_PIN     = GPIO_NUM_23;
    constexpr int           DARK_THRESHOLD = 2400;           // ← tune this!

    ESP_LOGI("MAIN", "Starting LDR-LED controller...");
    ESP_LOGI("MAIN", "Dark threshold set to: %d", DARK_THRESHOLD);

    LdrLedController controller(LDR_CHANNEL, LED_PIN, DARK_THRESHOLD);

    controller.run();  // blocks forever
}
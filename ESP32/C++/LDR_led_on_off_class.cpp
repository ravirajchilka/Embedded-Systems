#include <stdio.h>
#include "esp_adc/adc_oneshot.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char* TAG = "LDR";

class LDRledController
{
private:
    adc_channel_t ldr_channel_;
    gpio_num_t led_pin_;
    int dark_threshold_raw_;               // changed to int
    adc_oneshot_unit_handle_t adc_handle_ = nullptr;

    void setupLED()
    {
        gpio_config_t config = {
            .pin_bit_mask = (1ULL << led_pin_),
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE
        };
        ESP_ERROR_CHECK(gpio_config(&config));
        gpio_set_level(led_pin_, 0);
    }

    void setupADC()
    {
        adc_oneshot_unit_init_cfg_t init_cfg = {
            .unit_id = ADC_UNIT_1,
            .ulp_mode = ADC_ULP_MODE_DISABLE,
        };

        esp_err_t ret = adc_oneshot_new_unit(&init_cfg, &adc_handle_);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to init ADC unit: %s", esp_err_to_name(ret));
            return;
        }

        adc_oneshot_chan_cfg_t chan_cfg = {
            .atten = ADC_ATTEN_DB_12,
            .bitwidth = ADC_BITWIDTH_12
        };

        ret = adc_oneshot_config_channel(adc_handle_, ldr_channel_, &chan_cfg);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to config ADC channel: %s", esp_err_to_name(ret));
        }
    }

    // Prevent copying
    LDRledController(const LDRledController&) = delete;
    LDRledController& operator=(const LDRledController&) = delete;

public:
    LDRledController(adc_channel_t ldr_channel, gpio_num_t led_pin, int dark_threshold_raw)
        : ldr_channel_(ldr_channel),
          led_pin_(led_pin),                     // ← fixed here!
          dark_threshold_raw_(dark_threshold_raw)
    {
        setupLED();
        setupADC();
    }

    ~LDRledController()
    {
        if (adc_handle_) {
            adc_oneshot_del_unit(adc_handle_);
        }
    }

    void showDetails()
    {
        int raw = 0;
        while (true)
        {
            if (adc_handle_ == nullptr) {
                ESP_LOGE(TAG, "ADC not initialized!");
                vTaskDelay(pdMS_TO_TICKS(1000));
                continue;
            }

            esp_err_t ret = adc_oneshot_read(adc_handle_, ldr_channel_, &raw);
            if (ret == ESP_OK)
            {
                float voltage = raw * (3.3f / 4095.0f);
                ESP_LOGI(TAG, "Raw: %4d   Voltage: %.3f V", raw, voltage);

                // Optional: control LED based on threshold
                // gpio_set_level(led_pin_, (raw > dark_threshold_raw_) ? 1 : 0);
            }
            else
            {
                ESP_LOGE(TAG, "ADC read failed: %s", esp_err_to_name(ret));
            }

            vTaskDelay(pdMS_TO_TICKS(500));   // ← very important!
        }
    }
};

extern "C" void app_main(void)
{
    constexpr adc_channel_t LDR_CHANNEL    = ADC_CHANNEL_0;   // GPIO36 on most ESP32
    constexpr gpio_num_t    LED_PIN        = GPIO_NUM_23;
    constexpr int           DARK_THRESHOLD = 2400;

    ESP_LOGI("MAIN", "Starting LDR controller...");

    LDRledController controller(LDR_CHANNEL, LED_PIN, DARK_THRESHOLD);
    controller.showDetails();

    // Note: this function will never return because of infinite loop
}

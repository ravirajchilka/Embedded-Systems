#include <cstdio>
#include "esp_adc/adc_oneshot.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_check.h"

class LdrLedController {
public:
    // Constructor: configure ADC and LED
    LdrLedController(adc_channel_t ldr_channel, gpio_num_t led_pin, int threshold)
        : ldr_channel_(ldr_channel), led_pin_(led_pin), threshold_(threshold)
    {
        initADC();
        initLED();
    }

    // Main loop for reading and controlling LED
    void run()
    {
        while (true) {
            int raw = 0;
            ESP_ERROR_CHECK(adc_oneshot_read(adc_handle_, ldr_channel_, &raw));
            float voltage = raw * (3.3f / 4095.0f);

            printf("LDR raw=%d, V=%.2f (Dark<%d, Bright>%d)\n",
                   raw, voltage, threshold_, threshold_);

            // Dark → LED ON, Bright → LED OFF
            gpio_set_level(led_pin_, raw < threshold_ ? 1 : 0);

            vTaskDelay(pdMS_TO_TICKS(200));
        }
    }

private:
    adc_channel_t ldr_channel_;
    gpio_num_t led_pin_;
    int threshold_;
    adc_oneshot_unit_handle_t adc_handle_ = nullptr;

    void initADC()
    {
        adc_oneshot_unit_init_cfg_t init_config = {
            .unit_id = ADC_UNIT_1,
        };
        ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle_));

        adc_oneshot_chan_cfg_t channel_config = {
            .atten = ADC_ATTEN_DB_12,
            .bitwidth = ADC_BITWIDTH_12,
        };
        ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle_, ldr_channel_, &channel_config));
    }

    void initLED()
    {
        gpio_config_t led_cfg = {};
        led_cfg.pin_bit_mask = (1ULL << static_cast<uint32_t>(led_pin_));
        led_cfg.mode = GPIO_MODE_OUTPUT;
        led_cfg.pull_up_en = GPIO_PULLUP_DISABLE;
        led_cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
        led_cfg.intr_type = GPIO_INTR_DISABLE;
        gpio_config(&led_cfg);
    }
};

// ---- Entry point ----
extern "C" void app_main(void)
{
    LdrLedController controller(ADC_CHANNEL_0, GPIO_NUM_23, 1000);
    controller.run();  // Run forever inside the class
}

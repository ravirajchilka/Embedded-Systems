#include <cstdio>

extern "C" {
#include "esp_adc/adc_oneshot.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_check.h"
}

// ADC: use GPIO36 = ADC1_CHANNEL_0
static adc_oneshot_unit_handle_t adc_handle;
static constexpr adc_channel_t LDR_CHANNEL = ADC_CHANNEL_0; // GPIO36
static constexpr gpio_num_t LED_PIN = GPIO_NUM_23;

extern "C" void app_main(void)
{
    // ---- Configure ADC1 (one-shot mode) ----
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle));

    // Configure the channel
    adc_oneshot_chan_cfg_t channel_config = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, LDR_CHANNEL, &channel_config));

    // ---- Configure LED GPIO (GPIO23) ----
    gpio_config_t led_cfg = {};
    led_cfg.pin_bit_mask = (1ULL << static_cast<uint32_t>(LED_PIN));
    led_cfg.mode = GPIO_MODE_OUTPUT;
    led_cfg.pull_up_en = GPIO_PULLUP_DISABLE;
    led_cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
    led_cfg.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&led_cfg);

    const int threshold = 1000; // Tune for your LDR: LOW=dark, HIGH=bright

    while (true) {
        int raw = 0;
        ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, LDR_CHANNEL, &raw));
        float voltage = raw * (3.3f / 4095.0f);

        printf("LDR raw=%d, V=%.2f (Dark<2000, Bright>2000)\n", raw, voltage);

        // LDR: LOW values = DARK (turn LED ON), HIGH values = BRIGHT (LED OFF)
        if (raw < threshold) {
            gpio_set_level(LED_PIN, 1);  // Dark → LED ON
        } else {
            gpio_set_level(LED_PIN, 0);  // Bright → LED OFF
        }

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

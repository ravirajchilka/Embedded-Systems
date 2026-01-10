#include <cstdint>
#include <iostream>

extern "C" {
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
}

class Led {
public:
    explicit Led(gpio_num_t pin)
        : pin_(pin), state_(false)
    {
        gpio_config_t cfg{};
        cfg.pin_bit_mask = (1ULL << static_cast<uint32_t>(pin_));
        cfg.mode = GPIO_MODE_OUTPUT;
        cfg.pull_up_en = GPIO_PULLUP_DISABLE;
        cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
        cfg.intr_type = GPIO_INTR_DISABLE;
        gpio_config(&cfg);

         /*
            gpio_config_t cfg = {
            .pin_bit_mask = (1ULL << static_cast<uint32_t>(pin_)),
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE
            };
            gpio_config(&cfg);
        */

        off();
    }

    void on() {
        state_ = true;
        gpio_set_level(pin_, 1);
    }

    void off() {
        state_ = false;
        gpio_set_level(pin_, 0);
    }

    void toggle() {
        state_ = !state_;
        gpio_set_level(pin_, state_ ? 1 : 0);
        std::cout << "level" << state_ << std::endl;
    }

private:
    gpio_num_t pin_;
    bool state_;
};

extern "C" void app_main(void)
{
    constexpr gpio_num_t LED_PIN = GPIO_NUM_23;
    Led led(LED_PIN);

    for (;;) {
        led.toggle();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

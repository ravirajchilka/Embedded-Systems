#include<cstdint>
#include <iostream>

extern "C" {
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
}

class Led {

	private:
    	gpio_num_t pin;
		bool state;	

	public:
		explicit Led(gpio_num_t pin) : pin(pin), state(false) {

			gpio_config_t config = {
				.pin_bit_mask = 1ULL << static_cast<std::uint32_t>(pin),
				.mode = GPIO_MODE_OUTPUT,
				.pull_up_en = GPIO_PULLUP_DISABLE,
				.pull_down_en = GPIO_PULLDOWN_DISABLE,
				.intr_type = GPIO_INTR_DISABLE
			};
			gpio_config(&config);
			
		}
		
		void toggle_led() {
			state = !state;
			gpio_set_level(pin,state?1:0);
			std::cout << "pin state" << state << std::endl;
		}
	};

	extern "C" void app_main(void) 
	{
		constexpr gpio_num_t led_pin = GPIO_NUM_23;
		Led led(led_pin);
		while(true) {
			led.toggle_led();
			vTaskDelay(pdMS_TO_TICKS(500));
		}
	}


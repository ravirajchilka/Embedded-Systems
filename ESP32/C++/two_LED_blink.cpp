#include <cstdint>
#include <esp_log.h>
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <array>

const static char *TAG = "TWO LEDs TOGGLE";

class LEDsToggle {

	private:
		std::array<gpio_num_t,2U> pinsArray;
		gpio_num_t ledRED;
		gpio_num_t ledBLUE;
		bool ledStateRED;
		bool ledStateBLUE;

		void errorLogHelper(esp_err_t err, const char *operation) {
			for(const auto pin: pinsArray) {
				ESP_LOGE(TAG,"GPIO %d: %s failed: %s (0x%x)", static_cast<std::uint32_t>(pin), operation,esp_err_to_name(err),err);
			}
		}

		std::uint64_t makeMask() {
			std::uint64_t mask = 0;
			for(const auto pin:pinsArray) {
				mask |= 1ULL << static_cast<std::uint16_t>(pin);
			}
			return mask;
		};

		void LEDsSetup() {
			gpio_config_t config = {
				.pin_bit_mask = makeMask(),
				.mode = GPIO_MODE_OUTPUT,
				.pull_up_en = GPIO_PULLUP_DISABLE,
				.pull_down_en = GPIO_PULLDOWN_DISABLE,
				.intr_type = GPIO_INTR_DISABLE
			};

			esp_err_t err = gpio_config(&config);
			if(err != ESP_OK) {
				errorLogHelper(err,"leds config");
			}

			esp_err_t errLevel1 = gpio_set_level(pinsArray[0], ledStateRED ? 1 : 0);
			esp_err_t errLevel2 = gpio_set_level(pinsArray[1], ledStateBLUE ? 1 : 0);

			if (errLevel1 != ESP_OK) {
				errorLogHelper(errLevel1, "set LED RED level");
			}
			if (errLevel2 != ESP_OK) {
				errorLogHelper(errLevel2, "set LED BLUE level");
			};
		}

	public:
		LEDsToggle(std::array<gpio_num_t,2U> pinsArray, bool ledStateRED, bool ledStateBLUE):
		pinsArray(pinsArray),      
		ledRED(pinsArray[0]),       
		ledBLUE(pinsArray[1]),     
		ledStateRED(ledStateRED),  
		ledStateBLUE(ledStateBLUE) 
		{
			LEDsSetup();
		}
				
		void toggleLEDred() {
			ledStateRED = !ledStateRED;
			esp_err_t redLEDerr = gpio_set_level(pinsArray[0],ledStateRED?1:0);
			if(redLEDerr != ESP_OK) {
				errorLogHelper(redLEDerr,"toggle red led");
			}
		}

		void toggleLEDblue() {
			ledStateBLUE = !ledStateBLUE;
			esp_err_t blueLEDerr = gpio_set_level(pinsArray[1],ledStateBLUE?1:0);
			if(blueLEDerr != ESP_OK) {
				errorLogHelper(blueLEDerr,"toggle blue led");
			}
		}
};

extern "C" void app_main(void)
{
	auto pinsArray = std::array<gpio_num_t,2>{GPIO_NUM_23,GPIO_NUM_22};
	LEDsToggle ledsController(pinsArray,false,false);

	while(true) {
		ledsController.toggleLEDblue();
		vTaskDelay(pdMS_TO_TICKS(1000));
		ledsController.toggleLEDblue();

		ledsController.toggleLEDred();
		vTaskDelay(pdMS_TO_TICKS(1000));
		ledsController.toggleLEDred();
	}

}




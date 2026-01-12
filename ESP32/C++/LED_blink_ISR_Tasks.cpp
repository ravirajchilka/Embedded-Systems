#include <cstdint>
#include "esp_log.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "LED_BLINK";

constexpr gpio_num_t BUTTON_PIN = GPIO_NUM_22;  // ← change if needed

class LEDBlinkController {
private:
    gpio_num_t led_pin;
    bool led_state = false;
    static TaskHandle_t blinkControlTaskHandle;
    static volatile bool should_blink;

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
            log_error_helper(ret, "LED setup");
        }

        esp_err_t ret1 = gpio_set_level(led_pin, 0);
        log_error_helper(ret1, "initial led value setup");
        ESP_LOGI(TAG, "LED state -=-=-==- %d → %s", led_pin, "on");
    }

    // ISR - must be static because it's called from C context
    static void IRAM_ATTR button_isr_handler(void* arg) {
        BaseType_t higher = pdFALSE;
        vTaskNotifyGiveFromISR(blinkControlTaskHandle, &higher);
        portYIELD_FROM_ISR(higher);
    }

    // Button handling task - static because created from constructor
    static void button_control_task(void *pvParameters) {
        while (true) {
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
            
            vTaskDelay(pdMS_TO_TICKS(50));  // basic debounce
            
            if (gpio_get_level(BUTTON_PIN) == 0) {
                should_blink = !should_blink;
                ESP_LOGI(TAG, "Button pressed → blinking %s", 
                         should_blink ? "ENABLED" : "PAUSED");
            }
        }
    }

    void setup_button_interrupt() {
        gpio_config_t io_conf = {};
        io_conf.pin_bit_mask = (1ULL << BUTTON_PIN);
        io_conf.mode         = GPIO_MODE_INPUT;
        io_conf.pull_up_en   = GPIO_PULLUP_ENABLE;
        io_conf.intr_type    = GPIO_INTR_NEGEDGE;  // falling edge = press
        esp_err_t ret = gpio_config(&io_conf);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Button GPIO config failed");
            return;
        }

        // Install ISR service (only once)
        ret = gpio_install_isr_service(0);
        if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {  // already installed is ok
            ESP_LOGE(TAG, "ISR service install failed");
            return;
        }

        // Add handler
        gpio_isr_handler_add(BUTTON_PIN, button_isr_handler, NULL);

        // Create the button handling task
        xTaskCreate(button_control_task, "btn_ctrl", 2048, NULL, 10, &blinkControlTaskHandle);
    }

public:
    LEDBlinkController(gpio_num_t led_pin_param, bool initial_led_state) 
        : led_pin(led_pin_param), led_state(initial_led_state) 
    {
        setup_LED_config();
        setup_button_interrupt();  // ← button + interrupt + task setup here
        ESP_LOGI(TAG, "Button on GPIO %d configured to pause/resume blinking", BUTTON_PIN);
    }

    void toggle_LED_states() {
        if (!should_blink) return;

        led_state = !led_state;
        esp_err_t ret = gpio_set_level(led_pin, led_state ? 1 : 0);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "LED state -=-=-==- %d → %d", led_pin, led_state);
        } else {
            log_error_helper(ret, "set LED level");
        }
    }
};

// Static members initialization
TaskHandle_t LEDBlinkController::blinkControlTaskHandle = NULL;
volatile bool LEDBlinkController::should_blink = true;

extern "C" void app_main(void) 
{
    constexpr gpio_num_t led_pin = GPIO_NUM_23;
    bool led_state = false;

    LEDBlinkController controller(led_pin, led_state);

    while(true) {
        controller.toggle_LED_states();
        vTaskDelay(pdMS_TO_TICKS(300));
    }
}

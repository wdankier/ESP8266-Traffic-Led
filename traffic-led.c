#include <stdio.h>
#include <espressif/esp_wifi.h>
#include <espressif/esp_sta.h>
#include <esp/uart.h>
#include <esp8266.h>
#include <FreeRTOS.h>
#include <task.h>

#include <homekit/homekit.h>
#include <homekit/characteristics.h>
#include <wifi_config.h>
#include <button.h>



const int led_green_gpio = 2;
const int led_red_gpio = 4;
const int button_gpio = 0;
bool led_green_on = false;
bool led_red_on = false;

void led_green_write(bool on) {
    gpio_write(led_green_gpio, on ? 0 : 1);
}

void led_red_write(bool on) {
    gpio_write(led_red_gpio, on ? 0 : 1);
}

void led_init() {
    gpio_enable(led_green_gpio, GPIO_OUTPUT);
    led_green_write(led_green_on);
    gpio_enable(led_red_gpio, GPIO_OUTPUT);
    led_red_write(led_red_on);
}

void led_identify_task(void *_args) {
    for (int i=0; i<3; i++) {
        for (int j=0; j<2; j++) {
            led_green_write(true);
            led_red_write(false);
            vTaskDelay(100 / portTICK_PERIOD_MS);
            led_green_write(false);
            led_red_write(true);
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }

        vTaskDelay(250 / portTICK_PERIOD_MS);
    }

    led_green_write(led_green_on);
    led_red_write(led_red_on);

    vTaskDelete(NULL);
}

void led_identify(homekit_value_t _value) {
    printf("LED identify\n");
    xTaskCreate(led_identify_task, "LED identify", 128, NULL, 2, NULL);
}

homekit_value_t led_green_on_get() {
    return HOMEKIT_BOOL(led_green_on);
}

homekit_value_t led_red_on_get() {
    return HOMEKIT_BOOL(led_red_on);
}

void led_green_on_set(homekit_value_t value) {
    if (value.format != homekit_format_bool) {
        printf("Invalid value format: %d\n", value.format);
        return;
    }

    led_green_on = value.bool_value;
    led_green_write(led_green_on);
}

void led_red_on_set(homekit_value_t value) {
    if (value.format != homekit_format_bool) {
        printf("Invalid value format: %d\n", value.format);
        return;
    }

    led_red_on = value.bool_value;
    led_red_write(led_red_on);
}

homekit_characteristic_t led_green_characteristic = HOMEKIT_CHARACTERISTIC_(
    ON, false,
    .getter=led_green_on_get,
    .setter=led_green_on_set
);

homekit_characteristic_t led_red_characteristic = HOMEKIT_CHARACTERISTIC_(
    ON, false,
    .getter=led_red_on_get,
    .setter=led_red_on_set
);

void button_callback(button_event_t event, void *context) {
    switch (event) {
        case button_event_single_press:
            printf("single press\n");
            led_green_on = !led_green_on;
            led_green_write(led_green_on);
            homekit_characteristic_notify(&led_green_characteristic, led_green_on_get());
            break;
        case button_event_double_press:
            printf("double press\n");
            led_green_on = !led_green_on;
            led_green_write(led_green_on);
            homekit_characteristic_notify(&led_green_characteristic, led_green_on_get());
            break;
        case button_event_long_press:
            printf("long press\n");
            led_green_on = !led_green_on;
            led_green_write(led_green_on);
            homekit_characteristic_notify(&led_green_characteristic, led_green_on_get());
            break;
        default:
            printf("unknown button event: %d\n", event);
    }
}


homekit_accessory_t *accessories[] = {
    HOMEKIT_ACCESSORY(.id=1, .category=homekit_accessory_category_lightbulb, .services=(homekit_service_t*[]){
        HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]){
            HOMEKIT_CHARACTERISTIC(NAME, "Traffic LED"),
            HOMEKIT_CHARACTERISTIC(MANUFACTURER, "Tech From The Shed"),
            HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "137A2BABF19D"),
            HOMEKIT_CHARACTERISTIC(MODEL, "Traffic Led"),
            HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "0.0.1"),
            HOMEKIT_CHARACTERISTIC(IDENTIFY, led_identify),
            NULL
        }),
        HOMEKIT_SERVICE(LIGHTBULB, .primary=true, .characteristics=(homekit_characteristic_t*[]){
            HOMEKIT_CHARACTERISTIC(NAME, "Green LED"),
            &led_green_characteristic,
            NULL
        }),
        HOMEKIT_SERVICE(LIGHTBULB, .primary=true, .characteristics=(homekit_characteristic_t*[]){
            HOMEKIT_CHARACTERISTIC(NAME, "Red LED"),
            &led_red_characteristic,
            NULL
        }),
        NULL
    }),
    NULL
};

homekit_server_config_t config = {
    .accessories = accessories,
    .password = "111-11-111"
};

void on_wifi_ready() {
}

void user_init(void) {
    uart_set_baud(0, 115200);

    button_config_t button_config = BUTTON_CONFIG(
        button_active_low,
        .max_repeat_presses=2,
        .long_press_time=1000,
    );
    if (button_create(button_gpio, button_config, button_callback, NULL)) {
        printf("Failed to initialize button\n");
    }

    led_init();
    homekit_server_init(&config);
}

#include <stdio.h>
#include <stdbool.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <ultrasonic.h>
#include <esp_err.h>
#include "driver/gpio.h"
#include "sdkconfig.h"

#define MAX_DISTANCE_CM 0.8 // 4m max

// Ultrasonic sensor pins
#define TRIGGER_GPIO 21
#define ECHO_GPIO 47
#define YELLOW_LED 2

// Motor driver outputs
#define MOTOR_LEFT_A 35
#define MOTOR_LEFT_B 36
#define MOTOR_RIGHT_A 37
#define MOTOR_RIGHT_B 38

#define IR_SENSOR_BUTTON 39

void setup();
void idle();
float read_ultrasonic();
void target_detected();

ultrasonic_sensor_t sensor = {
    .trigger_pin = TRIGGER_GPIO,
    .echo_pin = ECHO_GPIO};

gpio_config_t io_conf = {
    .pin_bit_mask = (1ULL << IR_SENSOR_BUTTON), // Select GPIO 4
    .mode = GPIO_MODE_INPUT,                    // Set as input
    .pull_up_en = GPIO_PULLUP_ENABLE,           // Enable internal pull-up
    .pull_down_en = GPIO_PULLDOWN_DISABLE,      // Disable pull-down
    .intr_type = GPIO_INTR_DISABLE              // Disable interrupts
};

bool idle_flag = 0;

void ultrasonic_test(void *pvParameters)
{

    gpio_reset_pin(YELLOW_LED);
    gpio_set_direction(YELLOW_LED, GPIO_MODE_OUTPUT);

    ultrasonic_sensor_t sensor = {
        .trigger_pin = TRIGGER_GPIO,
        .echo_pin = ECHO_GPIO};

    ultrasonic_init(&sensor);

    while (true)
    {
        float distance;
        esp_err_t res = ultrasonic_measure(&sensor, MAX_DISTANCE_CM, &distance);
        if (res != ESP_OK)
        {
            printf("Error %d: ", res);
            switch (res)
            {
            case ESP_ERR_ULTRASONIC_PING:
                printf("Cannot ping (device is in invalid state)\n");
                break;
            case ESP_ERR_ULTRASONIC_PING_TIMEOUT:
                printf("Ping timeout (no device found)\n");
                break;
            case ESP_ERR_ULTRASONIC_ECHO_TIMEOUT:
                printf("Target not found!\n");
                break;
            default:
                printf("%s\n", esp_err_to_name(res));
            }
        }
        else
            printf("Distance: %0.04f cm\n", distance * 100);
        if (distance < 0.36)
        {
            gpio_set_level(YELLOW_LED, 1);
            printf("Target identified");
        }
        else
            gpio_set_level(YELLOW_LED, 0);

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

float read_ultrasonic()
{

    float distance;
    ultrasonic_measure(&sensor, MAX_DISTANCE_CM, &distance);

    vTaskDelay(pdMS_TO_TICKS(250));
    printf("Distance: %0.04f cm\n", distance * 100);
    return distance * 100;
}

void idle()
{
    // spin around
    gpio_set_level(YELLOW_LED, 0);
    idle_flag = 1;
    return;
}

/*
If target is found drive towards it , if a line is detected use intterrupt to back up to the middle of
circle and spin around (idle)
*/
void target_detected()
{
    // Drive forward;
    while (read_ultrasonic() < 36)
    {
        gpio_set_level(YELLOW_LED, 1);
    }

    idle_flag = 0;
    return;
}

/*If no target is found spin around*/

/*If line is detected back up to middle and go to idle as a interrupt*/
void line_detected()
{
}

void setup()
{

    // Motor drivers setup
    gpio_reset_pin(MOTOR_LEFT_A);
    gpio_set_direction(MOTOR_LEFT_A, GPIO_MODE_OUTPUT);

    gpio_reset_pin(MOTOR_LEFT_B);
    gpio_set_direction(MOTOR_LEFT_B, GPIO_MODE_OUTPUT);

    gpio_reset_pin(MOTOR_RIGHT_A);
    gpio_set_direction(MOTOR_RIGHT_A, GPIO_MODE_OUTPUT);

    gpio_reset_pin(MOTOR_RIGHT_B);
    gpio_set_direction(MOTOR_RIGHT_B, GPIO_MODE_OUTPUT);

    // Ultrasonic sensor setup
    gpio_reset_pin(YELLOW_LED);
    gpio_set_direction(YELLOW_LED, GPIO_MODE_OUTPUT);

    ultrasonic_init(&sensor);
    gpio_config(&io_conf);
}

void app_main()
{
    /*
    setup();

    while (true)
    {
        if (!idle_flag)
        {
            idle();
        }

        if (read_ultrasonic() < 36)
        {
            target_detected();
        }
    }
    */

    while (1)
    {
        // Read GPIO level (current state)
        int level = gpio_get_level(IR_SENSOR_BUTTON);
        if (level == 0)
        {
            printf("Button Pressed!\n");
        }
        else
        {
            printf("Button Released...\n");
        }
        vTaskDelay(200 / portTICK_PERIOD_MS); // Delay 200ms
    }

    // xTaskCreate(ultrasonic_test, "ultrasonic_test", configMINIMAL_STACK_SIZE * 3, NULL, 5, NULL);
}
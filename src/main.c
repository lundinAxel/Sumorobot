#include <stdio.h>
#include <stdbool.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <ultrasonic.h>
#include <esp_err.h>
#include "driver/gpio.h"
#include "sdkconfig.h"

#include "esp_attr.h"
#include "driver/mcpwm.h"
#include "soc/mcpwm_periph.h"

#define MAX_DISTANCE_CM 0.8 // 4m max

// Ultrasonic sensor pins
#define TRIGGER_GPIO 21
#define ECHO_GPIO 47
#define YELLOW_LED 2

// Motor driver outputs
#define GPIO_PWM0A_OUT 35
#define GPIO_PWM0B_OUT 36

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

mcpwm_config_t pwm_config = {
    .frequency = 1000, // frequency = 500Hz,
    .cmpr_a = 0,       // duty cycle of PWMxA = 0
    .cmpr_b = 0,       // duty cycle of PWMxb = 0
    .counter_mode = MCPWM_UP_COUNTER,
    .duty_mode = MCPWM_DUTY_MODE_0};

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
    printf("Initializing mcpwm gpio\n");
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, GPIO_PWM0A_OUT);
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0B, GPIO_PWM0B_OUT);

    // Ultrasonic sensor setup
    printf("Initializing ultrasonic gpio\n");
    gpio_reset_pin(YELLOW_LED);
    gpio_set_direction(YELLOW_LED, GPIO_MODE_OUTPUT);

    ultrasonic_init(&sensor);
    gpio_config(&io_conf);

    mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config)
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

void motor_forward(mcwpm_unit_t mcpwm_num, mcpwm_timer_t timer_num, float duty_cycle)
{
    mcpwm_set_signal_low(mcpwm_num, timer_num, MCPWM_OPR_B);
    mcpwm_set_duty(mcpwm_num, timer_num, MCPWM_OPR_A, duty_cycle);
    mcpwm_set_duty_type(mcpwm_num, timer_num, MCPWM_OPR_A, MCPWM_DUTY_MODE_0);
}

void motor_stop(mcpwm_unit_t mcpwm_num, mcpwm_timer_t timer_num)
{
    mcpwm_set_signal_low(mcpwm_num, timer_num, MCPWM_OPR_A);
    mcpwm_set_signal_low(mcpwm_num, timer_num, MCPWM_OPR_B);
}

void motor_backwards(mcpwm_unit_t mcwpm_num, mcpwm_timer_t timer_num, float duty_cycle)
{
}

void motor_turn_right(mcpwm_unit_t mcwpm_num, mcpwm_timer_t timer_num, float duty_cycle)
{
}
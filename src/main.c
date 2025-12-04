#include <stdio.h>
#include <stdbool.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <ultrasonic.h>
#include <esp_err.h>
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_intr_alloc.h"
#include "esp_log.h"

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
        .intr_type = GPIO_INTR_NEGEDGE,     // interrupt på fallande flank
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << IR_SENSOR_BUTTON),
        .pull_down_en = 0,
        .pull_up_en = 1              // Disable interrupts
};

mcpwm_config_t pwm_config = {
    .frequency = 1000, // frequency = 500Hz,
    .cmpr_a = 0,       // duty cycle of PWMxA = 0
    .cmpr_b = 0,       // duty cycle of PWMxb = 0
    .counter_mode = MCPWM_UP_COUNTER,
    .duty_mode = MCPWM_DUTY_MODE_0};

bool idle_flag = 0;
volatile bool button_pressed = false;

void IRAM_ATTR button_isr_handler(void* arg)
{
    // Sätt bara en flagga (snabbt!)
    button_pressed = true;

}

void ultrasonic_test(void *pvParameters)
{
    //esp_intr_alloc_flags_t flags = ESP_INTR_FLAG_LEVEL1;

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

    
    printf("Distance: %0.04f cm\n", distance * 100);
    vTaskDelay(pdMS_TO_TICKS(250));
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
    while (read_ultrasonic() < 36 && !button_pressed)
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
    // Back up to middle of circle
    gpio_set_level(YELLOW_LED, 0);
    return;
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
<<<<<<< HEAD

    mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config)
=======
    gpio_install_isr_service(0);
    gpio_isr_handler_add(IR_SENSOR_BUTTON, button_isr_handler, NULL);
    
    
>>>>>>> c3ef1ed1128d03ef7fd3ae7c37160d63fd6a74bf
}

void app_main()
{

    setup();


    while (true){
        


         if (!idle_flag)
        {
            idle();
        }

        if (read_ultrasonic() < 36)
        {
            target_detected();
        }
    
        if (button_pressed) 
        {
            line_detected();
            button_pressed = false;  // nollställ flaggan

            printf("Line detected!\n");
        }
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
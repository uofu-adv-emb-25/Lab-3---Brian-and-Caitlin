#include <FreeRTOS.h>
#include <pico/multicore.h>
#include <pico/cyw43_arch.h>
#include <stdio.h>
#include <pico/stdlib.h>
#include <stdint.h>
#include <unity.h>
#include "unity_config.h"
#include <semphr.h>
#include <projdefs.h>



void setUp(void) {}

void tearDown(void) {}

int counter;
int on;

void thread_counter(){
    counter = counter + 1;
    printf("hello world from %s! Count %d\n", "thread", counter);
}

// void main_counter(){
//     printf("hello world from %s! Count %d\n", "main", counter++);
// }

// void side_thread(void *params)
// {
// 	while (1) {
//         vTaskDelay(100);
//         if (semaphore != NULL){
//             if(xSemaphoreTake(semaphore, portMAX_DELAY) == pdTRUE) {
//                 thread_counter();
//                 // counter = counter + 1;
//                 // printf("hello world from %s! Count %d\n", "thread", counter);
//             }
//             else{
//                 printf("semaphore was not pdTRUE");
//             }
//             xSemaphoreGive(semaphore);
//         }
//         else{
//             printf("semaphore was not NULL");
//         }
// 	}
// }

// void main_thread(void *params)
// {
// 	while (1) {
//         cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, on);
//         vTaskDelay(100);
//         xSemaphoreTake(semaphore, portMAX_DELAY); {
// 		    main_counter();
//             // printf("hello world from %s! Count %d\n", "main", counter++);
//         }
//         xSemaphoreGive(semaphore);
//         on = !on;
// 	}
// }

int counter;

int test_semaphore(SemaphoreHandle_t semaphore){
    if(xSemaphoreTake(semaphore, portMAX_DELAY) == pdTRUE) {
        counter++;
        printf("semaphore was pdTRUE");
        xSemaphoreGive(semaphore);
        return pdTRUE;
    }
    else{
        printf("semaphore was not pdTRUE");
        return pdFALSE;
    }
}

void test_lock(void){
    SemaphoreHandle_t semaphore = xSemaphoreCreateCounting(1,1);
    counter = 0;
    int lock_result = test_semaphore(semaphore);

    TEST_ASSERT_EQUAL_INT(pdTRUE, lock_result);
    TEST_ASSERT_EQUAL_INT(1, counter);  
}


int main (void)
{
    stdio_init_all();
    while(1){
    sleep_ms(5000); // Give time for TTY to attach.
    printf("Start tests\n");
    UNITY_BEGIN();
    RUN_TEST(test_lock);
    sleep_ms(5000);
    UNITY_END();
    }
}

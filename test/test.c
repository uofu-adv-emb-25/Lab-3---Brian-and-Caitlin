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

SemaphoreHandle_t semA;
SemaphoreHandle_t semB;

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


void task_1(void *params) 
{
    xSemaphoreTake(semA, portMAX_DELAY);
    vTaskDelay(500);
    xSemaphoreTake(semB, portMAX_DELAY);
    vTaskDelete(NULL);
}

void task_2(void *params) 
{
    xSemaphoreTake(semB, portMAX_DELAY);
    vTaskDelay(500);
    xSemaphoreTake(semA, portMAX_DELAY);
    vTaskDelete(NULL);
}

void test_deadlock(void) 
{
    printf("Starting deadlock test.\n");
    semA = xSemaphoreCreateMutex();
    semB = xSemaphoreCreateMutex();

    TEST_ASSERT_NOT_NULL(semA);
    TEST_ASSERT_NOT_NULL(semB);

    TaskHandle_t task1, task2;
    TaskStatus_t status1, status2;

    printf("Creating Tasks.\n");
    xTaskCreate(task_1, "Task1", 256, NULL, tskIDLE_PRIORITY + 1, &task1);
    xTaskCreate(task_2, "Task2", 256, NULL, tskIDLE_PRIORITY + 1, &task2);

    printf("Tasks Created.\n");

    vTaskDelay(10);  // Hangs up here
                     // If you comment out the delay, it runs all the way
                     // through, but will fail.

    printf("Getting Task info.\n");
    vTaskGetInfo(task1, &status1, pdTRUE, eInvalid);
    vTaskGetInfo(task2, &status2, pdTRUE, eInvalid);
    printf("Got Tasks info.\n");
    eTaskState state1 = status1.eCurrentState;
    eTaskState state2 = status2.eCurrentState;
    printf("Second Tests.\n");

    TEST_ASSERT_EQUAL_MESSAGE(eBlocked, state1, "Task 1 is not blocked");
    TEST_ASSERT_EQUAL_MESSAGE(eBlocked, state2, "Task 2 is not blocked");

    printf("Second Tests passed.\n");
    vTaskSuspend(task1);
    vTaskSuspend(task2);
    vTaskDelete(task1);
    vTaskDelete(task2);
    vSemaphoreDelete(semA);
    vSemaphoreDelete(semB);
    printf("Deadlock Test completed.\n");
}


int main (void)
{
    stdio_init_all();
    while(1){
    sleep_ms(5000); // Give time for TTY to attach.
    printf("Start tests\n");
    UNITY_BEGIN();
    RUN_TEST(test_lock);
    RUN_TEST(test_deadlock);
    sleep_ms(5000);
    UNITY_END();
    }
}

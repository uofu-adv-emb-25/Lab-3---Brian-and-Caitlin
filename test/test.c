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

#define MAIN_TASK_PRIORITY      ( tskIDLE_PRIORITY + 2UL )
#define MAIN_TASK_STACK_SIZE    configMINIMAL_STACK_SIZE
#define SIDE_TASK_PRIORITY      ( tskIDLE_PRIORITY + 1UL )
#define SIDE_TASK_STACK_SIZE    configMINIMAL_STACK_SIZE

void setUp(void) {}

void tearDown(void) {}

typedef struct deadlockArgs {
    SemaphoreHandle_t A;
    SemaphoreHandle_t B;
    int counter;
} deadlockArgs;

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


void taskA(void *arg)
{
    printf ("Task A.\n");
    deadlockArgs *args = (deadlockArgs *) arg;

    args->counter += xSemaphoreTake(args->A, portMAX_DELAY);

    vTaskDelay(500);

    args->counter += xSemaphoreTake(args->B, portMAX_DELAY);

    xSemaphoreGive(args->B);
    xSemaphoreGive(args->A);

    vTaskSuspend(NULL);
}

void taskB(void *arg)
{
    printf ("Task B.\n");
    deadlockArgs *args = (deadlockArgs *) arg;

    args->counter += xSemaphoreTake(args->B, portMAX_DELAY);
    args->counter += xSemaphoreTake(args->A, portMAX_DELAY);

    xSemaphoreGive(args->A);
    xSemaphoreGive(args->B);

    vTaskSuspend(NULL);
}

void test_deadlock(void)
{
    printf ("Starting deadlock test.\n");
    SemaphoreHandle_t semaphore_A = xSemaphoreCreateMutex();
    SemaphoreHandle_t semaphore_B = xSemaphoreCreateMutex();

    deadlockArgs argsA = { semaphore_A, semaphore_B, 100 };
    deadlockArgs argsB = { semaphore_A, semaphore_B, 200 };

   
    TaskHandle_t task_A, task_B;

    printf ("Creating Tasks.\n");
    xTaskCreate(taskA, "taskA",
                SIDE_TASK_STACK_SIZE, (void *)&argsA,
                SIDE_TASK_PRIORITY, &task_A);

     vTaskDelay(100); //Allow the deadlock

    xTaskCreate(taskB, "taskB",
                SIDE_TASK_STACK_SIZE, (void *)&argsB,
                SIDE_TASK_PRIORITY, &task_B);

    vTaskDelay(200);
    TEST_ASSERT_EQUAL(101, argsA.counter);
    TEST_ASSERT_EQUAL(201, argsB.counter);

    vTaskDelete(task_A);
    vTaskDelete(task_B);
    vSemaphoreDelete(semaphore_A);
    vSemaphoreDelete(semaphore_B);
}

void runner_task(void *params) {
    for (;;)
    {
        printf("Start Tests,\n");
        UNITY_BEGIN();
        RUN_TEST(test_lock);
        RUN_TEST(test_deadlock);
        UNITY_END();
        vTaskDelay(1000);
    }
    
}

int main(void) {
    stdio_init_all();
    sleep_ms(5000);

    // Create runner task
    xTaskCreate(runner_task, "RunnerTask", MAIN_TASK_STACK_SIZE, NULL,
                MAIN_TASK_PRIORITY, NULL);

    vTaskStartScheduler();

    return 0;
}

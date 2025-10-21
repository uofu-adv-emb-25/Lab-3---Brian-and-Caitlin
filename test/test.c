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

//Structure to hold semaphores and counter - simulate deadlock between two tasks
typedef struct deadlockArgs {
    SemaphoreHandle_t A;
    SemaphoreHandle_t B;
    int counter;
} deadlockArgs;

//Structure to hold semaphore and counter - used for orphaned lock test
typedef struct orphanArgs {
    SemaphoreHandle_t Orph;
    int counter;
} orphanArgs;

int counter;
int on;

//Increments global counter and print message to keep track
void thread_counter(){
    counter = counter + 1;
    printf("hello world from %s! Count %d\n", "thread", counter);
}

int counter;

//Test to see if the semaphore can be taken and given and return true is it was taken
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

//Unit test to test basic semaphore lock and unlocking functionality
void test_lock(void){
    SemaphoreHandle_t semaphore = xSemaphoreCreateCounting(1,1);
    counter = 0;
    int lock_result = test_semaphore(semaphore);

    TEST_ASSERT_EQUAL_INT(pdTRUE, lock_result);
    TEST_ASSERT_EQUAL_INT(1, counter);  
}

//Take semaphore A first then semaphore B
void taskA(void *arg)
{
    //printf ("Task A.\n");
    deadlockArgs *args = (deadlockArgs *) arg;

    args->counter += xSemaphoreTake(args->A, portMAX_DELAY);

    vTaskDelay(500); //Give time for semaphore to take

    args->counter += xSemaphoreTake(args->B, portMAX_DELAY);

    xSemaphoreGive(args->B);
    xSemaphoreGive(args->A);

    vTaskSuspend(NULL);
}

//Take semaphore B then semaphore A
void taskB(void *arg)
{
    //printf ("Task B.\n");
    deadlockArgs *args = (deadlockArgs *) arg;

    args->counter += xSemaphoreTake(args->B, portMAX_DELAY);
    args->counter += xSemaphoreTake(args->A, portMAX_DELAY);

    xSemaphoreGive(args->A);
    xSemaphoreGive(args->B);

    vTaskSuspend(NULL);
}

//Test to deadlock between the two tasks (task A and task B)
void test_deadlock(void)
{
    printf ("Starting deadlock test.\n");
    SemaphoreHandle_t semaphore_A = xSemaphoreCreateMutex();
    SemaphoreHandle_t semaphore_B = xSemaphoreCreateMutex();

    deadlockArgs argsA = { semaphore_A, semaphore_B, 100 };
    deadlockArgs argsB = { semaphore_A, semaphore_B, 200 };

   
    TaskHandle_t task_A, task_B;

    //printf ("Creating Tasks.\n");
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

//Test that takes a semaphore but sometime never relases it causing a bug
void orphaned_lock(void *arg)
{
    orphanArgs *args = (orphanArgs *) arg;
    while (1) {
        xSemaphoreTake(args->Orph, portMAX_DELAY);
        args->counter++;
        if (args->counter % 2) {
            continue;
        }
        xSemaphoreGive(args->Orph);
    }
}

//Orphaned lock that always works and lrelesases the semaphore properly
void orphaned_lock_fix(void *arg)
{
    orphanArgs *args = (orphanArgs *) arg;
    while (1) {
        xSemaphoreTake(args->Orph, portMAX_DELAY);
        args->counter++;
        if (args->counter % 2) {
            xSemaphoreGive(args->Orph);
            continue;
        }
        xSemaphoreGive(args->Orph);
    }
}

//Test to identify the orphaned lock
void test_orphaned_lock(void)
{
    printf ("Starting orphaned test.\n");
    SemaphoreHandle_t orphSem = xSemaphoreCreateMutex();
    orphanArgs orphan = {orphSem, 0};

    TaskHandle_t orphan_task;

    xTaskCreate(orphaned_lock, "orphan_task",
                SIDE_TASK_STACK_SIZE, (void *)&orphan,
                SIDE_TASK_PRIORITY, &orphan_task);

    vTaskDelay(500);
    TEST_ASSERT_EQUAL(1, orphan.counter);
    vTaskDelete(orphan_task);
    vSemaphoreDelete(orphSem);
}

//Test that makes sure orphaned lock works continuously
void test_orphaned_lock_fix(void)
{
    printf ("Starting fixed orphaned test.\n");
    SemaphoreHandle_t fixorphSem = xSemaphoreCreateMutex();
    orphanArgs orphan_fix = {fixorphSem, 0};

    TaskHandle_t orphan_task_fix;

    xTaskCreate(orphaned_lock_fix, "orphan_task_fix",
                SIDE_TASK_STACK_SIZE, (void *)&orphan_fix,
                SIDE_TASK_PRIORITY, &orphan_task_fix);

    vTaskDelay(500);
    TEST_ASSERT_GREATER_OR_EQUAL(1, orphan_fix.counter);
    TEST_ASSERT_GREATER_OR_EQUAL(10, orphan_fix.counter);
    TEST_ASSERT_GREATER_OR_EQUAL(100, orphan_fix.counter);
    vTaskDelete(orphan_task_fix);
    vSemaphoreDelete(fixorphSem);
}

void runner_task(void *params)
{
    for (;;)
    {
        printf("Start Tests,\n");
        UNITY_BEGIN();
        RUN_TEST(test_lock);
        RUN_TEST(test_deadlock);
        RUN_TEST(test_orphaned_lock);
        RUN_TEST(test_orphaned_lock_fix);
        UNITY_END();
        vTaskDelay(1000);
    }
    
}

int main(void)
{
    stdio_init_all();
    sleep_ms(5000);

    // Create runner task
    xTaskCreate(runner_task, "RunnerTask", MAIN_TASK_STACK_SIZE, NULL,
                MAIN_TASK_PRIORITY, NULL);

    vTaskStartScheduler();

    return 0;
}
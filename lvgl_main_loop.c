/**
 * @file lvgl_main_loop.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_freertos_hooks.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_pm.h"

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "src/lv_core/lv_refr.h"
#else
#include "lvgl/src/lv_core/lv_refr.h"
#endif

#include "lvgl_main_loop.h"

/*********************
 *      DEFINES
 *********************/

#define TAG "lvgl_main_loop"

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

static bool lvgl_wait_eventloop(const uint32_t millis);

/**********************
 *  STATIC VARIABLES
 **********************/

static TaskHandle_t main_loop_task_handle = NULL;
volatile uint32_t lvgl_esp_sys_time;

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lvgl_disable_task(lv_task_t *read_task)
{
    lv_task_set_period(read_task, LV_NO_TASK_READY);
}

void IRAM_ATTR lvgl_enable_task_isr(lv_task_t *read_task)
{
    //lv_task_set_period(read_task, LV_INDEV_DEF_READ_PERIOD);  // not in IRAM
    read_task->period = LV_INDEV_DEF_READ_PERIOD;
    lvgl_notify_event_loop();
}

void IRAM_ATTR lvgl_notify_event_loop()
{
    if (main_loop_task_handle != NULL)
    {
        BaseType_t high_task_wakeup = pdFALSE;
        vTaskNotifyGiveFromISR(main_loop_task_handle, &high_task_wakeup);
        if (high_task_wakeup != pdFALSE)
        {
            portYIELD_FROM_ISR();
        }
    }
}

void lvgl_main_loop(uint32_t (*lvgl_task_handler)(void *), void *user_ptr)
{
#ifdef CONFIG_PM_ENABLE
    esp_pm_lock_handle_t loop_pm_lock;
    /* does not result in measurable reduced power consumption...
    const esp_pm_config_esp32_t loop_pm_config =
    {
        .max_freq_mhz = CONFIG_ESP32_DEFAULT_CPU_FREQ_MHZ,
        .min_freq_mhz = 10,
        .light_sleep_enable = false
    };
    ESP_ERROR_CHECK(esp_pm_configure(&loop_pm_config));
    */
    ESP_ERROR_CHECK(esp_pm_lock_create(ESP_PM_CPU_FREQ_MAX, 0, "CPU_FREQ_MAX", &loop_pm_lock));
    ESP_ERROR_CHECK(esp_pm_lock_acquire(loop_pm_lock));
#endif

    main_loop_task_handle = xTaskGetCurrentTaskHandle();

    while (1)
    {
        const uint32_t timeTillNext = lvgl_task_handler(user_ptr);
#ifdef CONFIG_PM_ENABLE
        // decrease power usage if LVGL is not too busy
        const bool slowDownCPU = (timeTillNext >= (CONFIG_LV_DISP_DEF_REFR_PERIOD / 2));
        if (slowDownCPU)
        {
            ESP_ERROR_CHECK(esp_pm_lock_release(loop_pm_lock));
        }
#endif
        bool hadEvent = lvgl_wait_eventloop(timeTillNext);
        (void) hadEvent;    // currently unused
#ifdef CONFIG_PM_ENABLE
        if (slowDownCPU)
        {
            ESP_ERROR_CHECK(esp_pm_lock_acquire(loop_pm_lock));
        }
#endif
    }
}

/**
 * Wait inside the LVGL event loop for the next event or until the given
 * number of milliseconds have elapsed, whatever occurrs earlier.
 *
 * @return true if an event occurred, false if the timeout occurred.
 */
static bool lvgl_wait_eventloop(const uint32_t millis)
{
    if (main_loop_task_handle == NULL)
    {
        main_loop_task_handle = xTaskGetCurrentTaskHandle();
    }
    uint32_t ticks = pdMS_TO_TICKS(millis);
    if (ticks < 1)
    {
        ticks = 1U;
    }
    const uint32_t notificationCount = ulTaskNotifyTake(pdTRUE, ticks);
    return (notificationCount != 0U);
}

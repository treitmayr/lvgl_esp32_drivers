/**
 * @file lvgl_main_loop.h
 */

#ifndef LVGL_MAIN_LOOP_H
#define LVGL_MAIN_LOOP_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include <stdbool.h>

#include "lvgl_spi_conf.h"
#include "lvgl_tft/disp_driver.h"
#include "lvgl_touch/touch_driver.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Disables the given LVGL task by setting its time of next execution to
 * the maximum time.
 */
void lvgl_disable_task(lv_task_t *read_task);

/**
 * Restores the regular task invocation period for input devices in the
 * given task and then retrigger the event loop.
 *
 * This method is intended to be used by an ISR when touching the display
 * was detected.
 */
void IRAM_ATTR lvgl_enable_task_isr(lv_task_t *read_task);

/**
 * Retrigger the event loop.
 * This method may be invoked from a different task or from an ISR.
 */
void IRAM_ATTR lvgl_notify_event_loop();

void lvgl_main_loop(uint32_t (*lvgl_task_handler)(void *), void *user_ptr);

inline uint32_t lvgl_esp_get_sys_time()
{
    return (uint32_t)(esp_timer_get_time() / 1000U);
}

/**********************
 *      MACROS
 **********************/

#if LV_TICK_CUSTOM != 0
#define LV_ESP_TICK_GET lvgl_esp_get_sys_time()
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* LVGL_MAIN_LOOP_H */

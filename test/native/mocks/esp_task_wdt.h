#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t timeout_ms;
    uint32_t idle_core_mask;
    bool trigger_panic;
} esp_task_wdt_config_t;

#define ESP_OK 0
#define ESP_ERR_INVALID_STATE 0x103

typedef int esp_err_t;

static inline esp_err_t esp_task_wdt_init(uint32_t timeout, bool panic) {
    (void)timeout; (void)panic; return ESP_OK;
}
static inline esp_err_t esp_task_wdt_add(void *task_handle) {
    (void)task_handle; return ESP_OK;
}
static inline esp_err_t esp_task_wdt_reset() { return ESP_OK; }
static inline esp_err_t esp_task_wdt_reconfigure(const esp_task_wdt_config_t *config) {
    (void)config; return ESP_OK;
}
static inline esp_err_t esp_task_wdt_delete(void *task_handle) {
    (void)task_handle; return ESP_OK;
}
static inline esp_err_t esp_task_wdt_status(void *task_handle) {
    (void)task_handle; return ESP_OK;
}

#ifdef __cplusplus
}
#endif

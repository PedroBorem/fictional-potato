#include "hw_test.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "idp_parser.h"

#include <string.h>
#include <stdio.h>

#define HW_TEST_TAG "hw_test"

extern char system_id[];
extern comm_type comm_main_mode;

typedef enum
{
    HW_EXPECT_EXACT = 0,
    HW_EXPECT_DWP_ENDS_2 = 1
} hw_expect_mode_t;

static app_callback hw_test_call = NULL;

static TaskHandle_t s_hw_test_task = NULL;
static volatile bool s_running = false;

static hw_expect_mode_t s_expect_mode = HW_EXPECT_EXACT;
static uint16_t s_expected_dwp = 0;
static uint16_t s_expected_percent = 0;

static uint16_t s_last_dwp = 0;
static uint16_t s_last_percent = 0;
static char s_last_pivot_id[50] = {};
static char s_last_pkt[220] = {};

static void hw_test_suite_task(void *arg);

void hw_test_init(const app_callback callback)
{
    hw_test_call = callback;
}

static void hw_test_set_expected_exact(uint16_t dwp, uint16_t percent)
{
    s_expect_mode = HW_EXPECT_EXACT;
    s_expected_dwp = dwp;
    s_expected_percent = percent;
    s_last_dwp = 0;
    s_last_percent = 0;
    s_last_pivot_id[0] = '\0';
    s_last_pkt[0] = '\0';
}

static void hw_test_set_expected_pivot_shutdown(void)
{
    s_expect_mode = HW_EXPECT_DWP_ENDS_2;
    s_expected_dwp = 0;
    s_expected_percent = 0;
    s_last_dwp = 0;
    s_last_percent = 0;
    s_last_pivot_id[0] = '\0';
    s_last_pkt[0] = '\0';
}

bool hw_test_start_suite(void)
{
    if (hw_test_call == NULL)
        return false;

    if (s_running)
        return false;

    s_running = true;

    if (xTaskCreate(hw_test_suite_task, "hw_test", 12288, NULL, 5, &s_hw_test_task) != pdPASS)
    {
        s_running = false;
        s_hw_test_task = NULL;
        return false;
    }

    return true;
}

void hw_test_on_tx_packet(const char *idp_pack)
{
    if (!s_running || s_hw_test_task == NULL || idp_pack == NULL)
        return;

    uint8_t idp = 0;
    char pivot_id[50] = {};
    uint16_t dwp = 0;
    uint16_t percent = 0;

    arg_pair_t arg_pairs[] = {
        {"uint8_t",  &idp},
        {"string",   pivot_id},
        {"uint16_t", &dwp},
        {"uint16_t", &percent},
        {NULL, NULL}
    };

    idp_parser_get_packet_data(idp_pack, arg_pairs);

    if (idp != 0)
        return;

    s_last_dwp = dwp;
    s_last_percent = percent;
    strncpy(s_last_pivot_id, pivot_id, sizeof(s_last_pivot_id) - 1);
    strncpy(s_last_pkt, idp_pack, sizeof(s_last_pkt) - 1);

    if (strcmp(pivot_id, system_id) != 0)
        return;

    if (s_expect_mode == HW_EXPECT_EXACT)
    {
        if (dwp == s_expected_dwp && percent == s_expected_percent)
        {
            xTaskNotify(s_hw_test_task, 1, eSetValueWithOverwrite);
        }
    }
    else if (s_expect_mode == HW_EXPECT_DWP_ENDS_2)
    {
        if ((dwp % 10U) == 2U)
        {
            xTaskNotify(s_hw_test_task, 1, eSetValueWithOverwrite);
        }
    }
}

static bool hw_test_wait_match(uint32_t timeout_ms)
{
    TickType_t t0 = xTaskGetTickCount();
    TickType_t timeout_ticks = pdMS_TO_TICKS(timeout_ms);

    while ((xTaskGetTickCount() - t0) < timeout_ticks)
    {
        hw_test_call("#00$", COMM_TEST);

        uint32_t note = 0;
        if (xTaskNotifyWait(0, 0xFFFFFFFF, &note, pdMS_TO_TICKS(2000)) == pdTRUE && note == 1)
        {
            return true;
        }
    }

    return false;
}

static void hw_test_send_off_and_wait(void)
{
    hw_test_set_expected_pivot_shutdown();

    char cmd_off[200] = {};
    snprintf(cmd_off, sizeof(cmd_off), "#01-%s-002-000-hw_test$",
             system_id);

    ESP_LOGI(HW_TEST_TAG, "Sending OFF command: %s", cmd_off);
    hw_test_call(cmd_off, COMM_TEST);

    bool ok_off = hw_test_wait_match(120000);

    if (ok_off)
    {
        ESP_LOGI(HW_TEST_TAG, "OFF OK: dwp=%u percent=%u", (unsigned)s_last_dwp, (unsigned)s_last_percent);
    }
    else
    {
        ESP_LOGE(HW_TEST_TAG, "OFF FAIL: last pivot=%s dwp=%u percent=%u | pkt=%s",
                 s_last_pivot_id,
                 (unsigned)s_last_dwp,
                 (unsigned)s_last_percent,
                 s_last_pkt);
    }
}

static void hw_test_suite_task(void *arg)
{
    (void)arg;

    const uint16_t dwps[] = {351, 451};
    const uint16_t percents[] = {5, 25};

    const int n = (int)(sizeof(dwps) / sizeof(dwps[0]));

    for (int i = 0; i < n; i++)
    {
        hw_test_set_expected_exact(dwps[i], percents[i]);

        char cmd_01[200] = {};
        snprintf(cmd_01, sizeof(cmd_01), "#01-%s-%u-%02u-hw_test$",
                 system_id, (unsigned)dwps[i], (unsigned)percents[i]);

        ESP_LOGI(HW_TEST_TAG, "Starting test %d/%d: dwp=%u percent=%u",
                 i + 1, n, (unsigned)dwps[i], (unsigned)percents[i]);

        hw_test_call(cmd_01, COMM_TEST);

        bool ok = hw_test_wait_match(5UL * 60UL * 1000UL);

        if (ok)
        {
            ESP_LOGI(HW_TEST_TAG, "PASS: dwp=%u percent=%u",
                     (unsigned)s_expected_dwp, (unsigned)s_expected_percent);
        }
        else
        {
            ESP_LOGE(HW_TEST_TAG,
                     "FAIL: expected dwp=%u percent=%u | last pivot=%s dwp=%u percent=%u | pkt=%s",
                     (unsigned)s_expected_dwp,
                     (unsigned)s_expected_percent,
                     s_last_pivot_id,
                     (unsigned)s_last_dwp,
                     (unsigned)s_last_percent,
                     s_last_pkt);
        }

        hw_test_send_off_and_wait();

        if (i < (n - 1))
        {
            vTaskDelay(pdMS_TO_TICKS(30000));
        }
    }

    s_running = false;
    s_hw_test_task = NULL;
    vTaskDelete(NULL);
}

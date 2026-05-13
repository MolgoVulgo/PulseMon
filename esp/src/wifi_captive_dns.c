#include "wifi_captive_dns.h"

#include <errno.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "lwip/inet.h"
#include "lwip/sockets.h"

static const char *TAG = "wifi_captive_dns";
static TaskHandle_t s_dns_task;

#define DNS_PORT 53
#define DNS_BUF_SIZE 512
#define DNS_HEADER_SIZE 12

static size_t dns_question_end(const uint8_t *buf, size_t len)
{
    size_t pos = DNS_HEADER_SIZE;
    while (pos < len && buf[pos] != 0) {
        uint8_t label_len = buf[pos];
        if ((label_len & 0xC0) != 0 || label_len > 63) {
            return 0;
        }
        pos += (size_t)label_len + 1;
    }

    if (pos + 5 > len) {
        return 0;
    }
    return pos + 5;
}

static size_t build_dns_response(uint8_t *buf, size_t len)
{
    if (len < DNS_HEADER_SIZE) {
        return 0;
    }

    size_t question_end = dns_question_end(buf, len);
    if (question_end == 0 || question_end + 16 > DNS_BUF_SIZE) {
        return 0;
    }

    buf[2] = 0x81;
    buf[3] = 0x80;
    buf[4] = 0x00;
    buf[5] = 0x01;
    buf[6] = 0x00;
    buf[7] = 0x01;
    buf[8] = 0x00;
    buf[9] = 0x00;
    buf[10] = 0x00;
    buf[11] = 0x00;

    size_t pos = question_end;
    buf[pos++] = 0xC0;
    buf[pos++] = 0x0C;
    buf[pos++] = 0x00;
    buf[pos++] = 0x01;
    buf[pos++] = 0x00;
    buf[pos++] = 0x01;
    buf[pos++] = 0x00;
    buf[pos++] = 0x00;
    buf[pos++] = 0x00;
    buf[pos++] = 0x3C;
    buf[pos++] = 0x00;
    buf[pos++] = 0x04;
    buf[pos++] = 192;
    buf[pos++] = 168;
    buf[pos++] = 4;
    buf[pos++] = 1;
    return pos;
}

static void dns_task(void *arg)
{
    (void)arg;

    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock < 0) {
        ESP_LOGE(TAG, "socket failed errno=%d", errno);
        s_dns_task = NULL;
        vTaskDelete(NULL);
        return;
    }

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(DNS_PORT),
        .sin_addr.s_addr = htonl(INADDR_ANY),
    };

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        ESP_LOGE(TAG, "bind failed errno=%d", errno);
        close(sock);
        s_dns_task = NULL;
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "captive dns started");
    uint8_t buf[DNS_BUF_SIZE];
    while (1) {
        struct sockaddr_in source_addr;
        socklen_t socklen = sizeof(source_addr);
        int len = recvfrom(sock, buf, sizeof(buf), 0, (struct sockaddr *)&source_addr, &socklen);
        if (len <= 0) {
            continue;
        }

        size_t response_len = build_dns_response(buf, (size_t)len);
        if (response_len == 0) {
            continue;
        }
        sendto(sock, buf, response_len, 0, (struct sockaddr *)&source_addr, socklen);
    }
}

esp_err_t pulsemon_wifi_captive_dns_start(void)
{
    if (s_dns_task != NULL) {
        return ESP_OK;
    }

    BaseType_t ok = xTaskCreate(dns_task, "wifi_dns", 4096, NULL, 3, &s_dns_task);
    if (ok != pdPASS) {
        s_dns_task = NULL;
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

#ifndef PRINTER_CONFIG_H
#define PRINTER_CONFIG_H

/*
 * Temporary fixed printer endpoint.
 * Fill these two values locally until they move to persistent configuration.
 */
#ifndef PRINTER_HOST
#define PRINTER_HOST ""
#endif

#ifndef PRINTER_ACCESS_CODE
#define PRINTER_ACCESS_CODE ""
#endif

#define PRINTER_HTTP_PORT 80
#define PRINTER_MQTT_PORT 1883
#define PRINTER_HTTP_TIMEOUT_MS 4000
#define PRINTER_MQTT_TIMEOUT_MS 5000
#define PRINTER_POLL_INTERVAL_MS 5000
#define PRINTER_RETRY_INTERVAL_MS 5000
#define PRINTER_APP_PING_INTERVAL_MS 30000

#endif

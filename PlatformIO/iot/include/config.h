#pragma once

#define WIFI_SSID     "Wokwi-GUEST"
#define WIFI_PASSWORD ""

#define MQTT_BROKER   "broker.emqx.io"
#define MQTT_PORT     1883
#define MQTT_CLIENT   "esp32_pzem_001"

#define TOPIC_DATA    "pzem/esp32/data"
#define TOPIC_ALERT   "pzem/esp32/alert"
#define TOPIC_STATUS  "pzem/esp32/status"

#define RATE_THB      4.2f    // บาท/kWh
#define ALERT_WATT    3000.0f

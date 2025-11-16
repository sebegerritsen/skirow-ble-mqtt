#ifndef CONFIG_H
#define CONFIG_H

// WiFi Configuration
// If USE_WIFI_MANAGER is true, WiFiManager will be used for configuration
// Otherwise, use the hardcoded credentials below
#define USE_WIFI_MANAGER true
#define WIFI_SSID "YourSSID"
#define WIFI_PASSWORD "YourPassword"

// MQTT Configuration
#define MQTT_BROKER "192.168.1.100"  // Change to your broker IP/hostname
#define MQTT_PORT 1883
#define MQTT_USER ""  // Leave empty if no auth
#define MQTT_PASSWORD ""  // Leave empty if no auth
#define MQTT_CLIENT_ID "skirow-bridge"

// MQTT Topics
#define MQTT_TOPIC_PREFIX "rowing"
#define MQTT_TOPIC_STROKE_RATE "rowing/stroke_rate"
#define MQTT_TOPIC_DISTANCE "rowing/distance"
#define MQTT_TOPIC_POWER "rowing/power"
#define MQTT_TOPIC_SPEED "rowing/speed"
#define MQTT_TOPIC_CALORIES "rowing/calories"
#define MQTT_TOPIC_TIME "rowing/time"
#define MQTT_TOPIC_STATUS "rowing/status"
#define MQTT_TOPIC_HEARTRATE "rowing/heart_rate"

// BLE Configuration
#define BLE_SCAN_TIME 5  // Scan time in seconds
#define BLE_SCAN_INTERVAL 1349  // Scan interval in ms
#define BLE_SCAN_WINDOW 449  // Scan window in ms

// FTMS Service UUID
#define FTMS_SERVICE_UUID "00001826-0000-1000-8000-00805f9b34fb"
#define FTMS_ROWING_DATA_CHAR "00002ad1-0000-1000-8000-00805f9b34fb"
#define FTMS_INDOOR_BIKE_DATA_CHAR "00002ad2-0000-1000-8000-00805f9b34fb"

// Device name filters (case insensitive)
const char* DEVICE_NAME_FILTERS[] = {
    "skirow",
    "energyfit",
    "pm5",
    "concept2",
    "rower",
    "rowing"
};
const int DEVICE_NAME_FILTER_COUNT = 6;

// OLED Display Configuration (Heltec LoRa 32 V2)
#define OLED_SDA 4
#define OLED_SCL 15
#define OLED_RST 16
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Reconnection settings
#define WIFI_RECONNECT_INTERVAL 10000  // 10 seconds
#define MQTT_RECONNECT_INTERVAL 5000   // 5 seconds
#define BLE_RECONNECT_INTERVAL 5000    // 5 seconds

// Data publish interval
#define PUBLISH_INTERVAL 1000  // Publish data every 1 second

#endif

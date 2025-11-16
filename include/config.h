#ifndef CONFIG_H
#define CONFIG_H

// WiFi Configuration
// If USE_WIFI_MANAGER is true, WiFiManager will be used for configuration
// Otherwise, use the hardcoded credentials below
#define USE_WIFI_MANAGER true
#define WIFI_SSID "YourSSID"
#define WIFI_PASSWORD "YourPassword"

// MQTT Configuration
#define MQTT_BROKER "192.168.1.100"  // Change to your MQTT broker IP
#define MQTT_PORT 1883
#define MQTT_USER "your_mqtt_user"  // Change to your MQTT username
#define MQTT_PASSWORD "your_mqtt_password"  // Change to your MQTT password
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
#define BLE_SCAN_TIME 30  // Scan time in seconds (longer to catch SkiRow's 10-15s advertising window)
#define BLE_SCAN_INTERVAL 1349  // Scan interval in ms
#define BLE_SCAN_WINDOW 449  // Scan window in ms

// FTMS Service UUID
#define FTMS_SERVICE_UUID "00001826-0000-1000-8000-00805f9b34fb"
#define FTMS_ROWING_DATA_CHAR "00002ad1-0000-1000-8000-00805f9b34fb"
#define FTMS_INDOOR_BIKE_DATA_CHAR "00002ad2-0000-1000-8000-00805f9b34fb"

// Device name filters (case insensitive)
const char* DEVICE_NAME_FILTERS[] = {
    "skirow",
    "ski-row",
    "ski row",
    "energyfit",
    "pm5",
    "concept2",
    "rower",
    "rowing"
};
const int DEVICE_NAME_FILTER_COUNT = 8;

// OLED Display Configuration (Heltec LoRa 32 V3)
#define OLED_SDA 17
#define OLED_SCL 18
#define OLED_RST 21  // V3 uses GPIO 21 for reset
#define VEXT_PIN 36  // Vext power control (LOW = ON, HIGH = OFF)
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Reconnection settings
#define WIFI_RECONNECT_INTERVAL 10000  // 10 seconds
#define MQTT_RECONNECT_INTERVAL 5000   // 5 seconds
#define BLE_RECONNECT_INTERVAL 1000    // 1 second (minimal gap between scans)

// Data publish interval
#define PUBLISH_INTERVAL 1000  // Publish data every 1 second

#endif

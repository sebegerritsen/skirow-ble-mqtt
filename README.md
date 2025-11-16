# SkiRow BLE to MQTT Bridge

ESP32-based bridge that connects Ski-Row (and other FTMS-compatible) rowing machines to Home Assistant via MQTT. Built this out of annoyance that there are very few apps that work with Ski-Row devices (mine is the Ski-Row Air from Energyfit). I have built this with a Lora32 (with a tiny screen). but will likely work with any other ESP32 device. 

## Features

- ✅ BLE connection to SkiRow and other FTMS rowing machines
- ✅ MQTT publishing of rowing metrics
- ✅ OLED display showing real-time data
- ✅ WiFiManager for easy WiFi setup
- ✅ Automatic reconnection and scanning
- ✅ Home Assistant integration ready

## Hardware Requirements

- **Heltec WiFi LoRa 32 V3** (ESP32-S3 based board with built-in OLED)
  - Other ESP32 boards may work with pin adjustments
- **SkiRow** or other FTMS-compatible rowing machine
- MQTT broker (Mosquitto, Home Assistant, etc.)

## Metrics Published

The following metrics are published to MQTT topics every second:

- `rowing/stroke_rate` - Strokes per minute
- `rowing/distance` - Distance in meters
- `rowing/power` - Power in watts
- `rowing/speed` - Speed in cm/s
- `rowing/calories` - Calories burned
- `rowing/time` - Elapsed time in seconds
- `rowing/heart_rate` - Heart rate (if available)
- `rowing/status` - Connection status

## Installation

### 1. Clone the Repository

```bash
git clone https://github.com/yourusername/skirow-ble-mqtt.git
cd skirow-ble-mqtt
```

### 2. Configure MQTT Settings

Edit `include/config.h` and update your MQTT broker settings:

```cpp
#define MQTT_BROKER "192.168.1.100"  // Your MQTT broker IP
#define MQTT_PORT 1883
#define MQTT_USER "your_mqtt_user"
#define MQTT_PASSWORD "your_mqtt_password"
```

### 3. Build and Upload

Using PlatformIO:

```bash
pio run --target upload
```

Or use PlatformIO IDE in VSCode.

### 4. First-Time WiFi Setup

On first boot, the device creates a WiFi access point:
- SSID: `SkiRow-Bridge`
- Password: `rowing123`

Connect to this network and configure your WiFi credentials.

## Home Assistant Integration

Add the following to your Home Assistant `configuration.yaml`:

```yaml
mqtt:
  sensor:
    - name: "SkiRow Stroke Rate"
      state_topic: "rowing/stroke_rate"
      unit_of_measurement: "SPM"
      icon: mdi:rowing
      
    - name: "SkiRow Distance"
      state_topic: "rowing/distance"
      unit_of_measurement: "m"
      icon: mdi:map-marker-distance
      
    - name: "SkiRow Power"
      state_topic: "rowing/power"
      unit_of_measurement: "W"
      icon: mdi:flash
      
    - name: "SkiRow Speed"
      state_topic: "rowing/speed"
      unit_of_measurement: "cm/s"
      icon: mdi:speedometer
      
    - name: "SkiRow Calories"
      state_topic: "rowing/calories"
      unit_of_measurement: "kcal"
      icon: mdi:fire
      
    - name: "SkiRow Time"
      state_topic: "rowing/time"
      unit_of_measurement: "s"
      icon: mdi:timer
      
    - name: "SkiRow Heart Rate"
      state_topic: "rowing/heart_rate"
      unit_of_measurement: "bpm"
      icon: mdi:heart-pulse

  binary_sensor:
    - name: "SkiRow Connected"
      state_topic: "rowing/status"
      payload_on: "connected"
      payload_off: "disconnected"
      device_class: connectivity
```

See `homeassistant_config.yaml` for advanced configuration with template sensors.

## Usage

1. Power on the ESP32
2. Wait for WiFi and MQTT connection (shown on OLED)
3. Press the **LE button** on your SkiRow
4. Device will automatically detect and connect
5. Start rowing - data appears in Home Assistant!

## Configuration

### Pin Configuration (Heltec V3)

Defined in `include/config.h`:

```cpp
#define OLED_SDA 17
#define OLED_SCL 18
#define OLED_RST 21
#define VEXT_PIN 36  // Power control for OLED
```

### BLE Scan Settings

```cpp
#define BLE_SCAN_TIME 30  // 30-second scan to catch SkiRow's advertising window
#define BLE_RECONNECT_INTERVAL 1000  // 1 second between scans
```

### Supported Devices

The device filters for these keywords (case-insensitive):
- skirow, ski-row, ski row
- energyfit
- pm5, concept2
- rower, rowing

Any FTMS-compatible device matching these names will connect.

## Troubleshooting

### SkiRow Not Detected
- Press the LE button on SkiRow (advertises for only 10-15 seconds)
- Wait for the full 30-second scan cycle
- Check serial monitor for device detection

### OLED Not Working
- Verify correct Heltec board version (V3 uses GPIO 21 for reset)
- Check Vext power pin is enabled (GPIO 36)

### MQTT Connection Fails
- Verify MQTT broker IP and port in config.h
- Check username/password
- Ensure firewall allows connection

## Development

### Serial Monitor

```bash
pio device monitor --port COMx --baud 115200
```

### Libraries Used

- NimBLE-Arduino - BLE communication
- PubSubClient - MQTT client
- WiFiManager - WiFi configuration
- ESP8266 and ESP32 OLED driver for SSD1306

## License

MIT License - see LICENSE file

## Credits

Created for the SkiRow community. Compatible with any FTMS-compliant fitness equipment.

## Contributing

Pull requests welcome! Please test thoroughly before submitting.

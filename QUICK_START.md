# Quick Start Guide

## Step 1: Upload the Code

1. Open the project in VS Code with PlatformIO extension, or use PlatformIO CLI

2. Connect your Heltec LoRa 32 via USB

3. Build and upload:
   ```bash
   cd skirow-ble-mqtt
   pio run --target upload
   ```

## Step 2: First Boot - WiFi Setup

Since `USE_WIFI_MANAGER` is enabled by default:

1. After upload completes, the device will create a WiFi access point
2. Look for WiFi network: **SkiRow-Bridge**
3. Password: **rowing123**
4. Connect to it with your phone/computer
5. A captive portal should open automatically (if not, go to http://192.168.4.1)
6. Select your home WiFi network and enter the password
7. Click Save

The device will restart and connect to your WiFi network.

## Step 3: Verify Connection

Open the serial monitor to see the connection status:
```bash
pio device monitor
```

You should see:
```
WiFi connected
IP address: 192.168.x.x
MQTT connected
Starting BLE scan...
```

## Step 4: Test with Your SkiRow

1. Turn on your SkiRow rowing machine
2. Make sure it's ready to pair (check machine's manual)
3. Watch the serial monitor and OLED display

The device should:
- Find your SkiRow in the BLE scan
- Connect to it automatically
- Display rowing data on the OLED
- Publish data to MQTT

## Step 5: Verify MQTT Data

You can verify MQTT data is being published using mosquitto_sub:

```bash
mosquitto_sub -h 192.168.178.2 -p 1885 -u sebe -P vague1 -t "rowing/#" -v
```

You should see messages like:
```
rowing/status online
rowing/stroke_rate 24
rowing/power 150
rowing/distance 500
```

## Step 6: Add to Home Assistant

1. Copy the contents of `homeassistant_config.yaml` to your Home Assistant configuration

2. Restart Home Assistant

3. Check Developer Tools → States for the new sensors:
   - `sensor.skirow_stroke_rate`
   - `sensor.skirow_distance`
   - `sensor.skirow_power`
   - etc.

## Troubleshooting

### SkiRow Not Found

If the device doesn't find your SkiRow:

1. **Run the BLE Scanner Test**:
   - Rename `src/main.cpp` to `src/main.cpp.backup`
   - Copy `scanner_test.cpp.txt` to `src/main.cpp`
   - Upload and open serial monitor
   - Turn on your SkiRow and see what it advertises

2. **Check the output** for:
   - Device name (add it to `DEVICE_NAME_FILTERS` in config.h)
   - Service UUIDs (verify FTMS service 0x1826 is present)

3. **Update config.h** with the correct device name if needed

### WiFi Issues

- **Can't see SkiRow-Bridge AP**:
  - Reset the device
  - Make sure you're looking for 2.4GHz networks (ESP32 doesn't support 5GHz)

- **WiFi won't connect**:
  - Double-check password
  - Make sure WiFi is 2.4GHz
  - Try resetting the device and reconfiguring

### MQTT Issues

- **MQTT won't connect**:
  - Verify broker IP, port, username, password in `config.h`
  - Test with mosquitto_pub/sub from command line
  - Check MQTT broker logs

- **No data published**:
  - Make sure BLE is connected first
  - Check serial monitor for errors
  - Verify device is actually rowing (data won't update when idle)

### Display Issues

If the OLED doesn't work:

1. Check if you have Heltec LoRa 32 V3 (different pins):
   - V3 uses different GPIO pins
   - Update `OLED_SDA`, `OLED_SCL`, `OLED_RST` in config.h

2. Common V3 pins:
   ```cpp
   #define OLED_SDA 17
   #define OLED_SCL 18
   #define OLED_RST 21
   ```

## Monitoring

### Serial Monitor
```bash
pio device monitor
```

Shows detailed logs of:
- WiFi connection
- MQTT connection
- BLE scanning and connection
- Raw data packets
- Parsed rowing metrics

### MQTT Monitor
```bash
mosquitto_sub -h 192.168.178.2 -p 1885 -u sebe -P vague1 -t "rowing/#" -v
```

### Home Assistant
Check the Logbook or History for the rowing sensors to see data updates.

## Next Steps

- Create a Home Assistant dashboard with rowing metrics
- Set up automations (e.g., notifications when workout is complete)
- Adjust publish interval if needed
- Customize MQTT topics for your setup

Enjoy your rowing workouts! 🚣‍♂️

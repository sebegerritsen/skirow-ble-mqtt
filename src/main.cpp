#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <NimBLEDevice.h>
#include <Wire.h>
#include <SSD1306Wire.h>
#include "config.h"

// OLED Display
SSD1306Wire display(0x3c, OLED_SDA, OLED_SCL);

// WiFi and MQTT
WiFiClient espClient;
PubSubClient mqttClient(espClient);

// BLE
NimBLEScan* pBLEScan;
NimBLEClient* pClient = nullptr;
NimBLERemoteService* pRemoteService = nullptr;
NimBLERemoteCharacteristic* pRemoteCharacteristic = nullptr;
NimBLEAdvertisedDevice* targetDevice = nullptr;

// State variables
bool deviceFound = false;
bool deviceConnected = false;
bool mqttConnected = false;
unsigned long lastReconnectAttempt = 0;
unsigned long lastPublishTime = 0;
unsigned long lastBLEReconnect = 0;

// Rowing data
struct RowingData {
    uint16_t strokeRate = 0;      // strokes per minute
    uint32_t distance = 0;         // meters
    uint16_t power = 0;            // watts
    uint16_t speed = 0;            // m/s * 100 (speed in cm/s)
    uint16_t calories = 0;         // kcal
    uint32_t elapsedTime = 0;      // seconds
    uint8_t heartRate = 0;         // bpm
} rowingData;

// Function declarations
void initOLED();
void updateDisplay(const char* line1, const char* line2 = "", const char* line3 = "", const char* line4 = "");
void setupWiFi();
void reconnectMQTT();
void publishRowingData();
void scanForDevices();
void connectToDevice();
void parseFTMSData(uint8_t* data, size_t length);
bool connectBLE();

// Notify callback for BLE characteristic
void notifyCallback(NimBLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
    Serial.print("Notify callback for characteristic ");
    Serial.print(pBLERemoteCharacteristic->getUUID().toString().c_str());
    Serial.print(" of data length ");
    Serial.println(length);

    // Print raw data
    Serial.print("Raw data: ");
    for (int i = 0; i < length; i++) {
        Serial.printf("%02X ", pData[i]);
    }
    Serial.println();

    parseFTMSData(pData, length);
}

// BLE Security callbacks
class SecurityCallbacks : public NimBLESecurityCallbacks {
    uint32_t onPassKeyRequest() {
        Serial.println("Passkey requested - returning 0");
        return 0;
    }

    void onPassKeyNotify(uint32_t pass_key) {
        Serial.printf("Passkey notify: %d\n", pass_key);
    }

    bool onSecurityRequest() {
        Serial.println("Security request - accepting");
        return true;
    }

    void onAuthenticationComplete(ble_gap_conn_desc* desc) {
        Serial.println("Authentication complete");
        if (desc->sec_state.encrypted) {
            Serial.println("Connection is encrypted");
        }
        if (desc->sec_state.bonded) {
            Serial.println("Connection is bonded");
        }
    }

    bool onConfirmPIN(uint32_t pin) {
        Serial.printf("Confirm PIN: %d\n", pin);
        return true;
    }
};

// BLE Client callbacks
class ClientCallbacks : public NimBLEClientCallbacks {
    void onConnect(NimBLEClient* pClient) {
        Serial.println("Connected to BLE device");
        deviceConnected = true;
        updateDisplay("BLE", "Connected!", "", "");
    }

    void onDisconnect(NimBLEClient* pClient) {
        Serial.println("Disconnected from BLE device");
        deviceConnected = false;
        deviceFound = false;  // Clear found flag to trigger new scan
        targetDevice = nullptr;  // Clear target device
        updateDisplay("BLE", "Disconnected", "Scanning...", "");
    }
};

// BLE Advertised Device callbacks
class AdvertisedDeviceCallbacks: public NimBLEAdvertisedDeviceCallbacks {
    void onResult(NimBLEAdvertisedDevice* advertisedDevice) {
        // Print detailed information about EVERY device found
        Serial.println("\n========================================");
        Serial.println("BLE Device Found!");
        Serial.println("========================================");

        // Device name
        if (advertisedDevice->haveName()) {
            Serial.print("Name: ");
            Serial.println(advertisedDevice->getName().c_str());
        } else {
            Serial.println("Name: (no name advertised)");
        }

        // MAC Address
        Serial.print("Address: ");
        Serial.println(advertisedDevice->getAddress().toString().c_str());

        // RSSI (signal strength)
        Serial.print("RSSI: ");
        Serial.print(advertisedDevice->getRSSI());
        Serial.println(" dBm");

        // Manufacturer data
        if (advertisedDevice->haveManufacturerData()) {
            Serial.print("Manufacturer Data: ");
            std::string md = advertisedDevice->getManufacturerData();
            for (int i = 0; i < md.length(); i++) {
                Serial.printf("%02X ", (uint8_t)md[i]);
            }
            Serial.println();
        }

        // Service UUIDs
        if (advertisedDevice->haveServiceUUID()) {
            Serial.println("Advertised Services:");

            // Check for FTMS service
            if (advertisedDevice->isAdvertisingService(NimBLEUUID("1826"))) {
                Serial.println("  ✓ FTMS Service (0x1826) - FITNESS MACHINE!");
            }

            // List all services
            for (int i = 0; i < advertisedDevice->getServiceUUIDCount(); i++) {
                NimBLEUUID uuid = advertisedDevice->getServiceUUID(i);
                Serial.print("  - ");
                Serial.println(uuid.toString().c_str());
            }
        } else {
            Serial.println("Services: (none advertised)");
        }

        Serial.println("========================================\n");

        // NOW check if this is a device we want to connect to
        // Check if device has FTMS service
        if (advertisedDevice->isAdvertisingService(NimBLEUUID(FTMS_SERVICE_UUID))) {
            Serial.println(">>> THIS IS A MATCH - HAS FTMS SERVICE!");
            deviceFound = true;
            targetDevice = advertisedDevice;
            pBLEScan->stop();
            return;
        }

        // Check device name
        if (advertisedDevice->haveName()) {
            String deviceName = advertisedDevice->getName().c_str();
            deviceName.toLowerCase();

            for (int i = 0; i < DEVICE_NAME_FILTER_COUNT; i++) {
                if (deviceName.indexOf(DEVICE_NAME_FILTERS[i]) >= 0) {
                    Serial.print(">>> THIS IS A MATCH - NAME CONTAINS: ");
                    Serial.println(DEVICE_NAME_FILTERS[i]);
                    deviceFound = true;
                    targetDevice = advertisedDevice;
                    pBLEScan->stop();
                    return;
                }
            }
        }
    }
};

void initOLED() {
    // Enable Vext power (LOW = ON for V3)
    pinMode(VEXT_PIN, OUTPUT);
    digitalWrite(VEXT_PIN, LOW);
    delay(100);  // Wait for power to stabilize

    // Reset OLED display
    pinMode(OLED_RST, OUTPUT);
    digitalWrite(OLED_RST, LOW);
    delay(50);
    digitalWrite(OLED_RST, HIGH);
    delay(50);

    // Initialize display
    display.init();
    display.flipScreenVertically();
    display.setFont(ArialMT_Plain_10);
    display.clear();
    updateDisplay("SkiRow BLE", "Bridge", "Initializing...", "");
}

void updateDisplay(const char* line1, const char* line2, const char* line3, const char* line4) {
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.drawString(0, 0, line1);
    display.drawString(0, 16, line2);
    display.drawString(0, 32, line3);
    display.drawString(0, 48, line4);
    display.display();
}

void setupWiFi() {
    updateDisplay("WiFi", "Connecting...", "", "");

    #if USE_WIFI_MANAGER
        WiFiManager wm;
        wm.setConfigPortalTimeout(180);  // 3 minutes timeout

        bool res = wm.autoConnect("SkiRow-Bridge", "rowing123");

        if(!res) {
            Serial.println("Failed to connect to WiFi");
            updateDisplay("WiFi", "Failed!", "Restarting...", "");
            delay(3000);
            ESP.restart();
        }
    #else
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 20) {
            delay(500);
            Serial.print(".");
            attempts++;
        }

        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("\nFailed to connect to WiFi");
            updateDisplay("WiFi", "Failed!", "Check config", "");
            delay(3000);
            ESP.restart();
        }
    #endif

    Serial.println("\nWiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    char ipStr[20];
    sprintf(ipStr, "IP:%s", WiFi.localIP().toString().c_str());
    updateDisplay("WiFi", "Connected!", ipStr, "");
    delay(2000);
}

void reconnectMQTT() {
    if (!mqttClient.connected()) {
        Serial.print("Attempting MQTT connection...");
        updateDisplay("MQTT", "Connecting...", "", "");

        bool connected = false;
        if (strlen(MQTT_USER) > 0) {
            connected = mqttClient.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASSWORD);
        } else {
            connected = mqttClient.connect(MQTT_CLIENT_ID);
        }

        if (connected) {
            Serial.println("connected");
            mqttConnected = true;
            mqttClient.publish(MQTT_TOPIC_STATUS, "online", true);
            updateDisplay("MQTT", "Connected!", "", "");
            delay(1000);
        } else {
            Serial.print("failed, rc=");
            Serial.println(mqttClient.state());
            mqttConnected = false;
            updateDisplay("MQTT", "Failed!", "Retrying...", "");
        }
    }
}

void publishRowingData() {
    if (!mqttClient.connected()) {
        return;
    }

    char buffer[20];

    // Publish stroke rate
    sprintf(buffer, "%d", rowingData.strokeRate);
    mqttClient.publish(MQTT_TOPIC_STROKE_RATE, buffer);

    // Publish distance
    sprintf(buffer, "%lu", rowingData.distance);
    mqttClient.publish(MQTT_TOPIC_DISTANCE, buffer);

    // Publish power
    sprintf(buffer, "%d", rowingData.power);
    mqttClient.publish(MQTT_TOPIC_POWER, buffer);

    // Publish speed (convert cm/s to m/s)
    float speedMs = rowingData.speed / 100.0;
    sprintf(buffer, "%.2f", speedMs);
    mqttClient.publish(MQTT_TOPIC_SPEED, buffer);

    // Publish calories
    sprintf(buffer, "%d", rowingData.calories);
    mqttClient.publish(MQTT_TOPIC_CALORIES, buffer);

    // Publish time
    sprintf(buffer, "%lu", rowingData.elapsedTime);
    mqttClient.publish(MQTT_TOPIC_TIME, buffer);

    // Publish heart rate if available
    if (rowingData.heartRate > 0) {
        sprintf(buffer, "%d", rowingData.heartRate);
        mqttClient.publish(MQTT_TOPIC_HEARTRATE, buffer);
    }

    Serial.println("Published rowing data to MQTT");
}

void parseFTMSData(uint8_t* data, size_t length) {
    if (length < 2) {
        return;
    }

    // Parse FTMS Rowing Machine Data characteristic (0x2AD1)
    // Based on FTMS specification

    uint16_t flags = data[0] | (data[1] << 8);
    int index = 2;

    // Bit 0: More Data
    // Bit 1: Average Stroke Present
    // Bit 2: Total Distance Present
    // Bit 3: Instantaneous Pace Present
    // Bit 4: Average Pace Present
    // Bit 5: Instantaneous Power Present
    // Bit 6: Average Power Present
    // Bit 7: Resistance Level Present
    // Bit 8: Expended Energy Present
    // Bit 9: Heart Rate Present
    // Bit 10: Metabolic Equivalent Present
    // Bit 11: Elapsed Time Present
    // Bit 12: Remaining Time Present

    // Stroke Rate (always present) - uint8
    if (index < length) {
        rowingData.strokeRate = data[index];
        index += 1;
    }

    // Stroke Count (always present) - uint16
    if (index + 1 < length) {
        uint16_t strokeCount = data[index] | (data[index + 1] << 8);
        index += 2;
    }

    // Average Stroke Rate
    if (flags & 0x02) {
        if (index < length) {
            index += 1;
        }
    }

    // Total Distance
    if (flags & 0x04) {
        if (index + 2 < length) {
            rowingData.distance = data[index] | (data[index + 1] << 8) | (data[index + 2] << 16);
            index += 3;
        }
    }

    // Instantaneous Pace
    if (flags & 0x08) {
        if (index + 1 < length) {
            rowingData.speed = data[index] | (data[index + 1] << 8);
            index += 2;
        }
    }

    // Average Pace
    if (flags & 0x10) {
        if (index + 1 < length) {
            index += 2;
        }
    }

    // Instantaneous Power
    if (flags & 0x20) {
        if (index + 1 < length) {
            rowingData.power = data[index] | (data[index + 1] << 8);
            index += 2;
        }
    }

    // Average Power
    if (flags & 0x40) {
        if (index + 1 < length) {
            index += 2;
        }
    }

    // Resistance Level
    if (flags & 0x80) {
        if (index + 1 < length) {
            index += 2;
        }
    }

    // Expended Energy
    if (flags & 0x100) {
        if (index + 1 < length) {
            rowingData.calories = data[index] | (data[index + 1] << 8);
            index += 2;
        }
        // Total energy and energy per hour/minute
        if (index + 3 < length) {
            index += 4;
        }
    }

    // Heart Rate
    if (flags & 0x200) {
        if (index < length) {
            rowingData.heartRate = data[index];
            index += 1;
        }
    }

    // Metabolic Equivalent
    if (flags & 0x400) {
        if (index < length) {
            index += 1;
        }
    }

    // Elapsed Time
    if (flags & 0x800) {
        if (index + 1 < length) {
            rowingData.elapsedTime = data[index] | (data[index + 1] << 8);
            index += 2;
        }
    }

    // Update display with rowing data
    char line1[32], line2[32], line3[32], line4[32];
    sprintf(line1, "SPM:%d PWR:%dW", rowingData.strokeRate, rowingData.power);
    sprintf(line2, "Dist:%lum", rowingData.distance);
    sprintf(line3, "Cal:%d Time:%lus", rowingData.calories, rowingData.elapsedTime);
    sprintf(line4, "HR:%d", rowingData.heartRate);
    updateDisplay(line1, line2, line3, line4);

    Serial.printf("Rowing Data - SPM:%d, Dist:%lu, Power:%d, Speed:%d, Cal:%d, Time:%lu, HR:%d\n",
                  rowingData.strokeRate, rowingData.distance, rowingData.power,
                  rowingData.speed, rowingData.calories, rowingData.elapsedTime, rowingData.heartRate);
}

bool connectBLE() {
    if (!targetDevice) {
        return false;
    }

    Serial.println("Connecting to BLE device...");

    if (pClient == nullptr) {
        pClient = NimBLEDevice::createClient();
        pClient->setClientCallbacks(new ClientCallbacks());
    }

    // Enable security/encryption for the connection
    pClient->setConnectTimeout(10);  // 10 second timeout

    if (!pClient->connect(targetDevice)) {
        Serial.println("Failed to connect to BLE device");
        return false;
    }

    Serial.println("Connected to BLE device");

    // Secure the connection (trigger pairing if needed)
    if (!pClient->secureConnection()) {
        Serial.println("Failed to secure connection");
        pClient->disconnect();
        return false;
    }

    Serial.println("Connection secured, discovering services...");

    // Get FTMS service
    pRemoteService = pClient->getService(FTMS_SERVICE_UUID);
    if (pRemoteService == nullptr) {
        Serial.println("Failed to find FTMS service");
        // Try to get all services and list them
        auto services = pClient->getServices(true);
        Serial.println("Available services:");
        for (auto &service : *services) {
            Serial.print("  - ");
            Serial.println(service->getUUID().toString().c_str());
        }
        pClient->disconnect();
        return false;
    }

    Serial.println("Found FTMS service");

    // Get Rowing Machine Data characteristic (0x2AD1)
    pRemoteCharacteristic = pRemoteService->getCharacteristic(FTMS_ROWING_DATA_CHAR);
    if (pRemoteCharacteristic == nullptr) {
        Serial.println("Failed to find Rowing Machine Data characteristic");
        // List all characteristics
        auto characteristics = pRemoteService->getCharacteristics(true);
        Serial.println("Available characteristics:");
        for (auto &characteristic : *characteristics) {
            Serial.print("  - ");
            Serial.println(characteristic->getUUID().toString().c_str());
        }
        pClient->disconnect();
        return false;
    }

    Serial.println("Found Rowing Machine Data characteristic");

    // Subscribe to notifications
    if (pRemoteCharacteristic->canNotify()) {
        pRemoteCharacteristic->subscribe(true, notifyCallback);
        Serial.println("Subscribed to notifications");
    } else {
        Serial.println("Characteristic cannot notify");
        pClient->disconnect();
        return false;
    }

    deviceConnected = true;
    return true;
}

void scanForDevices() {
    Serial.println("Starting BLE scan...");
    updateDisplay("BLE Scan", "Searching for", "SkiRow...", "");

    deviceFound = false;
    targetDevice = nullptr;

    NimBLEScanResults foundDevices = pBLEScan->start(BLE_SCAN_TIME, false);

    if (deviceFound && targetDevice != nullptr) {
        Serial.print("Found target device: ");
        Serial.println(targetDevice->toString().c_str());
        updateDisplay("BLE Scan", "Found device!", targetDevice->getName().c_str(), "Connecting...");
    } else {
        Serial.println("No matching device found");
        updateDisplay("BLE Scan", "No device found", "Retrying...", "");
    }

    pBLEScan->clearResults();
}

void setup() {
    Serial.begin(115200);
    Serial.println("\n\nSkiRow BLE to MQTT Bridge");
    Serial.println("==========================");

    // Initialize OLED
    initOLED();
    delay(2000);

    // Initialize BLE
    Serial.println("Initializing BLE...");
    updateDisplay("BLE", "Initializing...", "", "");
    NimBLEDevice::init("SkiRow-Bridge");
    NimBLEDevice::setPower(ESP_PWR_LVL_P9);

    // Configure BLE security for pairing
    NimBLEDevice::setSecurityAuth(true, true, true);  // bonding, MITM, secure connections
    NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);  // No input/output (Just Works pairing)
    NimBLEDevice::setSecurityInitKey(BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID);
    NimBLEDevice::setSecurityRespKey(BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID);
    NimBLEDevice::setSecurityCallbacks(new SecurityCallbacks());

    Serial.println("BLE security configured");

    pBLEScan = NimBLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new AdvertisedDeviceCallbacks());
    pBLEScan->setActiveScan(true);
    pBLEScan->setInterval(BLE_SCAN_INTERVAL);
    pBLEScan->setWindow(BLE_SCAN_WINDOW);

    Serial.println("BLE initialized");
    delay(1000);

    // Setup WiFi
    setupWiFi();

    // Setup MQTT
    mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
    mqttClient.setKeepAlive(60);
    mqttClient.setSocketTimeout(30);

    // Initial MQTT connection
    reconnectMQTT();

    // Scan for BLE devices
    scanForDevices();

    Serial.println("Setup complete");
}

void loop() {
    unsigned long currentMillis = millis();

    // Handle WiFi reconnection
    if (WiFi.status() != WL_CONNECTED) {
        if (currentMillis - lastReconnectAttempt > WIFI_RECONNECT_INTERVAL) {
            Serial.println("WiFi disconnected, reconnecting...");
            updateDisplay("WiFi", "Reconnecting...", "", "");
            WiFi.reconnect();
            lastReconnectAttempt = currentMillis;
        }
    }

    // Handle MQTT connection
    if (!mqttClient.connected() && WiFi.status() == WL_CONNECTED) {
        if (currentMillis - lastReconnectAttempt > MQTT_RECONNECT_INTERVAL) {
            reconnectMQTT();
            lastReconnectAttempt = currentMillis;
        }
    }
    mqttClient.loop();

    // Handle BLE connection
    if (!deviceConnected) {
        if (currentMillis - lastBLEReconnect > BLE_RECONNECT_INTERVAL) {
            if (!deviceFound) {
                scanForDevices();
            }

            if (deviceFound && targetDevice != nullptr) {
                connectBLE();
            }

            lastBLEReconnect = currentMillis;
        }
    }

    // Publish rowing data periodically
    if (deviceConnected && mqttClient.connected()) {
        if (currentMillis - lastPublishTime > PUBLISH_INTERVAL) {
            publishRowingData();
            lastPublishTime = currentMillis;
        }
    }

    delay(10);
}

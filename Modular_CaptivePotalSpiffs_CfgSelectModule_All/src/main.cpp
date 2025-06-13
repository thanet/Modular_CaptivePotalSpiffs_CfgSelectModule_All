#include "ESP32ConfigPortal.h"

//Pototype function
void runYourApplication();


// Create config portal instance
ESP32ConfigPortal configPortal(0, "MyDevice-Config", "my_device_config");

// Your application variables
String deviceName = "MyESP32Device";
bool applicationRunning = false;

void onConfigurationReceived(const ConfigData& config) {
    Serial.println("=== New Configuration Received ===");
    Serial.println("WiFi SSID: " + config.wifi_ssid);
    Serial.println("Telegram Active: " + String(config.tg_active));
    Serial.println("Host Active: " + String(config.host_active));
    
    // You can process the configuration here
    // For example, initialize Telegram bot, setup web connections, etc.
    
}

void onWiFiConnected() {
    Serial.println("WiFi connected successfully!");
    applicationRunning = true;
    
    // Initialize your application components here
    // For example: start web server, connect to MQTT, etc.
}

void onWiFiDisconnected() {
    Serial.println("WiFi disconnected!");
    applicationRunning = false;
    
    // Handle disconnection here
    // For example: stop services, save data, etc.
}

void onConfigReset() {
    Serial.println("Configuration was reset!");
    applicationRunning = false;
    
    // Handle reset here
    // For example: clear application data, stop services, etc.
}

void setup() {
    Serial.begin(115200);
    Serial.println("Starting " + deviceName);
    
    // Configure the portal settings (optional)
    configPortal.setWiFiTimeout(30000);  // 30 seconds
    configPortal.setResetHoldTime(3000); // 3 seconds
    configPortal.setStatusPrintInterval(60000); // 1 minute
    
    // Set up callbacks
    configPortal.onConfig(onConfigurationReceived);
    configPortal.onWiFiConnect(onWiFiConnected);
    configPortal.onWiFiDisconnect(onWiFiDisconnected);
    configPortal.onReset(onConfigReset);
    
    // Optional: Set custom HTML
    // configPortal.setCustomHTML(myCustomHTML, myCustomSuccessHTML);
    
    // Start the configuration portal
    if (configPortal.begin()) {
        Serial.println("Configuration portal started successfully");
        
        // Get current configuration
        ConfigData currentConfig = configPortal.getConfig();
        Serial.println("Current WiFi: " + currentConfig.wifi_ssid);
        
        applicationRunning = true;
    } else {
        Serial.println("Failed to start configuration portal");
    }
}

void loop() {
    // Handle configuration portal (must be called regularly)
    configPortal.handle();
    
    // Your main application logic here
    if (applicationRunning && configPortal.isWiFiConnected()) {
        // Example: Your application code
        runYourApplication();
    }
    
    delay(100);
}

void runYourApplication() {
    // Get current configuration when needed
    ConfigData config = configPortal.getConfig();
    
    // Example: Use Telegram if enabled
    if (config.tg_active && config.tg_token.length() > 0) {
        // Your Telegram bot code here
        // handleTelegramMessages(config.tg_token);
    }
    
    // Example: Use web host if enabled
    if (config.host_active && config.host_url.length() > 0) {
        // Your web host communication code here
        // sendDataToHost(config.host_url, someData);
    }
    
    // Your other application logic
    static unsigned long lastAction = 0;
    if (millis() - lastAction > 10000) { // Every 10 seconds
        lastAction = millis();
        Serial.println("Application running normally...");
        
        // Example: Force config mode under certain conditions
        // if (someErrorCondition) {
        //     configPortal.forceConfigMode();
        // }
        
        // Example: Reset configuration programmatically
        // if (factoryResetRequested) {
        //     configPortal.resetConfig();
        // }
    }
}
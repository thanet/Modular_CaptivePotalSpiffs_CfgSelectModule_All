# Modular_CaptivePotalSpiffs_CfgSelectModule_All
Wifi Telegram  Webhost


# ESP32 Configuration Portal Library

A modular, reusable WiFi configuration portal for ESP32 projects with captive portal functionality. This library provides an easy way to configure WiFi credentials, Telegram bot settings, and web host URLs through a user-friendly web interface.

## Features

- **Captive Portal**: Automatically redirects users to configuration page
- **WiFi Management**: Handles connection, reconnection, and timeout scenarios  
- **Persistent Storage**: Saves configuration to ESP32's preferences
- **Reset Button Support**: Long-press to reset configuration
- **Callback System**: Hooks for configuration events and status changes
- **Custom HTML**: Support for custom configuration pages
- **Modular Design**: Easy to integrate into existing projects
- **Status Monitoring**: Periodic status reporting and connection monitoring

## Hardware Requirements

- ESP32 development board
- Reset button connected to GPIO (default: GPIO0/Boot button)
- WiFi connectivity

## Software Dependencies  

- ESP32 Arduino Core
- ESPAsyncWebServer library
- AsyncTCP library

## Installation

### PlatformIO
```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
lib_deps = 
    ESP32ConfigPortal
    ESPAsyncWebServer
    AsyncTCP
```

### Arduino IDE
1. Download or clone this repository
2. Place in your Arduino libraries folder
3. Install required dependencies through Library Manager

## Basic Usage

```cpp
#include "ESP32ConfigPortal.h"

ESP32ConfigPortal configPortal;

void setup() {
    // Set up callbacks (optional)
    configPortal.onConfig([](const ConfigData& config) {
        Serial.println("New config: " + config.wifi_ssid);
    });
    
    configPortal.onWiFiConnect([]() {
        Serial.println("WiFi connected!");
    });
    
    // Start the portal
    configPortal.begin();
}

void loop() {
    configPortal.handle(); // Must be called regularly
    
    // Your application code here
    if (configPortal.isWiFiConnected()) {
        // Do connected tasks
    }
}
```

## Advanced Configuration

```cpp
ESP32ConfigPortal configPortal(0, "MyDevice-Setup", "my_config");

void setup() {
    // Configure settings
    configPortal.setWiFiTimeout(30000);        // 30 second timeout
    configPortal.setResetHoldTime(5000);       // 5 second hold to reset
    configPortal.setStatusPrintInterval(60000); // Status every minute
    
    // Set callbacks
    configPortal.onConfig(onNewConfig);
    configPortal.onWiFiConnect(onConnected);
    configPortal.onWiFiDisconnect(onDisconnected);
    configPortal.onReset(onConfigReset);
    
    configPortal.begin();
}
```

## Configuration Structure

The `ConfigData` structure contains:

```cpp
struct ConfigData {
    String wifi_ssid;      // WiFi network name
    String wifi_password;  // WiFi password
    String tg_token;       // Telegram bot token
    bool tg_active;        // Telegram enabled flag
    String host_url;       // Web host URL
    bool host_active;      // Web host enabled flag
};
```

## Available Methods

### Core Methods
- `bool begin()` - Initialize and start the portal
- `void handle()` - Process portal events (call in loop)
- `ConfigData getConfig()` - Get current configuration
- `bool isConfigured()` - Check if device is configured
- `bool isWiFiConnected()` - Check WiFi connection status

### Configuration Methods
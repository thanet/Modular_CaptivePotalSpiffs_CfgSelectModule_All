#ifndef ESP32_CONFIG_PORTAL_H
#define ESP32_CONFIG_PORTAL_H

#include <DNSServer.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include "ESPAsyncWebServer.h"
#include <Preferences.h>
#include <functional>

// Configuration structure to hold all settings
struct ConfigData {
    String wifi_ssid;
    String wifi_password;
    String tg_token;
    bool tg_active;
    String host_url;
    bool host_active;
    
    // Constructor with defaults
    ConfigData() : tg_active(false), host_active(false) {}
};

// Callback function types
typedef std::function<void(const ConfigData&)> ConfigCallback;
typedef std::function<void()> StatusCallback;

class ESP32ConfigPortal {
private:
    // Core components
    Preferences preferences;
    DNSServer dnsServer;
    AsyncWebServer server;
    
    // Configuration data
    ConfigData config;
    bool is_setup_done;
    bool config_received;
    bool wifi_timeout;
    
    // Settings
    int reset_button_pin;
    String ap_name;
    String preferences_namespace;
    int wifi_timeout_ms;
    int reset_hold_time_ms;
    int status_print_interval_ms;
    
    // Callbacks
    ConfigCallback onConfigReceived;
    StatusCallback onWiFiConnected;
    StatusCallback onWiFiDisconnected;
    StatusCallback onConfigReset;
    
    // Internal state
    unsigned long lastStatusPrint;
    
    // Private methods
    void setupServer();
    void WiFiSoftAPSetup();
    bool connectToWiFi();
    void startCaptivePortal();
    void loadConfiguration();
    void saveConfiguration();
    void clearConfiguration();
    bool checkResetButton();
    void printStatus();
    
public:
    // Constructor
    ESP32ConfigPortal(int resetPin = 0, 
                     const String& apName = "ESP32-Config",
                     const String& prefsNamespace = "esp32_config");
    
    // Configuration methods
    void setWiFiTimeout(int timeoutMs) { wifi_timeout_ms = timeoutMs; }
    void setResetHoldTime(int holdTimeMs) { reset_hold_time_ms = holdTimeMs; }
    void setStatusPrintInterval(int intervalMs) { status_print_interval_ms = intervalMs; }
    
    // Callback setters
    void onConfig(ConfigCallback callback) { onConfigReceived = callback; }
    void onWiFiConnect(StatusCallback callback) { onWiFiConnected = callback; }
    void onWiFiDisconnect(StatusCallback callback) { onWiFiDisconnected = callback; }
    void onReset(StatusCallback callback) { onConfigReset = callback; }
    
    // Main methods
    bool begin();
    void handle();
    
    // Utility methods
    bool isConfigured() const { return is_setup_done; }
    bool isWiFiConnected() const { return WiFi.status() == WL_CONNECTED; }
    ConfigData getConfig() const { return config; }
    void resetConfig();
    void forceConfigMode();
    
    // Static HTML content (can be customized)
    static String getDefaultHTML();
    static String getSuccessHTML();
    void setCustomHTML(const String& html, const String& successHtml = "");
    
private:
    String custom_html;
    String custom_success_html;
};

#endif // ESP32_CONFIG_PORTAL_H
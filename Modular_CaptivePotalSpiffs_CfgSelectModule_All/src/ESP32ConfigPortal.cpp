#include "ESP32ConfigPortal.h"

// Constructor
ESP32ConfigPortal::ESP32ConfigPortal(int resetPin, const String& apName, const String& prefsNamespace)
    : server(80), reset_button_pin(resetPin), ap_name(apName), preferences_namespace(prefsNamespace),
      is_setup_done(false), config_received(false), wifi_timeout(false),
      wifi_timeout_ms(20000), reset_hold_time_ms(3000), status_print_interval_ms(30000),
      lastStatusPrint(0) {
}

String ESP32ConfigPortal::getDefaultHTML() {
    return R"rawliteral(
<!DOCTYPE HTML><html><head>
  <title>ESP32 Configuration Portal</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: Arial, sans-serif; margin: 20px; background-color: #f0f0f0; }
    .container { background-color: white; padding: 20px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
    .section { margin-bottom: 25px; padding: 15px; border: 1px solid #ddd; border-radius: 5px; }
    .section h4 { margin-top: 0; color: #333; }
    input[type="text"], input[type="password"], input[type="url"] { width: 100%; padding: 8px; margin: 5px 0; border: 1px solid #ddd; border-radius: 4px; }
    input[type="checkbox"] { margin-right: 10px; }
    .wifi-profile { background-color: #f9f9f9; margin: 10px 0; padding: 10px; border-radius: 5px; }
    .submit-btn { background-color: #4CAF50; color: white; padding: 12px 20px; border: none; border-radius: 4px; cursor: pointer; font-size: 16px; }
    .submit-btn:hover { background-color: #45a049; }
  </style>
</head><body>
  <div class="container">
    <h2>ESP32 Configuration Portal</h2>
    <form action="/save" method="POST">
      
      <div class="section">
        <h4>WiFi Configuration</h4>
        <div class="wifi-profile">
          SSID: <input type="text" name="wifi_ssid" placeholder="Network Name" required><br>
          Password: <input type="password" name="wifi_password" placeholder="Network Password">
        </div>
      </div>
      
      <div class="section">
        <h4>Telegram Configuration</h4>
        <input type="checkbox" name="tg_active" value="1"> Enable Telegram<br>
        Bot Token: <input type="text" name="tg_token" placeholder="1234567890:ABCDEF...">
      </div>
      
      <div class="section">
        <h4>Web Host Configuration</h4>
        <input type="checkbox" name="host_active" value="1"> Enable Web Host<br>
        Host URL: <input type="url" name="host_url" placeholder="https://example.com/api">
      </div>
      
      <input type="submit" value="Save Configuration" class="submit-btn">
    </form>
  </div>
</body></html>)rawliteral";
}

String ESP32ConfigPortal::getSuccessHTML() {
    return R"rawliteral(
<!DOCTYPE HTML><html><head>
  <meta http-equiv="refresh" content="15">
  <style>
    body { font-family: Arial, sans-serif; margin: 20px; background-color: #f0f0f0; text-align: center; }
    .container { background-color: white; padding: 30px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); display: inline-block; }
  </style>
</head><body>
  <div class="container">
    <h3>Configuration Saved!</h3>
    <p>Connecting to WiFi network...<br>
    Please wait while the device connects.<br><br>
    This page will refresh automatically in 15 seconds.</p>
  </div>
</body></html>)rawliteral";
}

void ESP32ConfigPortal::setCustomHTML(const String& html, const String& successHtml) {
    custom_html = html;
    custom_success_html = successHtml.length() > 0 ? successHtml : getSuccessHTML();
}

void ESP32ConfigPortal::setupServer() {
    // Clear any existing handlers
    server.reset();
    
    String htmlContent = custom_html.length() > 0 ? custom_html : getDefaultHTML();
    String successContent = custom_success_html.length() > 0 ? custom_success_html : getSuccessHTML();

    // Root route
    server.on("/", HTTP_GET, [this, htmlContent](AsyncWebServerRequest *request) {
        request->send(200, "text/html", htmlContent);
        Serial.println("Configuration portal accessed");
    });

    // Configuration saving route
    server.on("/save", HTTP_POST, [this, successContent](AsyncWebServerRequest *request) {
        Serial.println("Configuration received, processing...");
        
        // Process WiFi credentials
        if (request->hasParam("wifi_ssid", true)) {
            config.wifi_ssid = request->getParam("wifi_ssid", true)->value();
            Serial.println("WiFi SSID: " + config.wifi_ssid);
        }
        if (request->hasParam("wifi_password", true)) {
            config.wifi_password = request->getParam("wifi_password", true)->value();
            Serial.println("WiFi password received");
        }
        
        // Process Telegram settings
        config.tg_active = request->hasParam("tg_active", true);
        if (request->hasParam("tg_token", true)) {
            config.tg_token = request->getParam("tg_token", true)->value();
            Serial.println("Telegram " + String(config.tg_active ? "enabled" : "disabled"));
        }
        
        // Process Host settings
        config.host_active = request->hasParam("host_active", true);
        if (request->hasParam("host_url", true)) {
            config.host_url = request->getParam("host_url", true)->value();
            Serial.println("Web Host " + String(config.host_active ? "enabled" : "disabled"));
        }
        
        config_received = true;
        request->send(200, "text/html", successContent);
    });

    // Wildcard route for captive portal
    server.onNotFound([this, htmlContent](AsyncWebServerRequest *request) {
        request->send(200, "text/html", htmlContent);
    });
}

void ESP32ConfigPortal::WiFiSoftAPSetup() {
    WiFi.mode(WIFI_AP);
    if (!WiFi.softAP(ap_name.c_str())) {
        Serial.println("Failed to start AP");
        delay(1000);
        ESP.restart();
    }
    Serial.print("AP IP address: ");
    Serial.println(WiFi.softAPIP());
}

bool ESP32ConfigPortal::connectToWiFi() {
    wifi_timeout = false;
    WiFi.mode(WIFI_STA);
    
    if (config.wifi_ssid.length() == 0) {
        Serial.println("No WiFi SSID configured");
        return false;
    }
    
    Serial.println("Connecting to WiFi: " + config.wifi_ssid);
    
    WiFi.begin(config.wifi_ssid.c_str(), config.wifi_password.c_str());
    
    uint32_t t1 = millis();
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
        if (millis() - t1 > wifi_timeout_ms) {
            Serial.println("\nTimeout connecting to " + config.wifi_ssid);
            return false;
        }
    }
    
    Serial.println("\nConnected to: " + config.wifi_ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    
    if (onWiFiConnected) {
        onWiFiConnected();
    }
    
    return true;
}

void ESP32ConfigPortal::startCaptivePortal() {
    Serial.println("\nStarting Configuration Portal");
    
    WiFiSoftAPSetup();
    setupServer();
    
    if (!dnsServer.start(53, "*", WiFi.softAPIP())) {
        Serial.println("Failed to start DNS server");
    }
    
    server.begin();
    Serial.println("Configuration Portal Ready");
    Serial.println("Connect to '" + ap_name + "' WiFi network and navigate to any website");
}

void ESP32ConfigPortal::loadConfiguration() {
    preferences.begin(preferences_namespace.c_str(), false);
    
    is_setup_done = preferences.getBool("is_setup_done", false);
    config.wifi_ssid = preferences.getString("wifi_ssid", "");
    config.wifi_password = preferences.getString("wifi_password", "");
    config.tg_token = preferences.getString("tg_token", "");
    config.tg_active = preferences.getBool("tg_active", false);
    config.host_url = preferences.getString("host_url", "");
    config.host_active = preferences.getBool("host_active", false);
    
    preferences.end();
}

void ESP32ConfigPortal::saveConfiguration() {
    preferences.begin(preferences_namespace.c_str(), false);
    
    preferences.putBool("is_setup_done", true);
    preferences.putString("wifi_ssid", config.wifi_ssid);
    preferences.putString("wifi_password", config.wifi_password);
    preferences.putString("tg_token", config.tg_token);
    preferences.putBool("tg_active", config.tg_active);
    preferences.putString("host_url", config.host_url);
    preferences.putBool("host_active", config.host_active);
    
    preferences.end();
    Serial.println("Configuration saved to preferences");
}

void ESP32ConfigPortal::clearConfiguration() {
    preferences.begin(preferences_namespace.c_str(), false);
    preferences.clear();
    preferences.end();
    Serial.println("All configuration cleared");
    
    if (onConfigReset) {
        onConfigReset();
    }
}

bool ESP32ConfigPortal::checkResetButton() {
    if (digitalRead(reset_button_pin) == LOW) {
        delay(50); // Debounce
        
        if (digitalRead(reset_button_pin) == LOW) {
            unsigned long pressStartTime = millis();
            bool heldLongEnough = false;
            int progressCount = 0;
            
            Serial.println("\nReset button detected - HOLD for " + String(reset_hold_time_ms/1000) + " seconds to reset...");
            
            while (digitalRead(reset_button_pin) == LOW) {
                unsigned long holdDuration = millis() - pressStartTime;
                
                if (holdDuration >= (progressCount * 500)) {
                    Serial.print(".");
                    progressCount++;
                }
                
                if (holdDuration >= reset_hold_time_ms) {
                    heldLongEnough = true;
                    break;
                }
                
                delay(10);
            }

            if (heldLongEnough) {
                Serial.println("\n\nResetting configuration...");
                clearConfiguration();
                
                // Wait for button release
                while (digitalRead(reset_button_pin) == LOW) {
                    delay(10);
                }
                
                Serial.println("Restarting device...");
                delay(500);
                ESP.restart();
                return true;
            } else {
                Serial.println("\nButton released - reset cancelled");
            }
        }
    }
    return false;
}

void ESP32ConfigPortal::printStatus() {
    if (millis() - lastStatusPrint > status_print_interval_ms) {
        lastStatusPrint = millis();
        Serial.println("=== Device Status ===");
        Serial.println("WiFi: " + WiFi.SSID() + " (" + WiFi.localIP().toString() + ")");
        Serial.println("Telegram: " + String(config.tg_active ? "ON" : "OFF") + 
                      (config.tg_active && config.tg_token.length() > 0 ? " | Token: " + config.tg_token.substring(0, 10) + "..." : 
                        (config.tg_active ? " | Token: NOT SET" : "")));
        Serial.println("Web Host: " + String(config.host_active ? "ON" : "OFF") + 
                      (config.host_active && config.host_url.length() > 0 ? " | URL: " + config.host_url : 
                        (config.host_active ? " | URL: NOT SET" : "")));
        Serial.println("====================");
    }
}

bool ESP32ConfigPortal::begin() {
    Serial.begin(115200);
    Serial.println("Starting ESP32 Configuration Portal");
    
    // Initialize button pin with internal pull-up
    pinMode(reset_button_pin, INPUT_PULLUP);
    
    // Check for reset button press with debounce
    delay(50);
    if (digitalRead(reset_button_pin) == LOW) {
        delay(50);
        if (digitalRead(reset_button_pin) == LOW) {
            Serial.println("Reset button pressed at startup - clearing saved configuration");
            clearConfiguration();
            
            // Wait for button release
            while (digitalRead(reset_button_pin) == LOW) {
                delay(10);
            }
            
            Serial.println("Restarting device...");
            delay(500);
            ESP.restart();
        }
    }

    // Load saved configuration
    loadConfiguration();
    
    if (is_setup_done) {
        Serial.println("Using saved configuration");
        if (connectToWiFi()) {
            is_setup_done = true;
            return true;
        } else {
            Serial.println("Failed to connect with saved settings, starting config portal");
            is_setup_done = false;
            startCaptivePortal();
        }
    } else {
        startCaptivePortal();
    }

    // Main configuration loop
    while (!is_setup_done) {
        dnsServer.processNextRequest();
        
        if (config_received) {
            config_received = false;
            
            // Trigger callback if set
            if (onConfigReceived) {
                onConfigReceived(config);
            }
            
            // Try to connect to WiFi
            if (connectToWiFi()) {
                is_setup_done = true;
                saveConfiguration();
                
                // Cleanup captive portal
                WiFi.softAPdisconnect(true);
                server.end();
                dnsServer.stop();
                
                Serial.println("Setup complete! WiFi connected and configuration saved.");
                return true;
            } else {
                Serial.println("Failed to connect to WiFi network. Portal remains active.");
                wifi_timeout = true;
                startCaptivePortal();
            }
        }
        
        delay(10);
    }

    Serial.println("Device setup completed successfully!");
    printStatus();
    return true;
}

void ESP32ConfigPortal::handle() {
    // Check WiFi connection status
    if (WiFi.status() != WL_CONNECTED && is_setup_done) {
        Serial.println("WiFi disconnected, attempting reconnection...");
        
        if (onWiFiDisconnected) {
            onWiFiDisconnected();
        }
        
        if (!connectToWiFi()) {
            Serial.println("Failed to reconnect, restarting device...");
            delay(5000);
            ESP.restart();
        }
    }

    // Check reset button
    checkResetButton();
    
    // Print status periodically
    if (is_setup_done) {
        printStatus();
    }
}

void ESP32ConfigPortal::resetConfig() {
    clearConfiguration();
    ESP.restart();
}

void ESP32ConfigPortal::forceConfigMode() {
    is_setup_done = false;
    WiFi.disconnect(true);
    startCaptivePortal();
}
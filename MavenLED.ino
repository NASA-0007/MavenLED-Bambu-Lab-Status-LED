#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include <ESPmDNS.h>
#include <WiFiManager.h>
#include <HTTPClient.h>

#include "src/config/Settings.h"
#include "src/printer/PrinterState.h"
#include "src/led/LEDAnimations.h"
#include "src/network/NetworkManager.h"
#include "src/web/WebHandlers.h"
#include "src/web/webpage.h"

// Task handles for dual-core operation
TaskHandle_t LEDTask;
TaskHandle_t NetworkTask;

// Forward declarations for task functions
void LEDTaskCode(void * pvParameters);
void NetworkTaskCode(void * pvParameters);

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("ESP32 MQTT Client Starting...");
  inAP = false;
  
  // Print initial heap status
  Serial.printf(" Initial Free Heap: %d bytes\n", ESP.getFreeHeap());
  
  // Initialize SPIFFS for token and settings storage
  if (!SPIFFS.begin(true)) {
    Serial.println(" SPIFFS initialization failed!");
  } else {
    Serial.println(" SPIFFS initialized");
  }
  
  // Load settings from SPIFFS
  loadSettings();
  
  Serial.printf(" Free Heap after settings load: %d bytes\n", ESP.getFreeHeap());
  
  // Initialize LED strip with saved pin from settings
  reinitializeStripPin(settings.led_pin);
  Serial.printf(" Using GPIO %d for LED data pin\n", settings.led_pin);
  
  // Initialize or validate RTC persistent state
  if (rtc_state.magic_number != 0xDEADBEEF) {
    rtc_state.printing_active = false;
    rtc_state.last_update_time = 0;
    rtc_state.magic_number = 0xDEADBEEF;
    Serial.println(" RTC state initialized");
  } else {
    Serial.printf(" RTC state valid - printing_active: %s\n", 
                  rtc_state.printing_active ? "true" : "false");
  }
  
  // Initialize animation frame buffer
  saved_frame_buffer = nullptr;
  has_saved_frame = false;
  rainbow_paused = false;
  saved_rainbow_offset = 0;
  saved_rainbow_time = 0;
  Serial.println(" Animation continuity system initialized (including rainbow)");
  
  // Initialize NeoPixel strip with dynamic count
  strip.updateLength(settings.led_count);
  strip.begin();
  strip.show();
  
  startupAnimation();    
  setup_wifi();
  
  // Initialize mDNS for mavenled.local
  if (MDNS.begin("mavenled")) {
    Serial.println(" mDNS responder started - Access via http://mavenled.local");
    MDNS.addService("http", "tcp", 80);
  } else {
    Serial.println(" Error setting up mDNS responder!");
  }
  
  // Setup web server routes
  setupWebServer();
  server.begin();
  Serial.println(" Web server started on port 80");

  // Create mutex for cross-core communication
  printerStateMutex = xSemaphoreCreateMutex();
  
  // Create LED task on Core 1 (dedicated to LED animations)
  xTaskCreatePinnedToCore(
    LEDTaskCode,   // Task function
    "LEDTask",     // Name of task
    10000,         // Stack size of task
    NULL,          // Parameter of the task
    2,             // Priority of the task (2 = higher priority for smooth animations)
    &LEDTask,      // Task handle to keep track of created task
    1);            // Pin task to core 1
    
  // Create Network task on Core 0 (dedicated to MQTT/WiFi)
  xTaskCreatePinnedToCore(
    NetworkTaskCode, // Task function
    "NetworkTask",   // Name of task
    8000,           // Stack size of task
    NULL,           // Parameter of the task
    1,              // Priority of the task (1 = lower priority)
    &NetworkTask,   // Task handle to keep track of created task
    0);             // Pin task to core 0
    
  Serial.println(" LED task created on Core 1 (Priority 2)");
  Serial.println(" Network task created on Core 0 (Priority 1)");
  
  // Initialize MQTT buffer size for large messages
  client.setBufferSize(16384); // 16KB for large JSON messages
  Serial.println(" MQTT buffer size set to 16KB for large JSON messages");
  
  // Initialize Remote Control MQTT client
  if (settings.remote_control_enabled) {
    remoteControlClient.setBufferSize(2048);
    remoteControlClient.setServer(settings.remote_mqtt_server, settings.remote_mqtt_port);
    remoteControlClient.setCallback(remoteControlCallback);
    remoteControlClient.setKeepAlive(60);
    reconnectRemoteControl();
    Serial.printf(" Remote MQTT configured: %s:%d (buffer: 2KB, keepalive: 60s)\n", 
                  settings.remote_mqtt_server, settings.remote_mqtt_port);
  }
  
  // IMPORTANT: Do NOT start MQTT here - defer it for system stability
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(" WiFi connected - scheduling delayed MQTT startup...");
  } else {
    Serial.println(" WiFi not connected - MQTT service will start when WiFi is available");
  }
  
  // Mark system startup time
  system_startup_time = millis();
  Serial.println(" Core system setup complete. Starting stabilization period...");
  
  Serial.println(" System initialization complete. Starting main loop with delayed services...");
  
  // Initialize connection state properly
  printer_state.is_connected = false;
  printer_state.status = "initializing";
  printer_state.last_temp_change_time = millis();
  lastMQTTupdate = millis();
  lastMQTTProcessTime = 0;
}

void loop() {
  unsigned long currentTime = millis();
  
  system_fully_initialized = true;
  
  // Handle web server requests
  server.handleClient();
  
  // Handle remote control MQTT (only after system is fully initialized)
  if (settings.remote_control_enabled && system_fully_initialized) {
    if (!remoteControlClient.connected()) {
      static unsigned long lastRemoteReconnectAttempt = 0;
      if (millis() - lastRemoteReconnectAttempt > 30000) {
        Serial.println(" Remote MQTT disconnected, attempting reconnect...");
        reconnectRemoteControl();
        lastRemoteReconnectAttempt = millis();
      }
    } else {
      remoteControlClient.loop();
      
      // Publish periodic status updates (every 30 seconds)
      static unsigned long lastStatusUpdate = 0;
      if (millis() - lastStatusUpdate > 30000) {
        publishDeviceStatus();
        lastStatusUpdate = millis();
      }
      
      // Publish periodic printer status updates (every 10 seconds if connected)
      static unsigned long lastPrinterStatusUpdate = 0;
      if (printer_state.is_connected && millis() - lastPrinterStatusUpdate > 10000) {
        publishPrinterStatus();
        lastPrinterStatusUpdate = millis();
      }
    }
  }
  
  // Main loop heartbeat
  static unsigned long lastMainHeartbeat = 0;
  if (millis() - lastMainHeartbeat > 60000) {
    Serial.println(" Main loop heartbeat - tasks running on dedicated cores");
    lastMainHeartbeat = millis();
  }
  
  // Feed watchdog
  yield();
}

// LED Task - runs on Core 1 for smooth animations
void LEDTaskCode(void * pvParameters) {
  Serial.println(" LED Task started on Core 1");
  for(;;) {
    if (xSemaphoreTake(printerStateMutex, 10 / portTICK_PERIOD_MS) == pdTRUE) {
      bool timeoutTriggered = false;
      
      // Check finish animation timeout
      if (printer_state.finish_animation_active) {
        unsigned long finishDuration = millis() - printer_state.finish_animation_start;
        if (finishDuration >= 120000) {
          printer_state.finish_animation_active = false;
          printer_state.state_override_active = true;
          printer_state.state_override_start = millis();
          printer_state.override_reason = "finish timeout";
          printer_state.status = "idle";
          Serial.println("️ Finish animation timeout - forcing idle state");
          timeoutTriggered = true;
        }
      }
      
      // Check error recovery timeout
      if (printer_state.error_recovery_active) {
        unsigned long errorDuration = millis() - printer_state.error_recovery_start;
        if (errorDuration >= 120000) {
          printer_state.error_recovery_active = false;
          printer_state.state_override_active = true;
          printer_state.state_override_start = millis();
          printer_state.override_reason = "error timeout";
          printer_state.status = "idle";
          Serial.println("️ Error recovery timeout - forcing idle state");
          timeoutTriggered = true;
        }
      }
      
      if (!timeoutTriggered) {
        determinePrinterStatus();
      }
      
      updateLEDDisplay();
      xSemaphoreGive(printerStateMutex);
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

// Network Task - runs on Core 0 for MQTT/WiFi operations
void NetworkTaskCode(void * pvParameters) {
  Serial.println(" Network Task started on Core 0");
  for(;;) {
    // WiFi connection check
    if (WiFi.status() != WL_CONNECTED) {
      if (client.connected()) {
        Serial.println(" WiFi disconnected - stopping MQTT service");
        stopMQTTService();
      }
      
      unsigned long currentTime = millis();
      if (currentTime - last_wifi_attempt > WIFI_RETRY_INTERVAL || last_wifi_attempt == 0) {
        last_wifi_attempt = currentTime;
        
        Serial.printf(" WiFi disconnected! Failure count: %d/%d\n", wifi_failure_count, MAX_WIFI_FAILURES);
        
        if (wifi_failure_count >= MAX_WIFI_FAILURES) {
          bool deferRestart = false;
          if (xSemaphoreTake(printerStateMutex, 10 / portTICK_PERIOD_MS) == pdTRUE) {
            if (printer_state.status == "printing" || (printer_state.progress > 0 && printer_state.progress < 100)) {
              deferRestart = true;
            }
            xSemaphoreGive(printerStateMutex);
          }
          
          if (!deferRestart && rtc_state.printing_active && rtc_state.magic_number == 0xDEADBEEF) {
            unsigned long timeSinceLastUpdate = millis() - rtc_state.last_update_time;
            if (timeSinceLastUpdate < 600000) {
              deferRestart = true;
              Serial.println("️ RTC indicates printing was active - deferring restart");
            }
          }

          if (deferRestart) {
            Serial.println("️ WiFi failures reached but print job active; deferring restart");
            last_wifi_attempt = currentTime;
          } else {
            Serial.println(" Maximum WiFi failures reached - RESTARTING DEVICE!");
            delay(2000);
            ESP.restart();
          }
        }
        
        if(!inAP){
          if (!connectToWiFi()) {
            Serial.printf(" WiFi connection failed (%d/%d). Next retry in 1 minute.\n", 
                          wifi_failure_count, MAX_WIFI_FAILURES);
            
            if (xSemaphoreTake(printerStateMutex, 10 / portTICK_PERIOD_MS) == pdTRUE) {
              printer_state.is_connected = false;
              printer_state.status = "unknown";
              printer_state.raw_gcode_state = "unknown";
              xSemaphoreGive(printerStateMutex);
            }
          } else {
            mqtt_restart_pending = true;
            mqtt_restart_time = millis() + MQTT_RESTART_DELAY;
            Serial.printf(" WiFi reconnected - MQTT restart scheduled in %d seconds\n", MQTT_RESTART_DELAY / 1000);
          }
        }
      }
    }
    
    // Handle MQTT restart after WiFi reconnection
    if (mqtt_restart_pending && system_fully_initialized && millis() >= mqtt_restart_time) {
      mqtt_restart_pending = false;
      Serial.println(" Starting MQTT service after WiFi reconnection...");
      startMQTTService();
    }
    
    // Handle MQTT connection
    if (system_fully_initialized && WiFi.status() == WL_CONNECTED && !mqtt_restart_pending && !client.connected()) {
      unsigned long currentTime = millis();
      
      if (currentTime - lastMQTTReconnectAttempt >= 10000) {
        lastMQTTReconnectAttempt = currentTime;
        
        Serial.println(" MQTT disconnected. Attempting to reconnect...");
        
        if (isGlobalMode()) {
          Serial.println(" Global mode: Freeing memory before reconnect...");
          client.disconnect();
          delay(500);
          Serial.printf(" Free heap before reconnect: %d bytes\n", ESP.getFreeHeap());
        }
        
        startMQTTService();
      }
    }
    
    // Process MQTT messages
    if (system_fully_initialized && client.connected()) {
      if (!client.loop()) {
        Serial.println("️ MQTT loop failed - connection may be unstable");
      }
    }
    
    // Check connection timeout
    checkConnectionTimeout();
    
    // Network heartbeat
    static unsigned long lastHeartbeat = 0;
    if (millis() - lastHeartbeat > 30000) {
      if (xSemaphoreTake(printerStateMutex, 10 / portTICK_PERIOD_MS) == pdTRUE) {
        Serial.printf(" Network Heartbeat - Status: %s | Progress: %d%% | Connected: %s | WiFi: %s (Fails: %d/%d) | MQTT: %s\n", 
                      printer_state.status.c_str(), printer_state.progress,
                      printer_state.is_connected ? "Yes" : "No",
                      WiFi.status() == WL_CONNECTED ? "OK" : "FAIL",
                      wifi_failure_count, MAX_WIFI_FAILURES,
                      client.connected() ? "OK" : "FAIL");
        xSemaphoreGive(printerStateMutex);
      }
      lastHeartbeat = millis();
    }
    
    yield();
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

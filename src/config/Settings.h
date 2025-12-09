#ifndef SETTINGS_H
#define SETTINGS_H

#include <Arduino.h>

// Settings structure (stored in SPIFFS as JSON)
struct LEDSettings {
  // Hardware settings
  int led_count = 60;     // Number of LEDs in the strip (default 60)
  int led_pin = 17;       // GPIO pin for LED data (default 17)
  
  // Custom colors (RGB values 0-255)
  struct {
    uint8_t r, g, b;
  } colors[8] = {
    {0, 50, 0},     // idle
    {75, 0, 130},   // printing
    {0, 0, 255},    // download
    {255, 255, 0},  // paused
    {255, 0, 0},    // error
    {255, 100, 0},  // heating
    {0, 255, 255},  // cooling
    {0, 255, 0}     // finished
  };
  
  // Animation directions (1 = normal, -1 = reversed)
  int rainbow_direction = 1;
  int idle_direction = 1;
  int printing_direction = 1;
  int download_direction = 1;
  
  // Brightness settings (0-255)
  uint8_t global_brightness = 255;
  uint8_t night_mode_brightness = 25;
  bool night_mode_enabled = false;
  
  // State timeout tracking (persistent across power cycles)
  bool state_timeout_reached = false;
  
  // Lights control (persistent across power cycles)
  bool lights_off_override = false;
  
  // P1 Series Mode (prevents heating/cooling states during printing)
  bool p1_series_mode = false;
  
  // Idle timeout settings
  bool idle_timeout_enabled = false;
  int idle_timeout_minutes = 5;
  
  // WiFi credentials
  char wifi_ssid[64] = "";
  char wifi_password[64] = "";
  
  // MQTT credentials (Local Mode)
  char mqtt_server[64] = "";
  char mqtt_password[32] = "";
  char device_serial[32] = "";
  
  // MQTT Mode Selection
  bool mqtt_mode_global = false;  // false = local, true = global
  
  // Global MQTT credentials
  char global_email[128] = "";
  char global_username[32] = "";
  unsigned long token_expires_at = 0;
  unsigned long pushall_last_request = 0;
  bool has_valid_auth = false;
  
  // Remote Control MQTT Settings
  bool remote_control_enabled = true;
  char remote_mqtt_server[64] = "broker.hivemq.com";
  int remote_mqtt_port = 1883;
  char device_id[32] = "";
  char remote_username[32] = "";
  char remote_password[64] = "";
};

extern LEDSettings settings;

// Settings management functions
void saveSettings();
void loadSettings();

// Token management functions
bool saveTokenToSPIFFS(const char* filename, const String& token);
String loadTokenFromSPIFFS(const char* filename);
void deleteTokenFromSPIFFS(const char* filename);
void saveAccessToken(const String& token);
void saveRefreshToken(const String& token);
String getAccessToken();
String getRefreshToken();
void clearAllTokens();

#endif

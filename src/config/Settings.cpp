#include "Settings.h"
#include <SPIFFS.h>
#include <ArduinoJson.h>

LEDSettings settings;

void saveSettings() {
  DynamicJsonDocument doc(2048);
  
  // Hardware settings
  doc["led_count"] = settings.led_count;
  doc["led_pin"] = settings.led_pin;
  
  // Colors
  JsonArray colors = doc.createNestedArray("colors");
  for (int i = 0; i < 8; i++) {
    JsonObject color = colors.createNestedObject();
    color["r"] = settings.colors[i].r;
    color["g"] = settings.colors[i].g;
    color["b"] = settings.colors[i].b;
  }
  
  // Directions
  doc["rainbow_direction"] = settings.rainbow_direction;
  doc["idle_direction"] = settings.idle_direction;
  doc["printing_direction"] = settings.printing_direction;
  doc["download_direction"] = settings.download_direction;
  
  // Brightness
  doc["global_brightness"] = settings.global_brightness;
  doc["night_mode_brightness"] = settings.night_mode_brightness;
  doc["night_mode_enabled"] = settings.night_mode_enabled;
  
  // State flags
  doc["state_timeout_reached"] = settings.state_timeout_reached;
  doc["lights_off_override"] = settings.lights_off_override;
  
  // P1 Series Mode
  doc["p1_series_mode"] = settings.p1_series_mode;
  
  // Idle timeout
  doc["idle_timeout_enabled"] = settings.idle_timeout_enabled;
  doc["idle_timeout_minutes"] = settings.idle_timeout_minutes;
  
  // WiFi
  doc["wifi_ssid"] = settings.wifi_ssid;
  doc["wifi_password"] = settings.wifi_password;
  
  // MQTT
  doc["mqtt_server"] = settings.mqtt_server;
  doc["mqtt_password"] = settings.mqtt_password;
  doc["device_serial"] = settings.device_serial;
  doc["mqtt_mode_global"] = settings.mqtt_mode_global;
  
  // Global mode
  doc["global_email"] = settings.global_email;
  doc["global_username"] = settings.global_username;
  doc["token_expires_at"] = settings.token_expires_at;
  doc["pushall_last_request"] = settings.pushall_last_request;
  doc["has_valid_auth"] = settings.has_valid_auth;
  
  // Remote control
  doc["remote_control_enabled"] = settings.remote_control_enabled;
  doc["remote_mqtt_server"] = settings.remote_mqtt_server;
  doc["remote_mqtt_port"] = settings.remote_mqtt_port;
  doc["device_id"] = settings.device_id;
  doc["remote_username"] = settings.remote_username;
  doc["remote_password"] = settings.remote_password;
  
  File file = SPIFFS.open("/settings.json", "w");
  if (!file) {
    Serial.println(" Failed to open settings file for writing");
    return;
  }
  
  serializeJson(doc, file);
  file.close();
  Serial.println(" Settings saved to SPIFFS");
  Serial.printf("   MQTT Mode: %s\n", settings.mqtt_mode_global ? "Global" : "Local");
}

void loadSettings() {
  if (!SPIFFS.exists("/settings.json")) {
    Serial.println("️ Settings file not found, using defaults");
    saveSettings();
    return;
  }
  
  File file = SPIFFS.open("/settings.json", "r");
  if (!file) {
    Serial.println(" Failed to open settings file");
    return;
  }
  
  DynamicJsonDocument doc(2048);
  DeserializationError error = deserializeJson(doc, file);
  file.close();
  
  if (error) {
    Serial.printf(" Failed to parse settings: %s\n", error.c_str());
    Serial.println("️ Using defaults");
    saveSettings();
    return;
  }
  
  // Load all settings
  settings.led_count = doc["led_count"] | 60;
  settings.led_pin = doc["led_pin"] | 17;
  
  // Colors
  JsonArray colors = doc["colors"];
  if (colors.size() == 8) {
    for (int i = 0; i < 8; i++) {
      settings.colors[i].r = colors[i]["r"];
      settings.colors[i].g = colors[i]["g"];
      settings.colors[i].b = colors[i]["b"];
    }
  }
  
  // Directions
  settings.rainbow_direction = doc["rainbow_direction"] | 1;
  settings.idle_direction = doc["idle_direction"] | 1;
  settings.printing_direction = doc["printing_direction"] | 1;
  settings.download_direction = doc["download_direction"] | 1;
  
  // Brightness
  settings.global_brightness = doc["global_brightness"] | 255;
  settings.night_mode_brightness = doc["night_mode_brightness"] | 25;
  settings.night_mode_enabled = doc["night_mode_enabled"] | false;
  
  // State flags
  settings.state_timeout_reached = doc["state_timeout_reached"] | false;
  settings.lights_off_override = doc["lights_off_override"] | false;
  
  // P1 Series Mode
  settings.p1_series_mode = doc["p1_series_mode"] | false;
  
  // Idle timeout
  settings.idle_timeout_enabled = doc["idle_timeout_enabled"] | false;
  settings.idle_timeout_minutes = doc["idle_timeout_minutes"] | 5;
  
  // WiFi
  strlcpy(settings.wifi_ssid, doc["wifi_ssid"] | "", sizeof(settings.wifi_ssid));
  strlcpy(settings.wifi_password, doc["wifi_password"] | "", sizeof(settings.wifi_password));
  
  // MQTT
  strlcpy(settings.mqtt_server, doc["mqtt_server"] | "", sizeof(settings.mqtt_server));
  strlcpy(settings.mqtt_password, doc["mqtt_password"] | "", sizeof(settings.mqtt_password));
  strlcpy(settings.device_serial, doc["device_serial"] | "", sizeof(settings.device_serial));
  settings.mqtt_mode_global = doc["mqtt_mode_global"] | false;
  
  // Global mode
  strlcpy(settings.global_email, doc["global_email"] | "", sizeof(settings.global_email));
  strlcpy(settings.global_username, doc["global_username"] | "", sizeof(settings.global_username));
  settings.token_expires_at = doc["token_expires_at"] | 0;
  settings.pushall_last_request = doc["pushall_last_request"] | 0;
  settings.has_valid_auth = doc["has_valid_auth"] | false;
  
  // Remote control
  settings.remote_control_enabled = doc["remote_control_enabled"] | true;
  strlcpy(settings.remote_mqtt_server, doc["remote_mqtt_server"] | "broker.hivemq.com", sizeof(settings.remote_mqtt_server));
  settings.remote_mqtt_port = doc["remote_mqtt_port"] | 1883;
  strlcpy(settings.device_id, doc["device_id"] | "MavenLED", sizeof(settings.device_id));
  strlcpy(settings.remote_username, doc["remote_username"] | "", sizeof(settings.remote_username));
  strlcpy(settings.remote_password, doc["remote_password"] | "", sizeof(settings.remote_password));
  
  Serial.println(" Settings loaded from SPIFFS");
  Serial.printf("   MQTT Mode: %s\n", settings.mqtt_mode_global ? "Global" : "Local");
  Serial.printf("   Serial: %s\n", settings.device_serial);
}

// Token storage functions
bool saveTokenToSPIFFS(const char* filename, const String& token) {
  File file = SPIFFS.open(filename, "w");
  if (!file) {
    Serial.printf(" Failed to open %s for writing\n", filename);
    return false;
  }
  file.print(token);
  file.close();
  Serial.printf(" Saved token to SPIFFS: %s (%d bytes)\n", filename, token.length());
  return true;
}

String loadTokenFromSPIFFS(const char* filename) {
  if (!SPIFFS.exists(filename)) {
    Serial.printf("️ Token file not found: %s\n", filename);
    return "";
  }
  File file = SPIFFS.open(filename, "r");
  if (!file) {
    Serial.printf(" Failed to open %s for reading\n", filename);
    return "";
  }
  
  size_t fileSize = file.size();
  String token;
  token.reserve(fileSize + 1);
  
  while (file.available()) {
    token += (char)file.read();
  }
  
  file.close();
  Serial.printf(" Loaded token from SPIFFS: %s (%d bytes)\n", filename, token.length());
  return token;
}

void deleteTokenFromSPIFFS(const char* filename) {
  if (SPIFFS.exists(filename)) {
    SPIFFS.remove(filename);
    Serial.printf("️ Deleted token: %s\n", filename);
  }
}

void saveAccessToken(const String& token) {
  saveTokenToSPIFFS("/access_token.txt", token);
}

void saveRefreshToken(const String& token) {
  saveTokenToSPIFFS("/refresh_token.txt", token);
}

String getAccessToken() {
  return loadTokenFromSPIFFS("/access_token.txt");
}

String getRefreshToken() {
  return loadTokenFromSPIFFS("/refresh_token.txt");
}

void clearAllTokens() {
  deleteTokenFromSPIFFS("/access_token.txt");
  deleteTokenFromSPIFFS("/refresh_token.txt");
  settings.has_valid_auth = false;
  settings.token_expires_at = 0;
  saveSettings();
  Serial.println("️ All tokens cleared");
}

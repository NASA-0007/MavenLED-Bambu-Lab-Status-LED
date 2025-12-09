#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <HTTPClient.h>
#include <Arduino.h>
#include <ArduinoJson.h>

// Relay server configuration
extern const char* RELAY_SERVER_URL;

// Network clients
extern WiFiClientSecure espClient;
extern PubSubClient client;
extern WiFiClient remoteClient;
extern PubSubClient remoteControlClient;

// Network state
extern unsigned long lastMQTTupdate;
extern unsigned long lastMQTTProcessTime;
extern unsigned long lastMQTTReconnectAttempt;
extern unsigned long mqttConnectionTime;
extern bool inAP;

// WiFi failure tracking
extern volatile int wifi_failure_count;
extern volatile unsigned long last_wifi_attempt;
extern volatile bool wifi_just_reconnected;
extern volatile unsigned long wifi_reconnect_time;
extern const int MAX_WIFI_FAILURES;
extern const unsigned long WIFI_RETRY_INTERVAL;

// MQTT restart timing
extern volatile bool mqtt_restart_pending;
extern volatile unsigned long mqtt_restart_time;
extern const unsigned long MQTT_RESTART_DELAY;

// System initialization
extern volatile bool system_fully_initialized;
extern volatile unsigned long system_startup_time;
extern volatile bool delayed_mqtt_start_pending;
extern volatile unsigned long delayed_mqtt_start_time;
extern const unsigned long SYSTEM_STABILIZATION_MS;

// Remote control topics
String getRemoteControlTopicBase();
String getRemoteCommandTopic();
String getRemoteStatusTopic();
String getRemoteAckTopic();
String getRemotePrinterStatusTopic();

// Remote control command structure
struct PendingCommand {
  String command_id = "";
  unsigned long timestamp = 0;
  bool acknowledged = false;
};
extern PendingCommand lastCommand;

// Network functions
void setup_wifi();
void startAPMode();
bool connectToWiFi();
void reconnect();
void startMQTTService(bool isInitialConnection = false);
void stopMQTTService();
void callback(char* topic, byte* payload, unsigned int length);
void remoteControlCallback(char* topic, byte* payload, unsigned int length);
void processRemoteCommand(DynamicJsonDocument& cmd);
void sendCommandAck(String command_id, bool success, String message = "");
void publishDeviceStatus();
void publishPrinterStatus();
void reconnectRemoteControl();

// Global mode support
bool isGlobalMode();
String getGlobalMQTTUsername();
String getMQTTTopic();
bool useSavedMQTTSettings();
bool isTokenExpired();
bool shouldRenewToken();

// Authentication
struct AuthResult {
  bool success;
  String error;
  String accessToken;
  String refreshToken;
  unsigned long expiresIn;
};

AuthResult performInitialLogin(const String& email, const String& password);
AuthResult performVerificationLogin(const String& email, const String& code);

#endif

#include "NetworkManager.h"
#include "../config/Settings.h"
#include "../printer/PrinterState.h"
#include "../web/WebHandlers.h"
#include <ArduinoJson.h>
#include <ESPmDNS.h>

// Constants
const char* RELAY_SERVER_URL = "https://maven-led-server.vercel.app";
const int MAX_WIFI_FAILURES = 5;
const unsigned long WIFI_RETRY_INTERVAL = 30000;
const unsigned long MQTT_RESTART_DELAY = 2000;
const unsigned long SYSTEM_STABILIZATION_MS = 10000;

// Network Global Variables
WiFiClientSecure espClient;
PubSubClient client(espClient);
WiFiClient remoteClient;
PubSubClient remoteControlClient(remoteClient);

unsigned long lastMQTTupdate = 0;
unsigned long lastMQTTProcessTime = 0;
unsigned long lastMQTTReconnectAttempt = 0;

volatile int wifi_failure_count = 0;
volatile unsigned long last_wifi_attempt = 0;
volatile bool wifi_just_reconnected = false;
volatile unsigned long wifi_reconnect_time = 0;
volatile bool mqtt_restart_pending = false;
volatile unsigned long mqtt_restart_time = 0;

// System initialization variables
volatile bool system_fully_initialized = false;
volatile unsigned long system_startup_time = 0;
volatile bool delayed_mqtt_start_pending = false;
volatile unsigned long delayed_mqtt_start_time = 0;

PendingCommand lastCommand;
bool inAP = false;

// Global Mode Functions
bool isGlobalMode() {
	return settings.mqtt_mode_global;
}

String getGlobalMQTTUsername() {
	if (strlen(settings.global_username) == 0) return "";
	return "u_" + String(settings.global_username);
}

AuthResult performInitialLogin(const String& email, const String& password) {
	AuthResult result = {false, "", "", "", 0};
	
	WiFiClientSecure *client = new WiFiClientSecure;
	client->setInsecure();
	client->setTimeout(30);
	
	HTTPClient http;
	String apiUrl = String(RELAY_SERVER_URL) + "/bambu/login";
	http.begin(*client, apiUrl);
	
	http.addHeader("Content-Type", "application/json");
	http.setTimeout(30000);
	http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
	
	DynamicJsonDocument requestDoc(256);
	requestDoc["account"] = email;
	requestDoc["password"] = password;
	
	String requestBody;
	serializeJson(requestDoc, requestBody);

	Serial.println(" Sending login request to relay server:");
	Serial.println(requestBody);
	Serial.printf(" WiFi Status: %d, Free Heap: %d\n", WiFi.status(), ESP.getFreeHeap());
	
	int httpResponseCode = http.POST(requestBody);
	
	if (httpResponseCode > 0) {
		String response = http.getString();
		Serial.printf(" Relay server response: %d\n", httpResponseCode);
		Serial.printf(" Response body: %s\n", response.c_str());
		
		DynamicJsonDocument responseDoc(2048);
		DeserializationError error = deserializeJson(responseDoc, response);
		
		if (!error) {
			if (httpResponseCode == 200) {
				if (responseDoc.containsKey("accessToken") && responseDoc["accessToken"].as<String>().length() > 0) {
					result.success = true;
					result.accessToken = responseDoc["accessToken"].as<String>();
					result.refreshToken = responseDoc["refreshToken"].as<String>();
					result.expiresIn = responseDoc["expiresIn"];
					result.error = "Login successful";
				} else if (responseDoc.containsKey("loginType") && responseDoc["loginType"] == "verifyCode") {
					result.success = true;
					result.error = "Verification code sent to email";
				} else {
					result.success = true;
					result.error = "Verification code sent to email";
				}
			} else {
				result.error = responseDoc["error"].as<String>();
				if (result.error.isEmpty()) {
					result.error = responseDoc["message"].as<String>();
				}
				if (result.error.isEmpty()) {
					result.error = "Login failed";
				}
			}
		} else {
			result.error = "Invalid response format: " + String(error.c_str());
			Serial.printf(" JSON parse error: %s\n", error.c_str());
		}
	} else {
		result.error = "Network error: " + String(httpResponseCode);
		Serial.printf(" HTTP Error: %d\n", httpResponseCode);
	}
	
	http.end();
	delete client;
	return result;
}

AuthResult performVerificationLogin(const String& email, const String& code) {
	AuthResult result = {false, "", "", "", 0};
	
	WiFiClientSecure *client = new WiFiClientSecure;
	client->setInsecure();
	client->setTimeout(30);
	
	HTTPClient http;
	String apiUrl = String(RELAY_SERVER_URL) + "/bambu/login";
	http.begin(*client, apiUrl);
	
	http.addHeader("Content-Type", "application/json");
	http.setTimeout(30000);
	http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
	
	DynamicJsonDocument requestDoc(256);
	requestDoc["account"] = email;
	requestDoc["code"] = code;
	
	String requestBody;
	serializeJson(requestDoc, requestBody);
	
	Serial.println(" Sending verification code to relay server:");
	Serial.println(requestBody);
	
	int httpResponseCode = http.POST(requestBody);
	
	if (httpResponseCode > 0) {
		String response = http.getString();
		Serial.printf(" Verification response: %d\n", httpResponseCode);
		Serial.printf(" Response body: %s\n", response.c_str());
		
		DynamicJsonDocument responseDoc(2048);
		DeserializationError error = deserializeJson(responseDoc, response);
		
		if (!error) {
			if (httpResponseCode == 200) {
				if (responseDoc.containsKey("accessToken") && responseDoc["accessToken"].as<String>().length() > 0) {
					result.success = true;
					result.accessToken = responseDoc["accessToken"].as<String>();
					result.refreshToken = responseDoc["refreshToken"].as<String>();
					result.expiresIn = responseDoc["expiresIn"];
					
					Serial.printf(" Verification successful! Access token: %.20s...\n", result.accessToken.c_str());
					Serial.printf(" Token expires in: %lu seconds\n", result.expiresIn);
				} else {
					result.error = "No access token in response";
				}
			} else {
				result.error = responseDoc["error"].as<String>();
				if (result.error.isEmpty()) {
					result.error = responseDoc["message"].as<String>();
				}
				if (result.error.isEmpty()) {
					result.error = "Verification failed";
				}
			}
		} else {
			result.error = "Invalid response format: " + String(error.c_str());
			Serial.printf(" JSON parse error: %s\n", error.c_str());
		}
	} else {
		result.error = "Network error: " + String(httpResponseCode);
		Serial.printf(" HTTP Error: %d\n", httpResponseCode);
	}
	
	http.end();
	delete client;
	return result;
}

bool isTokenExpired() {
	if (!isGlobalMode() || getAccessToken().length() == 0) return false;
	if (settings.token_expires_at == 0) return false;
	
	unsigned long currentTime = millis() / 1000;
	return currentTime >= settings.token_expires_at;
}

bool shouldRenewToken() {
	if (!isGlobalMode() || getAccessToken().length() == 0) return false;
	if (settings.token_expires_at == 0) return false;
	
	unsigned long currentTime = millis() / 1000;
	unsigned long timeUntilExpiry = settings.token_expires_at - currentTime;
	
	return timeUntilExpiry < (7 * 24 * 60 * 60);
}

String getMQTTTopic() {
	return "device/" + String(settings.device_serial) + "/report";
}

bool useSavedMQTTSettings() {
	return strlen(settings.mqtt_server) > 0 && strlen(settings.device_serial) > 0;
}

void callback(char* topic, byte* payload, unsigned int length) {
	unsigned long currentTime = millis();
	if (currentTime - lastMQTTProcessTime < 500) {
		lastMQTTupdate = currentTime;
		printer_state.is_connected = true;
		return;
	}
	
	if (isGlobalMode() && length < 50) {
		lastMQTTupdate = currentTime;
		printer_state.is_connected = true;
		Serial.printf("️ Dumping small message (%d bytes)\n", length);
		return;
	}
	
	if (length > 50 && printer_state.status == "initializing") {
		printer_state.status = "idle";
		Serial.println(" Printer connected - status changed to idle");
	}
	
	size_t freeHeap = ESP.getFreeHeap();
	if (freeHeap < 40000) {
		Serial.printf("️ Low heap (%d bytes) - dumping message\n", freeHeap);
		lastMQTTupdate = currentTime;
		printer_state.is_connected = true;
		return;
	}
	
	DynamicJsonDocument doc(16384);
	DeserializationError error = deserializeJson(doc, payload, length);
	
	if (error) {
		Serial.printf("️ JSON parsing failed: %s (length: %d) - dumping\n", error.c_str(), length);
		printer_state.is_connected = true;
		lastMQTTupdate = millis();
		return;
	}
	
	updatePrinterState(doc);
	lastMQTTupdate = millis();
	lastMQTTProcessTime = currentTime;
	printer_state.is_connected = true;
	
	Serial.println(" MQTT Message processed:");
	Serial.printf("   Size: %d bytes\n", length);
	Serial.printf("   Status: %s\n", printer_state.status.c_str());
	Serial.printf("   Progress: %d%%\n", printer_state.progress);
	Serial.printf("   Temps: N:%d°C B:%d°C\n", printer_state.nozzle_temp, printer_state.bed_temp);
	
	doc.clear();
}

bool connectToWiFi() {
	const char* wifiSSID = settings.wifi_ssid;
	const char* wifiPassword = settings.wifi_password;
	
	if (strlen(wifiSSID) == 0) {
		Serial.println(" No WiFi credentials available!");
		return false;
	}
	
	Serial.printf(" Attempting WiFi connection to: %s\n", wifiSSID);
	
	WiFi.disconnect(true);
	WiFi.mode(WIFI_OFF);
	delay(1000);
	WiFi.mode(WIFI_STA);
	delay(1000);
	
	WiFi.setAutoReconnect(false);
	WiFi.setSleep(false);
	
	WiFi.begin(wifiSSID, wifiPassword);
	
	int attempts = 0;
	const int maxAttempts = 40;
	
	while (WiFi.status() != WL_CONNECTED && attempts < maxAttempts) {
		delay(500);
		yield();
		attempts++;
		
		if (attempts % 4 == 0) {
			Serial.printf("    Attempt %d/%d - Status: %d\n", attempts/4, maxAttempts/4, WiFi.status());
		}
		
		if (WiFi.status() == WL_CONNECT_FAILED) {
			Serial.println("    Connection failed - wrong credentials?");
			break;
		}
		if (WiFi.status() == WL_NO_SSID_AVAIL) {
			Serial.println("    SSID not found - network unavailable?");
			break;
		}
	}
	
	if (WiFi.status() == WL_CONNECTED) {
		Serial.println(" WiFi connected successfully!");
		Serial.printf("    IP: %s (Static) | RSSI: %d dBm | Channel: %d\n", 
					  WiFi.localIP().toString().c_str(), 
					  WiFi.RSSI(), 
					  WiFi.channel());
		Serial.printf("    Gateway: %s | Subnet: %s\n", 
					  WiFi.gatewayIP().toString().c_str(),
					  WiFi.subnetMask().toString().c_str());
		wifi_failure_count = 0;
		
		wifi_just_reconnected = true;
		wifi_reconnect_time = millis();
		Serial.println(" WiFi reconnected - MQTT will restart in 2 seconds...");
		
		return true;
	} else {
		Serial.printf(" WiFi connection failed after %d attempts (Status: %d)\n", attempts, WiFi.status());
		wifi_failure_count++;
		return false;
	}
}

void startAPMode() {
	inAP = true;
	WiFi.mode(WIFI_AP);
	WiFi.softAP("MavenLED-Setup", "mavenled123");
	
	Serial.println(" AP Mode started!");
	Serial.println(" Connect to: MavenLED-Setup");
	Serial.println(" Password: mavenled123");
	Serial.print(" Config page: http://");
	Serial.println(WiFi.softAPIP());
	Serial.println("   or http://192.168.4.1");
	Serial.println("   or http://mavenled.local (after mDNS starts)");
	
	if (MDNS.begin("mavenled")) {
		MDNS.addService("http", "tcp", 80);
		Serial.println(" mDNS responder started in AP mode");
	}
}

void setup_wifi() {
	delay(10);
	Serial.println();
	
	bool hasSavedCredentials = strlen(settings.wifi_ssid) > 0;
	
	if (!hasSavedCredentials) {
		Serial.println("️ No WiFi credentials available! Starting AP mode...");
		startAPMode();
		return;
	}
	
	wifi_failure_count = 0;
	
	if (!inAP && connectToWiFi()) {
		Serial.println(" Initial WiFi setup successful!");
		return;
	}
	
	Serial.println(" All WiFi connections failed! Starting AP mode...");
	startAPMode();
}

void reconnect() {
	int reconnectAttempts = 0;
	const int maxReconnectAttempts = 2;
	
	String serverIP, username, password, mqttTopic;
	
	if (isGlobalMode()) {
		serverIP = "us.mqtt.bambulab.com";
		username = getGlobalMQTTUsername();
		password = getAccessToken();
		mqttTopic = "device/" + String(settings.device_serial) + "/report";
		
		if (username.length() == 0 || password.length() == 0) {
			Serial.println(" Global mode: Missing username or access token");
			printer_state.is_connected = false;
			printer_state.status = "unknown";
			return;
		}
		
		if (isTokenExpired()) {
			Serial.println("️ Global mode: Access token has expired");
			printer_state.is_connected = false;
			printer_state.status = "unknown";
			return;
		}
		
		Serial.printf(" Global mode connection to %s with username: %s\n", serverIP.c_str(), username.c_str());
	} else {
		serverIP = useSavedMQTTSettings() ? String(settings.mqtt_server) : "";
		username = "bblp";
		password = useSavedMQTTSettings() ? String(settings.mqtt_password) : "";
		mqttTopic = useSavedMQTTSettings() ? getMQTTTopic() : "";
		
		Serial.printf(" Local mode connection to %s\n", serverIP.c_str());
	}
	
	while (!client.connected() && reconnectAttempts < maxReconnectAttempts) {
		reconnectAttempts++;
		Serial.println();
		Serial.printf("Attempting MQTT connection #%d to %s:8883 (TLS)...\n", reconnectAttempts, serverIP.c_str());
		
		String clientId = "ESP32Client-";
		clientId += String(random(0xffff), HEX);
		
		if (client.connect(clientId.c_str(), username.c_str(), password.c_str())) {
			Serial.println("connected!");
			Serial.print("Subscribing to topic: ");
			Serial.println(mqttTopic);
			if (client.subscribe(mqttTopic.c_str())) {
				Serial.println("Successfully subscribed to topic");
				reconnectAttempts = 0;
			} else {
				Serial.println("Failed to subscribe to topic");
			}
		} else {
			Serial.print("failed, rc=");
			int state = client.state();
			Serial.print(state);
			Serial.print(" (");
			switch(state) {
				case -4: Serial.print("Connection timeout"); break;
				case -3: Serial.print("Connection lost"); break;
				case -2: Serial.print("Connect failed"); break;
				case -1: Serial.print("Disconnected"); break;
				case 1: Serial.print("Bad protocol"); break;
				case 2: Serial.print("Bad client ID"); break;
				case 3: Serial.print("Unavailable"); break;
				case 4: Serial.print("Bad credentials"); break;
				case 5: Serial.print("Unauthorized"); break;
				default: Serial.print("Unknown error"); break;
			}
			Serial.println(") try again in 5 seconds");
			
			for (int i = 0; i < 25; i++) {
				delay(200);
				yield();
				vTaskDelay(1 / portTICK_PERIOD_MS);
			}
		}
	}
	
	if (reconnectAttempts >= maxReconnectAttempts) {
		Serial.println("Returning to unknown state after max reconnect attempts");
		printer_state.is_connected = false;
		printer_state.status = "unknown";
		printer_state.raw_gcode_state = "unknown";
		printer_state.last_stable_status = "unknown";
		printer_state.last_status_change = millis();
		printer_state.temp_readings_count = 0;
		printer_state.is_heating = false;
		printer_state.is_cooling = false;
		printer_state.heating_exit_timer_started = false;
		printer_state.cooling_exit_timer_started = false;
		printer_state.finish_animation_active = false;
		printer_state.error_recovery_active = false;
		printer_state.state_override_active = false;
		lastMQTTupdate = millis();
	}
}

void startMQTTService(bool isInitialConnection) {
	Serial.println(" Starting MQTT service...");
	Serial.printf(" Current MQTT Mode: %s\n", settings.mqtt_mode_global ? "GLOBAL" : "LOCAL");
	
	espClient.setInsecure();
	
	char serverIP[64];
	char mqttTopic[128];
	
	if (isGlobalMode()) {
		strcpy(serverIP, "us.mqtt.bambulab.com");
		snprintf(mqttTopic, sizeof(mqttTopic), "device/%s/report", settings.device_serial);
		Serial.println(" Configuring for Global MQTT mode");
	} else {
		if (useSavedMQTTSettings()) {
			strcpy(serverIP, settings.mqtt_server);
			snprintf(mqttTopic, sizeof(mqttTopic), "device/%s/report", settings.device_serial);
		}
		Serial.println(" Configuring for Local MQTT mode");
	}
	
	Serial.print(" Setting MQTT server to: ");
	Serial.print(serverIP);
	Serial.println(":8883 (TLS)");
	
	client.setServer(serverIP, 8883);
	client.setCallback(callback);
	client.setKeepAlive(60);
	client.setSocketTimeout(30);
	
	int attempts = 0;
	const int maxAttempts = 2;
	
	String username;
	String password;
	String clientId;
	
	if (isGlobalMode()) {
		username = getGlobalMQTTUsername();
		password = getAccessToken();
	} else {
		username = "bblp";
		password = useSavedMQTTSettings() ? String(settings.mqtt_password) : "";
	}
	
	while (!client.connected() && attempts < maxAttempts) {
		attempts++;
		Serial.printf(" MQTT connection attempt %d/%d...\n", attempts, maxAttempts);
		
		clientId = "ESP32Client-" + String(random(0xffff), HEX);
		
		if (client.connect(clientId.c_str(), username.c_str(), password.c_str())) {
			Serial.println(" MQTT connected successfully!");
			
			if (client.subscribe(mqttTopic)) {
				Serial.printf(" Subscribed to topic: %s\n", mqttTopic);
				printer_state.is_connected = true;
				lastMQTTupdate = millis();
			} else {
				Serial.println(" Failed to subscribe to MQTT topic");
			}
		} else {
			Serial.printf(" MQTT connection failed, rc=%d. Retrying in 2 seconds...\n", client.state());
			for (int i = 0; i < 20; i++) {
				delay(100);
				yield();
			}
		}
	}
	
	if (!client.connected()) {
		Serial.println(" MQTT connection failed after all attempts");
		printer_state.is_connected = false;
		Serial.println(" Reverting to rainbow animation - MQTT connection failed");
	}
}

void stopMQTTService() {
	Serial.println(" Stopping MQTT service...");
	
	if (client.connected()) {
		String mqttTopic = useSavedMQTTSettings() ? getMQTTTopic() : "";
		client.unsubscribe(mqttTopic.c_str());
		Serial.printf(" Unsubscribed from topic: %s\n", mqttTopic.c_str());
		
		client.disconnect();
		Serial.println(" MQTT disconnected");
	}
	
	printer_state.is_connected = false;
	printer_state.status = "unknown";
	Serial.println(" MQTT service stopped");
}

// Remote Control Functions
String getRemoteControlTopicBase() {
	String chipId = String((uint32_t)ESP.getEfuseMac(), HEX);
	chipId.toLowerCase();
	String deviceId = (strlen(settings.device_id) > 0) ? String(settings.device_id) : chipId;
	return "mavenled/" + deviceId;
}

String getRemoteCommandTopic() {
	return getRemoteControlTopicBase() + "/cmd";
}

String getRemoteStatusTopic() {
	return getRemoteControlTopicBase() + "/status";
}

String getRemoteAckTopic() {
	return getRemoteControlTopicBase() + "/ack";
}

String getRemotePrinterStatusTopic() {
	return getRemoteControlTopicBase() + "/printer";
}

void remoteControlCallback(char* topic, byte* payload, unsigned int length) {
	Serial.printf(" Message received on topic: %s\n", topic);
	Serial.printf(" Payload length: %d bytes\n", length);
	
	String message = "";
	message.reserve(length + 1);
	for (unsigned int i = 0; i < length; i++) {
		message += (char)payload[i];
	}
	
	Serial.printf(" Remote command received: %s\n", message.c_str());
	
	DynamicJsonDocument cmd(1024);
	DeserializationError error = deserializeJson(cmd, message);
	
	if (error) {
		Serial.printf(" JSON parsing failed: %s\n", error.c_str());
		return;
	}
	
	processRemoteCommand(cmd);
}

void processRemoteCommand(DynamicJsonDocument& cmd) {
	if (!cmd.containsKey("command") || !cmd.containsKey("id")) {
		Serial.println(" Invalid command format - missing 'command' or 'id'");
		return;
	}
	
	String command = cmd["command"];
	String command_id = cmd["id"];
	
	lastCommand.command_id = command_id;
	lastCommand.timestamp = millis();
	lastCommand.acknowledged = false;
	
	Serial.printf(" Processing command: %s (ID: %s)\n", command.c_str(), command_id.c_str());
	
	bool success = true;
	String message = "Command executed successfully";
	
	if (command == "set_brightness") {
		if (cmd.containsKey("value")) {
			int brightness = constrain((int)cmd["value"], 1, 255);
			settings.global_brightness = brightness;
			saveSettings();
			message = "Brightness set to " + String(brightness);
			Serial.printf(" Brightness set to %d via remote\n", brightness);
		} else {
			success = false;
			message = "Missing brightness value";
		}
	}
	else if (command == "set_night_mode") {
		if (cmd.containsKey("enabled")) {
			settings.night_mode_enabled = cmd["enabled"];
			if (cmd.containsKey("brightness")) {
				settings.night_mode_brightness = constrain((int)cmd["brightness"], 1, 255);
			}
			saveSettings();
			message = "Night mode " + String(settings.night_mode_enabled ? "enabled" : "disabled");
			Serial.printf(" Night mode %s via remote\n", settings.night_mode_enabled ? "enabled" : "disabled");
		} else {
			success = false;
			message = "Missing enabled parameter";
		}
	}
	else if (command == "get_status") {
		publishDeviceStatus();
		message = "Status published";
	}
	else {
		success = false;
		message = "Unknown command: " + command;
		Serial.printf(" Unknown remote command: %s\n", command.c_str());
	}
	
	sendCommandAck(command_id, success, message);
}

void sendCommandAck(String command_id, bool success, String message) {
	if (!remoteControlClient.connected() || !settings.remote_control_enabled || isGlobalMode()) {
		return;
	}
	
	DynamicJsonDocument ack(512);
	ack["id"] = command_id;
	ack["success"] = success;
	ack["message"] = message;
	ack["timestamp"] = millis();
	
	String ackStr;
	serializeJson(ack, ackStr);
	
	String ackTopic = getRemoteAckTopic();
	remoteControlClient.publish(ackTopic.c_str(), ackStr.c_str());
	
	lastCommand.acknowledged = true;
	
	Serial.printf(" Sent acknowledgment for command %s: %s\n", command_id.c_str(), success ? "SUCCESS" : "FAILED");
}

void publishDeviceStatus() {
	if (!remoteControlClient.connected() || !settings.remote_control_enabled) {
		return;
	}
	
	DynamicJsonDocument status(2048);
	
	status["device_id"] = settings.device_id;
	status["timestamp"] = millis();
	status["wifi_connected"] = (WiFi.status() == WL_CONNECTED);
	status["mqtt_connected"] = client.connected();
	status["remote_control_enabled"] = settings.remote_control_enabled;
	
	JsonObject led_settings = status.createNestedObject("led_settings");
	led_settings["brightness"] = settings.global_brightness;
	led_settings["night_mode_enabled"] = settings.night_mode_enabled;
	led_settings["night_mode_brightness"] = settings.night_mode_brightness;
	led_settings["lights_off_override"] = settings.lights_off_override;
	led_settings["led_count"] = settings.led_count;
	
	JsonArray colors = led_settings.createNestedArray("colors");
	for (int i = 0; i < 8; i++) {
		JsonObject color = colors.createNestedObject();
		color["r"] = settings.colors[i].r;
		color["g"] = settings.colors[i].g;
		color["b"] = settings.colors[i].b;
	}
	
	JsonObject directions = led_settings.createNestedObject("directions");
	directions["rainbow"] = (settings.rainbow_direction == 1);
	directions["idle"] = (settings.idle_direction == 1);
	directions["printing"] = (settings.printing_direction == 1);
	directions["download"] = (settings.download_direction == 1);
	
	status["printer_connected"] = printer_state.is_connected;
	
	String statusStr;
	serializeJson(status, statusStr);
	
	String statusTopic = getRemoteStatusTopic();
	remoteControlClient.publish(statusTopic.c_str(), statusStr.c_str());
	
	Serial.printf(" Published device status (%d bytes)\n", statusStr.length());
	
	if (printer_state.is_connected) {
		publishPrinterStatus();
	}
}

void reconnectRemoteControl() {
	if (!settings.remote_control_enabled) {
		return;
	}
	
	int attempts = 0;
	const int maxAttempts = 3;
	
	while (!remoteControlClient.connected() && attempts < maxAttempts) {
		attempts++;
		Serial.printf(" Connecting to remote MQTT broker %s:%d (attempt %d/%d)...\n", 
					  settings.remote_mqtt_server, settings.remote_mqtt_port, attempts, maxAttempts);
		
		String chipId = String((uint32_t)ESP.getEfuseMac(), HEX);
		chipId.toUpperCase();
		String deviceId = (strlen(settings.device_id) > 0) ? String(settings.device_id) : chipId;
		String clientId = "MavenLED_" + deviceId;
		
		bool connected = false;
		if (strlen(settings.remote_username) > 0) {
			connected = remoteControlClient.connect(clientId.c_str(), 
												  settings.remote_username, 
												  settings.remote_password);
		} else {
			connected = remoteControlClient.connect(clientId.c_str());
		}
		
		if (connected) {
			Serial.println(" Connected to remote MQTT broker!");
			
			String chipId = String((uint32_t)ESP.getEfuseMac(), HEX);
			chipId.toUpperCase();
			String actualDeviceId = (strlen(settings.device_id) > 0) ? String(settings.device_id) : chipId;
			Serial.printf(" Using Device ID: %s (Chip ID: %s)\n", actualDeviceId.c_str(), chipId.c_str());
			
			String cmdTopic = getRemoteCommandTopic();
			Serial.printf(" Attempting to subscribe to: %s\n", cmdTopic.c_str());
			
			if (remoteControlClient.subscribe(cmdTopic.c_str())) {
				Serial.printf(" Subscribed to: %s\n", cmdTopic.c_str());
				publishDeviceStatus();
				return;
			} else {
				Serial.println(" Failed to subscribe to command topic");
			}
		} else {
			Serial.printf(" Remote MQTT connection failed, rc=%d\n", remoteControlClient.state());
			for (int i = 0; i < 20; i++) {
				delay(100);
				yield();
			}
		}
	}
	
	if (attempts >= maxAttempts) {
		Serial.println(" Failed to connect to remote MQTT broker after maximum attempts");
	}
}

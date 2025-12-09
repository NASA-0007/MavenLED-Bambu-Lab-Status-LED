#include "WebHandlers.h"
#include "../config/Settings.h"
#include "../printer/PrinterState.h"
#include "../network/NetworkManager.h"
#include "../led/LEDAnimations.h"

// Web Server Global Variable
WebServer server(80);

// Include the web page
#include "webpage.h"

void handleRoot() {
	server.send(200, "text/html", webPage);
}

void handleNotFound() {
	server.send(404, "text/plain", "Not found");
}

void handleStatus() {
	DynamicJsonDocument doc(1024);
	
	doc["printer_status"] = printer_state.status;
	doc["progress"] = printer_state.progress;
	doc["printer_connected"] = printer_state.is_connected;
	doc["wifi_connected"] = WiFi.status() == WL_CONNECTED;
	doc["wifi_ssid"] = WiFi.SSID();
	doc["ip_address"] = WiFi.localIP().toString();
	doc["night_mode"] = settings.night_mode_enabled;
	doc["brightness"] = settings.global_brightness;
	doc["lights_enabled"] = !settings.lights_off_override;
	
	doc["night_mode_enabled"] = settings.night_mode_enabled;
	doc["global_brightness"] = settings.global_brightness;
	doc["night_mode_brightness"] = settings.night_mode_brightness;
	doc["led_count"] = settings.led_count;
	
	doc["rainbow_direction"] = settings.rainbow_direction;
	doc["idle_direction"] = settings.idle_direction;
	doc["printing_direction"] = settings.printing_direction;
	doc["download_direction"] = settings.download_direction;
	
	if (isGlobalMode()) {
		doc["mqtt_mode"] = "global";
		doc["token_expired"] = isTokenExpired();
		doc["token_needs_renewal"] = shouldRenewToken();
		
		if (settings.token_expires_at > 0) {
			unsigned long currentTime = millis() / 1000;
			unsigned long timeUntilExpiry = (settings.token_expires_at > currentTime) ? 
											 (settings.token_expires_at - currentTime) : 0;
			doc["token_expires_in_seconds"] = timeUntilExpiry;
		}
	} else {
		doc["mqtt_mode"] = "local";
	}
	
	String response;
	serializeJson(doc, response);
	server.send(200, "application/json", response);
}

void handleGetSettings() {
	DynamicJsonDocument doc(2048);
	
	JsonArray colors = doc.createNestedArray("colors");
	for (int i = 0; i < 8; i++) {
		JsonObject color = colors.createNestedObject();
		color["r"] = settings.colors[i].r;
		color["g"] = settings.colors[i].g;
		color["b"] = settings.colors[i].b;
	}
	
	doc["rainbow_direction"] = settings.rainbow_direction;
	doc["idle_direction"] = settings.idle_direction;
	doc["printing_direction"] = settings.printing_direction;
	doc["download_direction"] = settings.download_direction;
	
	doc["global_brightness"] = settings.global_brightness;
	doc["night_mode_brightness"] = settings.night_mode_brightness;
	doc["night_mode_enabled"] = settings.night_mode_enabled;
	
	String response;
	serializeJson(doc, response);
	server.send(200, "application/json", response);
}

void handleSetSettings() {
	if (server.hasArg("plain")) {
		DynamicJsonDocument doc(2048);
		DeserializationError error = deserializeJson(doc, server.arg("plain"));
		
		if (!error) {
			if (doc.containsKey("colors")) {
				JsonArray colors = doc["colors"];
				for (int i = 0; i < 9 && i < colors.size(); i++) {
					if (colors[i].containsKey("r")) settings.colors[i].r = colors[i]["r"];
					if (colors[i].containsKey("g")) settings.colors[i].g = colors[i]["g"];
					if (colors[i].containsKey("b")) settings.colors[i].b = colors[i]["b"];
				}
			}
			
			if (doc.containsKey("rainbow_direction")) settings.rainbow_direction = doc["rainbow_direction"];
			if (doc.containsKey("idle_direction")) settings.idle_direction = doc["idle_direction"];
			if (doc.containsKey("printing_direction")) settings.printing_direction = doc["printing_direction"];
			if (doc.containsKey("download_direction")) settings.download_direction = doc["download_direction"];
			if (doc.containsKey("global_brightness")) settings.global_brightness = doc["global_brightness"];
			if (doc.containsKey("night_mode_brightness")) settings.night_mode_brightness = doc["night_mode_brightness"];
			if (doc.containsKey("night_mode_enabled")) settings.night_mode_enabled = doc["night_mode_enabled"];
			
			saveSettings();
			server.send(200, "application/json", "{\"status\":\"success\"}");
		} else {
			server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
		}
	} else {
		server.send(400, "application/json", "{\"error\":\"No data\"}");
	}
}

void handleSetColors() {
	if (server.hasArg("plain")) {
		DynamicJsonDocument doc(1024);
		DeserializationError error = deserializeJson(doc, server.arg("plain"));
		
		if (!error && doc.containsKey("colors")) {
			JsonArray colors = doc["colors"];
			for (int i = 0; i < 8 && i < colors.size(); i++) {
				if (colors[i].containsKey("r") && colors[i].containsKey("g") && colors[i].containsKey("b")) {
					settings.colors[i].r = colors[i]["r"];
					settings.colors[i].g = colors[i]["g"];
					settings.colors[i].b = colors[i]["b"];
				}
			}
			saveSettings();
			server.send(200, "application/json", "{\"status\":\"success\"}");
		} else {
			server.send(400, "application/json", "{\"error\":\"Invalid color data\"}");
		}
	} else {
		server.send(400, "application/json", "{\"error\":\"No data\"}");
	}
}

void handleSetBrightness() {
	if (server.hasArg("plain")) {
		DynamicJsonDocument doc(256);
		DeserializationError error = deserializeJson(doc, server.arg("plain"));
		
		if (!error) {
			if (doc.containsKey("brightness")) {
				settings.global_brightness = constrain((int)doc["brightness"], 1, 255);
			}
			if (doc.containsKey("night_brightness")) {
				settings.night_mode_brightness = constrain((int)doc["night_brightness"], 1, 255);
			}
			saveSettings();
			server.send(200, "application/json", "{\"status\":\"success\"}");
		} else {
			server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
		}
	} else {
		server.send(400, "application/json", "{\"error\":\"No data\"}");
	}
}

void handleSetDirections() {
	if (server.hasArg("plain")) {
		DynamicJsonDocument doc(512);
		DeserializationError error = deserializeJson(doc, server.arg("plain"));
		
		if (!error) {
			if (doc.containsKey("rainbow")) settings.rainbow_direction = doc["rainbow"];
			if (doc.containsKey("idle")) settings.idle_direction = doc["idle"];
			if (doc.containsKey("printing")) settings.printing_direction = doc["printing"];
			if (doc.containsKey("download")) settings.download_direction = doc["download"];
			
			saveSettings();
			server.send(200, "application/json", "{\"status\":\"success\"}");
		} else {
			server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
		}
	} else {
		server.send(400, "application/json", "{\"error\":\"No data\"}");
	}
}

void handleNightMode() {
	if (server.hasArg("plain")) {
		DynamicJsonDocument doc(128);
		DeserializationError error = deserializeJson(doc, server.arg("plain"));
		
		if (!error && doc.containsKey("enabled")) {
			settings.night_mode_enabled = doc["enabled"];
			saveSettings();
			server.send(200, "application/json", "{\"status\":\"success\"}");
		} else {
			server.send(400, "application/json", "{\"error\":\"Invalid data\"}");
		}
	} else {
		server.send(400, "application/json", "{\"error\":\"No data\"}");
	}
}

void handleWiFiScan() {
	DynamicJsonDocument doc(4096);
	JsonArray networks = doc.to<JsonArray>();
	
	int n = WiFi.scanNetworks();
	for (int i = 0; i < n; i++) {
		JsonObject network = networks.createNestedObject();
		network["ssid"] = WiFi.SSID(i);
		network["rssi"] = WiFi.RSSI(i);
		network["encryption"] = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "Open" : "Encrypted";
	}
	
	String response;
	serializeJson(doc, response);
	server.send(200, "application/json", response);
}

void handleWiFiConnect() {
	if (server.hasArg("plain")) {
		DynamicJsonDocument doc(256);
		DeserializationError error = deserializeJson(doc, server.arg("plain"));
		
		if (!error && doc.containsKey("ssid")) {
			String ssid = doc["ssid"];
			String password = doc.containsKey("password") ? doc["password"].as<String>() : "";
			
			ssid.toCharArray(settings.wifi_ssid, sizeof(settings.wifi_ssid));
			password.toCharArray(settings.wifi_password, sizeof(settings.wifi_password));
			saveSettings();
			
			server.send(200, "application/json", "{\"status\":\"saved\",\"message\":\"WiFi credentials saved. Device will restart.\"}");
			
			delay(1000);
			Serial.println(" Restarting ESP32 to apply new WiFi credentials...");
			ESP.restart();
		} else {
			server.send(400, "application/json", "{\"error\":\"Invalid data\"}");
		}
	} else {
		server.send(400, "application/json", "{\"error\":\"No data\"}");
	}
}

void handleGetMQTTConfig() {
	DynamicJsonDocument doc(1024);
	
	doc["serial"] = settings.device_serial;
	doc["mode"] = settings.mqtt_mode_global ? "global" : "local";
	
	if (settings.mqtt_mode_global) {
		doc["email"] = settings.global_email;
		doc["username"] = settings.global_username;
		doc["hasToken"] = (getAccessToken().length() > 0);
		doc["hasValidAuth"] = settings.has_valid_auth;
		doc["tokenExpired"] = isTokenExpired();
	} else {
		doc["server"] = settings.mqtt_server;
		doc["password"] = settings.mqtt_password;
	}
	
	String response;
	serializeJson(doc, response);
	server.send(200, "application/json", response);
}

void handleSetMQTTConfig() {
	if (server.hasArg("plain")) {
		DynamicJsonDocument doc(1024);
		DeserializationError error = deserializeJson(doc, server.arg("plain"));
		
		if (!error && doc.containsKey("serial")) {
			String serial = doc["serial"];
			String mode = doc["mode"] | "local";
			
			serial.toCharArray(settings.device_serial, sizeof(settings.device_serial));
			
			if (mode == "global") {
				settings.mqtt_mode_global = true;
				
				if (doc.containsKey("email") && doc.containsKey("username")) {
					String email = doc["email"];
					String username = doc["username"];
					
					email.toCharArray(settings.global_email, sizeof(settings.global_email));
					username.toCharArray(settings.global_username, sizeof(settings.global_username));
				}
			} else {
				settings.mqtt_mode_global = false;
				
				if (doc.containsKey("server") && doc.containsKey("password")) {
					String serverIP = doc["server"];
					String password = doc["password"];
					
					serverIP.toCharArray(settings.mqtt_server, sizeof(settings.mqtt_server));
					password.toCharArray(settings.mqtt_password, sizeof(settings.mqtt_password));
				}
			}
			
			Serial.printf(" Saving MQTT config - Mode: %s\n", settings.mqtt_mode_global ? "Global" : "Local");
			Serial.printf("   Serial: %s\n", settings.device_serial);
			
			saveSettings();
			
			server.send(200, "application/json", "{\"status\":\"saved\"}");
			
			Serial.println(" Waiting 3 seconds before restart to ensure EEPROM commit...");
			delay(3000);
			Serial.println(" Restarting ESP32 to apply new MQTT settings...");
			ESP.restart();
		} else {
			server.send(400, "application/json", "{\"error\":\"Invalid data - serial number required\"}");
		}
	} else {
		server.send(400, "application/json", "{\"error\":\"No data\"}");
	}
}

void handleAuthLogin() {
	if (server.hasArg("plain")) {
		DynamicJsonDocument doc(512);
		DeserializationError error = deserializeJson(doc, server.arg("plain"));
		
		if (!error && doc.containsKey("email") && doc.containsKey("password")) {
			String email = doc["email"];
			String password = doc["password"];
			
			strncpy(settings.global_email, email.c_str(), sizeof(settings.global_email) - 1);
			settings.global_email[sizeof(settings.global_email) - 1] = '\0';
			
			AuthResult result = performInitialLogin(email, password);
			
			if (result.success && result.accessToken.length() > 0) {
				saveAccessToken(result.accessToken);
				saveRefreshToken(result.refreshToken);
				
				settings.token_expires_at = (millis() / 1000) + result.expiresIn;
				settings.has_valid_auth = true;
				Serial.println(" Login successful - tokens saved to SPIFFS");
			}
			
			saveSettings();
			
			DynamicJsonDocument response(512);
			response["success"] = result.success;
			response["error"] = result.error;
			response["hasTokens"] = (result.accessToken.length() > 0);
			
			String responseStr;
			serializeJson(response, responseStr);
			server.send(200, "application/json", responseStr);
		} else {
			server.send(400, "application/json", "{\"error\":\"Email and password required\"}");
		}
	} else {
		server.send(400, "application/json", "{\"error\":\"No data\"}");
	}
}

void handleAuthVerify() {
	if (server.hasArg("plain")) {
		DynamicJsonDocument doc(512);
		DeserializationError error = deserializeJson(doc, server.arg("plain"));
		
		if (!error && doc.containsKey("email") && doc.containsKey("code")) {
			String email = doc["email"];
			String code = doc["code"];
			
			AuthResult result = performVerificationLogin(email, code);
			
			if (result.success && result.accessToken.length() > 0) {
				strncpy(settings.global_email, email.c_str(), sizeof(settings.global_email) - 1);
				settings.global_email[sizeof(settings.global_email) - 1] = '\0';
				
				saveAccessToken(result.accessToken);
				saveRefreshToken(result.refreshToken);
				
				settings.token_expires_at = (millis() / 1000) + result.expiresIn;
				settings.has_valid_auth = true;
				saveSettings();
				Serial.println(" Verification successful - tokens saved to SPIFFS");
			}
			
			DynamicJsonDocument response(512);
			response["success"] = result.success;
			response["error"] = result.error;
			
			String responseStr;
			serializeJson(response, responseStr);
			server.send(200, "application/json", responseStr);
		} else {
			server.send(400, "application/json", "{\"error\":\"Email and verification code required\"}");
		}
	} else {
		server.send(400, "application/json", "{\"error\":\"No data\"}");
	}
}

void handleAuthRenew() {
	if (server.hasArg("plain")) {
		DynamicJsonDocument doc(512);
		DeserializationError error = deserializeJson(doc, server.arg("plain"));
		
		if (!error && doc.containsKey("email") && doc.containsKey("password")) {
			String email = doc["email"];
			String password = doc["password"];
			
			AuthResult result = performInitialLogin(email, password);
			
			DynamicJsonDocument response(512);
			response["success"] = result.success;
			response["error"] = result.error;
			
			String responseStr;
			serializeJson(response, responseStr);
			server.send(200, "application/json", responseStr);
		} else {
			server.send(400, "application/json", "{\"error\":\"Email and password required\"}");
		}
	} else {
		server.send(400, "application/json", "{\"error\":\"No data\"}");
	}
}

void handleGetLEDCount() {
	DynamicJsonDocument doc(256);
	doc["count"] = settings.led_count;
	
	String response;
	serializeJson(doc, response);
	server.send(200, "application/json", response);
}

void handleGetLEDPin() {
	DynamicJsonDocument doc(256);
	doc["pin"] = settings.led_pin;
	
	String response;
	serializeJson(doc, response);
	server.send(200, "application/json", response);
}

void handleSetLEDCount() {
	if (server.hasArg("plain")) {
		DynamicJsonDocument doc(256);
		DeserializationError error = deserializeJson(doc, server.arg("plain"));
		
		if (!error && doc.containsKey("count")) {
			int newCount = doc["count"];
			
			// Validate LED count range
			if (newCount >= 1 && newCount <= 300) {
				settings.led_count = newCount;
				saveSettings();
				
				server.send(200, "application/json", "{\"status\":\"saved\"}");
				
				// Restart to apply new LED count
				delay(1000);
				ESP.restart();
			} else {
				server.send(400, "application/json", "{\"error\":\"LED count must be between 1 and 300\"}");
			}
		} else {
			server.send(400, "application/json", "{\"error\":\"Invalid data - count required\"}");
		}
	} else {
		server.send(400, "application/json", "{\"error\":\"No data\"}");
	}
}

void handleSetLEDPin() {
	if (server.hasArg("plain")) {
		DynamicJsonDocument doc(256);
		DeserializationError error = deserializeJson(doc, server.arg("plain"));
		
		if (!error && doc.containsKey("pin")) {
			int newPin = doc["pin"];
			
			// Validate GPIO pin (common usable pins on ESP32)
			if (newPin >= 0 && newPin <= 33 && newPin != 6 && newPin != 7 && 
			    newPin != 8 && newPin != 9 && newPin != 10 && newPin != 11) {
				settings.led_pin = newPin;
				saveSettings();
				
				server.send(200, "application/json", "{\"status\":\"saved\"}");
				
				// Restart to apply new GPIO pin
				delay(1000);
				ESP.restart();
			} else {
				server.send(400, "application/json", "{\"error\":\"Invalid GPIO pin (use 0-5,12-33, avoid 6-11)\"}");
			}
		} else {
			server.send(400, "application/json", "{\"error\":\"Invalid data - pin required\"}");
		}
	} else {
		server.send(400, "application/json", "{\"error\":\"No data\"}");
	}
}


void handleLightsToggle() {
	if (server.hasArg("plain")) {
		DynamicJsonDocument doc(256);
		DeserializationError error = deserializeJson(doc, server.arg("plain"));
		
		if (!error && doc.containsKey("enabled")) {
			bool enabled = doc["enabled"];
			
			if (enabled && settings.lights_off_override) {
				lights_turning_on = true;
				lights_turning_off = false;
				lights_animation_start = millis();
				lights_animation_progress = 0;
				Serial.println(" Starting lights ON animation (middle to ends)");
			} else if (!enabled && !settings.lights_off_override) {
				captureCurrentFrame();
				lights_turning_off = true;
				lights_turning_on = false;
				lights_animation_start = millis();
				lights_animation_progress = 0;
				Serial.println(" Starting lights OFF animation (ends to middle)");
			}
			
			DynamicJsonDocument response(256);
			response["status"] = "success";
			response["lights_enabled"] = enabled;
			response["lights_off_override"] = settings.lights_off_override;
			
			String responseStr;
			serializeJson(response, responseStr);
			server.send(200, "application/json", responseStr);
			
			Serial.printf(" Lights toggle: %s (override: %s) - Saved to EEPROM\n", 
						  enabled ? "ON" : "OFF",
						  settings.lights_off_override ? "true" : "false");
		} else {
			server.send(400, "application/json", "{\"error\":\"Invalid data - enabled required\"}");
		}
	} else {
		server.send(400, "application/json", "{\"error\":\"No data\"}");
	}
}

void handleGetP1Mode() {
	DynamicJsonDocument doc(256);
	doc["p1_series_mode"] = settings.p1_series_mode;
	
	String response;
	serializeJson(doc, response);
	server.send(200, "application/json", response);
}

void handleSetP1Mode() {
	if (server.hasArg("plain")) {
		DynamicJsonDocument doc(256);
		DeserializationError error = deserializeJson(doc, server.arg("plain"));
		
		if (!error && doc.containsKey("p1_series_mode")) {
			settings.p1_series_mode = doc["p1_series_mode"];
			saveSettings();
			
			DynamicJsonDocument response(256);
			response["status"] = "success";
			response["p1_series_mode"] = settings.p1_series_mode;
			
			String responseStr;
			serializeJson(response, responseStr);
			server.send(200, "application/json", responseStr);
			
			Serial.printf("️ P1 Series Mode: %s\n", settings.p1_series_mode ? "ENABLED" : "DISABLED");
		} else {
			server.send(400, "application/json", "{\"error\":\"Invalid data - p1_series_mode required\"}");
		}
	} else {
		server.send(400, "application/json", "{\"error\":\"No data\"}");
	}
}

void handleGetIdleTimeout() {
	DynamicJsonDocument doc(256);
	doc["enabled"] = settings.idle_timeout_enabled;
	doc["timeout_minutes"] = settings.idle_timeout_minutes;
	
	String response;
	serializeJson(doc, response);
	server.send(200, "application/json", response);
}

void handleSetIdleTimeout() {
	if (server.hasArg("plain")) {
		DynamicJsonDocument doc(256);
		DeserializationError error = deserializeJson(doc, server.arg("plain"));
		
		if (!error) {
			bool updated = false;
			
			if (doc.containsKey("enabled")) {
				settings.idle_timeout_enabled = doc["enabled"];
				updated = true;
			}
			
			if (doc.containsKey("timeout_minutes")) {
				int timeout = doc["timeout_minutes"];
				if (timeout >= 1 && timeout <= 120) {
					settings.idle_timeout_minutes = timeout;
					updated = true;
				} else {
					server.send(400, "application/json", "{\"error\":\"Timeout must be between 1 and 120 minutes\"}");
					return;
				}
			}
			
			if (updated) {
				saveSettings();
				
				DynamicJsonDocument response(256);
				response["status"] = "success";
				response["enabled"] = settings.idle_timeout_enabled;
				response["timeout_minutes"] = settings.idle_timeout_minutes;
				
				String responseStr;
				serializeJson(response, responseStr);
				server.send(200, "application/json", responseStr);
				
				Serial.printf(" Idle Timeout: %s, %d minutes\n", 
							  settings.idle_timeout_enabled ? "ENABLED" : "DISABLED",
							  settings.idle_timeout_minutes);
			} else {
				server.send(400, "application/json", "{\"error\":\"No valid parameters provided\"}");
			}
		} else {
			server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
		}
	} else {
		server.send(400, "application/json", "{\"error\":\"No data\"}");
	}
}

void handleGetRemoteConfig() {
	DynamicJsonDocument doc(1024);
	
	doc["enabled"] = settings.remote_control_enabled;
	doc["server"] = settings.remote_mqtt_server;
	doc["port"] = settings.remote_mqtt_port;
	doc["device_id"] = settings.device_id;
	doc["username"] = settings.remote_username;
	doc["connected"] = remoteControlClient.connected();
	
	JsonObject topics = doc.createNestedObject("topics");
	topics["command"] = getRemoteCommandTopic();
	topics["status"] = getRemoteStatusTopic();
	topics["ack"] = getRemoteAckTopic();
	
	String response;
	serializeJson(doc, response);
	server.send(200, "application/json", response);
}

void handleSetRemoteConfig() {
	if (server.hasArg("plain")) {
		DynamicJsonDocument doc(1024);
		DeserializationError error = deserializeJson(doc, server.arg("plain"));
		
		if (!error) {
			bool needsRestart = false;
			
			if (doc.containsKey("enabled")) {
				settings.remote_control_enabled = doc["enabled"];
			}
			
			if (doc.containsKey("server")) {
				String server = doc["server"];
				if (server != String(settings.remote_mqtt_server)) {
					server.toCharArray(settings.remote_mqtt_server, sizeof(settings.remote_mqtt_server));
					needsRestart = true;
				}
			}
			
			if (doc.containsKey("port")) {
				int port = doc["port"];
				if (port != settings.remote_mqtt_port) {
					settings.remote_mqtt_port = port;
					needsRestart = true;
				}
			}
			
			if (doc.containsKey("device_id")) {
				String deviceId = doc["device_id"];
				if (deviceId != String(settings.device_id)) {
					deviceId.toCharArray(settings.device_id, sizeof(settings.device_id));
					needsRestart = true;
				}
			}
			
			if (doc.containsKey("username")) {
				String username = doc["username"];
				username.toCharArray(settings.remote_username, sizeof(settings.remote_username));
			}
			
			if (doc.containsKey("password")) {
				String password = doc["password"];
				password.toCharArray(settings.remote_password, sizeof(settings.remote_password));
			}
			
			saveSettings();
			
			DynamicJsonDocument response(256);
			response["status"] = "success";
			response["restart_required"] = needsRestart;
			
			String responseStr;
			serializeJson(response, responseStr);
			server.send(200, "application/json", responseStr);
			
			Serial.printf(" Remote control config saved - Enabled: %s\n", settings.remote_control_enabled ? "true" : "false");
			
			if (needsRestart) {
				delay(1000);
				ESP.restart();
			} else {
				if (settings.remote_control_enabled) {
					remoteControlClient.setServer(settings.remote_mqtt_server, settings.remote_mqtt_port);
					reconnectRemoteControl();
				} else {
					remoteControlClient.disconnect();
				}
			}
		} else {
			server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
		}
	} else {
		server.send(400, "application/json", "{\"error\":\"No data\"}");
	}
}

void publishPrinterStatus() {
	if (!remoteControlClient.connected() || !settings.remote_control_enabled) {
		return;
	}
	
	DynamicJsonDocument printer(1536);
	
	printer["status"] = printer_state.status;
	printer["raw_gcode_state"] = printer_state.raw_gcode_state;
	printer["is_connected"] = printer_state.is_connected;
	printer["timestamp"] = millis();
	
	printer["progress"] = printer_state.progress;
	printer["download_progress"] = printer_state.download_progress;
	printer["stage"] = printer_state.stage;
	
	JsonObject temps = printer.createNestedObject("temperature");
	temps["bed_temp"] = printer_state.bed_temp;
	temps["nozzle_temp"] = printer_state.nozzle_temp;
	temps["target_bed_temp"] = printer_state.target_bed_temp;
	temps["target_nozzle_temp"] = printer_state.target_nozzle_temp;
	temps["is_heating"] = printer_state.is_heating;
	temps["is_cooling"] = printer_state.is_cooling;
	
	if (printer_state.filename.length() > 0) {
		printer["filename"] = printer_state.filename;
	}
	printer["remaining_time"] = printer_state.remaining_time;
	
	if (printer_state.total_layers > 0) {
		JsonObject layers = printer.createNestedObject("layers");
		layers["current"] = printer_state.current_layer;
		layers["total"] = printer_state.total_layers;
		layers["info"] = printer_state.layer_info;
	}
	
	printer["has_error"] = printer_state.has_error;
	if (printer_state.has_error && printer_state.error_message.length() > 0) {
		printer["error_message"] = printer_state.error_message;
	}
	
	printer["finish_animation_active"] = printer_state.finish_animation_active;
	printer["state_override_active"] = printer_state.state_override_active;
	if (printer_state.state_override_active && printer_state.override_reason.length() > 0) {
		printer["override_reason"] = printer_state.override_reason;
	}
	
	String printerStr;
	serializeJson(printer, printerStr);
	
	String printerTopic = getRemotePrinterStatusTopic();
	remoteControlClient.publish(printerTopic.c_str(), printerStr.c_str());
	
	Serial.printf("️ Published printer status (%d bytes)\n", printerStr.length());
}

void setupWebServer() {
	server.on("/", HTTP_GET, handleRoot);
	
	server.on("/api/status", HTTP_GET, handleStatus);
	server.on("/api/settings", HTTP_GET, handleGetSettings);
	server.on("/api/settings", HTTP_POST, handleSetSettings);
	server.on("/api/colors", HTTP_POST, handleSetColors);
	server.on("/api/brightness", HTTP_POST, handleSetBrightness);
	server.on("/api/directions", HTTP_POST, handleSetDirections);
	server.on("/api/wifi/scan", HTTP_GET, handleWiFiScan);
	server.on("/api/wifi/connect", HTTP_POST, handleWiFiConnect);
	server.on("/api/nightmode", HTTP_POST, handleNightMode);
	server.on("/api/mqtt/config", HTTP_GET, handleGetMQTTConfig);
	server.on("/api/mqtt/config", HTTP_POST, handleSetMQTTConfig);
	server.on("/api/auth/login", HTTP_POST, handleAuthLogin);
	server.on("/api/auth/verify", HTTP_POST, handleAuthVerify);
	server.on("/api/auth/renew", HTTP_POST, handleAuthRenew);
	server.on("/api/led/count", HTTP_GET, handleGetLEDCount);
	server.on("/api/led/count", HTTP_POST, handleSetLEDCount);
	server.on("/api/led/pin", HTTP_GET, handleGetLEDPin);
	server.on("/api/led/pin", HTTP_POST, handleSetLEDPin);
	server.on("/api/lights/toggle", HTTP_POST, handleLightsToggle);
	server.on("/api/p1mode", HTTP_GET, handleGetP1Mode);
	server.on("/api/p1mode", HTTP_POST, handleSetP1Mode);
	server.on("/api/idle/timeout", HTTP_GET, handleGetIdleTimeout);
	server.on("/api/idle/timeout", HTTP_POST, handleSetIdleTimeout);
	server.on("/api/remote/config", HTTP_GET, handleGetRemoteConfig);
	server.on("/api/remote/config", HTTP_POST, handleSetRemoteConfig);
	server.on("/deviceid", HTTP_GET, []() {
		String chipId = String((uint32_t)ESP.getEfuseMac(), HEX);
		chipId.toUpperCase();
		DynamicJsonDocument doc(256);
		doc["chip_id"] = chipId;
		doc["device_id"] = settings.device_id;
		String response;
		serializeJson(doc, response);
		server.send(200, "application/json", response);
	});
	
	server.onNotFound(handleNotFound);
	
	server.enableCORS(true);
}

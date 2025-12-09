#ifndef WEB_HANDLERS_H
#define WEB_HANDLERS_H

#include <WebServer.h>
#include <Arduino.h>

// Web server
extern WebServer server;

// Web server functions
void setupWebServer();
void handleRoot();
void handleNotFound();
void handleStatus();
void handleGetSettings();
void handleSetSettings();
void handleSetColors();
void handleSetBrightness();
void handleSetDirections();
void handleNightMode();
void handleWiFiScan();
void handleWiFiConnect();
void handleGetMQTTConfig();
void handleSetMQTTConfig();
void handleAuthLogin();
void handleAuthVerify();
void handleAuthRenew();
void handleGetLEDCount();
void handleSetLEDCount();
void handleGetLEDPin();
void handleSetLEDPin();
void handleLightsToggle();
void handleGetP1Mode();
void handleSetP1Mode();
void handleGetIdleTimeout();
void handleSetIdleTimeout();
void handleGetRemoteConfig();
void handleSetRemoteConfig();
void publishPrinterStatus();

#endif

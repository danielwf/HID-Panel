#ifndef WIFI_PORTAL_H
#define WIFI_PORTAL_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include "config_manager.h"

// Status-Variablen
extern bool configModeActive;
extern AsyncWebServer server;
extern DNSServer dnsServer;
extern const char* ssid;
extern const char* pass;

// Funktionen
void startWifiPortal();
void handlePortal();
void setupWebRoutes();

#endif
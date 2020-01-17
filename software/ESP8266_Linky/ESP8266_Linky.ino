#include <ESP8266WiFi.h>

#include <FS.h>                   //this needs to be first, or it all crashes and burns...

#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino

//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>


#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager

#include <ArduinoJson.h>

#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <WiFiUdp.h>

#include "linky.h"


/*
struct Config {
	bool need_to_setup;
	char wifi_ssid[64];
	char wifi_psk[64];
};


Config config;

void loadConfiguration(Config &config) {
	File configFile = SPIFFS.open("/config.json","r");

	if (!configFile) {
		Serial.println("Failed to open config file");
		return;
	}
	
	StaticJsonDocument<512> jsonBuffer;
	deserializeJson(jsonBuffer, configFile);
	
	JsonObject root = jsonBuffer.as<JsonObject>(); // get the root object
	
	config.need_to_setup = root["module"]["setup"] | true;
	strlcpy(config.wifi_ssid,  root["module"]["wifi"]["ssid"] | "LinkyLink" ,  sizeof(config.wifi_ssid));
	strlcpy(config.wifi_psk ,  root["module"]["wifi"]["psk"]  | "LinkySetup",  sizeof(config.wifi_psk ));

	configFile.close();
}

void saveConfiguration(const Config &config) {
	SPIFFS.remove("config.json");

	File file = SPIFFS.open("/config.json", "w");
	if (!file) { Serial.println(F("Failed to create file")); return; }

	StaticJsonDocument<512> doc;

	doc["module"]["wifi"]["ssid"] = config.wifi_ssid;
	doc["module"]["wifi"]["ssid"] = config.wifi_psk;
	doc["module"]["setup"] = config.need_to_setup;

	if (serializeJson(doc, file) == 0) {  Serial.println(F("Failed to write to file")); }

	file.close();
}

void www_api_config_setup(AsyncWebServerRequest *request) {
	if (request->hasParam("wifi_ssid", true)) {
		String ssid = request->getParam("wifi_ssid", true)->value();
		if(ssid.length() > 32 || ssid.length() < 1) {
				request->send(418, "text/plain", "SSID de mauvaise taille");
		} else {
			if (request->hasParam("wifi_psk", true)) {
				String psk = request->getParam("wifi_psk", true)->value();
				if(psk.length() > 32 || psk.length() < 8) {
						request->send(418, "text/plain", "Mot de passe de mauvaise taille");
				} else {
					config.need_to_setup = false;
					ssid.toCharArray(config.wifi_ssid, 32);
					ssid.toCharArray(config.wifi_psk , 32);
					saveConfiguration(config);
					request->send(200, "text/plain", "Ok LinkyLink va redemarer et essaye de se connecter au reseau wifi.");
				}
			} else {
				request->send(418, "text/plain", "Mot de passe manquant");
			}
		}
	} else {
		request->send(418, "text/plain", "SSID Manquant");
	}
}


void www_api_linky(AsyncWebServerRequest *request) {
	File file = SPIFFS.open("api_template.json","r");

	StaticJsonDocument<8192> doc;
	DeserializationError error = deserializeJson(doc, file);
	if (error) {
		Serial.println(F("Failed to read api template file"));
	}

	LinkyData linkyData = mLinky.grab();
	doc["ADCO"   ]["value"] = linkyData.ADCO;
	doc["OPTARIF"]["value"] = linkyData.OPTARIF;
	doc["ISOUSC" ]["value"] = linkyData.ISOUSC;
	doc["HCHC"   ]["value"] = linkyData.HCHC;
	doc["HCHP"   ]["value"] = linkyData.HCHP;
	doc["EJPHN"  ]["value"] = linkyData.EJPHN;
	doc["EJPHPM" ]["value"] = linkyData.EJPHPM;
	doc["BBRHCJB"]["value"] = linkyData.BBRHCJB;
	doc["BBRHPJB"]["value"] = linkyData.BBRHPJB;
	doc["BBRHCJW"]["value"] = linkyData.BBRHCJW;
	doc["BBRHPJW"]["value"] = linkyData.BBRHPJW;
	doc["BBRHCJR"]["value"] = linkyData.BBRHCJR;
	doc["BBRHPJR"]["value"] = linkyData.BBRHPJR;
	doc["PEJP"   ]["value"] = linkyData.PEJP;
	doc["PTEC"   ]["value"] = linkyData.PTEC;
	doc["DEMAIN" ]["value"] = linkyData.DEMAIN;
	doc["IINST"  ]["value"] = linkyData.IINST;
	doc["IINST1" ]["value"] = linkyData.IINST1;
	doc["IINST2" ]["value"] = linkyData.IINST2;
	doc["IINST3" ]["value"] = linkyData.IINST3;
	doc["ADPS"   ]["value"] = linkyData.ADPS;
	doc["ADIR1"  ]["value"] = linkyData.ADIR1;
	doc["ADIR2"  ]["value"] = linkyData.ADIR2;
	doc["ADIR3"  ]["value"] = linkyData.ADIR3;
	doc["IMAX"   ]["value"] = linkyData.IMAX;
	doc["IMAX1"  ]["value"] = linkyData.IMAX1;
	doc["IMAX2"  ]["value"] = linkyData.IMAX2;
	doc["IMAX3"  ]["value"] = linkyData.IMAX3;
	doc["PMAX"   ]["value"] = linkyData.PMAX;
	doc["PAPP"   ]["value"] = linkyData.PAPP;
	doc["HHPHC"  ]["value"] = linkyData.HHPHC;
	
	String test;
	serializeJson(doc, test);
	request->send(200, "application/json", test);

	file.close();
}

String www_processor(const String& var) {
	return var;
}
*/

Linky mLinky(D8,D7);
//ESP8266WebServer server(80);
//std::unique_ptr<ESP8266WebServer> server;
ESP8266WebServer server(80);


void handleRoot() {
	if(!SPIFFS.begin()) {
		server.send(200, "text/html", F("<h1>Il y'a eu une erreur lors du chargement de la page (Code d'erreur: SPIFFS_FAIL_FS_MOUNT </h1>"));
		return;
	}
	Serial.println("*FS: Beginned");
	Serial.flush();

	if(!SPIFFS.exists("/www_index.html"))  {
		server.send(200, "text/html", F("<h1>Il y'a eu une erreur lors du chargement de la page (Code d'erreur: SPIFFS_FAIL_EXIST_INDEX </h1>"));
		return;
	}
	Serial.println("*FS: File exist");
	Serial.flush();

	File file = SPIFFS.open("/www_index.html", "r");
	if (!file) {
		server.send(200, "text/html", F("<h1>Il y'a eu une erreur lors du chargement de la page (Code d'erreur: SPIFFS_FAIL_READ_INDEX </h1>"));
		return;
	}
	Serial.println("*FS: Reading ....");
	Serial.flush();

	server.setContentLength(file.size());
    server.send ( 200, "text/html", "<!--init-->");

	while (file.available()){
        server.sendContent(file.readStringUntil('\n'));
		yield();
	}

	/*if (server.streamFile(file, "text/html") != file.size()) {
		Serial.println("*WEB: Sent less data than expected!");
	}*/
	
	server.send(200, "text/html", F("<h1>Il y'a eu une erreur lors du chargement de la page (Code d'erreur: SPIFFS_FAIL_READ_INDEX </h1>"));
	
}

void handleNotFound() {
	server.send(404, "text/plain", "File not found");
}


void setup() {
	Serial.begin(115200);
	Serial.println();
	Serial.println();
	Serial.println("Begin");
	
	SPIFFS.begin();
	
	Dir dir = SPIFFS.openDir("/");
	while (dir.next()) { Serial.print(dir.fileName());  Serial.print(" / "); Serial.println(dir.fileSize()); }


	WiFiManager wifiManager;
	wifiManager.autoConnect("LinkyLink", "LinkySetup");

	if (MDNS.begin("linkylink")) {
		Serial.println("MDNS responder started");
	}

	////server.reset(new ESP8266WebServer(WiFi.localIP(), 80));

	server.on("/", handleRoot);
	server.onNotFound(handleNotFound);
	server.begin();
	Serial.println(F("*WEB: Begin ok"));

	ArduinoOTA.setHostname("LinkyLink");
	ArduinoOTA.setPassword("admin");

	ArduinoOTA.onStart([]() {
		String type;
		if (ArduinoOTA.getCommand() == U_FLASH) { type = "sketch"; } 
		else { type = "filesystem"; }
		Serial.println("Start updating " + type);
	});
	ArduinoOTA.onEnd([]() {  Serial.println("\nEnd"); });
	ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) { Serial.printf("Progress: %u%%\r", (progress / (total / 100))); });
	ArduinoOTA.onError([](ota_error_t error) {
		Serial.printf("Error[%u]: ", error);
		if (error == OTA_AUTH_ERROR) {
			Serial.println("Auth Failed");
		} else if (error == OTA_BEGIN_ERROR) {
			Serial.println("Begin Failed");
		} else if (error == OTA_CONNECT_ERROR) {
			Serial.println("Connect Failed");
		} else if (error == OTA_RECEIVE_ERROR) {
			Serial.println("Receive Failed");
		} else if (error == OTA_END_ERROR) {
			Serial.println("End Failed");
		}
	});
	ArduinoOTA.begin();
	Serial.println(F("*OTA: Begin ok"));

	mLinky.begin();
	Serial.println(F("LinkyLink V1 is in the place"));
}

/*
void dumpStruct(LinkyData linkyData) {
	Serial.print(F("linkyData.ADCO     => ")); Serial.println(linkyData.ADCO);
	Serial.print(F("linkyData.OPTARIF  => ")); Serial.println(linkyData.OPTARIF);
	Serial.print(F("linkyData.ISOUSC   => ")); Serial.println(linkyData.ISOUSC);
	Serial.print(F("linkyData.HCHC     => ")); Serial.println(linkyData.HCHC);
	Serial.print(F("linkyData.HCHP     => ")); Serial.println(linkyData.HCHP);
	Serial.print(F("linkyData.EJPHN    => ")); Serial.println(linkyData.EJPHN);
	Serial.print(F("linkyData.EJPHPM   => ")); Serial.println(linkyData.EJPHPM);
	Serial.print(F("linkyData.BBRHCJB  => ")); Serial.println(linkyData.BBRHCJB);
	Serial.print(F("linkyData.BBRHPJB  => ")); Serial.println(linkyData.BBRHPJB);
	Serial.print(F("linkyData.BBRHCJW  => ")); Serial.println(linkyData.BBRHCJW);
	Serial.print(F("linkyData.BBRHPJW  => ")); Serial.println(linkyData.BBRHPJW);
	Serial.print(F("linkyData.BBRHCJR  => ")); Serial.println(linkyData.BBRHCJR);
	Serial.print(F("linkyData.BBRHPJR  => ")); Serial.println(linkyData.BBRHPJR);
	Serial.print(F("linkyData.PEJP     => ")); Serial.println(linkyData.PEJP);
	Serial.print(F("linkyData.PTEC     => ")); Serial.println(linkyData.PTEC);
	Serial.print(F("linkyData.DEMAIN   => ")); Serial.println(linkyData.DEMAIN);
	Serial.print(F("linkyData.IINST    => ")); Serial.println(linkyData.IINST);
	Serial.print(F("linkyData.IINST1   => ")); Serial.println(linkyData.IINST1);
	Serial.print(F("linkyData.IINST2   => ")); Serial.println(linkyData.IINST2);
	Serial.print(F("linkyData.IINST3   => ")); Serial.println(linkyData.IINST3);
	Serial.print(F("linkyData.ADPS     => ")); Serial.println(linkyData.ADPS);
	Serial.print(F("linkyData.ADIR1    => ")); Serial.println(linkyData.ADIR1);
	Serial.print(F("linkyData.ADIR2    => ")); Serial.println(linkyData.ADIR2);
	Serial.print(F("linkyData.ADIR3    => ")); Serial.println(linkyData.ADIR3);
	Serial.print(F("linkyData.IMAX     => ")); Serial.println(linkyData.IMAX);
	Serial.print(F("linkyData.IMAX1    => ")); Serial.println(linkyData.IMAX1);
	Serial.print(F("linkyData.IMAX2    => ")); Serial.println(linkyData.IMAX2);
	Serial.print(F("linkyData.IMAX3    => ")); Serial.println(linkyData.IMAX3);
	Serial.print(F("linkyData.PMAX     => ")); Serial.println(linkyData.PMAX);
	Serial.print(F("linkyData.PAPP     => ")); Serial.println(linkyData.PAPP);
	Serial.print(F("linkyData.HHPHC    => ")); Serial.println(linkyData.HHPHC);
	Serial.print(F("linkyData.MOTDETA  => ")); Serial.println(linkyData.MOTDETAT);
	Serial.print(F("linkyData.PPOT     => ")); Serial.println(linkyData.PPOT);
	Serial.println();
	Serial.println();
}
*/

//int a,b;
void loop() {
	//a = ESP.getFreeHeap();
	server.handleClient();
	ArduinoOTA.handle();
	mLinky.update(1000);
	//LinkyData data = mLinky.grab();
	//dumpStruct(data);
	//delay(10);


	//b = ESP.getFreeHeap();
	//Serial.print("Memory leak: ");
	//Serial.println(a-b);
	//Serial.print("Heap left: ");
	//Serial.println(b);
	//Serial.println();
	//Serial.println();
}

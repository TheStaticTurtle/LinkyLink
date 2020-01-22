#include <FS.h>                   //this needs to be first, or it all crashes and burns...
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncWiFiManager.h>
#include <ArduinoJson.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <WiFiUdp.h>
#include <Ticker.h>

#include "linky.h"

const char* MODULE_SERIAL_NUM   = "LinkyLinky-01-DEV";
const char* MODULE_VERSION_SOFT = "V1.0";
const char* MODULE_VERSION_HARD = "V0.0-DEV";

const int   PINS_LED_STATUS_WIFI = D3;
const int   PINS_LED_STATUS_LINK = D5;
const int   PINS_LED_STATUS_ERR  = D4;
const int   PINS_CTRL_DATA_IN    = D6;


Linky mLinky(D7,-1);
AsyncWebServer server(80);
DNSServer dns;
Ticker tickerWifiLed;

long lastRequestCounter = 0;
bool shouldReEnableListingToLinky = false;

const size_t ApiJsonTemplate_capacity = 31*JSON_OBJECT_SIZE(5) + JSON_OBJECT_SIZE(31) + 2190;
DynamicJsonDocument ApiJsonTemplate_doc(ApiJsonTemplate_capacity);
bool ApiJsonTemplate_IsThisShitSetup = false;

void handleAPI_Info(AsyncWebServerRequest *request) {
	Serial.println("*WEB: Serving 200 for: /api/info");
	Serial.flush();
	AsyncResponseStream *response = request->beginResponseStream("application/json");
	
	StaticJsonDocument<200> doc;
	doc["serial_number"]    = MODULE_SERIAL_NUM;
	doc["version_software"] = MODULE_VERSION_SOFT;
	doc["version_hardware"] = MODULE_VERSION_HARD;
	doc["uptime"]           = millis() / 1000;

	serializeJson(doc, *response);

	request->send(response);
	lastRequestCounter = millis();
	shouldReEnableListingToLinky = true;
}

void handleAPI_Linky(AsyncWebServerRequest *request) {
	Serial.println("*WEB: Serving 200 for: /api/linky");
	Serial.flush();

	LinkyData linkyData = mLinky.grab();

	if(request->hasParam("key")) {
		AsyncWebParameter* p = request->getParam("key");
		String output = F("Key is invalid");
		if(p->value() == "ADCO"   ) { output = linkyData.ADCO;    }   
		if(p->value() == "OPTARIF") { output = linkyData.OPTARIF; }      
		if(p->value() == "ISOUSC" ) { output = linkyData.ISOUSC;  }     
		if(p->value() == "HCHC"   ) { output = linkyData.HCHC;    }   
		if(p->value() == "HCHP"   ) { output = linkyData.HCHP;    }   
		if(p->value() == "EJPHN"  ) { output = linkyData.EJPHN;   }    
		if(p->value() == "EJPHPM" ) { output = linkyData.EJPHPM;  }     
		if(p->value() == "BBRHCJB") { output = linkyData.BBRHCJB; }      
		if(p->value() == "BBRHPJB") { output = linkyData.BBRHPJB; }      
		if(p->value() == "BBRHCJW") { output = linkyData.BBRHCJW; }      
		if(p->value() == "BBRHPJW") { output = linkyData.BBRHPJW; }      
		if(p->value() == "BBRHCJR") { output = linkyData.BBRHCJR; }      
		if(p->value() == "BBRHPJR") { output = linkyData.BBRHPJR; }      
		if(p->value() == "PEJP"   ) { output = linkyData.PEJP;    }   
		if(p->value() == "PTEC"   ) { output = linkyData.PTEC;    }   
		if(p->value() == "DEMAIN" ) { output = linkyData.DEMAIN;  }     
		if(p->value() == "IINST"  ) { output = linkyData.IINST;   }    
		if(p->value() == "IINST1" ) { output = linkyData.IINST1;  }     
		if(p->value() == "IINST2" ) { output = linkyData.IINST2;  }     
		if(p->value() == "IINST3" ) { output = linkyData.IINST3;  }     
		if(p->value() == "ADPS"   ) { output = linkyData.ADPS;    }   
		if(p->value() == "ADIR1"  ) { output = linkyData.ADIR1;   }    
		if(p->value() == "ADIR2"  ) { output = linkyData.ADIR2;   }    
		if(p->value() == "ADIR3"  ) { output = linkyData.ADIR3;   }    
		if(p->value() == "IMAX"   ) { output = linkyData.IMAX;    }
		if(p->value() == "IMAX1"  ) { output = linkyData.IMAX1;   }
		if(p->value() == "IMAX2"  ) { output = linkyData.IMAX2;   }
		if(p->value() == "IMAX3"  ) { output = linkyData.IMAX3;   }    
		if(p->value() == "PMAX"   ) { output = linkyData.PMAX;    }
		if(p->value() == "PAPP"   ) { output = linkyData.PAPP;    }
		if(p->value() == "HHPHC"  ) { output = linkyData.HHPHC;   }

		AsyncWebServerResponse *response = request->beginResponse(200,"text/plain",output);
		response->addHeader("Access-Control-Allow-Methods", "POST,GET,OPTIONS");
   	 	response->addHeader("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");
		response->addHeader("Access-Control-Allow-Origin", "*");
		request->send(response);

	} else {

		if(ApiJsonTemplate_IsThisShitSetup == false) {
			digitalWrite(PINS_CTRL_DATA_IN,true);
			Serial.println("*WEB: Api tempalte not cached yet.");
			if(!SPIFFS.begin()) { request->send(520, "application/json", F("{\"ERROR\":\"SPIFFS_FAIL_FS_MOUNT\"}")); return; }
			if(!SPIFFS.exists("/api_template.json")) { request->send(520, "application/json", F("{\"ERROR\":\"SPIFFS_FAIL_EXIST\"}")); return; }

			File file = SPIFFS.open("/api_template.json", "r");
			if (!file) { request->send(503, "application/json", F("{\"ERROR\":\"SPIFFS_FAIL_READ\"}")); return;  }

			deserializeJson(ApiJsonTemplate_doc, file);
			ApiJsonTemplate_IsThisShitSetup = true;
			file.close();
		} else {
			Serial.println("*WEB: Api template cached.");
		}

		ApiJsonTemplate_doc["ADCO"   ]["value"] = linkyData.ADCO;
		ApiJsonTemplate_doc["OPTARIF"]["value"] = linkyData.OPTARIF;
		ApiJsonTemplate_doc["ISOUSC" ]["value"] = linkyData.ISOUSC;
		ApiJsonTemplate_doc["HCHC"   ]["value"] = linkyData.HCHC;
		ApiJsonTemplate_doc["HCHP"   ]["value"] = linkyData.HCHP;
		ApiJsonTemplate_doc["EJPHN"  ]["value"] = linkyData.EJPHN;
		ApiJsonTemplate_doc["EJPHPM" ]["value"] = linkyData.EJPHPM;
		ApiJsonTemplate_doc["BBRHCJB"]["value"] = linkyData.BBRHCJB;
		ApiJsonTemplate_doc["BBRHPJB"]["value"] = linkyData.BBRHPJB;
		ApiJsonTemplate_doc["BBRHCJW"]["value"] = linkyData.BBRHCJW;
		ApiJsonTemplate_doc["BBRHPJW"]["value"] = linkyData.BBRHPJW;
		ApiJsonTemplate_doc["BBRHCJR"]["value"] = linkyData.BBRHCJR;
		ApiJsonTemplate_doc["BBRHPJR"]["value"] = linkyData.BBRHPJR;
		ApiJsonTemplate_doc["PEJP"   ]["value"] = linkyData.PEJP;
		ApiJsonTemplate_doc["PTEC"   ]["value"] = linkyData.PTEC;
		ApiJsonTemplate_doc["DEMAIN" ]["value"] = linkyData.DEMAIN;
		ApiJsonTemplate_doc["IINST"  ]["value"] = linkyData.IINST;
		ApiJsonTemplate_doc["IINST1" ]["value"] = linkyData.IINST1;
		ApiJsonTemplate_doc["IINST2" ]["value"] = linkyData.IINST2;
		ApiJsonTemplate_doc["IINST3" ]["value"] = linkyData.IINST3;
		ApiJsonTemplate_doc["ADPS"   ]["value"] = linkyData.ADPS;
		ApiJsonTemplate_doc["ADIR1"  ]["value"] = linkyData.ADIR1;
		ApiJsonTemplate_doc["ADIR2"  ]["value"] = linkyData.ADIR2;
		ApiJsonTemplate_doc["ADIR3"  ]["value"] = linkyData.ADIR3;
		ApiJsonTemplate_doc["IMAX"   ]["value"] = linkyData.IMAX;
		ApiJsonTemplate_doc["IMAX1"  ]["value"] = linkyData.IMAX1;
		ApiJsonTemplate_doc["IMAX2"  ]["value"] = linkyData.IMAX2;
		ApiJsonTemplate_doc["IMAX3"  ]["value"] = linkyData.IMAX3;
		ApiJsonTemplate_doc["PMAX"   ]["value"] = linkyData.PMAX;
		ApiJsonTemplate_doc["PAPP"   ]["value"] = linkyData.PAPP;
		ApiJsonTemplate_doc["HHPHC"  ]["value"] = linkyData.HHPHC;
		

		AsyncResponseStream *response = request->beginResponseStream("application/json");
		response->addHeader("Access-Control-Allow-Methods", "POST,GET,OPTIONS");
	    response->addHeader("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");
		response->addHeader("Access-Control-Allow-Origin", "*");
		serializeJson(ApiJsonTemplate_doc, *response);
		request->send(response);
	}

	lastRequestCounter = millis();
	shouldReEnableListingToLinky = true;
	//digitalWrite(PINS_CTRL_DATA_IN,false);
}


void sendFileToServer(char* filename, AsyncWebServerRequest *request) {
	digitalWrite(PINS_CTRL_DATA_IN,true);

	Serial.println("*WEB: Serving 200 for url: "+request->url());
	Serial.flush();

	AsyncWebServerResponse *response = request->beginResponse(SPIFFS, filename);
	
	response->addHeader("Access-Control-Allow-Methods", "POST,GET,OPTIONS");
    response->addHeader("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");
	response->addHeader("Access-Control-Allow-Origin", "*");

	request->send(response);
	lastRequestCounter = millis();
	shouldReEnableListingToLinky = true;
	//digitalWrite(PINS_CTRL_DATA_IN,false);
}

void handleNotFound( AsyncWebServerRequest *request) {
	digitalWrite(PINS_CTRL_DATA_IN,true);
	Serial.println("*WEB: Serving 404 for url: "+request->url());
	Serial.flush();
    request->send(404, "text/plain", "File "+request->url()+" not found");
	lastRequestCounter = millis();
	shouldReEnableListingToLinky = true;
	//digitalWrite(PINS_CTRL_DATA_IN,false);
}

void tickWifiLed() {
  int state = digitalRead(PINS_LED_STATUS_WIFI);  // get the current state of GPIO1 pin
  digitalWrite(PINS_LED_STATUS_WIFI, !state);     // set pin to the opposite state
}

void configModeCallback (AsyncWiFiManager *myWiFiManager) {
  tickerWifiLed.attach(0.1, tickWifiLed);
}


void setup() {
	pinMode(PINS_LED_STATUS_ERR ,OUTPUT);
	pinMode(PINS_LED_STATUS_LINK,OUTPUT);
	pinMode(PINS_LED_STATUS_WIFI,OUTPUT);
	pinMode(PINS_CTRL_DATA_IN   ,OUTPUT);
	digitalWrite(PINS_LED_STATUS_ERR ,true);
	digitalWrite(PINS_LED_STATUS_LINK,true);
	digitalWrite(PINS_LED_STATUS_WIFI,true);
	digitalWrite(PINS_CTRL_DATA_IN   ,true);
	delay(100);
	digitalWrite(PINS_LED_STATUS_ERR ,false);
	digitalWrite(PINS_LED_STATUS_LINK,false);
	digitalWrite(PINS_LED_STATUS_WIFI,true);
	digitalWrite(PINS_CTRL_DATA_IN   ,false);



	Serial.begin(115200);
	Serial.println();
	Serial.println();
	Serial.println("Begin");
	
	SPIFFS.begin();
	
	Dir dir = SPIFFS.openDir("/");
	while (dir.next()) { Serial.print(dir.fileName());  Serial.print(" / "); Serial.println(dir.fileSize()); }

	tickerWifiLed.attach(0.6, tickWifiLed);

	AsyncWiFiManager wifiManager(&server,&dns);
	
	wifiManager.setAPCallback(configModeCallback);
	wifiManager.setConfigPortalTimeout(180);

	if (!wifiManager.autoConnect(MODULE_SERIAL_NUM, "LinkyLinkSetup")) {
		Serial.println("failed to connect and hit timeout");
		digitalWrite(PINS_LED_STATUS_ERR ,true);
		ESP.reset(); delay(1000);
	}

	tickerWifiLed.detach();
	digitalWrite(PINS_LED_STATUS_WIFI, true);

    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

	if (MDNS.begin("LinkyLink")) { Serial.println("MDNS responder started"); }
	
	server.on("/"                 , HTTP_GET, [](AsyncWebServerRequest *request){ sendFileToServer("/www/index.html"       ,request); });
	server.on("/logo.svg"         , HTTP_GET, [](AsyncWebServerRequest *request){ sendFileToServer("/www/logo.svg"         ,request); });
	server.on("/fa-solid-900.ttf" , HTTP_GET, [](AsyncWebServerRequest *request){ sendFileToServer("/www/fa-solid-900.ttf" ,request); });
	server.on("/js.js"            , HTTP_GET, [](AsyncWebServerRequest *request){ sendFileToServer("/www/js.js"            ,request); });
	server.on("/css.css"          , HTTP_GET, [](AsyncWebServerRequest *request){ sendFileToServer("/www/css.css"          ,request); });
	server.on("/api/linky"        , HTTP_GET, handleAPI_Linky);
	//server.on("/api/linky_custom" , HTTP_GET, handleAPI_LinkyCustom);
	server.on("/api/info"         , HTTP_GET, handleAPI_Info);
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


void loop() {
	//a = ESP.getFreeHeap();
	ArduinoOTA.handle();
	mLinky.updateAsync(PINS_LED_STATUS_LINK,true);

	if(((lastRequestCounter + 30000) < millis()) && shouldReEnableListingToLinky){
		shouldReEnableListingToLinky = false;
		digitalWrite(PINS_CTRL_DATA_IN,false);
	}
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

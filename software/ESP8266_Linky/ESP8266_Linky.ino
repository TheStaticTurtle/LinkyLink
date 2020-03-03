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
#include <SPIFFSLogger.h>

#include "linky.h"

const char* MODULE_SERIAL_NUM   = "LinkyLinky-00001";
const char* MODULE_VERSION_SOFT = "V1.0";
const char* MODULE_VERSION_HARD = "V1.0";

const int   PINS_LED_STATUS_WIFI = D3;
const int   PINS_LED_STATUS_LINK = D5;
const int   PINS_LED_STATUS_ERR  = D4;
const int   PINS_CTRL_DATA_IN    = D6;
const int   LOGGING_INTERVAL     = 10000;
const int   LOGGING_DAY_KEPT     = 1;
const char* LOGGING_FOLDER       = "/log/linkydata";

 
Linky mLinky(D7,-1,PINS_CTRL_DATA_IN);
AsyncWebServer server(80);
DNSServer dns;
Ticker tickerWifiLed;

long lastRequestCounter = 0;
bool shouldReEnableListingToLinky = false;

const size_t ApiJsonTemplate_capacity = JSON_OBJECT_SIZE(31) + 310;

struct LoggingData {
	long PAPP;
};

SPIFFSLogger<LoggingData> logger(LOGGING_FOLDER, LOGGING_DAY_KEPT);


void handleAPI_Reboot(AsyncWebServerRequest *request) {
	Serial.println("[WEB] Serving 200 for: /api/reboot");
	Serial.flush();

	AsyncResponseStream *response = request->beginResponseStream("plain/text");
	response->addHeader("Access-Control-Allow-Methods", "POST,GET,OPTIONS");
	response->addHeader("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");
	response->addHeader("Access-Control-Allow-Origin", "*");
	response->printf("REBOOTING");
	request->send(response);

	ESP.restart();
	lastRequestCounter = millis();
	shouldReEnableListingToLinky = true;
}

void handleAPI_Info(AsyncWebServerRequest *request) {
	Serial.println("[WEB] Serving 200 for: /api/info");
	Serial.flush();
	AsyncResponseStream *response = request->beginResponseStream("application/json");
	response->addHeader("Access-Control-Allow-Methods", "POST,GET,OPTIONS");
	response->addHeader("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");
	response->addHeader("Access-Control-Allow-Origin", "*");
	
	StaticJsonDocument<200> doc;
	doc["serial_number"]    = MODULE_SERIAL_NUM;
	doc["version_software"] = MODULE_VERSION_SOFT;
	doc["version_hardware"] = MODULE_VERSION_HARD;
	doc["uptime"]           = millis() / 1000;
	doc["lastUpdate"]       = mLinky.lastFullFrame() /1000;
	
	serializeJson(doc, *response);

	request->send(response);
	lastRequestCounter = millis();
	shouldReEnableListingToLinky = true;
}

void handleAPI_Debug(AsyncWebServerRequest *request) {
	Serial.println("[WEB] Serving 200 for: /api/debug");
	Serial.flush();
	AsyncResponseStream *response = request->beginResponseStream("application/json");
	response->addHeader("Access-Control-Allow-Methods", "POST,GET,OPTIONS");
	response->addHeader("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");
	response->addHeader("Access-Control-Allow-Origin", "*");
	
	StaticJsonDocument<200> doc;
	doc["serial_number"]     = MODULE_SERIAL_NUM;
	doc["version_software"]  = MODULE_VERSION_SOFT;
	doc["version_hardware"]  = MODULE_VERSION_HARD;
	doc["uptime"]            = millis() / 1000;
	doc["lastUpdate"]        = mLinky.lastFullFrame() /1000;
	doc["wifi_status"]       = WiFi.status();
	doc["wifi_macaddr"]      = WiFi.macAddress();
	doc["free_heap"]         = ESP.getFreeHeap();
	doc["max_block_size"]    = ESP.getMaxFreeBlockSize();
	doc["last_reset_cause"]  = ESP.getResetReason();
	doc["core_version"]      = ESP.getCoreVersion();
	doc["vcc"]               = ESP.getVcc();
	
	serializeJson(doc, *response);

	request->send(response);
	lastRequestCounter = millis();
	shouldReEnableListingToLinky = true;
}

void handleAPI_Linky(AsyncWebServerRequest *request) {
	Serial.println("[WEB] Serving 200 for: /api/linky");
	Serial.flush();

	LinkyData linkyData = mLinky.grab();

	if(request->hasParam("key")) {
		AsyncWebParameter* p = request->getParam("key");
		String output = F("Key is invalid");
		if(p->value() == "ADCO"    ) { output = linkyData.ADCO;    }   
		if(p->value() == "OPTARIF" ) { output = linkyData.OPTARIF; }      
		if(p->value() == "ISOUSC"  ) { output = linkyData.ISOUSC;  }     
		if(p->value() == "HCHC"    ) { output = linkyData.HCHC;    }   
		if(p->value() == "HCHP"    ) { output = linkyData.HCHP;    }   
		if(p->value() == "EJPHN"   ) { output = linkyData.EJPHN;   }    
		if(p->value() == "EJPHPM"  ) { output = linkyData.EJPHPM;  }     
		if(p->value() == "BBRHCJB" ) { output = linkyData.BBRHCJB; }      
		if(p->value() == "BBRHPJB" ) { output = linkyData.BBRHPJB; }      
		if(p->value() == "BBRHCJW" ) { output = linkyData.BBRHCJW; }      
		if(p->value() == "BBRHPJW" ) { output = linkyData.BBRHPJW; }      
		if(p->value() == "BBRHCJR" ) { output = linkyData.BBRHCJR; }      
		if(p->value() == "BBRHPJR" ) { output = linkyData.BBRHPJR; }      
		if(p->value() == "PEJP"    ) { output = linkyData.PEJP;    }   
		if(p->value() == "PTEC"    ) { output = linkyData.PTEC;    }   
		if(p->value() == "DEMAIN"  ) { output = linkyData.DEMAIN;  }     
		if(p->value() == "IINST"   ) { output = linkyData.IINST;   }    
		if(p->value() == "IINST1"  ) { output = linkyData.IINST1;  }     
		if(p->value() == "IINST2"  ) { output = linkyData.IINST2;  }     
		if(p->value() == "IINST3"  ) { output = linkyData.IINST3;  }     
		if(p->value() == "ADPS"    ) { output = linkyData.ADPS;    }   
		if(p->value() == "ADIR1"   ) { output = linkyData.ADIR1;   }    
		if(p->value() == "ADIR2"   ) { output = linkyData.ADIR2;   }    
		if(p->value() == "ADIR3"   ) { output = linkyData.ADIR3;   }    
		if(p->value() == "IMAX"    ) { output = linkyData.IMAX;    }
		if(p->value() == "IMAX1"   ) { output = linkyData.IMAX1;   }
		if(p->value() == "IMAX2"   ) { output = linkyData.IMAX2;   }
		if(p->value() == "IMAX3"   ) { output = linkyData.IMAX3;   }    
		if(p->value() == "PMAX"    ) { output = linkyData.PMAX;    }
		if(p->value() == "PAPP"    ) { output = linkyData.PAPP;    }
		if(p->value() == "HHPHC"   ) { output = linkyData.HHPHC;   }
		if(p->value() == "MOTDETAT") { output = linkyData.MOTDETAT;   }
		if(p->value() == "PPOT"    ) { output = linkyData.PPOT;   }

		AsyncWebServerResponse *response = request->beginResponse(200,"text/plain",output);
		response->addHeader("Access-Control-Allow-Methods", "POST,GET,OPTIONS");
		response->addHeader("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");
		response->addHeader("Access-Control-Allow-Origin", "*");
		request->send(response);

	} else {
		DynamicJsonDocument ApiJsonTemplate_doc(ApiJsonTemplate_capacity);
		ApiJsonTemplate_doc["ADCO"    ] = linkyData.ADCO;
		ApiJsonTemplate_doc["OPTARIF" ] = linkyData.OPTARIF;
		ApiJsonTemplate_doc["ISOUSC"  ] = linkyData.ISOUSC;
		ApiJsonTemplate_doc["HCHC"    ] = linkyData.HCHC;
		ApiJsonTemplate_doc["HCHP"    ] = linkyData.HCHP;
		ApiJsonTemplate_doc["EJPHN"   ] = linkyData.EJPHN;
		ApiJsonTemplate_doc["EJPHPM"  ] = linkyData.EJPHPM;
		ApiJsonTemplate_doc["BBRHCJB" ] = linkyData.BBRHCJB;
		ApiJsonTemplate_doc["BBRHPJB" ] = linkyData.BBRHPJB;
		ApiJsonTemplate_doc["BBRHCJW" ] = linkyData.BBRHCJW;
		ApiJsonTemplate_doc["BBRHPJW" ] = linkyData.BBRHPJW;
		ApiJsonTemplate_doc["BBRHCJR" ] = linkyData.BBRHCJR;
		ApiJsonTemplate_doc["BBRHPJR" ] = linkyData.BBRHPJR;
		ApiJsonTemplate_doc["PEJP"    ] = linkyData.PEJP;
		ApiJsonTemplate_doc["PTEC"    ] = linkyData.PTEC;
		ApiJsonTemplate_doc["DEMAIN"  ] = linkyData.DEMAIN;
		ApiJsonTemplate_doc["IINST"   ] = linkyData.IINST;
		ApiJsonTemplate_doc["IINST1"  ] = linkyData.IINST1;
		ApiJsonTemplate_doc["IINST2"  ] = linkyData.IINST2;
		ApiJsonTemplate_doc["IINST3"  ] = linkyData.IINST3;
		ApiJsonTemplate_doc["ADPS"    ] = linkyData.ADPS;
		ApiJsonTemplate_doc["ADIR1"   ] = linkyData.ADIR1;
		ApiJsonTemplate_doc["ADIR2"   ] = linkyData.ADIR2;
		ApiJsonTemplate_doc["ADIR3"   ] = linkyData.ADIR3;
		ApiJsonTemplate_doc["IMAX"    ] = linkyData.IMAX;
		ApiJsonTemplate_doc["IMAX1"   ] = linkyData.IMAX1;
		ApiJsonTemplate_doc["IMAX2"   ] = linkyData.IMAX2;
		ApiJsonTemplate_doc["IMAX3"   ] = linkyData.IMAX3;
		ApiJsonTemplate_doc["PMAX"    ] = linkyData.PMAX;
		ApiJsonTemplate_doc["PAPP"    ] = linkyData.PAPP;
		ApiJsonTemplate_doc["HHPHC"   ] = linkyData.HHPHC;
		ApiJsonTemplate_doc["MOTDETAT"] = linkyData.MOTDETAT;
		ApiJsonTemplate_doc["PPOT"    ] = linkyData.PPOT;

		AsyncResponseStream *response = request->beginResponseStream("application/json");
		response->addHeader("Access-Control-Allow-Methods", "POST,GET,OPTIONS");
		response->addHeader("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");
		response->addHeader("Access-Control-Allow-Origin", "*");
		serializeJson(ApiJsonTemplate_doc, *response);
		request->send(response);
	}

	lastRequestCounter = millis();
	shouldReEnableListingToLinky = true;
}

float max(float a, float b) {
	if( a > b ) { return a; }
	return b;
}

void pathFromDate(char *output, time_t date) {
    struct tm *tinfo = gmtime(&date);
    sprintf_P(output,
              "%s/%d%02d%02d",
              "/log/linkydata",
              1900 + tinfo->tm_year,
              tinfo->tm_mon + 1,
              tinfo->tm_mday);
}

void handleAPI_History(AsyncWebServerRequest *request) {
	digitalWrite(PINS_CTRL_DATA_IN,true);
	Serial.println("[WEB] Serving 200 for: /api/history");
	Serial.flush();

	const time_t todayTime = time(nullptr);
	const time_t yesterday = todayTime - 3600 * 24;
	const size_t rowCountY = logger.rowCount(yesterday);
	const size_t rowCountT = logger.rowCount(todayTime);

	struct SPIFFSLogData<LoggingData> readback;

	AsyncResponseStream *response = request->beginResponseStream("text/plain");
	response->addHeader("Access-Control-Allow-Methods", "POST,GET,OPTIONS");
	response->addHeader("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");
	response->addHeader("Access-Control-Allow-Origin", "*");

	//RESPONSE V2 - Optimized - Fast re-implementation of SPIFFSLogger read for this use case (Mainly not opening and closing the file at each read)
	int count = 1111;
	float step = (float)(rowCountY+rowCountT) / (float)count;
	time_t begin_time = 0;
	time_t end_time = 0;

	response->printf("{\"data\":[");

	unsigned long valuesSmoothSum = 0;
	int valuesCount = 0;
	float counter = 0;


    size_t size = sizeof(SPIFFSLogData<LoggingData>);
	char path[32];
	pathFromDate(path,yesterday);
	if(SPIFFS.exists(path)) {
    	File f = SPIFFS.open(path, "r");

		for(int i=0; i <  rowCountY ; i++ ) {
		    f.seek(i * size);
		    f.read((uint8_t *)&readback , size);


			if(i == 0) { begin_time = readback.timestampUTC; }
			end_time = readback.timestampUTC;
			valuesSmoothSum += readback.data.PAPP;
			valuesCount ++;

			counter ++;

			if(counter > step) {
				response->printf("%lu,", (valuesSmoothSum / valuesCount) );
				valuesSmoothSum = readback.data.PAPP;
				valuesCount = 1;
				counter-=step;
			}

		}
		f.close();
	}

	pathFromDate(path,todayTime);
	if(SPIFFS.exists(path)) {
    	File f = SPIFFS.open(path, "r");

		for(int i=0; i <  rowCountT ; i++ ) {
		    f.seek(i * size);
		    f.read((uint8_t *)&readback , size);

			if(i == 0 && begin_time == 0) { begin_time = readback.timestampUTC; }
			end_time = readback.timestampUTC;
			valuesSmoothSum += readback.data.PAPP;
			valuesCount ++;

			counter ++;

			if(counter > step) {
				response->printf("%lu,", (valuesSmoothSum / valuesCount) );
				valuesSmoothSum = readback.data.PAPP;
				valuesCount = 1;
				counter-=step;
			}
		}
		f.close();
	}

	response->printf("], \"db_value_count\": %d,\"result_steps\": %f, \"begin_time\": %lu, \"end_time\": %lu}",(rowCountY+rowCountT),step,begin_time,end_time);

	request->send(response);

	lastRequestCounter = millis();
	shouldReEnableListingToLinky = true;
}

void sendFileToServer(char* filename, AsyncWebServerRequest *request) {
	digitalWrite(PINS_CTRL_DATA_IN,true);

	Serial.println("[WEB] Serving 200 for url: "+request->url());
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
	Serial.println("[WEB] Serving 404 for url: "+request->url());
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
	Serial.println("Booting up....");
	
	SPIFFS.begin();

	logger.init();
	
	Dir dir = SPIFFS.openDir("/");
	Serial.println("[SPIFFS] Files in memory: ");
	while (dir.next()) { Serial.print("\t"); Serial.print(dir.fileName());  Serial.print(" / "); Serial.println(dir.fileSize()); }

	tickerWifiLed.attach(0.6, tickWifiLed);

	AsyncWiFiManager wifiManager(&server,&dns);
	
	wifiManager.setAPCallback(configModeCallback);
	wifiManager.setConfigPortalTimeout(60*10);

	WiFi.hostname(MODULE_SERIAL_NUM);

	if (!wifiManager.autoConnect(MODULE_SERIAL_NUM, "LinkyLinkSetup")) {
		Serial.println("[WiFi] Failed to connect and hit timeout");
		digitalWrite(PINS_LED_STATUS_ERR ,true);
		ESP.reset(); delay(1000);
	}
	tickerWifiLed.detach();
	digitalWrite(PINS_LED_STATUS_WIFI, true);

	Serial.print("[WiFi] IP Address: ");
	Serial.println(WiFi.localIP());

	configTime(0, 0, "pool.ntp.org");

	if (MDNS.begin("LinkyLink")) { Serial.println("[MDNS] Begin ok"); } else  { Serial.println("[MDNS] Begin failed"); }
	
	server.on("/"                 , HTTP_GET, [](AsyncWebServerRequest *request){ sendFileToServer("/www/index.html"       ,request); });
	server.on("/logo.svg"         , HTTP_GET, [](AsyncWebServerRequest *request){ sendFileToServer("/www/logo.svg"         ,request); });
	server.on("/fa-solid-900.ttf" , HTTP_GET, [](AsyncWebServerRequest *request){ sendFileToServer("/www/fa-solid-900.ttf" ,request); });
	server.on("/js.js"            , HTTP_GET, [](AsyncWebServerRequest *request){ sendFileToServer("/www/js.js"            ,request); });
	server.on("/css.css"          , HTTP_GET, [](AsyncWebServerRequest *request){ sendFileToServer("/www/css.css"          ,request); });
	server.on("/favicon.png"      , HTTP_GET, [](AsyncWebServerRequest *request){ sendFileToServer("/www/favicon.png"      ,request); });
	server.on("/api/linky"        , HTTP_GET, handleAPI_Linky);

	server.on("/api/info"         , HTTP_GET, handleAPI_Info);
	server.on("/api/debug"        , HTTP_GET, handleAPI_Debug);
	server.on("/api/history"      , HTTP_GET, handleAPI_History);
	server.on("/api/reboot"      , HTTP_GET, handleAPI_Reboot);
	server.onNotFound(handleNotFound);

	server.begin();

	Serial.println(F("[WEB] Begin"));

	ArduinoOTA.setHostname("LinkyLink");
	// ArduinoOTA.setPassword("admin");

	ArduinoOTA.onStart([]() {
		String type;
		if (ArduinoOTA.getCommand() == U_FLASH) { type = "sketch"; } 
		else { type = "filesystem"; }
		Serial.println("[OTA] Start updating " + type);
	});
	ArduinoOTA.onEnd([]() {  Serial.println("\nEnd"); });
	ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) { Serial.printf("[OTA] Progress: %u%%\r", (progress / (total / 100))); });
	ArduinoOTA.onError([](ota_error_t error) {
		Serial.printf("[OTA] Error[%u]: ", error);
		if (error == OTA_AUTH_ERROR) {
			Serial.println("[OTA] Auth Failed");
		} else if (error == OTA_BEGIN_ERROR) {
			Serial.println("[OTA] Begin Failed");
		} else if (error == OTA_CONNECT_ERROR) {
			Serial.println("[OTA] Connect Failed");
		} else if (error == OTA_RECEIVE_ERROR) {
			Serial.println("[OTA] Receive Failed");
		} else if (error == OTA_END_ERROR) {
			Serial.println("[OTA] End Failed");
		}
	});
	ArduinoOTA.begin();
	Serial.println(F("[OTA] Begin"));

	mLinky.begin();
	Serial.println(F("LinkyLink V1 is in the place"));
}

#ifdef DEBUG
	void dumpStruct(LinkyData linkyData) {
		Serial.print(F("[LINKY] Data dump incomming"));
		Serial.print(F("[LINKY] linkyData.ADCO     => ")); Serial.println(linkyData.ADCO);
		Serial.print(F("[LINKY] linkyData.OPTARIF  => ")); Serial.println(linkyData.OPTARIF);
		Serial.print(F("[LINKY] linkyData.ISOUSC   => ")); Serial.println(linkyData.ISOUSC);
		Serial.print(F("[LINKY] linkyData.HCHC     => ")); Serial.println(linkyData.HCHC);
		Serial.print(F("[LINKY] linkyData.HCHP     => ")); Serial.println(linkyData.HCHP);
		Serial.print(F("[LINKY] linkyData.EJPHN    => ")); Serial.println(linkyData.EJPHN);
		Serial.print(F("[LINKY] linkyData.EJPHPM   => ")); Serial.println(linkyData.EJPHPM);
		Serial.print(F("[LINKY] linkyData.BBRHCJB  => ")); Serial.println(linkyData.BBRHCJB);
		Serial.print(F("[LINKY] linkyData.BBRHPJB  => ")); Serial.println(linkyData.BBRHPJB);
		Serial.print(F("[LINKY] linkyData.BBRHCJW  => ")); Serial.println(linkyData.BBRHCJW);
		Serial.print(F("[LINKY] linkyData.BBRHPJW  => ")); Serial.println(linkyData.BBRHPJW);
		Serial.print(F("[LINKY] linkyData.BBRHCJR  => ")); Serial.println(linkyData.BBRHCJR);
		Serial.print(F("[LINKY] linkyData.BBRHPJR  => ")); Serial.println(linkyData.BBRHPJR);
		Serial.print(F("[LINKY] linkyData.PEJP     => ")); Serial.println(linkyData.PEJP);
		Serial.print(F("[LINKY] linkyData.PTEC     => ")); Serial.println(linkyData.PTEC);
		Serial.print(F("[LINKY] linkyData.DEMAIN   => ")); Serial.println(linkyData.DEMAIN);
		Serial.print(F("[LINKY] linkyData.IINST    => ")); Serial.println(linkyData.IINST);
		Serial.print(F("[LINKY] linkyData.IINST1   => ")); Serial.println(linkyData.IINST1);
		Serial.print(F("[LINKY] linkyData.IINST2   => ")); Serial.println(linkyData.IINST2);
		Serial.print(F("[LINKY] linkyData.IINST3   => ")); Serial.println(linkyData.IINST3);
		Serial.print(F("[LINKY] linkyData.ADPS     => ")); Serial.println(linkyData.ADPS);
		Serial.print(F("[LINKY] linkyData.ADIR1    => ")); Serial.println(linkyData.ADIR1);
		Serial.print(F("[LINKY] linkyData.ADIR2    => ")); Serial.println(linkyData.ADIR2);
		Serial.print(F("[LINKY] linkyData.ADIR3    => ")); Serial.println(linkyData.ADIR3);
		Serial.print(F("[LINKY] linkyData.IMAX     => ")); Serial.println(linkyData.IMAX);
		Serial.print(F("[LINKY] linkyData.IMAX1    => ")); Serial.println(linkyData.IMAX1);
		Serial.print(F("[LINKY] linkyData.IMAX2    => ")); Serial.println(linkyData.IMAX2);
		Serial.print(F("[LINKY] linkyData.IMAX3    => ")); Serial.println(linkyData.IMAX3);
		Serial.print(F("[LINKY] linkyData.PMAX     => ")); Serial.println(linkyData.PMAX);
		Serial.print(F("[LINKY] linkyData.PAPP     => ")); Serial.println(linkyData.PAPP);
		Serial.print(F("[LINKY] linkyData.HHPHC    => ")); Serial.println(linkyData.HHPHC);
		Serial.print(F("[LINKY] linkyData.MOTDETA  => ")); Serial.println(linkyData.MOTDETAT);
		Serial.print(F("[LINKY] linkyData.PPOT     => ")); Serial.println(linkyData.PPOT);
		Serial.println();
		Serial.println();
	}
#endif

unsigned long lastLog = 0;
time_t  _filenameToDate(const char *filename) {
    struct tm tm = {0};
    char datePart[5] = {0};
    strncpy(datePart, filename, 4);
    tm.tm_year = atoi(datePart) - 1900;
    strncpy(datePart, filename + 4, 2);
    datePart[2] = '\0';
    tm.tm_mon = atoi(datePart) - 1;
    strncpy(datePart, filename + 6, 2);
    tm.tm_mday = atoi(datePart);
    return _timegm(&tm) / 86400 * 86400;
}
time_t _timegm(struct tm *tm) {
    struct tm start2000 = { 0,  0,  0, 1,  0,  100, 0, 0, 0,  };
    return mktime(tm) - (mktime(&start2000) - 946684800);
}
void clearingOldLogFiles() {
    const uint8_t dirLen = strlen(LOGGING_FOLDER);
    Dir tempDir = SPIFFS.openDir(LOGGING_FOLDER);

    while (tempDir.next())
    {
        const char *dateStart = tempDir.fileName().c_str() + dirLen + 1;
        const time_t logdate = _filenameToDate(dateStart);
        const time_t today = time(nullptr) / 86400 * 86400; // remove the time part

        if (logdate < (today - LOGGING_DAY_KEPT * 86400)) {
        	Serial.print("[DB] Removing old log file: ");
        	Serial.println(tempDir.fileName());
            SPIFFS.remove(tempDir.fileName());
        }
        #ifdef DEBUG
	        else {
	        	Serial.printf("[DB] %d / %d / %s / Log file: ",today,logdate,dateStart);
	        	Serial.println(tempDir.fileName());
	        }
        #endif
    }
}

void loop() {
	ArduinoOTA.handle();
	if( mLinky.updateAsync(PINS_LED_STATUS_LINK,true) ) {
		Serial.println("[LINKY] Got frame");
	}

	if(((lastRequestCounter + 30000) < millis()) && shouldReEnableListingToLinky){
		shouldReEnableListingToLinky = false;
		digitalWrite(PINS_CTRL_DATA_IN,false);
		Serial.println("[LINKY] Restoring connection");
	} 
	
	#ifdef DEBUG
		LinkyData data = mLinky.grab();
		dumpStruct(data);
	#endif

	if(digitalRead(PINS_CTRL_DATA_IN) == false) {
		if (millis() - lastLog > LOGGING_INTERVAL) {
			mLinky.waitEndMessage();
			digitalWrite(PINS_CTRL_DATA_IN,true);
			Serial.println("[DB] Proccesing values");

			LinkyData linkyData = mLinky.grab();
			struct LoggingData data;

			if(mLinky.lastFullFrame() > 10000) {
				data.PAPP = 0;
			} else {
				data.PAPP = linkyData.PAPP;
			}

			logger.write(data);
			lastLog = millis();

			clearingOldLogFiles();
			logger.process();

			digitalWrite(PINS_CTRL_DATA_IN,false);
			mLinky.waitEndMessage();
			Serial.println("[DB] Done");
		}
	}
}

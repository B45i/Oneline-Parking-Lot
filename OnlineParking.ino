#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <FirebaseArduino.h>
#include <ArduinoJson.h> //From FirebaseArduino
#include <FS.h>
#include <NewPing.h>

#define FIREBASE_HOST "your-firebase-host"
#define FIREBASE_AUTH "your-auth-key"
#define MAX_DISTANCE 400

bool shouldSaveConfig 	= false;
char NodeID[64]		  	= ""; //NodeID 

const int trigPin1    	= D0;
const int echoPin1    	= D1;
const int trigPin2    	= D2;
const int echoPin2    	= D3;
const int trigPin3    	= D4;
const int echoPin3    	= D5;

const int trigDistance  = 100; // 1 meter

NewPing sonarOne(trigPin1,   echoPin1, MAX_DISTANCE);
NewPing sonarTwo(trigPin2,   echoPin2, MAX_DISTANCE);
NewPing sonarThree(trigPin3, echoPin3, MAX_DISTANCE);



void saveConfigCallback () {
	Serial.println("Should save config");
	shouldSaveConfig = true;
}


void loadSavedConfig() {
	Serial.println("mounting FS...");
	if (SPIFFS.begin()) {
		Serial.println("mounted file system");
		if (SPIFFS.exists("/config.json")) {
			//file exists, reading and loading
			Serial.println("reading config file");
			File configFile = SPIFFS.open("/config.json", "r");
			if (configFile) {
				Serial.println("opened config file");
				size_t size = configFile.size();
				// Allocate a buffer to store contents of the file.
				std::unique_ptr<char[]> buf(new char[size]);

				configFile.readBytes(buf.get(), size);
				DynamicJsonBuffer jsonBuffer;
				JsonObject& json = jsonBuffer.parseObject(buf.get());
				json.printTo(Serial);
				if (json.success()) {
					Serial.println("\nparsed json");
					strcpy(NodeID, json["NodeID"]);
				}
				else {
					Serial.println("failed to load json config");
				}
			}
		}
	}
	else {
		Serial.println("failed to mount FS");
	}
}


void setupConnection() {
	// id/name placeholder/prompt default length
	WiFiManagerParameter custom_NodeID("NodeID", "NodeID", NodeID, 64);
	WiFiManager wifiManager;
	wifiManager.setSaveConfigCallback(saveConfigCallback);
	wifiManager.addParameter(&custom_NodeID);
	wifiManager.autoConnect("IoT Parking Lot");
	// default IP: 192.168.4.1
	Serial.println("connected...");

	strcpy(NodeID, custom_NodeID.getValue());

	// save  to FS
	if (shouldSaveConfig) {
		Serial.println("saving config");
		DynamicJsonBuffer jsonBuffer;
		JsonObject& json = jsonBuffer.createObject();
		json["NodeID"] = NodeID;

		File configFile = SPIFFS.open("/config.json", "w");
		if (!configFile) {
			Serial.println("failed to open config file for writing");
		}

		json.printTo(Serial);
		json.printTo(configFile);
		configFile.close();
	}

	Serial.print("local ip");
	Serial.println(WiFi.localIP());

	Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
}


void sendToFireBase(bool parked, String path) {
	Firebase.setBool(path, parked);
	if (Firebase.failed()) {
		Serial.print("setting value failed:");
		Serial.println(Firebase.error());  
		return;
	}
}


void checkParkingData(int distance, String path) {
	String fullPath = NodeID + path;
	sendToFireBase(distance < trigDistance, fullPath);
}


void setup() {
	Serial.begin(9600);
	//SPIFFS.format();
	loadSavedConfig();
	setupConnection();
}


void loop() {
	//Serial.println(NodeID);

	//delay(29);
	int distanceOne     = sonarOne.ping_cm();
	int distanceTwo  	= sonarTwo.ping_cm();
	int distanceThree   = sonarThree.ping_cm();

	checkParkingData(distanceOne, "lot_1");
	checkParkingData(distanceTwo, "lot_2"); 
	checkParkingData(distanceThree, "lot_3");

}


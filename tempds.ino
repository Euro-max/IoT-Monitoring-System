
#if defined(ESP32)
#include <WiFi.h>
#include <DHT.h>;
#include <Wire.h>
#include <DHT22.h>
#include <DHT22.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif

#include "time.h"

#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
//Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

#define WIFI_SSID "#####"
#define WIFI_PASSWORD "#######"

// Provide the token generation process info.
#include <addons/TokenHelper.h>

// Provide the RTDB payload printing info and other helper functions.
#include <addons/RTDBHelper.h>


#include <OneWire.h>
#include <DallasTemperature.h>

#include <Firebase_ESP_Client.h>
/* 2. Define the API Key */
#define API_KEY "API KEY"

/* 3. Define the RTDB URL */
#define DATABASE_URL "capstone-323-cf62c-default-rtdb.europe-west1.firebasedatabase.app" //<databaseName>.firebaseio.com or <databaseName>.<region>.firebasedatabase.app

/* 4. Define the user Email and password that alreadey registerd or added in your project */
#define USER_EMAIL "#####@gmail.com"
#define USER_PASSWORD "######"



// Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
unsigned long timerDelay = 10000;

unsigned long getTime() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return(0);
  }
  time(&now);
  return now;
}


#define SENSOR_PIN  27 // ESP32 pin GIOP21 connected to DS18B20 sensor's DQ pin
#define AOUT_PIN 35 // ESP32 pin GIOP36 (ADC0) that connects to AOUT pin of moisture sensor
#define DHTPIN 33     // what pin we're connected to
#define DHTTYPE DHT22   // DHT 22
DHT dht(DHTPIN, DHTTYPE); //// Initialize DHT sensor for normal 16mhz Arduino



OneWire oneWire(SENSOR_PIN);
DallasTemperature DS18B20(&oneWire);

float tempC; // temperature in Celsius
float tempF;
 // temperature in Fahrenheit
 int t;
String timePath = "/timestamp";
String databasePath = "/readings";
String parentPath;

int timestamp;
FirebaseJson json;

const char* ntpServer = "pool.ntp.org";

void setup() {
  Serial.begin(115200); // initialize serial
  DS18B20.begin();    // initialize the DS18B20 sensor
  pinMode(AOUT_PIN, INPUT);
  pinMode(25, INPUT);
   Serial.begin(115200);
   
    dht.begin();

  configTime(0, 0, ntpServer);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the user sign in credentials */
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h

#if defined(ESP8266)
  // In ESP8266 required for BearSSL rx/tx buffer for large data handle, increase Rx size as needed.
  fbdo.setBSSLBufferSize(2048 /* Rx buffer size in bytes from 512 - 16384 */, 2048 /* Tx buffer size in bytes from 512 - 16384 */);
#endif

  // Limit the size of response payload to be collected in FirebaseData
  fbdo.setResponseSize(2048);

  Firebase.begin(&config, &auth);

  // Comment or pass false value when WiFi reconnection will control by your code or third party library
  Firebase.reconnectWiFi(true);

  Firebase.setDoubleDigits(5);

  config.timeout.serverResponse = 10 * 1000;

}


void loop() {
      
  DS18B20.requestTemperatures();       // send the command to get temperatures
  tempC = DS18B20.getTempCByIndex(0);  // read temperature in °C
  tempF = tempC * 9 / 5 + 32; // convert °C to °F
  t = dht.readTemperature();

  Serial.print("Temperature: ");
  Serial.print(tempC);    // print the temperature in °C
  Serial.print("C");
  Serial.print("  ~  ");  // separator between °C and °F
  Serial.print(tempF);    // print the temperature in °F
  Serial.println("F");
 Serial.print("  Air Temp: ");
    Serial.print(t);
    Serial.println("  Celsius");  

  delay(100);
  int value = analogRead(AOUT_PIN); // read the analog value from sensor
  //int moisture percentage = (100 -(value/1023.00) * 100 );
 // float moisture = ((value-205) *100) /800;
 float moisture = map(value,170,208,0,100);
    
  Serial.print("Moisture value: ");
  Serial.println (moisture);
 

  delay(100);
   int reading  = analogRead(34);
  float air_temp = (float(reading) * (3.3/4096))*100 + 9.21 ; 
  Serial.print("air temprature: ");
  Serial.println(air_temp);
  //Serial.println(reading);
  delay(500) ;
      
// connect to firebase
if (Firebase.ready() && (millis() - sendDataPrevMillis > timerDelay || sendDataPrevMillis == 0))
  {
    sendDataPrevMillis = millis();


    timestamp = getTime();
    Serial.print ("time: ");
    Serial.println (timestamp);

    parentPath= databasePath + "/" + String(timestamp);

    json.set("/SoilTemperature", float(tempC));
    json.set("/Temperature F", float(tempF));
    json.set("/Moisture", float(moisture));
    json.set("/AirTemperature", int(t)); 
    json.set("/Timestamp", int(timestamp));   
    Serial.printf("Set json... %s\n", Firebase.RTDB.setJSON(&fbdo, parentPath.c_str(), &json) ? "ok" : fbdo.errorReason().c_str());
  }

  
    Serial.println();


    // For generic set/get functions.

    // For generic set, use Firebase.RTDB.set(&fbdo, <path>, <any variable or value>)

    // For generic get, use Firebase.RTDB.get(&fbdo, <path>).
    // And check its type with fbdo.dataType() or fbdo.dataTypeEnum() and
    // cast the value from it e.g. fbdo.to<int>(), fbdo.to<std::string>().

    // The function, fbdo.dataType() returns types String e.g. string, boolean,
    // int, float, double, json, array, blob, file and null.

    // The function, fbdo.dataTypeEnum() returns type enum (number) e.g. fb_esp_rtdb_data_type_null (1),
    // fb_esp_rtdb_data_type_integer, fb_esp_rtdb_data_type_float, fb_esp_rtdb_data_type_double,
    // fb_esp_rtdb_data_type_boolean, fb_esp_rtdb_data_type_string, fb_esp_rtdb_data_type_json,
    // fb_esp_rtdb_data_type_array, fb_esp_rtdb_data_type_blob, and fb_esp_rtdb_data_type_file (10)

  

  delay(500);
}

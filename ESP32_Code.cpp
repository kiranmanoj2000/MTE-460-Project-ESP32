/*
OLED Screen Display Setup and Firebase connection based on code originally written by Rui Santos.
OLED: https://randomnerdtutorials.com/esp32-built-in-oled-ssd1306/
FIREBASE: https://randomnerdtutorials.com/esp32-firebase-realtime-database/
*/

#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

//Provide the token generation process info.
#include "addons/TokenHelper.h"
//Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

// Insert your network credentials
#define WIFI_SSID "*********"
#define WIFI_PASSWORD "***********"


// Insert Firebase project API Key
#define API_KEY "***************"

// Insert RTDB URLefine the RTDB URL */
#define DATABASE_URL "**********" 

// Insert Authorized Email and Corresponding Password
#define USER_EMAIL "***********"
#define USER_PASSWORD "***********"

//Define Firebase Data object
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Variable to save USER UID
String uid;

unsigned long sendDataPrevMillis = 0;

//available pins
//16, , 13 //input

//12 and 14 are the transmitter LEDs
//15 Alert LED
//OLED Pins
//4,5
//set pins 
const int IRT_1 = 12;
const int IRT_2 = 14;

const int IRR_1 = 16; //receivers are default high
const int IRR_2 = 13;
const int convID = random(1,1000);

//will use OLED screen and 1 LED (15 to 
const int LED1 = 15;
//const int LED2 = 8;

int statusR1 = 0;
int statusR2 = 0;

unsigned long numFallen = 0;

const int freq = 37900;
const int resolution = 1;
const int ledChannel = 0; //16 different channels
bool preState = false;

void setup() {

  Wire.begin(5,4);
  display.begin(SSD1306_SWITCHCAPVCC,0x3C);
  display.clearDisplay();
  delay(1000);

  //start the display 
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,10);
  display.println("Welcome! Beginning MTE460 Program Setup.");
  display.display();
  delay(2000);

  ledcSetup(ledChannel, freq, resolution);

  ledcAttachPin(IRT_1, ledChannel);
  ledcAttachPin(IRT_2, ledChannel);

  pinMode(LED1,OUTPUT);
  pinMode(IRR_1,INPUT);
  //pinMode(IRR_2,INPUT);

  ledcWrite(ledChannel, 1);

  display.print("IR Transmitter/Receiver Pair Setup Complete.");
  display.print("Connecting to Internet");
  display.display();
  delay(500);
  Serial.begin(115200);

  //set up http connection
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to Wi-Fi");
    while (WiFi.status() != WL_CONNECTED){
      Serial.print(".");
      display.println(".");
      display.display();
      delay(300);
    }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  display.print("Connected with IP: ");
  display.println(WiFi.localIP());
  display.display();
  delay(50);

// Assign the api key (required)
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  // Assign the user sign in credentials
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  Firebase.reconnectWiFi(true);
  fbdo.setResponseSize(4096);

  // Assign the callback function for the long running token generation task
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h

  // Assign the maximum retry of token generation
  config.max_token_generation_retry = 5;

  // Initialize the library with the Firebase authen and config
  Firebase.begin(&config, &auth);

  display.clearDisplay();
//start the display 
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,10);
  display.println("Connected to Database");
  display.display();

 // Getting the user UID might take a few seconds
  Serial.println("Getting User UID");
  display.println("Getting User UID");
  while ((auth.token.uid) == "") {
    Serial.print('.');
    Serial.println('.');
    delay(1000);
  }
  // Print user UID
  uid = auth.token.uid.c_str();
  Serial.print("User UID: ");
  Serial.print(uid);
  display.print("User UID: ");
  display.println(uid);

  config.token_status_callback = tokenStatusCallback;
  Firebase.begin(&config, &auth);

  display.print("Setup Compete!");
  delay(1000);

  display.clearDisplay();
  //start the display 
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,17);
  display.print("ConveyorID: ");
  display.println(convID);
  display.print("Number Boxes Fallen: ");
  display.println(numFallen);
  display.display();
}  

void loop() {
  
  if (Firebase.isTokenExpired()){
        Firebase.refreshToken(&config);
        Serial.println("Refresh token");
  }

  if (Firebase.ready() && uid != "" ){//&& (millis() - sendDataPrevMillis > 15000 || sendDataPrevMillis == 0)){
    sendDataPrevMillis = millis();

    //check receiver status
    if(digitalRead(IRR_1) == HIGH){
      digitalWrite(LED1,LOW); //turn off LED
      //check previous state, if not detecting anything, don't update the display (saves processing time)
      if(preState){
        display.clearDisplay();
        delay(20);

        //start the display 
        display.setTextSize(1);
        display.setTextColor(WHITE);
        display.setCursor(0,17);
        display.print("ConveyorID: ");
        display.println(convID);
        display.print("Number Boxes Fallen: ");
        display.println(numFallen);
        display.display();
      }
      preState = false;
    }
    else{
      digitalWrite(LED1,HIGH); //turn on LED
      numFallen++; //increment internal box count
      //update OLED display
      display.clearDisplay();
      delay(20);
      //start the display 
      display.setTextSize(1);
      display.setTextColor(WHITE);
      display.setCursor(0,17);
      display.print("ConveyorID: ");
      display.println(convID);
      display.print("Number Boxes Fallen: ");
      display.println(numFallen);
      display.display();

      FirebaseJson jsonMain;
      jsonMain.add("conveyorBeltId", "-NFpSRmV9d0j3sHEA-hk");
      jsonMain.add("companyId", "-NFpRzghQ2GAVj8IleOF");
      FirebaseJson jsonTime;
      jsonTime.add(".sv", "timestamp");
      jsonMain.add("createdDate", jsonTime);
      
      // send it
      Serial.printf("Push json... %s\n", Firebase.RTDB.pushJSON(&fbdo, "/fallenPackageEvents", &jsonMain) ? "ok" : fbdo.errorReason().c_str());
      //set state variables
      preState = true;
      delay(500);
    }
  }
}

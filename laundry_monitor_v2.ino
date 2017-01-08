/*
 * General stratagy:
 * Feather uses MQTT PubSubClient to publish change of modes while Node-red handles notification (email etc).
 * to raspberry pi running MQTT.
 * https://learn.adafruit.com/diy-esp8266-home-security-with-lua-and-mqtt/configuring-mqtt-on-the-raspberry-pi
 * 
 * Node-red subscribes and publishes to MQTT server.
 * Node-red can do other IoT things like email or twitter.
 * 
 * >> I am running them in a Rasberry PI so they are almost always on 
 * http://nodered.org/docs/hardware/raspberrypi.html
 * 
 * Adafruit Feather Huzzah ESP8266
 * https://learn.adafruit.com/adafruit-feather-huzzah-esp8266/overview
 * #1 (washer)LIS3DH I2C Address default 0x18
 * https://learn.adafruit.com/adafruit-lis3dh-triple-axis-accelerometer-breakout/overview
 * #2 (dryer) LIS3DH > Wiring 3.3v to SDO pin to change address to 0x19
 * SDO - When in I2C mode, this pin can be used for address selection. 
 *   --> When connected to GND or left open, the address is 0x18
 *   --> it can also be connected to 3.3V to set the address to 0x19 <--
 * Wire the following from the Feather {3v, GND, SLC, SLA} to both LIS3DH modules in parallel.
 * 
 * I used doublesided tap to attach magnets to the LIS3DH sensors making this a completely
 * portable and non-intrusive project.
 * 
 * NOTE: This code is configured for I2C only.
 * 
 */

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_LIS3DH.h>

// Used for I2C
Adafruit_LIS3DH lis[2] = Adafruit_LIS3DH();
#define CLICKTHRESHHOLD 80

/*
 * Variables for Laundry
 */
int applianceMode[2] = {0,0}; // defined below
int ledPin = 0;
// Connect to the WiFi
const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";
const char* mqtt_server ="YOUR_MQTT_SERVER";
String myHostname = "Laundry";

// Callback function header
void callback(char* topic, byte* payload, unsigned int length);

WiFiClient espClient;
PubSubClient client(espClient);
String replyString;
/*
 * These two sections handles requests from MQTT
 * 
 * Publish "status" to "laundry" returns "ok"
 * Publish "washer" or "dryer" to "laundry" and this publishes one of the following
 * values to the "laundryStatus" topic.
 * 
 * 0 = startup
 * 1 = ready
 * 2 = startup period (this may go back to 1)
 * 3 = steady detection
 * 4 = finished
 *
 * Mode changes are published to "dryerStatus" or "washerStatus" topics
 * and will respond with the same codes as above.
 * 
 */
void replyBack(String replyString, String replyTopic = "laundryStatus") {
      char myBuf[replyString.length()+1];
      replyString.toCharArray(myBuf, replyString.length()+1);
      char myBuf2[replyTopic.length()+1];
      replyTopic.toCharArray(myBuf2, replyTopic.length()+1);
      client.publish(myBuf2,myBuf);
}
void callback(char* topic, byte* payload, unsigned int length) {
 Serial.print("Message arrived [");
 Serial.print(topic);
 Serial.print("] ");
 String receivedWord = String("");
 for (int i=0;i<length;i++) {
  char receivedChar = (char)payload[i];
  receivedWord.concat(receivedChar);
 }
  Serial.print(receivedWord);
  Serial.println();

  if(String(topic) == String("laundry")) {
  // reply to a Status check / use as heartbeat for integration with node-red or similar.
    if (receivedWord == "status") {
      // applianceMode[0]
      replyString = String("ok");
      replyBack(replyString);
    }
    if (receivedWord == "washer") {
      replyString = String(applianceMode[0]);
      replyBack(replyString);
    }
    if (receivedWord == "dryer") {
      replyString = String(applianceMode[1]);
      replyBack(replyString);
    }
  }
}
void reconnect() {
 // Loop until we're reconnected
 String clientId = "ESP8266Client-";
// clientId += String(random(0xffff), HEX);
 clientId += String(WiFi.localIP());

 while (!client.connected()) {
 Serial.print("Attempting MQTT connection...");
 // Attempt to connect
 // Each MQTT Client needs to be unique!
 if (client.connect(clientId.c_str())) {
  Serial.println("connected");
  // ... and subscribe to topic
  client.subscribe("laundry");
 } else {
  Serial.print("failed, rc=");
  Serial.print(client.state());
  Serial.println(" try again in 5 seconds");
  // Wait 5 seconds before retrying
  delay(5000);
  }
 }
}

int previousApplianceMode[2] = {0,0};
bool applianceTap[2];
bool tapDetected[2] = {false, false};
int tapDetectionThreshold = 5000; // 5 seconds
unsigned long startupTimer[2];
unsigned long tapTimer[4];
unsigned long modeTimer[2];

// function for tapDetection.
bool sensorTap(uint8_t click) {
  if(click & 0x10 || click & 0x20) {
    return true;
    } else {
    return false;
    }
}

void applianceMonitor(int i) {
// Detect taps
   uint8_t click = lis[i].getClick();
   applianceTap[i] = sensorTap(click);
//
if(applianceTap[i] == true) {
  tapTimer[i] = millis();
} else {
  tapTimer[i+2] = millis() - tapTimer[i];
}
/*
 * tap detection
 */
tapDetected[i] = (tapTimer[i+2] < tapDetectionThreshold);
/*
 * Main part of code that makes it easy to swich from modes to modes.
 */
switch(applianceMode[i]) {
  case 0: // startup /first run | unknown mode switch to 1.
  applianceMode[i] = 1;
  previousApplianceMode[i] = 1;
  break;
  case 1:
  if(tapDetected[i]) {
    startupTimer[i] = millis();
    applianceMode[i] = 2;
  }
  break;
  case 2:
  if(tapDetected[i]) {
    if ((millis() - modeTimer[i]) > 30000) {
      applianceMode[i] = 3;
    }
  } else {
    applianceMode[i] = 1;
    break;
  }
  break;
  case 3:
    if(!tapDetected[i]) {
    if ((millis() - modeTimer[i]) > 30000) {
      applianceMode[i] = 4;
    }
  }
  break;
  case 4:
  /*
   * Send out notifications and switch to mode 1
   */
  if(tapDetected[i]) {
    if((millis() - modeTimer[i]) > 1000) {
      applianceMode[i] = 1;
    }
  }
  break;
}
/*
 * If our current mode is different then our last mode
 * then notify MQTT.
 */
if(applianceMode[i] != previousApplianceMode[i]) {
  previousApplianceMode[i] = applianceMode[i];
  Serial.print("New Mode = ");
  Serial.println(previousApplianceMode[i]);
  String replyTopic = "washerStatus";
  if(i == 1) {
    replyTopic = "dryerStatus";
  }
  replyString = String(applianceMode[i]);
  replyBack(replyString, replyTopic);
  modeTimer[i] = millis();
}
}

void setup()
{
  pinMode(ledPin,OUTPUT); //blink to indicate power is on.
  
// startup WiFi
 WiFi.hostname(myHostname); // I like my device to have descriptive host names.
 Serial.begin(115200);
 delay(10);
 /*
  * Startup WiFi and output the IP Address when connected.
  * 
  */
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

// Initial MQTT startup
 client.setServer(mqtt_server, 1883);
 client.setCallback(callback);

// Setup LIS3DH sensors.
  if (! lis[0].begin(0x18)) {   // change this to 0x19 for alternative i2c address
    Serial.println("Could not start washer sensor");
    while (1);
  }
  Serial.println("LIS3DH found at 0x18");
  lis[0].setRange(LIS3DH_RANGE_2_G);   // 2, 4, 8 or 16 G!
  lis[0].setClick(2, CLICKTHRESHHOLD);
  delay(100);
  
  if (! lis[1].begin(0x19)) {   // change this to 0x19 for alternative i2c address
    Serial.println("Could not start dryer sensor");
    while (1);
  }
  Serial.println("LIS3DH found at 0x19");
  lis[1].setRange(LIS3DH_RANGE_2_G);   // 2, 4, 8 or 16 G!
  lis[1].setClick(2, CLICKTHRESHHOLD);
  delay(100);
}

void loop()
{
// Startup MQTT Client and try to keep connected.
// if not connected to MQTT there is no reason to run any other code ;-)
 if (!client.connected()) {
  reconnect();
 } 
  else
 {
  // call the main code that determines what is going on with the washer/dryer
  applianceMonitor(0);  // Washer
  applianceMonitor(1);  // Dryer
  /* 
   * Blink the led every once in a while. 
   * --> Led blinking could be placed elsewhere.
   */
  //
  digitalWrite(ledPin,LOW);
  delay(250);
  digitalWrite(ledPin,HIGH);
  delay(1000);
}
 client.loop();
}


 */

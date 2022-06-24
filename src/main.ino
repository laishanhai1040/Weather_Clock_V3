#include <Arduino.h>
#include <TimeLib.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <TFT_eSPI.h>

#include <WiFiManager.h>
#include <PubSubClient.h>
#include "setting.h"

TFT_eSPI tft = TFT_eSPI();
bool BLChangeFlage = false;
bool PMSChangeFlage = false;
bool MESSChangeFlage = false;
int BackLightValue = 128;

WiFiUDP Udp;
unsigned int localPort = 8888;
time_t prevDisplay = 0;

const int NTP_PACKET_SIZE = 48;
byte packetBuffer[NTP_PACKET_SIZE];

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
ESP8266WebServer esp8266_server(80);

String topicString;
byte receiveVar;
byte printVar;

int receivePM10, receivePM25, receivePM100;
int receiveTEMP, receiveHUMI, receiveBackLight;
String receiveMessage1;


void setup() {
  Serial.begin(9600);

  pinMode(D2, OUTPUT);
  analogWrite(D2, 50);

  tft.begin();
  tft.setRotation(1);

  tft.fillScreen(TFT_BLACK);

  WiFiManager wifiManager;
  wifiManager.autoConnect("AutoConnectAP");
  Serial.print("WiFi Connected!");
  //tft.setTextColor(TFT_BLUE);
  //tft.drawString("WiFi Connected!", 0,30,4);
  tft.setTextColor(TFT_BLUE);
  //tft.drawString("WiFi SSID:", 0,160,4);
  tft.drawString(WiFi.SSID(),0,190,4);

  Serial.println('\n');
  Serial.print("Connected to ");
  Serial.println(WiFi.SSID());
  Serial.print("IP address:\t");
  tft.setTextColor(TFT_YELLOW);
  Serial.println(WiFi.localIP());
  String ip01 = WiFi.localIP().toString();
  tft.drawString(ip01,0,220,4);

  ntpInit();

  mqttClient.setServer(mqttServer, 1883);
  mqttClient.setCallback(receiveCallback);
  connectMQTTserver();
}

void loop() {
  if (timeStatus() != timeNotSet) {
    if (now() != prevDisplay) {
      prevDisplay = now();
      displayTime();
    }
  }

  if (mqttClient.connected()) {
    mqttClient.loop();
  } else {
    connectMQTTserver();
  }

  if (BLChangeFlage == true) {
    BackLightValue = map(receiveBackLight, 0,10, 0,255);
    Serial.print("BackLightValue=" );
    Serial.println(BackLightValue);
    BLChangeFlage = false;
  }

  analogWrite(D2, BackLightValue);

  if (MESSChangeFlage) {
    tft.drawRect(0,120,320,50,TFT_BLACK);
    tft.setTextColor(TFT_YELLOW,TFT_BLACK);
    tft.drawString(receiveMessage1, 0,120,4);
    MESSChangeFlage = false;
  }

  if (PMSChangeFlage) {
    PrintPMS();
    PMSChangeFlage = false;
  }
}


void PrintPMS() {
  tft.drawRect(190,160,130,80,TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("PM2.5: ", 190,160,4);
  tft.drawNumber(receivePM25, 280,160,4);
  tft.drawString("TEMP: ", 190,190,4);
  tft.drawNumber(receiveTEMP, 280,190,4);
  tft.drawString("HUMI: ", 190,220,4);
  tft.drawNumber(receiveHUMI, 280,220,4);
}

void ntpInit() {
  Serial.println("Starting UDP");
  Udp.begin(localPort);
  Serial.print("Local UDP port: ");
  Serial.println(Udp.localPort());
  Serial.println("---------------");
  setSyncProvider(getNtpTime);
  setSyncInterval(timeCheckInterval);
}

time_t getNtpTime() {
  IPAddress ntpServerIP;

  while (Udp.parsePacket() > 0);
  Serial.println("Transmit NTP Request");
  WiFi.hostByName(ntpServerName, ntpServerIP);
  Serial.print(ntpServerName);
  Serial.print(": ");
  Serial.println(ntpServerIP);
  sendNTPpacket(ntpServerIP);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Serial.println("Receive NTP response");
      Udp.read(packetBuffer, NTP_PACKET_SIZE);
      unsigned long secsSince1900;

      secsSince1900 = (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  Serial.println("No NTP Response :-(");
  return 0;
}

void sendNTPpacket(IPAddress &address) {

  memset(packetBuffer, 0, NTP_PACKET_SIZE);


  packetBuffer[0] = 0b11100011;
  packetBuffer[1] = 0;
  packetBuffer[2] = 6;
  packetBuffer[3] = 0xEC;

  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;

  Udp.beginPacket(address, 123);
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}

void displayTime() {
  String timeString;
  String dateString;
  String yearString;
  String dateTimeString;

  timeString = adjDigit(hour()) + ":" + adjDigit(minute()) + ":" + adjDigit(second());
  dateString = adjDigit(month()) + "-" + adjDigit(day());
  yearString = adjDigit(year());
  dateTimeString = yearString + "-" + dateString;

  tft.setViewport(0,0, BANNER_WIDTH, BANNER_HEIGHT);
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(BANNER_FG, BANNER_BG);
  tft.setTextPadding(10);
  tft.drawString(dateTimeString, 0, 0, 6);
  tft.drawString(timeString, 0, 50, 6);

  String week1 = "";
  String week2 = " ";
  switch (weekday())
  {
  case 1:
      week2 = "Sunday";
    break;
  case 2:
      week2 = "Monday";
    break;
  case 3:
      week2 = "Tuesday";
    break;
  case 4:
      week2 = "Wednesday";
    break;
  case 5:
      week2 = "Thursday";
    break;
  case 6:
      week2 = "Friday";
    break;
  case 7:
      week2 = "Saturday";
    break;
  default:
    week2 = "Wrong";
    break;
  }
  if (week2 != week1) {
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.drawString(week2, 200, 50,4);
    week1 = week2;
  }

  tft.resetViewport();
}

String adjDigit(int number) {
  if(number >= 10) {
    return String(number);
  } else {
    return String("0" + String(number));
  }
}


void receiveCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message Received [");
  Serial.print(topic);
  Serial.print("]: ");
  for (u_int i=0; i<length; i++) {
    Serial.print((char)payload[i]);
    //receiveVar = receiveVar + (char)payload[i];
  }
  Serial.println("");
  Serial.print("Message Length(Bytes) ");
  Serial.println(length);

  payload[length] = '\0';
  String s = String((char*)payload);
  int ii = s.toInt();
  Serial.print("ii=");
  Serial.println(ii);

  String topic2 = topic;
  if (topic2 == PM10) {
    receivePM10 = ii;

  }
  else if (topic2 == PM25) {
    receivePM25 = ii;
    PMSChangeFlage = true;
  }
  else if (topic2 == PM100) {
    receivePM100 = ii;

  }
  else if (topic2 == TEMPTopic) {
    receiveTEMP = ii;
    PMSChangeFlage = true;
  }
  else if (topic2 == HUMITopic) {
    receiveHUMI = ii;
    PMSChangeFlage = true;
  }
  else if (topic2 == BACKLIGHT) {
    receiveBackLight = ii;
    BLChangeFlage = true;
  }
  else if (topic2 == MESS1) {
    receiveMessage1 = s;
    MESSChangeFlage = true;
  }
  
}

void connectMQTTserver() {
  String clientId = mqttClientId;

  if (mqttClient.connect(clientId.c_str(), mqttUserName, mqttPassWord,
                          willTopic, willQos, willRetain, willMsg, cleanSession)) {
    Serial.println("MQTT Server Connected.");
    Serial.println("Server Address: ");
    Serial.println(mqttServer);
    Serial.println("ClientId: ");
    Serial.println(clientId);
    subscribeTopic();
  } else {
    Serial.print("MQTT Server Connect Failed. Client State: ");
    Serial.println(mqttClient.state());
    delay(5000);
  }
}

void subscribeTopic() {
  for(u_int i=0; i<7; i++) {
    switch (i) {
      case 0:
        topicString = PM10;
        break;
      case 1:
        topicString = PM25;
        break;
      case 2:
        topicString = PM100;
        break;
      case 3:
        topicString = TEMPTopic;
        break;
      case 4:
        topicString = HUMITopic;
        break;
      case 5:
        topicString = BACKLIGHT;
        break;
      case 6:
        topicString = MESS1;
        break;
      default:
        topicString = PM10;
    }
    char subTopic[topicString.length()+1];
    strcpy(subTopic, topicString.c_str());

    if(mqttClient.subscribe(subTopic, subQos)) {
      Serial.println("Subscrib Topic: ");
      Serial.println(subTopic);
    } else {
      Serial.print("Subscribe Fail...");
    }
  }
}

#include <Arduino.h>

String reqUserKey = "888888";
String reqLocation = "99999";
static const char ntpServerName[] = "ntp.aliyun.com";
const int timeZone = 8;

const char* mqttServer = "iot.ranye-iot.net";
const char* mqttUserName = "hai1040_3";
const char* mqttPassWord = "245006";
const char* mqttClientId = "hai1040_3_id";

const int subQos = 1;
const bool cleanSession = false;

const char* willTopic = "willTopic";
const char* willMsg = "willMsg";
const int willQos = 0;
const int willRetain = false;

const char* PM10 = "hai1040/PMS5003T/pmat10";
const char* PM25 = "hai1040/PMS5003T/pmat25";
const char* PM100 = "hai1040/PMS5003T/pmat100";
const char* TEMPTopic = "hai1040/PMS5003T/temp";
const char* HUMITopic = "hai1040/PMS5003T/humi";
const char* BACKLIGHT = "hai1040/ILI9341/backlight";
const char* MESS1 = "hai1040/ILI9341/message1";

int timeCheckInterval = 300;


#define BANNER_HEIGHT 150
#define BANNER_WIDTH 320
#define HALF_BANNER_WIDTH 160
#define BANNER_BG TFT_BLACK
#define BANNER_FG TFT_WHITE
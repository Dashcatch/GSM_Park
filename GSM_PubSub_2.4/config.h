#define TINY_GSM_MODEM_SIM800 // Modem is SIM800L
#define TINY_GSM_DEBUG Serial
#define GSM_PIN ""
#define EEPROM_SIZE 1 //Declared EEPROM
#define HEART_BEAT_INTERVAL 31
#define relay 2
#define LedR 18
#define LedG 19
#define LedB 21
#define Node_ID 101
#define Network_ID 254

const char apn[] = "airtelgprs.com";
const char gprsUser[] = "";
const char gprsPass[] = "";
const char* broker = "173.248.136.206";
bool mqttStat;
String gheartBeatMsg;

#define SubTopic "TO_EDGE"
#define PubTopic "TO_SERVER"

#include <Wire.h>
#include <TinyGsmClient.h>
#include <ArduinoJson.h>
#include <EEPROM.h> //EEPROM Declaration

#include <Ticker.h>
Ticker HeartBeatSignal;

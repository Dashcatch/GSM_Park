/* Code with
    EEPROM
    GSM and MQTT Connect
    LED Indication for GSM and MQTT Connect
    Turn On and Off relay module
    Publishing data on receiving.
    Acknowledgement
    Adding healthStatus
*/

#include "config.h"

int BulbState;
int lastRelayState;
int Node;
int Network;
String msg;
int msgID;
String Type;
String relayState;

#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(Serial2, Serial);
TinyGsm modem(debugger);
#else
TinyGsm modem(Serial2);
#endif

#include <PubSubClient.h>

TinyGsmClient client(modem);
PubSubClient mqtt(client); //DECLARING A OBJECT

uint32_t lastReconnectAttempt = 0;
long lastMsg = 0;

void RedLedOn() {
  digitalWrite(LedR, HIGH);
  digitalWrite(LedG, LOW);
  digitalWrite(LedB, LOW);
}

void BlueLedOn() {
  digitalWrite(LedR, LOW);
  digitalWrite(LedG, LOW);
  digitalWrite(LedB, HIGH);
}

void GreenLedOn() {
  digitalWrite(LedG, HIGH);
  digitalWrite(LedB, LOW);
  digitalWrite(LedR, LOW);
}

void mqttCallback(char* topic, byte* message, unsigned int len)
{
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  Serial.println();
  StaticJsonDocument<400> recData;
  Serial.println("Parsing start: ");
  DeserializationError error = deserializeJson(recData, message);

  // {"network_add":0,"node_add":1,"msg_id":1,"msg_content":"m:ON"}

  Node = recData["node_add"]; //Address Id of GSM : device id-1
  Network = recData["network_add"]; //network address to compare
  msg = (const char*) recData["msg_content"]; // on off
  msgID = recData["msg_id"]; //Message ID for Ack
  Type = msg.substring(0, 1);
  relayState = msg.substring(2);
  Serial.print("Relay State: ");
  Serial.println(relayState);

  //desrial data {"network_add":255,"node_add":1,"msg_id"/:5,"msg_content":"a:ack"}

  StaticJsonDocument<300> traData;

  traData["node_add"] = Node;
  traData["network_add"] = Network;
  traData["msg_id"] = msgID;
  traData["msg_content"] = "a:ACK";

  char TransMsg[500];
  serializeJson(traData, TransMsg);

  if (Network == Network_ID)
  {
    if (Node == Node_ID)
    {
      if (strcmp(relayState.c_str(), "OFF") == 0)
      {
        BulbState = 0;
        digitalWrite(relay, LOW);
        Serial.println("Turn off relay");
        EEPROM.write(0, BulbState);
        EEPROM.commit();
        mqtt.publish(PubTopic, TransMsg);
        Serial.print("Ack Sent: ");
        Serial.println(TransMsg);
      }
      else if (strcmp(relayState.c_str(), "ON") == 0)
      {
        BulbState = 1;
        digitalWrite(relay, HIGH);
        Serial.println("Turn on relay");
        EEPROM.write(0, BulbState);
        EEPROM.commit();
        mqtt.publish(PubTopic, TransMsg);
        Serial.print("Ack Sent: ");
        Serial.println(TransMsg);
      }
    }
  }
}

String getHeartBeatMessage() {
  StaticJsonDocument<300> heartdata;

  heartdata["node_add"] = Node;
  heartdata["network_add"] = Network;
  heartdata["msg_id"] = 0;
  heartdata["msg_content"] = "h:200";

  char heartBeatMsg[500];
  serializeJson(heartdata, heartBeatMsg);

  return heartBeatMsg;
}

void heartBeatSignal() {
  gheartBeatMsg = getHeartBeatMessage();
  mqttStat = true;
  //mqtt.publish(PubTopic, gheartBeatMsg.c_str());
  Serial.print(gheartBeatMsg);
}

boolean mqttConnect() {
  Serial.print("Connecting to ");
  Serial.print(broker);
  boolean status = mqtt.connect("GsmClientPar");

  if (status == false)
  {
    Serial.println(" fail");
    return false;
  }

  Serial.println(" success");
  GreenLedOn();
  mqtt.subscribe(SubTopic);
  return mqtt.connected();
}

void setup() {
  Serial.begin(9600);
  delay(10);
  pinMode(relay, OUTPUT);
  pinMode(LedR, OUTPUT);
  pinMode(LedG, OUTPUT);
  pinMode(LedB, OUTPUT);
  RedLedOn();
  digitalWrite(relay, LOW);
  Serial.println("Wait...");
  Serial2.begin(9600);
  delay(6000);

  EEPROM.begin(EEPROM_SIZE);
  lastRelayState = EEPROM.read(0);
  digitalWrite(relay, lastRelayState);

  Serial.println("Initializing modem...");
  modem.init();
  RedLedOn();

  Serial.println("Connecting to APN: ");
  Serial.println("Waiting for network...");

  if (!modem.waitForNetwork())
  {
    Serial.println("Connection failed");
    delay(10000);
    return;
  }

  if (modem.isNetworkConnected())
  {
    Serial.print("Network connected to : ");
    Serial.println(apn);
  }

  if (!modem.gprsConnect(apn))
  {
    Serial.println("GPRS connection failed");
    ESP.restart();
    RedLedOn();
  }
  else {
    Serial.println("GPRS connection success ");
  }

  if (modem.isGprsConnected())
  {
    BlueLedOn();
  }

  mqtt.setServer(broker, 1883);
  mqtt.setCallback(mqttCallback);
  HeartBeatSignal.attach(HEART_BEAT_INTERVAL, heartBeatSignal);
}

void loop() {
  if (!modem.isNetworkConnected())
  {
    Serial2.println("Network disconnected");
    if (!modem.waitForNetwork(180000L, true))
    {
      Serial2.println(" fail");
      delay(10000);
      return;
    }
    if (modem.isNetworkConnected())
    {
      Serial2.println("Network re-connected");
    }

#if TINY_GSM_USE_GPRS
    if (!modem.isGprsConnected())
    {
      Serial2.println("GPRS disconnected!");
      Serial2.print(F("Connecting to "));
      Serial2.println(apn);
      if (!modem.gprsConnect(apn))
      {
        Serial2.println(" Fail");
        delay(10000);
        return;
      }
      if (modem.isGprsConnected())
      {
        Serial.println("GPRS reconnected");
      }
    }
#endif
  }

  if (!mqtt.connected())
  {
    BlueLedOn();
    Serial.println(" ***** MQTT NOT CONNECTED ***** ");
    uint32_t t = millis();
    if (t - lastReconnectAttempt > 10000L)
    {
      lastReconnectAttempt = t;
      if (mqttConnect())
      {
        lastReconnectAttempt = 0;
      }
    }
    delay(100);
    return;
  }
  if (mqttStat == true)
  {
    mqttStat = false;
    mqtt.publish(PubTopic, gheartBeatMsg.c_str());
  }
  mqtt.loop();
  delay(1000);
}

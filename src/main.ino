#include <MKRNB.h>
#include <Wire.h>
#include <SPI.h>
#include "arduino_secrets.h"
#include <Adafruit_BMP3XX.h>
#include <Adafruit_HTU31D.h>
#include <Adafruit_MCP9808.h>
#include <AS5600.h>
#include <ArduinoHttpClient.h>
#include <Adafruit_SleepyDog.h>
#include <MQTT.h>

/*
    Variable Declaration
*/

#define SEALEVELPRESSURE_HPA (1013.25)

const char BROKER[] = "broker.hivemq.com";
const char TOPIC[] = "reponseCode";
const char MQTT_USERNAME[] = "";
const char MQTT_PASSWORD[] = "";
const char PINNUMBER[] = SECRET_PINNUMBER;
char HOST_NAME[] = "3d.chordsrt.com";
int HTTP_PORT = 80;
int MQTT_PORT = 1883;
int modem_reset = 0;
const int interruptPin = 5;
volatile int interruptcounter = 0;
const float Pi = 3.14159;

/*
    Constructors
*/

Adafruit_BMP3XX bmp;
Adafruit_HTU31D htu;
Adafruit_MCP9808 mcp;
AS5600 as5600;
NBClient client;
GPRS gprs;
NB nbAccess(true);
NBScanner scannerNetworks;
NBModem modem;
MQTTClient mqttClient(750);

/*
    Function Creation
*/

void cb1()
{
  interruptcounter++;
}

void reconnect()
{
  while (nbAccess.isAccessAlive() == 0)
  {
    MODEM.begin(true);
    MODEM.sendf("AT+COPS=0");
    if ((nbAccess.begin(PINNUMBER) == NB_READY) &&
        (gprs.attachGPRS() == GPRS_READY))
    {
      Serial.println("Connecting!");
    }
    else
    {
      Serial.println("Not connected");
      MODEM.waitForResponse(2000);
    }
  }
}

/*
    Setup
*/

void setup()
{

  Serial.println("Reset");
  Watchdog.disable();

  Serial.begin(115200);

  pinMode(interruptPin, INPUT);
  attachInterrupt(digitalPinToInterrupt(interruptPin), cb1, RISING);

  boolean connected = false;

  while (!connected)
  {
    if ((nbAccess.begin(PINNUMBER) == NB_READY) &&
        (gprs.attachGPRS() == GPRS_READY))
    {
      connected = true;
      Serial.println("Connected!");
    }
    else
    {
      Serial.println("Not connected");
      delay(1000);
    }
  }

  Wire.begin();

  if (!bmp.begin_I2C())
  {
    Serial.println(F("Could not find a valid BMP3XX sensor, check wiring or "
                     "try a different address!"));
  }

  if (!htu.begin())
  {
    Serial.println(F("Could not find a valid HTU31D sensor, check wiring or "
                     "try a different address!"));
  }

  if (!mcp.begin(0x18))
  {
    Serial.println(F("Could not find a valid MCP9808 sensor, checking wiring or "
                     "try a different address!"));
  }

  mqttClient.setCleanSession(true);
  mqttClient.setTimeout(5000);
  mqttClient.begin(BROKER, MQTT_PORT, client);

  Watchdog.enable(8000);
}

void loop()
{

  if (nbAccess.isAccessAlive())
  {
    Serial.println("***MQTT PRE-LOOP CONNECTED***");
  }
  else
  {
    Serial.println("***MQTT PRE-LOOP DISCONNECTED***");
  }

  mqttClient.loop();

  if (nbAccess.isAccessAlive())
  {
    Serial.println("***MQTT POST-LOOP CONNECTED***");
  }
  else
  {
    Serial.println("***MQTT POST-LOOP DISCONNECTED***");
  }

  Watchdog.reset();
  delay(200);
  reconnect();
  Watchdog.reset();
  sensors_event_t htu_humidity, htu_temp;
  htu.getEvent(&htu_humidity, &htu_temp);
  String urlRequest = "/measurements/url_create?instrument_id=92&bmp_temp=" + String(bmp.temperature) + "&bmp_pressure=" + String(bmp.pressure / 100) + "&bmp_slp=" + String(SEALEVELPRESSURE_HPA) + "&bmp_altitude=" + String(bmp.readAltitude(SEALEVELPRESSURE_HPA)) + "&bmp_humidity=-999.9" + "&htu21d_temp=" + String(htu_temp.temperature) + "&htu21d_humidity=" + String(htu_humidity.relative_humidity) + "&mcp9808=" + String(mcp.readTempC()) + "&wind_direction=" + String(as5600.readAngle() * AS5600_RAW_TO_DEGREES) + "&wind_speed=" + String(0.019 * (interruptcounter / 2) + 0.36) + "&key=" + SECRET_KEY;
  Serial.println(urlRequest);
  Watchdog.reset();

  if (nbAccess.isAccessAlive())
  {
    Serial.println("");
    Serial.println("***HTTP PRE-OPEN CONNECTED***");
    Serial.println("");
  }
  else
  {
    Serial.println("");
    Serial.println("***HTTP PRE-OPEN DISCONNECTED***");
    Serial.println("");
  }

  HttpClient client2 = HttpClient(client, HOST_NAME, HTTP_PORT);

  if (nbAccess.isAccessAlive())
  {
    Serial.println("");
    Serial.println("***HTTP POST-OPEN CONNECTED***");
    Serial.println("");
  }
  else
  {
    Serial.println("");
    Serial.println("***HTTP POST-OPEN DISCONNECTED***");
    Serial.println("");
  }

  client2.setHttpResponseTimeout(5000);
  client2.setTimeout(5000);
  Watchdog.reset();

  if (nbAccess.isAccessAlive())
  {
    Serial.println("");
    Serial.println("***HTTP PRE-GET CONNECTED***");
    Serial.println("");
  }
  else
  {
    Serial.println("");
    Serial.println("***HTTP PRE-GET DISCONNECTED***");
    Serial.println("");
  }

  client2.get(urlRequest);

  if (nbAccess.isAccessAlive())
  {
    Serial.println("");
    Serial.println("***HTTP POST-GET CONNECTED***");
    Serial.println("");
  }
  else
  {
    Serial.println("");
    Serial.println("***HTTP POST-GET DISCONNECTED***");
    Serial.println("");
  }

  Watchdog.reset();

  if (nbAccess.isAccessAlive())
  {
    Serial.println("");
    Serial.println("***HTTP PRE-RESPONSE-STATUS CONNECTED***");
    Serial.println("");
  }
  else
  {
    Serial.println("");
    Serial.println("***HTTP PRE-RESPONSE-STATUS DISCONNECTED***");
    Serial.println("");
  }

  int statusCode = client2.responseStatusCode();

  if (nbAccess.isAccessAlive())
  {
    Serial.println("");
    Serial.println("***HTTP POST-RESPONSE-STATUS CONNECTED***");
    Serial.println("");
  }
  else
  {
    Serial.println("");
    Serial.println("***HTTP POST-RESPONSE-STATUS DISCONNECTED***");
    Serial.println("");
  }

  Watchdog.reset();

  if (nbAccess.isAccessAlive())
  {
    Serial.println("");
    Serial.println("***HTTP PRE-RESPONSE-BODY CONNECTED***");
    Serial.println("");
  }
  else
  {
    Serial.println("");
    Serial.println("***HTTP PRE-RESPONSE-BODY DISCONNECTED***");
    Serial.println("");
  }

  String response = client2.responseBody();

  if (nbAccess.isAccessAlive())
  {
    Serial.println("");
    Serial.println("***HTTP POST-RESPONSE-BODY CONNECTED***");
    Serial.println("");
  }
  else
  {
    Serial.println("");
    Serial.println("***HTTP POST-RESPONSE-BODY***");
    Serial.println("");
  }

  Watchdog.reset();

  if (nbAccess.isAccessAlive())
  {
    Serial.println("");
    Serial.println("***HTTP PRE-CLOSE CONNECTED***");
    Serial.println("");
  }
  else
  {
    Serial.println("");
    Serial.println("***HTTP PRE-CLOSE DISCONNECTED***");
    Serial.println("");
  }

  client2.stop();

  if (nbAccess.isAccessAlive())
  {
    Serial.println("");
    Serial.println("***HTTP POST-CLOSE CONNECTED***");
    Serial.println("");
  }
  else
  {
    Serial.println("");
    Serial.println("***HTTP POST-CLOSE DISCONNECTED***");
    Serial.println("");
  }

  Watchdog.reset();
  String clientID = "MKRNBClient";
  clientID += String(random(0xffff), HEX);

  while (!mqttClient.connected())
  {

    if (nbAccess.isAccessAlive())
    {
      Serial.println("");
      Serial.println("***MQTT PRE-OPEN CONNECTED***");
      Serial.println("");
    }
    else
    {
      Serial.println("");
      Serial.println("***MQTT PRE-OPEN DISCONNECTED***");
      Serial.println("");
    }

    while (!mqttClient.connected())
    {
      if (!nbAccess.isAccessAlive())
      {
        Serial.println("");
        Serial.println("***NB Disconnected***");
        Serial.println("");
        reconnect();
      }

      Serial.println("");
      Serial.println("***CONNECTING TO MQTT BROKER***");
      Serial.println("");

      if (mqttClient.connect(clientID.c_str(), MQTT_USERNAME, MQTT_PASSWORD))
      {
        Serial.println("");
        Serial.println("***CONNECTED TO MQTT BROKER***");
        Serial.println("");
      }
      else
      {
        delay(5000);
      }
    }

    if (nbAccess.isAccessAlive())
    {
      Serial.println("");
      Serial.println("***MQTT POST-OPEN CONNECTED***");
      Serial.println("");
    }
    else
    {
      Serial.println("");
      Serial.println("***MQTT POST-OPEN DISCONNECTED***");
      Serial.println("");
    }
  }

  Watchdog.reset();

  if (nbAccess.isAccessAlive())
  {
    Serial.println("");
    Serial.println("***MQTT PRE-PUBLISH CONNECTED***");
    Serial.println("");
  }
  else
  {
    Serial.println("");
    Serial.println("***MQTT PRE-PUBLISH DISCONNECTED***");
    Serial.println("");
  }

  mqttClient.publish("returnCode", String(statusCode));

  if (nbAccess.isAccessAlive())
  {
    Serial.println("");
    Serial.println("***MQTT POST-PUBLISH CONNECTED***");
    Serial.println("");
  }
  else
  {
    Serial.println("");
    Serial.println("***MQTT POST-PUBLISH DISCONNECTED***");
    Serial.println("");
  }

  Watchdog.reset();

  if (nbAccess.isAccessAlive())
  {
    Serial.println("");
    Serial.println("***WDT PRE-LOOP CONNECTED***");
    Serial.println("");
  }
  else
  {
    Serial.println("");
    Serial.println("***WDT PRE-LOOP DISCONNECTED***");
    Serial.println("");
  }

  interruptcounter = 0;
  for (int i = 0; i < 55; i++)
  {
    Watchdog.reset();
    delay(1000);
  }

  if (nbAccess.isAccessAlive())
  {
    Serial.println("");
    Serial.println("***WDT POST-LOOP CONNECTED***");
    Serial.println("");
  }
  else
  {
    Serial.println("");
    Serial.println("***WDT POST-LOOP DISCONNECTED***");
    Serial.println("");
  }
}

/*
  Changed line 141 of NBClient.cpp from "MODEM.send("AT+USECPRF=0,0,1");"
  to "MODEM.send("AT+USECPRF=0,0,0");"
*/

/*
  Changed line 506 of NBClient.cpp from "MODEM.waitForResponse(10000);"
  to "MODEM.waitForResponse(120000);"
*/

/*
  Changed line 197 of NBClient.cpp from "MODEM.sendf("AT+USOCL=%d", _socket);"
  to MODEM.sendf("AT+USOCL=%d,1", _socket);
*/

/*
  Changed line 332 of NBClient.cpp from "command.reserve(19 + (size > 256 ? 256 : size) * 2);"
  to "command.reserve(19 + (size > 512 ? 512 : size) * 2);"
*/

/*
  Changed lines 338-340 of NBClient.cpp from
  "if (chunkSize > 256)
    {
      chunkSize = 256;"
  to
  "if (chunkSize > 512)
    {
      chunkSize = 512;"
*/

/*
  Changed line 505 of NBClient.cpp from "MODEM.sendf("AT+USOCL=%d", _socket);"
  to "MODEM.sendf("AT+USOCL=%d,1", _socket);"
*/

#include <Arduino.h>
#include <MKRNB.h>
#include <Adafruit_BMP3XX.h>
#include <Adafruit_HTU31D.h>
#include <Adafruit_MCP9808.h>
#include <Adafruit_SI1145.h>
#include <AS5600.h>
#include <ArduinoHttpClient.h>
#include <MQTT.h>
#include <Arduino_PMIC.h>
#include "secrets.h"

/*
  =============================================
  VARIABLES
  =============================================
*/

const char NB_PINNUMBER[] = "";
const char HTTP_HOST_NAME[] = "3d.chordsrt.com";
const char MQTT_BROKER[] = "broker.hivemq.com";
const char MQTT_TOPIC[] = "httpStatusCode";
const char MQTT_USERNAME[] = "";
const char MQTT_PASSWORD[] = "";

String URL_REQUEST;
String URL_REQUEST_ADDRESS = "/measurements/url_create?";
String INSTRUMENT_ID = "94";
String BMP390_TEMP;
String BMP390_PRESSURE;
String BMP390_ALTITUDE;
String HTU31D_TEMP;
String HTU31D_HUMIDITY;
String MCP9808_TEMP;
String WIND_DIRECTION;
String WIND_SPEED;
String RAIN_AMOUNT;
String HTTP_RESPONSE;
String HTTP_STATUS_CODE;

volatile float RAIN_COUNTER = 0.0;
volatile float WIND_COUNTER = 0.0;
const float RAIN_CALIBRATION = 0.19;
float SEA_LEVEL_PRESSURE;

const int RAIN_PIN = 4;
const int ANEMOMETER_PIN = 5;
int HTTP_PORT = 80;
int MQTT_PORT = 1883;
unsigned long lastMillis = 0;

sensors_event_t htu_humidity, htu_temp;

/*
  =============================================
  CONSTRUCTORS
  =============================================
*/

Adafruit_BMP3XX bmp;
Adafruit_HTU31D htu;
Adafruit_MCP9808 mcp;
Adafruit_SI1145 light = Adafruit_SI1145();
AS5600 windVane;
NBClient nbClient;
GPRS gprs;
NB nbAccess(true);

/*
  ==============================================
  FUNCTIONS
  ==============================================
*/

// Increments the volatile WIND_COUNTER variable
void WindInterrupt()
{
  WIND_COUNTER += 1.0;
}

// Increments the volatile RAIN_COUNTER variable
void RainInterrupt()
{
  RAIN_COUNTER += 1.0;
}

void NBConnect()
{

  while (nbAccess.ready() != 1 || nbAccess.status() != 3 || nbAccess.getLocalTime() == 0)
  {
    if ((nbAccess.begin(NB_PINNUMBER) == NB_READY) && (gprs.attachGPRS() == GPRS_READY))
    {
      Serial.println("Connected to NB network");
    }
    else
      Serial.println("Connection to NB network failed");
    delay(2000);
  }
}

void I2CSensorInitialize()
{

  Wire.begin();

  if (!bmp.begin_I2C())
  {
    Serial.println("***BMP390 Not Detected***");
  }

  if (!htu.begin())
  {
    Serial.println("***HTU31D Not Detected***");
  }

  if (!mcp.begin(0x18))
  {
    Serial.println("***MCP9808 Not Detected***");
  }

  if (!light.begin())
  {
    Serial.println("***SI1145 Not Detected***");
  }
}

void GetI2CSensorData()
{
  SEA_LEVEL_PRESSURE = ((bmp.readPressure() / 100) * exp(556 / (29.3 * (bmp.readTemperature() + 273.15))));
  Serial.print("SLP: ");
  Serial.println(SEA_LEVEL_PRESSURE);
  BMP390_TEMP = String(float(bmp.readTemperature()));
  BMP390_PRESSURE = String(float(bmp.readPressure() / 100));
  BMP390_ALTITUDE = String(float(bmp.readAltitude(SEA_LEVEL_PRESSURE)));

  htu.getEvent(&htu_humidity, &htu_temp);
  HTU31D_HUMIDITY = String(float(htu_humidity.relative_humidity));
  HTU31D_TEMP = String(float(htu_temp.temperature));

  MCP9808_TEMP = String(float(mcp.readTempC()));
}

void GetWindDirectionData()
{
  WIND_DIRECTION = String(float(analogRead(A0) * 0.08789));
}

void GetWindSpeedData()
{
  WIND_SPEED = String(float(0.019 * (WIND_COUNTER / 2) + 0.36));
}

void GetRainData()
{
  RAIN_AMOUNT = String(float(RAIN_COUNTER * RAIN_CALIBRATION));
}

void CreateUrlRequest()
{
  URL_REQUEST = URL_REQUEST_ADDRESS + "instrument_id=" + INSTRUMENT_ID + "&bmp_temp=" + BMP390_TEMP + "&bmp_pressure=" + BMP390_PRESSURE + "&bmp_slp=" + String(SEA_LEVEL_PRESSURE) + "&bmp_altitude=" + BMP390_ALTITUDE + "&htu21d_temp=" + HTU31D_TEMP + "&htu21d_humidity=" + HTU31D_HUMIDITY + "&mcp9808=" + MCP9808_TEMP + "&wind_direction=" + WIND_DIRECTION + "&wind_speed=" + WIND_SPEED + "&rain=" + RAIN_AMOUNT + "&key=" + SECRET_KEY;
}

/*
  ==============================================
  BEGIN CODE BODY
  ==============================================
*/

void setup()
{
  // Initialize serial output
  delay(500);
  Serial.begin(115200);
  delay(25000);

  // Attach interrupt to anemometer
  pinMode(ANEMOMETER_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(ANEMOMETER_PIN), WindInterrupt, RISING);

  // Attach interrupt to rain bucket
  pinMode(RAIN_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(RAIN_PIN), RainInterrupt, RISING);

  analogReadResolution(12);

  NBConnect();
}

void loop()
{
  lastMillis = millis();

  if (nbAccess.ready() != 1 || nbAccess.status() != 3 || nbAccess.getLocalTime() == 0)
  {
    NBConnect();
    Serial.println("NB Reconnected");
  }

  I2CSensorInitialize();
  GetI2CSensorData();
  GetWindSpeedData();
  GetRainData();
  GetWindDirectionData();
  CreateUrlRequest();

  Serial.print("BMP390: ");
  Serial.println(BMP390_TEMP);
  Serial.print("MCP: ");
  Serial.println(MCP9808_TEMP);
  Serial.print("HTU: ");
  Serial.println(HTU31D_TEMP);

  HttpClient httpClient = HttpClient(nbClient, HTTP_HOST_NAME, HTTP_PORT);
  Serial.println(URL_REQUEST);
  httpClient.get(String(URL_REQUEST));

  HTTP_STATUS_CODE = httpClient.responseStatusCode();
  HTTP_RESPONSE = httpClient.responseBody();

  Serial.print("HTTP Response: ");
  Serial.println(HTTP_RESPONSE);

  Serial.print("HTTP Status Code: ");
  Serial.println(HTTP_STATUS_CODE);

  RAIN_COUNTER = 0;
  WIND_COUNTER = 0;

  while ((millis() - lastMillis) < 60000)
    ;

  lastMillis = 0;

  if (httpClient.available())
  {
    Serial.print("HTTP Client Read: ");
    Serial.println((char)httpClient.read());
  }

  if (nbClient.available())
  {
    Serial.print("NB Client Read: ");
    Serial.println((char)nbClient.read());
  }

  if (!httpClient.available() && !httpClient.connected())
  {
    httpClient.stop();
    Serial.println("HTTP Client Stopped");
  }
}
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
  DEFINITIONS
  =============================================
*/

#define SEA_LEVEL_PRESSURE (1013.25)
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
  BMP390_TEMP = String(bmp.temperature);
  BMP390_PRESSURE = String(bmp.pressure / 100);
  BMP390_ALTITUDE = String(bmp.readAltitude(SEA_LEVEL_PRESSURE));

  htu.getEvent(&htu_humidity, &htu_temp);
  HTU31D_HUMIDITY = String(htu_humidity.relative_humidity);
  HTU31D_TEMP = String(htu_temp.temperature);

  MCP9808_TEMP = String(mcp.readTempC());

  // WIND_DIRECTION = String(windVane.readAngle() * AS5600_RAW_TO_DEGREES);
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
  URL_REQUEST = URL_REQUEST_ADDRESS + "instrument_id=" + INSTRUMENT_ID + "&bmp_temp=" + BMP390_TEMP + "&bmp_pressure=" + BMP390_PRESSURE + "&bmp_slp=" + String(SEA_LEVEL_PRESSURE) + "&bmp_altitude=" + BMP390_ALTITUDE + "&htu21d_temp=" + HTU31D_TEMP + "&htu21d_humidity=" + HTU31D_HUMIDITY + "&mcp9808=" + MCP9808_TEMP + /* "&wind_direction=" + WIND_DIRECTION + */ "&wind_speed=" + WIND_SPEED + "&rain=" + RAIN_AMOUNT + "&key=" + SECRET_KEY;
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
  delay(500);

  // Attach interrupt to anemometer
  pinMode(ANEMOMETER_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(ANEMOMETER_PIN), WindInterrupt, RISING);

  // Attach interrupt to rain bucket
  pinMode(RAIN_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(RAIN_PIN), RainInterrupt, RISING);

  NBConnect();
}

void loop()
{

  lastMillis = millis();

  if (nbAccess.ready() != 1 || nbAccess.status() != 3 || nbAccess.getLocalTime() == 0)
    NBConnect();

  I2CSensorInitialize();
  GetI2CSensorData();
  GetWindSpeedData();
  GetRainData();
  CreateUrlRequest();

  Serial.println(RAIN_COUNTER);

  HttpClient httpClient = HttpClient(nbClient, HTTP_HOST_NAME, HTTP_PORT);

  Serial.println(URL_REQUEST);
  httpClient.get(String(URL_REQUEST));

  HTTP_STATUS_CODE = httpClient.responseStatusCode();
  HTTP_RESPONSE = httpClient.responseBody();

  Serial.print("HTTP Response: ");
  Serial.println(HTTP_RESPONSE);

  Serial.print("HTTP Status Code: ");
  Serial.println(HTTP_STATUS_CODE);

  httpClient.flush();

  RAIN_COUNTER = 0;
  WIND_COUNTER = 0;

  httpClient.stop();

  while ((millis() - lastMillis) < 60000)
    delay(500);

  lastMillis = 0;
}
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

/*
    Variable Declaration
*/

#define SEALEVELPRESSURE_HPA (1013.25)

const char PINNUMBER[] = SECRET_PINNUMBER;
char HOST_NAME[] = "3d.chordsrt.com";
int HTTP_PORT = 80;
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

/*
    Function Creation
*/

void cb1()
{
  interruptcounter++;
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

  Watchdog.enable(8000);
}

void loop()
{
  Watchdog.reset();

  if (nbAccess.isAccessAlive() == 0)
  {
    Serial.println("Disconnected");
    delay(8500);
  }

  sensors_event_t htu_humidity, htu_temp;
  htu.getEvent(&htu_humidity, &htu_temp);
  String urlRequest = "/measurements/url_create?instrument_id=92&bmp_temp=" + String(bmp.temperature) + "&bmp_pressure=" + String(bmp.pressure / 100) + "&bmp_slp=" + String(SEALEVELPRESSURE_HPA) + "&bmp_altitude=" + String(bmp.readAltitude(SEALEVELPRESSURE_HPA)) + "&bmp_humidity=-999.9" + "&htu21d_temp=" + String(htu_temp.temperature) + "&htu21d_humidity=" + String(htu_humidity.relative_humidity) + "&mcp9808=" + String(mcp.readTempC()) + "&wind_direction=" + String(as5600.readAngle() * AS5600_RAW_TO_DEGREES) + "&wind_speed=" + String(0.0188 * (interruptcounter / 2) + 0.39) + "&key=" + SECRET_KEY;
  Serial.println(urlRequest);

  Watchdog.reset();
  HttpClient client2 = HttpClient(client, HOST_NAME, HTTP_PORT);
  client2.setHttpResponseTimeout(5000);
  Watchdog.reset();
  client2.get(urlRequest);
  Watchdog.reset();
  int statusCode = client2.responseStatusCode();
  Watchdog.reset();
  String response = client2.responseBody();
  Watchdog.reset();

  if (statusCode != 200)
  {
    client2.stop();
    delay(10000);
  }

  Serial.print("Status code: ");
  Serial.println(statusCode);
  Serial.print("Response: ");
  Serial.println(response);

  client2.stop();
  interruptcounter = 0;

  for (int i = 0; i < 12; i++)
  {
    Watchdog.reset();
    delay(5000);
  }
}

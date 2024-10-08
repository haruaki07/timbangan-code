#include "Arduino.h"
#include "WiFi.h"
#include "HTTPClient.h"
#include <HX711.h>
#include <WiFiManager.h>

const char *box_id = "1";

// ultrasonic
const int trigPin1 = 13;
const int echoPin1 = 12;
const int trigPin2 = 14;
const int echoPin2 = 27;

// loadcell
const int doutPin = 4;
const int clkPin = 15;
const float calibration_factor = 106025.00;

const char *ssid = "HUAWEI-562C";
const char *password = "82743342";
const float send_record_weight_thres = 1.0;
const char *endpoint = "http://128.199.72.109/api/v1/record";

unsigned long lastTime = 0;
unsigned long read_delay = 1000;

float read_ultra_1();
float read_ultra_2();
float read_weight();
float read_length();

bool wifiConnected = false;

HX711 scale;
WiFiManager wifiManager;

void setup()
{
  pinMode(trigPin1, OUTPUT);
  pinMode(echoPin1, INPUT);
  pinMode(trigPin2, OUTPUT);
  pinMode(echoPin2, INPUT);

  Serial.begin(9600);

  scale.begin(doutPin, clkPin);
  scale.set_scale(calibration_factor); // Adjust to this calibration factor
  scale.tare();                        // Reset the scale to 0

  wifiConnected = wifiManager.autoConnect(String("dhs-" + String(box_id) + "-wifi-setup").c_str());

  if (!wifiConnected)
  {
    Serial.println("Failed to connect wifi");
    ESP.restart();
  }
  else
  {
    Serial.print("Connected to WiFi network with IP Address: ");
    Serial.println(WiFi.localIP());
  }

  // WiFi.begin(ssid, password);
  // Serial.println("Connecting");
  // while (WiFi.status() != WL_CONNECTED)
  // {
  // delay(500);
  // Serial.print(".");
  // }
  // Serial.println("");
  // // Serial.print("Connected to WiFi network with IP Address: ");
  // Serial.println(WiFi.localIP());
  Serial.print("Running on version: ");
  Serial.println(COMMIT_HASH);
}

void loop()
{
  // delay for 1000ms
  if ((millis() - lastTime) > read_delay)
  {
    float weight = read_weight();
    Serial.print(weight);
    Serial.print(" ");
    Serial.print(read_ultra_1());
    Serial.print(" ");
    Serial.println(read_ultra_2());
    if (weight >= send_record_weight_thres)
    {
      float length = read_length();

      // Check WiFi connection status
      if (WiFi.status() == WL_CONNECTED)
      {
        HTTPClient http;
        http.begin(endpoint);

        http.addHeader("content-type", "application/json");
        String body = "{\"box_id\":\"" + String(box_id) + "\",\"weight\":" + weight + ",\"length\":" + length + "}";
        Serial.println(body);

        int httpResponseCode = http.POST(body);

        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
        String payload = http.getString();
        Serial.println(payload);

        // Free resources
        http.end();
      }
      else
      {
        Serial.println("WiFi Disconnected");
      }
    }

    lastTime = millis();
  }
}

float read_length()
{
  float sensor1 = read_ultra_1();
  float sensor2 = read_ultra_2();

  return 80.0 - (sensor1 + sensor2);
}

float read_weight()
{
  if (scale.is_ready())
  {
    return scale.get_units(2);
  }

  return 0.0;
}

float read_ultra_1()
{
  digitalWrite(trigPin1, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin1, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin1, LOW);

  float duration = pulseIn(echoPin1, HIGH);
  float distance = (duration * .0343) / 2;
  return distance;
}

float read_ultra_2()
{
  digitalWrite(trigPin2, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin2, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin2, LOW);

  float duration = pulseIn(echoPin2, HIGH);
  float distance = (duration * .0343) / 2;
  return distance;
}
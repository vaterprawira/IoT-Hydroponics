#define BLYNK_TEMPLATE_ID "TMPL6Zx2JhZnI"
#define BLYNK_TEMPLATE_NAME "PPM"
#define BLYNK_AUTH_TOKEN "ogyz2EIyX-cW0ihsxOvMVEOMO9fXuwA6"

#define BLYNK_PRINT Serial
#include <BlynkMultiClient.h>

#include <ESP8266WiFi.h>

#define TdsSensorPin A0
#define VREF 3.3      // analog reference voltage(Volt) of the ADC
#define SCOUNT  30           // sum of sample point
#define RelayPin 5 //pin d1
#define RelayPin2 12 //pin d6
int analogBuffer[SCOUNT];    // store the analog value in the array, read from ADC
int analogBufferTemp[SCOUNT];
int analogBufferIndex = 0,copyIndex = 0;
int pinValue;
float averageVoltage = 0,tdsValue = 0,temperature = 25;

static WiFiClient blynkWiFiClient;

const char* ssid = "Ruijie-ASP";
const char* pass = "alamsari$";
// const char* ssid = "asus";
// const char* pass = "komputer22";
//const char* ssid = "PCU_Sistem_Kontrol";
//const char* pass = "lasikonn";

void connectWiFi()
{
  Serial.print("Connecting to ");
  Serial.println(ssid);

  if (pass && strlen(pass)) {
    WiFi.begin((char*)ssid, (char*)pass);
  } else {
    WiFi.begin((char*)ssid);
  }

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
}

BLYNK_WRITE(V1)
{
  pinValue = param.asInt() + 10;
  digitalWrite(RelayPin, pinValue);
  digitalWrite(RelayPin2, pinValue);
}

BLYNK_WRITE(V2)
{
  int pinValue2 = param.asInt() + 10;

  if (pinValue2 == 11 && pinValue == 10) {
    digitalWrite(RelayPin, LOW);
    digitalWrite(RelayPin2, LOW);
  } else if (pinValue2 == 10 && pinValue == 10) {
    digitalWrite(RelayPin, HIGH);
    digitalWrite(RelayPin2, HIGH);
  }
}

BLYNK_CONNECTED()
{
  Blynk.syncVirtual(V1);  
}

void setup()
{
    Serial.begin(9600);
    pinMode(TdsSensorPin,INPUT);
    pinMode(RelayPin,OUTPUT);
    pinMode(RelayPin2,OUTPUT);
    connectWiFi();

    // Setup Blynk
    Blynk.addClient("WiFi", blynkWiFiClient, 80);
    Blynk.config(BLYNK_AUTH_TOKEN);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
    return;
  }
  
  static unsigned long analogSampleTimepoint = millis();
  if (millis() - analogSampleTimepoint > 40U) {
    analogSampleTimepoint = millis();
    analogBuffer[analogBufferIndex] = analogRead(TdsSensorPin);
    analogBufferIndex++;
    if (analogBufferIndex == SCOUNT) {
      analogBufferIndex = 0;
    }
  }
  
  static unsigned long printTimepoint = millis();
  if (millis() - printTimepoint > 800U) {
    printTimepoint = millis();
    for (copyIndex = 0; copyIndex < SCOUNT; copyIndex++)
      analogBufferTemp[copyIndex] = analogBuffer[copyIndex];
    
    averageVoltage = getMedianNum(analogBufferTemp, SCOUNT) * (float)VREF / 1024.0;
    float compensationCoefficient = 1.0 + 0.02 * (temperature - 25.0);
    float compensationVoltage = averageVoltage / compensationCoefficient;
    tdsValue = (133.42 * compensationVoltage * compensationVoltage * compensationVoltage - 255.86 * compensationVoltage * compensationVoltage + 857.39 * compensationVoltage) * 0.5;
    
    Serial.print("TDS Value:");
    Serial.print(tdsValue + tdsValue * 0.3, 0);
    Serial.println("ppm");

    Blynk.run();
    Blynk.virtualWrite(V0, tdsValue + tdsValue * 0.3);

    static bool relayState = false;
    static unsigned long relayDelayTime = 0;
    if (tdsValue + tdsValue * 0.3 < 1200 && pinValue == 11) {
      if (!relayState) {
        relayState = true;
        Serial.print("relayState = true");
        relayDelayTime = millis();
        digitalWrite(RelayPin, LOW);
        digitalWrite(RelayPin2, LOW);
      } else if (relayState && millis() - relayDelayTime >= 11000) {
        relayState = false;
        Serial.print("relayState = false");
        relayDelayTime = millis();
        digitalWrite(RelayPin, HIGH);
        digitalWrite(RelayPin2, HIGH);

        //delay(5000);
        for (int i = 0; i < 30; i++) {
          delay(30000);
        }
      }
    }
  }
}

int getMedianNum(int bArray[], int iFilterLen) {
      int bTab[iFilterLen];
      for (byte i = 0; i<iFilterLen; i++)
      bTab[i] = bArray[i];
      int i, j, bTemp;
      for (j = 0; j < iFilterLen - 1; j++) 
      {
      for (i = 0; i < iFilterLen - j - 1; i++) 
          {
        if (bTab[i] > bTab[i + 1]) 
            {
        bTemp = bTab[i];
            bTab[i] = bTab[i + 1];
        bTab[i + 1] = bTemp;
         }
      }
      }
      if ((iFilterLen & 1) > 0) {
        bTemp = bTab[(iFilterLen - 1) / 2];
      }
      else {
        bTemp = (bTab[iFilterLen / 2] + bTab[iFilterLen / 2 - 1]) / 2;
      }
      return bTemp;
}
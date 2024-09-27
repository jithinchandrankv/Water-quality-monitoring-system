#define BLYNK_TEMPLATE_ID "TMPL6--cYXtS-"  
#define BLYNK_TEMPLATE_NAME "test"         
#define BLYNK_PRINT Serial

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// Blynk credentials
char auth[] = "b4L-iHEfYpA0IPMvEXNcvtT4NQw9Gyct";
char ssid[] = "Happy star12";
char pass[] = "villa12@3036";

// Pin Definitions
int sewageValve = 14;
int chemicalValve = 15;
int inputValve = 12;
int calibrationValve = 16;

// Sensor Pin Definitions
#define TDS_PIN 33
#define PH_PIN 35
#define EC_PIN 34  // Added EC sensor pin
#define ONE_WIRE_BUS 5

// Sensor variables
float ph_value;
unsigned int tds_value;
float ec_value;  // EC sensor value
float water_temp;

// Calibration variables
float ph_calibration_value = 7.0;
float tds_calibration_value = 500;
float ec_calibration_value = 3500;  // Added EC calibration threshold
bool isCalibrating = false;
unsigned long calibrationStartTime = 0;
bool bufferFilled = false;

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature dallasTemperature(&oneWire);

BlynkTimer timer;

WidgetTerminal terminal(V10);

void setup() {
    Serial.begin(115200);
    Blynk.begin(auth, ssid, pass);

    pinMode(sewageValve, OUTPUT);
    pinMode(chemicalValve, OUTPUT);
    pinMode(inputValve, OUTPUT);
    pinMode(calibrationValve, OUTPUT);

    digitalWrite(sewageValve, LOW);
    digitalWrite(chemicalValve, LOW);
    digitalWrite(inputValve, LOW);
    digitalWrite(calibrationValve, LOW);

    dallasTemperature.begin();

    timer.setInterval(2000L, monitorWaterQuality);
    timer.setInterval(1000L, sendSensorDataToBlynk);
}

void loop() {
    Blynk.run();
    timer.run();

    if (isCalibrating && !bufferFilled) {
        unsigned long currentMillis = millis();
        if (currentMillis - calibrationStartTime >= 60000) {
            bufferFilled = true;
            terminal.println("Buffer solution filled, starting calibration...");
            terminal.flush();
            performCalibration();
        }
    }
}

void readPH() {
    int sensorValue = analogRead(PH_PIN);
    float voltage = sensorValue * (3.3 / 4096.0);
    ph_value = -5.70 * voltage + ph_calibration_value;
}

void readTDS() {
    int sensorValue = analogRead(TDS_PIN);
    float voltage = sensorValue * (3.3 / 4096.0);
    tds_value = voltage * tds_calibration_value;
}

void readEC() {
    int sensorValue = analogRead(EC_PIN);
    float voltage = sensorValue * (3.3 / 4096.0);
    ec_value = voltage * ec_calibration_value;
}

void readTemperature() {
    dallasTemperature.requestTemperatures();
    water_temp = dallasTemperature.getTempCByIndex(0);
}

void monitorWaterQuality() {
    if (!isCalibrating) {
        readPH();
        readTDS();
        readEC();
        readTemperature();

        if (ph_value < 6.0 || ph_value > 9.0 || tds_value > 1000 || ec_value > 3500) {
            digitalWrite(chemicalValve, HIGH);
            digitalWrite(sewageValve, LOW);
            digitalWrite(inputValve, LOW);
            Blynk.virtualWrite(V5, 255);
            Blynk.virtualWrite(V4, 0);
            Blynk.virtualWrite(V6, 0);

            terminal.println("Chemical Water Detected: Chemical Valve ON, Sewage Valve OFF, Input Valve OFF");
        } else {
            digitalWrite(chemicalValve, LOW);
            digitalWrite(sewageValve, HIGH);
            digitalWrite(inputValve, HIGH);
            Blynk.virtualWrite(V5, 0);
            Blynk.virtualWrite(V4, 255);
            Blynk.virtualWrite(V6, 255);

            terminal.println("Normal Water Detected: Sewage Valve ON, Chemical Valve OFF, Input Valve ON");
        }

        terminal.flush();
    }
}

void sendSensorDataToBlynk() {
    if (!isCalibrating) {
        Blynk.virtualWrite(V1, ph_value);
        Blynk.virtualWrite(V2, tds_value);
        Blynk.virtualWrite(V3, water_temp);
        Blynk.virtualWrite(V7, ec_value);  // Added EC value to Blynk
    }
}

BLYNK_WRITE(V8) {
    int buttonState = param.asInt();
    if (buttonState == 1) {
        isCalibrating = true;
        bufferFilled = false;
        calibrationStartTime = millis();
        digitalWrite(calibrationValve, HIGH);
        closeAllValves();

        terminal.println("Calibration Started: Waiting for 1 minute to fill the buffer solution.");
        terminal.flush();
    } else {
        isCalibrating = false;
        digitalWrite(calibrationValve, LOW);
        terminal.println("Calibration Stopped. New values applied.");
        terminal.flush();
    }
}

void performCalibration() {
    ph_calibration_value += (7.0 - ph_value) * 0.1;
    tds_calibration_value += (500 - tds_value) * 0.1;
    ec_calibration_value += (3500 - ec_value) * 0.1;  // Added EC calibration logic

    ph_value = ph_calibration_value;
    tds_value = tds_calibration_value;
    ec_value = ec_calibration_value;  // Apply new calibration

    Blynk.virtualWrite(V1, ph_value);
   

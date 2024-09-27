#define BLYNK_TEMPLATE_ID "TMPL6--cYXtS-"  
#define BLYNK_TEMPLATE_NAME "test"         
#define BLYNK_PRINT Serial

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <OneWire.h>
#include <Wire.h>
#include <DallasTemperature.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <HardwareSerial.h>

// Blynk credentials
char auth[] = "b4L-iHEfYpA0IPMvEXNcvtT4NQw9Gyct";
char ssid[] = "Happy star12";
char pass[] = "villa12@3036";

// Distance sensor start
HardwareSerial Ultrasonic_Sensor(2);
int pinRX = 16;
int pinTX = 17;

unsigned char data_buffer[4] = {0};
int distance;
unsigned char CS;

int Relay = 13;
int sewageValve = 14;
int chemicalValve = 15;

int Relay_Status = 0;
int waterLevelPer = 0;
int Percentage = 0;

int previousDistance;
unsigned long distanceChangeTime;

BlynkTimer timer;
BlynkTimer timer1;

float calibration_value = 15.65 - 0.7; 
int phval = 0;
unsigned long int avgval;
int buffer_arr[10], temp;

float ph_act;

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

namespace pin {
    const byte tds_sensor = A0;
    const byte one_wire_bus = 5;
}

namespace device {
    float aref = 3.3;
}

namespace sensor {
    float ec = 0;
    unsigned int tds = 0;
    float waterTemp = 0;
    float ecCalibration = 1;
}

OneWire oneWire(pin::one_wire_bus);
DallasTemperature dallasTemperature(&oneWire);

void setup() {
    Serial.begin(115200);
    Ultrasonic_Sensor.begin(9600, SERIAL_8N1, pinRX, pinTX);
    dallasTemperature.begin();
    Blynk.begin(auth, ssid, pass);

    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    delay(2000);
    display.clearDisplay();
    display.setTextColor(WHITE);

    pinMode(Relay, OUTPUT);
    pinMode(sewageValve, OUTPUT);
    pinMode(chemicalValve, OUTPUT);
    digitalWrite(Relay, LOW);
    digitalWrite(sewageValve, LOW);
    digitalWrite(chemicalValve, LOW);

    timer.setInterval(1000L, display_pHValue);
    timer1.setInterval(5000L, EC_and_ph);
}

void loop() {
    Blynk.run();
    timer.run();
    timer1.run();
    A02YYUW_Sensor();
}

void readTdsQuick() {
    dallasTemperature.requestTemperatures();
    sensor::waterTemp = dallasTemperature.getTempCByIndex(0);
    float rawEc = analogRead(pin::tds_sensor) * device::aref / 4096;
    float temperatureCoefficient = 1.0 + 0.02 * (sensor::waterTemp - 25.0);
    sensor::ec = (rawEc / temperatureCoefficient) * sensor::ecCalibration;
    sensor::tds = (133.42 * pow(sensor::ec, 3) - 255.86 * sensor::ec * sensor::ec + 857.39 * sensor::ec) * 0.5;
}

void ph_Sensor() {
    for (int i = 0; i < 10; i++) {
        buffer_arr[i] = analogRead(35);
        delay(30);
    }
    for (int i = 0; i < 9; i++) {
        for (int j = i + 1; j < 10; j++) {
            if (buffer_arr[i] > buffer_arr[j]) {
                temp = buffer_arr[i];
                buffer_arr[i] = buffer_arr[j];
                buffer_arr[j] = temp;
            }
        }
    }
    avgval = 0;
    for (int i = 2; i < 8; i++) {
        avgval += buffer_arr[i];
    }
    float volt = (float)avgval * 3.3 / 4096.0 / 6;  
    ph_act = -5.70 * volt + calibration_value;
}

void display_pHValue() {
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(0, 0);
    display.print("pH:");

    display.setTextSize(2);
    display.setCursor(55, 0);
    display.print(ph_act);

    display.setTextSize(2);
    display.setCursor(0, 20);
    display.print("EC:");
    
    display.setTextSize(2);
    display.setCursor(60, 20);
    display.print(sensor::ec);

    display.setTextSize(2);
    display.setCursor(0, 40);
    display.print("D:");

    display.setTextSize(2);
    display.setCursor(60, 40);
    display.print(distance);
    display.display();

    Blynk.virtualWrite(V0, waterLevelPer);
    Blynk.virtualWrite(V1, ph_act);
    Blynk.virtualWrite(V2, sensor::tds);
    Blynk.virtualWrite(V3, sensor::waterTemp);
}

void A02YYUW_Sensor() {
    if (Ultrasonic_Sensor.available() > 0) {
        delay(4);
        if (Ultrasonic_Sensor.read() == 0xff) {
            data_buffer[0] = 0xff;
            for (int i = 1; i < 4; i++) {
                data_buffer[i] = Ultrasonic_Sensor.read();
            }
            CS = data_buffer[0] + data_buffer[1] + data_buffer[2];
            if (data_buffer[3] == CS) {
                distance = (data_buffer[1] << 8) + data_buffer[2];
                distance = distance / 10;

                if (distance >= 80) {
                    digitalWrite(Relay, HIGH);
                } else if (distance <= 10) {
                    digitalWrite(Relay, LOW);
                }

                waterLevelPer = map(distance, 26, 73, 100, 0);
            }
        }
    }
}

void EC_and_ph() {
    readTdsQuick();
    ph_Sensor();

    if ((ph_act < 6 || ph_act > 9) || sensor::ec > 3500) {
        digitalWrite(chemicalValve, HIGH);
        digitalWrite(sewageValve, LOW);
    } else if ((ph_act >= 6 && ph_act <= 9) && sensor::ec <= 3500) {
        digitalWrite(chemicalValve, LOW);
    }
}


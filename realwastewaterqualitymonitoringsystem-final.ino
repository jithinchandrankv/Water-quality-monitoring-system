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

// Distance sensor configuration
HardwareSerial Ultrasonic_Sensor(2); 
int pinRX = 16;  
int pinTX = 17;  

// Data buffer for ultrasonic sensor
unsigned char data_buffer[4] = {0};

int distance;
unsigned char CS;

int Relay = 13;
int sewageValve = 14;   
int chemicalValve = 15; 
int sewageSwitch = 19;   
int chemicalSwitch = 18; 
int calibrationValve = 16;

int waterLevelPer = 0;

BlynkTimer timer;  
BlynkTimer timer1;

float calibration_value = 15.65 - 0.7; 
int buffer_arr[10], temp;
float ph_act;

#define SCREEN_WIDTH 128 
#define SCREEN_HEIGHT 64 

#define OLED_RESET -1 
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

namespace pin {
    const byte tds_sensor = 33; 
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

// Terminal widget on V10
WidgetTerminal terminal(V10);

bool isCalibrating = false;

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
    pinMode(sewageSwitch, INPUT_PULLDOWN);   
    pinMode(chemicalSwitch, INPUT_PULLDOWN); 
    pinMode(calibrationValve, OUTPUT);       

    digitalWrite(Relay, LOW);
    digitalWrite(sewageValve, LOW);   
    digitalWrite(chemicalValve, LOW); 
    digitalWrite(calibrationValve, LOW); 

    timer.setInterval(1000L, display_pHValue);
    timer1.setInterval(5000L, EC_and_ph);
}

void loop() {
    Blynk.run();
    timer.run(); 
    timer1.run();
    A02YYUW_Sensor();

    int sewageSwitchState = digitalRead(sewageSwitch);
    int chemicalSwitchState = digitalRead(chemicalSwitch);

    if (sewageSwitchState == HIGH) {
        digitalWrite(sewageValve, HIGH);    
        digitalWrite(chemicalValve, LOW);   
        Serial.println("Manual: Sewage Valve ON");
        terminal.println("Manual: Sewage Valve ON"); 
        terminal.flush();
        Blynk.virtualWrite(V4, 255); 
        Blynk.virtualWrite(V5, 0);   
    } else if (chemicalSwitchState == HIGH) {
        digitalWrite(chemicalValve, HIGH);  
        digitalWrite(sewageValve, LOW);     
        Serial.println("Manual: Chemical Valve ON");
        terminal.println("Manual: Chemical Valve ON"); 
        terminal.flush();
        Blynk.virtualWrite(V4, 0);   
        Blynk.virtualWrite(V5, 255); 
    }
}

BLYNK_WRITE(V6) {
    int buttonState = param.asInt();
    if (buttonState == 1) {
        digitalWrite(sewageValve, HIGH);    
        digitalWrite(chemicalValve, LOW);   
        Serial.println("Blynk: Sewage Valve ON");
        terminal.println("Blynk: Sewage Valve ON");
        terminal.flush();
        Blynk.virtualWrite(V4, 255); 
        Blynk.virtualWrite(V5, 0);   
    } else {
        digitalWrite(sewageValve, LOW);    
        Serial.println("Blynk: Sewage Valve OFF");
        terminal.println("Blynk: Sewage Valve OFF");
        terminal.flush();
        Blynk.virtualWrite(V4, 0); 
    }
}

BLYNK_WRITE(V7) {
    int buttonState = param.asInt();
    if (buttonState == 1) {
        digitalWrite(chemicalValve, HIGH);  
        digitalWrite(sewageValve, LOW);     
        Serial.println("Blynk: Chemical Valve ON");
        terminal.println("Blynk: Chemical Valve ON"); 
        terminal.flush();
        Blynk.virtualWrite(V4, 0);   
        Blynk.virtualWrite(V5, 255); 
    } else {
        digitalWrite(chemicalValve, LOW);   
        Serial.println("Blynk: Chemical Valve OFF");
        terminal.println("Blynk: Chemical Valve OFF"); 
        terminal.flush();
        Blynk.virtualWrite(V5, 0); 
    }
}

BLYNK_WRITE(V8) {
    int buttonState = param.asInt();
    if (buttonState == 1) {
        Serial.println("Calibration Started");
        terminal.println("Calibration Started"); 
        terminal.flush();
        digitalWrite(calibrationValve, HIGH); 
        isCalibrating = true;
        delay(5000); 
        performCalibration();
    } else {
        isCalibrating = false;
        digitalWrite(calibrationValve, LOW); 
        Serial.println("Calibration Stopped");
        terminal.println("Calibration Stopped"); 
        terminal.flush();
    }
}

void performCalibration() {
    readTdsQuick();
    ph_Sensor();

    if (ph_act >= 6.5 && ph_act <= 7.5 && sensor::ec <= 1500) {
        Serial.println("Parameters OK: No Calibration Needed");
        terminal.println("Parameters OK: No Calibration Needed"); 
        terminal.flush();
    } else {
        Serial.println("Calibrating...");
        terminal.println("Calibrating..."); 
        terminal.flush();
        calibration_value += (7.0 - ph_act) * 0.1; 
        sensor::ecCalibration += (1000 - sensor::ec) * 0.001;
        Serial.println("Calibration Complete");
        terminal.println("Calibration Complete"); 
        terminal.flush();
    }
}

void readTdsQuick() {
    dallasTemperature.requestTemperatures();
    sensor::waterTemp = dallasTemperature.getTempCByIndex(0);
    float rawEc = analogRead(pin::tds_sensor) * device::aref / 4096; 
    float temperatureCoefficient = 1.0 + 0.02 ; 
    sensor::ec = (rawEc / temperatureCoefficient) * sensor::ecCalibration;
    sensor::tds = (133.42 * pow(sensor::ec, 3) - 255.86 * sensor::ec * sensor::ec + 857.39 * sensor::ec) * 0.5;

    Serial.print(F("TDS: "));
    Serial.println(sensor::tds);
    Serial.print(F("EC: "));
    Serial.println(sensor::ec, 2);
    Serial.print(F("Temperature: "));
    Serial.println(sensor::waterTemp, 2);
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
    unsigned long int avgval = 0;
    for (int i = 2; i < 8; i++) {
        avgval += buffer_arr[i];
    }
    float volt = (float)avgval * 3.3 / 4096.0 / 6;  
    ph_act = -5.70 * volt + calibration_value;

    Serial.print("pH Val: ");
    Serial.println(ph_act);
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

                Serial.print("Distance: ");
                Serial.print(distance);
                Serial.println(" cm");
                
                waterLevelPer = map(distance, 26, 73, 100, 0);
            }
        }
    }
}

void EC_and_ph() {
    readTdsQuick();
    ph_Sensor();

    if ((ph_act < 6 || ph_act > 9) || sensor::ec > 3500) {
        Serial.println("Chemical Water Detected");
        digitalWrite(chemicalValve, HIGH);  
        digitalWrite(sewageValve, LOW);     

        Blynk.virtualWrite(V4, 0); 
        Blynk.virtualWrite(V5, 255); 
    } else if ((ph_act >= 6 && ph_act <= 9) && sensor::ec <= 3500) {
        Serial.println("Sewage Water Detected");
        digitalWrite(chemicalValve, LOW);   
        digitalWrite(sewageValve, HIGH);    

        Blynk.virtualWrite(V4, 255); 
        Blynk.virtualWrite(V5, 0);   
    }
}

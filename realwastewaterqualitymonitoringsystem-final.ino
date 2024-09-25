#define BLYNK_TEMPLATE_ID "TMPL6--cYXtS-"  // Replace with your actual Blynk template ID
#define BLYNK_TEMPLATE_NAME "test"         // Replace with your actual Blynk template name
#define BLYNK_PRINT Serial

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// Blynk credentials
char auth[] = "b4L-iHEfYpA0IPMvEXNcvtT4NQw9Gyct"; // Your Blynk Auth Token
char ssid[] = "Happy star12";                     // Your Wi-Fi SSID
char pass[] = "villa12@3036";                     // Your Wi-Fi Password

// Pin Definitions for controlling valves
int sewageValve = 14;       // Pin for controlling sewage valve
int chemicalValve = 15;     // Pin for controlling chemical valve
int inputValve = 12;        // Pin for controlling input valve
int calibrationValve = 16;  // Pin for controlling calibration valve

// Sensor Pin Definitions
#define TDS_PIN 33           // Pin connected to TDS sensor
#define PH_PIN 35            // Pin connected to pH sensor
#define ONE_WIRE_BUS 5       // Pin connected to Dallas temperature sensor

// Variables for storing sensor values
float ph_value;
unsigned int tds_value;
float water_temp;  // Variable for storing water temperature

// Variables for calibration settings
float ph_calibration_value = 7.0;  // pH calibration value
float tds_calibration_value = 500; // TDS calibration value
bool isCalibrating = false;        // Flag to indicate calibration mode
unsigned long calibrationStartTime = 0; // Stores the timestamp when calibration begins
bool bufferFilled = false;         // Flag to indicate whether the buffer solution is filled
bool sewageValveOpen = false;      // Flag to indicate if the sewage valve is open

// Dallas temperature sensor object
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature dallasTemperature(&oneWire);

// Blynk timer object for scheduling tasks
BlynkTimer timer;

// Blynk terminal widget setup on V10
WidgetTerminal terminal(V10);

void setup() {
    Serial.begin(115200);  // Initialize serial communication at 115200 baud rate
    Blynk.begin(auth, ssid, pass);  // Initialize connection to Blynk server

    // Configure valve control pins as outputs
    pinMode(sewageValve, OUTPUT);
    pinMode(chemicalValve, OUTPUT);
    pinMode(inputValve, OUTPUT);
    pinMode(calibrationValve, OUTPUT);

    // Ensure that all valves are closed initially
    digitalWrite(sewageValve, LOW);
    digitalWrite(chemicalValve, LOW);
    digitalWrite(inputValve, LOW);
    digitalWrite(calibrationValve, LOW);

    // Initialize Dallas temperature sensor
    dallasTemperature.begin();

    // Set timers to monitor water quality and send sensor data to Blynk periodically
    timer.setInterval(2000L, monitorWaterQuality);  // Check water quality every 2 seconds
    timer.setInterval(1000L, sendSensorDataToBlynk);  // Send sensor data to Blynk every second
}

void loop() {
    Blynk.run();  // Process Blynk commands and maintain connection
    timer.run();  // Execute scheduled tasks using Blynk timer

    // Non-blocking wait for 1 minute to allow buffer solution to fill during calibration
    if (isCalibrating && !bufferFilled) {
        unsigned long currentMillis = millis();
        if (currentMillis - calibrationStartTime >= 60000) {
            bufferFilled = true;  // Indicate that buffer solution is filled
            terminal.println("Buffer solution filled, starting calibration...");
            terminal.flush();
            performCalibration();  // Perform calibration process after buffer is filled
        }
    }
}

// Function to read pH sensor value
void readPH() {
    int sensorValue = analogRead(PH_PIN);
    float voltage = sensorValue * (3.3 / 4096.0); // Convert the raw reading to voltage
    ph_value = -5.70 * voltage + ph_calibration_value;  // Apply the formula to calculate pH value
}

// Function to read TDS sensor value
void readTDS() {
    int sensorValue = analogRead(TDS_PIN);
    float voltage = sensorValue * (3.3 / 4096.0);  // Convert the raw reading to voltage
    tds_value = voltage * tds_calibration_value;  // Calculate TDS value (ppm) based on calibration
}

// Function to read water temperature using Dallas sensor
void readTemperature() {
    dallasTemperature.requestTemperatures();
    water_temp = dallasTemperature.getTempCByIndex(0);  // Get water temperature in Celsius
}

// Function to monitor water quality and control valves based on sensor readings
void monitorWaterQuality() {
    if (!isCalibrating) {  // Skip monitoring during calibration process
        readPH();
        readTDS();
        readTemperature();

        // Control valves based on sensor readings
        if (ph_value < 6.0 || ph_value > 9.0 || tds_value > 1000) {
            // Open chemical valve, close sewage and input valves if chemical water is detected
            digitalWrite(chemicalValve, HIGH);
            digitalWrite(sewageValve, LOW);
            digitalWrite(inputValve, LOW);
            Blynk.virtualWrite(V5, 255);  // Update chemical valve status in Blynk
            Blynk.virtualWrite(V4, 0);    // Update sewage valve status in Blynk
            Blynk.virtualWrite(V6, 0);    // Update input valve status in Blynk

            terminal.println("Chemical Water Detected: Chemical Valve ON, Sewage Valve OFF, Input Valve OFF");
        } else {
            // Open sewage valve, close chemical valve, and open input valve for normal water
            digitalWrite(chemicalValve, LOW);
            digitalWrite(sewageValve, HIGH);
            digitalWrite(inputValve, HIGH);
            Blynk.virtualWrite(V5, 0);   // Update chemical valve status in Blynk
            Blynk.virtualWrite(V4, 255); // Update sewage valve status in Blynk
            Blynk.virtualWrite(V6, 255); // Update input valve status in Blynk

            terminal.println("Normal Water Detected: Sewage Valve ON, Chemical Valve OFF, Input Valve ON");
        }

        terminal.flush();  // Ensure terminal output is sent
    }
}

// Function to send sensor data to Blynk app
void sendSensorDataToBlynk() {
    if (!isCalibrating) {
        Blynk.virtualWrite(V1, ph_value);  // Send pH value to Blynk
        Blynk.virtualWrite(V2, tds_value); // Send TDS value to Blynk
        Blynk.virtualWrite(V3, water_temp); // Send temperature value to Blynk
    }
}

// Blynk button handler to start calibration process (V8)
BLYNK_WRITE(V8) {
    int buttonState = param.asInt();
    if (buttonState == 1) {
        isCalibrating = true;
        bufferFilled = false;  // Reset buffer flag for calibration
        calibrationStartTime = millis();  // Record the current time to start buffer fill
        digitalWrite(calibrationValve, HIGH);  // Open calibration valve
        closeAllValves();  // Ensure all other valves are closed during calibration

        terminal.println("Calibration Started: Waiting for 1 minute to fill the buffer solution.");
        terminal.flush();
    } else {
        isCalibrating = false;  // Mark the calibration process as complete
        digitalWrite(calibrationValve, LOW);  // Close calibration valve
        terminal.println("Calibration Stopped. New values applied.");
        terminal.flush();
    }
}

// Function to perform sensor calibration
void performCalibration() {
    // Adjust calibration values for pH and TDS based on current readings
    ph_calibration_value += (7.0 - ph_value) * 0.1;
    tds_calibration_value += (500 - tds_value) * 0.1;

    // Update sensor values immediately after calibration
    ph_value = ph_calibration_value;
    tds_value = tds_calibration_value;

    // Send updated values to Blynk app after calibration
    Blynk.virtualWrite(V1, ph_value);  // Update pH value
    Blynk.virtualWrite(V2, tds_value); // Update TDS value

    terminal.println("Calibration Complete");
    terminal.println("Updated pH Calibration Value: " + String(ph_calibration_value));
    terminal.println("Updated TDS Calibration Value: " + String(tds_calibration_value));

    // Automatically turn off the calibration button and open sewage valve
    Blynk.virtualWrite(V8, 0);  // Turn off calibration button in Blynk app
    digitalWrite(chemicalValve, LOW); // Ensure chemical valve is closed
    digitalWrite(sewageValve, HIGH);  // Open sewage valve after calibration
    Blynk.virtualWrite(V4, 255);      // Update sewage valve status in Blynk
    Blynk.virtualWrite(V5, 0);        // Update chemical valve status in Blynk

    terminal.println("Sewage Valve Opened after Calibration.");

    // Open input valve after calibration
    digitalWrite(inputValve, HIGH);
    Blynk.virtualWrite(V6, 255); // Update input valve status in Blynk
    terminal.println("Input Valve Opened after Calibration.");
    terminal.flush();
}

// Function to close all valves during calibration
void closeAllValves() {
    digitalWrite(sewageValve, LOW);    // Close sewage valve
    digitalWrite(chemicalValve, LOW);  // Close chemical valve
    digitalWrite(inputValve, LOW);     // Close input valve
    Blynk.virtualWrite(V4, 0);         // Update sewage valve status in Blynk
    Blynk.virtualWrite(V5, 0);         // Update chemical valve status in Blynk
    Blynk.virtualWrite(V6, 0);         // Update input valve status in Blynk
}

// Blynk button handler to manually open sewage valve (V7)
BLYNK_WRITE(V7) {
    if (!isCalibrating) {  // Manual control is allowed only when calibration is not in progress
        int buttonState = param.asInt();
        if (buttonState == 1) {
            digitalWrite(sewageValve, HIGH);
            digitalWrite(chemicalValve, LOW);
            digitalWrite(inputValve, HIGH);  // Open input valve manually
            Blynk.virtualWrite(V4, 255);    // Update sewage valve status in Blynk
            Blynk.virtualWrite(V5, 0);      // Update chemical valve status in Blynk
            Blynk.virtualWrite(V6, 255);    // Update input valve status in Blynk
            terminal.println("Manual: Sewage Valve ON, Chemical Valve OFF, Input Valve ON");
        } else {
            digitalWrite(sewageValve, LOW);
            terminal.println("Manual: Sewage Valve OFF");
            Blynk.virtualWrite(V4, 0);  // Update sewage valve status in Blynk
        }
        terminal.flush();
    }
}

// Blynk button handler to manually open chemical valve (V9)
BLYNK_WRITE(V9) {
    if (!isCalibrating) {  // Manual control is allowed only when calibration is not in progress
        int buttonState = param.asInt();
        if (buttonState == 1) {
            digitalWrite(chemicalValve, HIGH);
            digitalWrite(sewageValve, LOW);
            digitalWrite(inputValve, LOW);  // Close input valve when chemical valve is open
            Blynk.virtualWrite(V5, 255);  // Update chemical valve status in Blynk
            Blynk.virtualWrite(V4, 0);    // Update sewage valve status in Blynk
            Blynk.virtualWrite(V6, 0);    // Update input valve status in Blynk
            terminal.println("Manual: Chemical Valve ON, Sewage Valve OFF, Input Valve OFF");
        } else {
            digitalWrite(chemicalValve, LOW);
            terminal.println("Manual: Chemical Valve OFF");
            Blynk.virtualWrite(V5, 0);  // Update chemical valve status in Blynk
        }
        terminal.flush();
    }
}

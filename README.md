Real time waste water quality monitoring system with automated valve control using IoT



This system utilizes an ESP32 microcontroller to track key water quality metrics, including pH, TDS, EC, and temperature. By responding to sensor input, it controls three valves — managing water flow in different directions depending on the detected quality. The system is accessible remotely via a mobile app, offering real-time monitoring and control features.

#Advantages
Tracks key water parameters: pH, TDS, EC, and temperature.
Manages valve operations: input, sewage, and chemical discharge.
Includes a mode for sensor calibration.
Offers remote access and control through a mobile interface.

Hardware Components


ESP32 for managing operations.
Sensors: pH, TDS, EC, and temperature.
Valves controlled by the system.
Mobile app interface for monitoring and control.

Setup Steps

Insert the wireless network name and password in the microcontroller code.
Use the provided mobile application to check values and operate the valves.

Functionality

The system performs readings at fixed intervals and adjusts valves as needed:
pH: Within a specific range (6.0 to 9.0).
TDS: Less than 1000 ppm.
EC: Less than 3500 µS/cm.
When the quality of water doesn't meet the desired standards, the system will open the chemical valve while stopping the sewage valve.

AutoCalibration Mode

Can be initiated via the app.
The system waits for one minute before adjusting sensor readings.
After calibration, the sewage valve is reactivated.
Manual Control of Valves
Valves can be manually adjusted using the application.
Application Configuration

Virtual Pin Assignments:

V1: pH Reading
V2: TDS Reading
V3: Temperature Reading
V4: Sewage Valve
V5: Chemical Valve
V6: Input Valve
V7: EC Reading
V8: Calibration Trigger
V9: Manual Control of Chemical Valve

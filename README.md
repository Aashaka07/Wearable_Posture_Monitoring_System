# Wearable Posture Detection and Monitoring 

### ESP32 & Dual-IMU Edge AI Posture Detection System

 IoT wearable prototype that monitors back posture using two MPU6050 inertial measurement units (IMUs) connected to an ESP32. It estimates posture-related angles, classifies posture on-device using a lightweight Decision Tree model, and streams real-time status and alerts to a Blynk dashboard over Wi-Fi.

> **Project status:** Prototype / active development. Test the complete system on your own hardware before describing any feature as fully working.

---

## Features

- **Dual-Sensor Tracking:** Uses upper-back and lower-back MPU6050 IMUs over a shared I²C bus.
- **Angle Estimation:** Estimates roll and pitch using accelerometer and gyroscope readings with a complementary filter.
- **Baseline Calibration:** Calibrates a personal upright-posture baseline on startup or command.
- **On-Device Classification:** Real-time inference classifying posture into:
  - `GOOD` — Upright posture
  - `SLOUCH` — Leaning forward / rounded back
  - `SIDE LEAN` — Leaning sideways
- **Temporal Smoothing:** Majority voting over a rolling window (25 samples) to stabilize predictions.
- **Cloud Telemetry:** Sends posture status, bad-posture duration, and alert state to Blynk over Wi-Fi.
- **Python ML Pipeline:** Scripts for collecting data, evaluating sessions, training a model, exporting the Decision Tree to C/C++ (`tree.h`), and testing live predictions.
- **Voice Alerts:** Includes an optional desktop voice-alert script.

---

## System Architecture

```text
Upper-back MPU6050 ─┐
                    ├── I²C ──> ESP32
Lower-back MPU6050 ─┘             │
                                  ├── Sensor calibration and angle estimation
                                  ├── Feature extraction
                                  ├── Decision Tree inference (tree.h)
                                  ├── Majority-vote smoothing
                                  │
                                  └── Wi-Fi ──> Blynk Cloud ──> Dashboard

Python tools on a computer:
  Data collection ──> CSV sessions ──> Preprocessing/evaluation
                                      └──> Model training ──> tree.h
```

The ESP32 firmware performs the final posture prediction locally. A Raspberry Pi is not required by this project.

---

## Hardware Configuration

- ESP32 development board
- 2 × MPU6050 modules
- Jumper wires and wearable harness/mounting arrangement
- Computer for Arduino firmware development, Python model tools, and optional voice alerts
- Wi-Fi connection for Blynk connectivity

### MPU6050 Pinout & Wiring

| Signal | ESP32 Connection | Notes |
| :--- | :--- | :--- |
| **SDA** | GPIO 21 | Shared I²C Data line |
| **SCL** | GPIO 22 | Shared I²C Clock line |
| **Upper IMU Address** | `0x68` | Connect AD0 to GND |
| **Lower IMU Address** | `0x69` | Connect AD0 to 3.3V |

*Connect both sensors to the same I²C bus. Confirm the pinout of your specific ESP32 board and sensor modules before powering the circuit. Use compatible voltage levels and a common ground.*

---

## Repository Structure

```text
PostureSense/
├── data/
│   └── raw/                       # Recorded posture sessions (CSV)
├── firmware/
│   ├── posture_logger/
│   │   ├── posture_logger.ino     # Streams labeled sensor data over serial
│   │   └── imu_pair.h             # Two-IMU reading and feature calculation
│   └── posture_ml_final/
│       ├── posture_ml_final.ino   # On-device posture classification + Blynk
│       ├── imu_pair.h
│       ├── tree.h                 # Generated Decision Tree inference code
│       └── posture_voice_alert.py # Optional computer voice alert
└── ml/
    ├── record_data.py             # Record labeled CSV sessions
    ├── check_session.py           # Inspect session data quality
    ├── compare_sessions.py        # Compare features and evaluate sessions
    ├── improve_slouch.py          # Explore feature choices and model behavior
    ├── export_tree.py              # Export trained model to C/C++
    ├── live_predict.py            # Live prediction on computer via serial
    ├── posture_model.pkl           # Saved scikit-learn model bundle
    └── tree.h                      # Generated model header
```

---

## Software Requirements

- **Arduino IDE** with ESP32 board support
- **Python 3**
- **Python packages:** `numpy`, `pandas`, `scikit-learn`, `joblib`, `pyserial`
- **Blynk Account** and an ESP32-compatible Wi-Fi connection

Install the core Python dependencies:

```bash
python -m pip install numpy pandas scikit-learn joblib pyserial
```

---

## Setup & Deployment

### 1. Configure Credentials Safely

Create a local `secrets.h` file in `firmware/posture_ml_final/`:

```cpp
#pragma once
#define WIFI_SSID "YOUR_WIFI_NAME"
#define WIFI_PASS "YOUR_WIFI_PASSWORD"
#define BLYNK_AUTH_TOKEN "YOUR_BLYNK_DEVICE_TOKEN"
```

*Note: `secrets.h` should be added to your `.gitignore` to prevent committing network credentials.*

### 2. Prepare Blynk Dashboard

Configure the following Virtual Pins in your Blynk project:

| Virtual Pin | Purpose | Type |
| :--- | :--- | :--- |
| `V0` | Current posture class | String |
| `V1` | Continuous bad-posture duration (minutes) | Integer / Double |
| `V2` | Alert indicator state | Integer (0/1) |
| `V3` | Calibration trigger button | Push Button |

### 3. Data Collection & Training Workflow

1. **Upload Logger Firmware:** Open `firmware/posture_logger/posture_logger.ino` in Arduino IDE and upload to ESP32.
2. **Collect Sessions:** Run `python ml/record_data.py session_1` from the `ml/` directory while wearing the prototype. Follow prompts to log `GOOD`, `SLOUCH`, and `SIDE LEAN` actions.
3. **Inspect & Evaluate:**
   ```bash
   python ml/check_session.py session_1
   python ml/compare_sessions.py
   ```
4. **Export Decision Tree:** Export the trained Decision Tree into a C++ header file:
   ```bash
   python ml/export_tree.py
   ```
5. **Deploy Firmware:** Copy the output `tree.h` into `firmware/posture_ml_final/`, then upload `posture_ml_final.ino` to the ESP32.

---

## Data and Model Notes

- **Label Encodings:** `0 = GOOD`, `1 = SLOUCH`, `2 = SIDE LEAN`.
- **Sampling Rate:** Firmware samples at ~50 Hz and processes a rolling window of 25 samples.
- **Medical Disclaimer:** PostureSense is an educational prototype developed for learning and posture awareness; it is not a certified medical device.

---

## References

1. N. H. M. Nor, M. Yakno, N. Abd Ghani, M. H. M. Ariff, N. Fadilah, and Z. Musa, *"Enhancing Sitting Posture Detection and Alert System with Multi-Sensor Integration,"* 2025 IEEE 8th International Conference on Electrical, Control and Computer Engineering (InECCE), Kuantan, Malaysia, 2025.
2. A. Rajitha, N. Mallavarapu, P. S. V. Susmitha, S. Roshini, and P. Tejaswi, *"Posture Correction Device Using ESP32 and MPU6050,"* *International Journal for Research in Applied Science & Engineering Technology (IJRASET)*, vol. 13, Oct. 2025. DOI: [10.22214/ijraset.2025.74767](https://doi.org/10.22214/ijraset.2025.74767).

---

## Troubleshooting

- **MPU6050 Initialization Failure:** Check SDA/SCL connections, power, and ground. Ensure AD0 on one sensor is connected to GND (`0x68`) and the other to 3.3V (`0x69`).
- **Serial Connection Issues:** Ensure no other application (e.g., Arduino Serial Monitor) is occupying the serial port when executing Python scripts.
- **Blynk Connectivity Issues:** Verify Wi-Fi credentials in `secrets.h` and check that virtual pin data types match the firmware macros.

---

## Team

Developed as an Electronics & Communication Engineering IoT Project:

- **Neel Lakhmani**
- **Jay Jain** 
- **Joshi Aashaka**

---

## License

This project is licensed under the [MIT License](LICENSE).
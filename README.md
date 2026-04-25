# AquaNexus

**An IoT and Deep Learning powered automated aquaponics system for real-time water quality monitoring and plant growth stage classification.**

AquaNexus combines fish farming and soil-less plant cultivation into a closed-loop ecosystem, then layers on IoT sensing, cloud dashboards, and computer-vision based plant growth analysis to remove the need for constant manual monitoring. Built as a final-year capstone project at the Thapar Institute of Engineering and Technology.

> **Best model accuracy: 98.73% (DenseNet121)** on plant growth stage classification.

---

## Table of Contents

- [Overview](#overview)
- [System Architecture](#system-architecture)
- [Hardware](#hardware)
- [Repository Structure](#repository-structure)
- [Getting Started](#getting-started)
  - [1. Flash the ESP32 (sensors + ThingSpeak)](#1-flash-the-esp32-sensors--thingspeak)
  - [2. Flash the pump controller](#2-flash-the-pump-controller)
  - [3. Train the deep learning models](#3-train-the-deep-learning-models)
- [Deep Learning Model Comparison](#deep-learning-model-comparison)
- [ThingSpeak Dashboard](#thingspeak-dashboard)
- [Results](#results)
- [Future Work](#future-work)
- [Authors](#authors)
- [License](#license)
- [Acknowledgements](#acknowledgements)

---

## Overview

Traditional aquaponics works, but managing it is painful: constant manual checks of pH, water temperature, turbidity and nutrient levels, plus subjective eyeballing of plant growth. AquaNexus addresses this with three integrated layers:

1. **Hardware / IoT layer** — ESP32 + sensors (pH, TDS, turbidity, DHT11, DS18B20 water temp), pumps, biofilter, grow bed, and aquarium.
2. **Processing / Intelligence layer** — Sensor data acquisition over Wi-Fi, image preprocessing in OpenCV, and deep learning inference (CNN, EfficientNet, DenseNet, LNN) for growth stage classification.
3. **Cloud / UI layer** — ThingSpeak dashboard for real-time visualisation, alerts, and remote control.

### Key Features

- Real-time water quality monitoring (pH, water temp, air temp, humidity, turbidity, TDS, EC)
- Automated pump and circulation control via L298N / motor driver
- Local LCD readout (16x2 I2C) cycling through sensor pages
- Cloud sync to ThingSpeak with field-level uploads
- Plant growth stage classification using four DL architectures, benchmarked head-to-head
- DenseNet121 fine-tuned to 98.73% validation accuracy on the plant growth dataset

---

## System Architecture

```
   ┌─────────────────────────────┐
   │         GROW BED            │
   │  (TDS + water temp sensors) │◄─────────┐
   └──────────────┬──────────────┘          │
                  │ filtered water          │ filtered water + nutrients
                  ▼                         │
   ┌─────────────────────────────┐    ┌─────┴────────────┐
   │          AQUARIUM           │───►│  BIOFILTRATION   │
   │  (turbidity, level sensors) │    │  (pH sensor)     │
   └──────────────┬──────────────┘    └──────────────────┘
                  │
                  │ raw sensor data
                  ▼
   ┌──────────────────────────────────┐
   │   ESP32 MICROCONTROLLER          │
   │   - Reads all sensors            │
   │   - Drives LCD                   │
   │   - POSTs to ThingSpeak via WiFi │
   └──────────────┬───────────────────┘
                  │
                  ▼
   ┌──────────────────────────────────┐         ┌──────────────────────┐
   │       THINGSPEAK CLOUD           │────────►│  Dashboard / Alerts  │
   └──────────────────────────────────┘         └──────────────────────┘

   ┌──────────────────────────────────┐
   │  RASPBERRY PI / LOCAL MACHINE    │
   │  - Camera capture                │
   │  - DenseNet121 inference         │
   │  - Growth stage classification   │
   └──────────────────────────────────┘
```

---

## Hardware

| Component                   | Purpose                                       |
| --------------------------- | --------------------------------------------- |
| ESP32                       | Main microcontroller (Wi-Fi + sensor hub)     |
| Raspberry Pi (optional)     | Image capture + local DL inference            |
| DHT11                       | Air temperature & humidity                    |
| DS18B20                     | Water temperature (OneWire)                   |
| Turbidity sensor (analog)   | Water clarity (NTU)                           |
| TDS sensor (analog)         | Total dissolved solids → EC, ppm              |
| pH sensor                   | Biofilter water acidity                       |
| Water level sensor          | Aquarium fill level                           |
| 16x2 I2C LCD (0x27)         | Local readout                                 |
| L298N / motor driver        | Drives 3 DC pumps with PWM                    |
| Pumps × 3, air pump         | Water circulation, aeration                   |
| LED grow light, exhaust fan | Photosynthesis support, humidity control      |
| Camera module               | Plant image capture for DL pipeline           |

### ESP32 Pin Map

| Function               | GPIO   |
| ---------------------- | ------ |
| DHT11 data             | GPIO26 |
| DS18B20 (OneWire)      | GPIO23 |
| Turbidity (analog in)  | GPIO34 |
| TDS (analog in)        | GPIO35 |
| I2C LCD                | SDA / SCL (default) |

---

## Repository Structure

```
Aquanexus/
├── all_sensors_final.ino     # ESP32: reads all sensors, drives LCD, uploads to ThingSpeak
├── motor.ino                 # ESP8266/Arduino: 3-pump PWM controller
├── densenet.ipynb            # DenseNet121 training, fine-tuning, eval, confusion matrix
├── Efficient_Man (2).ipynb   # EfficientNet baseline + training
├── LNN_Man (1).ipynb         # Lightweight Neural Network experiment
├── gitattributes
└── README.md
```

> **Note on the dataset:** the notebooks expect a folder named `dataset/` in the working directory, organised as one subfolder per growth stage class (Keras `image_dataset_from_directory` convention). The dataset is not included in this repo — the original work used a publicly available Mendeley plant growth dataset.

---

## Getting Started

### Prerequisites

- **Hardware side:** Arduino IDE 2.x with the ESP32 board package installed.
- **ML side:** Python 3.9+, TensorFlow 2.10+, Jupyter or Google Colab. A GPU is strongly recommended for DenseNet121 training.

### 1. Flash the ESP32 (sensors + ThingSpeak)

Install the following Arduino libraries via the Library Manager:

- `WiFi` (built-in for ESP32)
- `DHT sensor library` by Adafruit
- `OneWire`
- `DallasTemperature`
- `ThingSpeak`
- `LiquidCrystal_I2C`

Then in `all_sensors_final.ino` update your credentials:

```cpp
const char* ssid    = "YOUR_WIFI_SSID";
const char* pass    = "YOUR_WIFI_PASSWORD";
String      apiKey  = "YOUR_THINGSPEAK_WRITE_API_KEY";
```

> ⚠️ The committed file contains placeholder credentials from development. **Rotate the ThingSpeak write key before pushing your fork to a public repo.**

Select **ESP32 Dev Module** as the board, pick the right COM port, upload, and open the Serial Monitor at **115200 baud** to verify the sensor readings and the ThingSpeak HTTP response.

### 2. Flash the pump controller

`motor.ino` is written for an ESP8266-style board (uses `D0`–`D8` and 0–1023 PWM range). Open it in Arduino IDE, select your ESP8266 board, and upload. The default behaviour cycles all 3 pumps on for 10 s and off for 5 s — adjust `analogWrite()` values and delays to suit your circulation needs.

### 3. Train the deep learning models

```bash
git clone
cd Aquanexus
pip install tensorflow matplotlib numpy pillow
```

Place your image dataset in a `dataset/` folder, with one subfolder per class:

```
dataset/
├── seedling/
├── vegetative/
├── flowering/
└── harvest/
```

Then run the notebooks:

- `densenet.ipynb` — best performer; uses DenseNet121 with ImageNet weights, trained in two phases (head only, then full fine-tune at `lr=1e-5`)
- `Efficient_Man (2).ipynb` — EfficientNet baseline
- `LNN_Man (1).ipynb` — lightweight neural network for comparison

The DenseNet notebook saves checkpoints to `best_densenet_model.h5` and includes confusion matrix plus per-class precision / recall / F1 reporting.

---

## Deep Learning Model Comparison

| Model           | Validation Accuracy | Validation Loss | Notes                                          |
| --------------- | ------------------- | --------------- | ---------------------------------------------- |
| **DenseNet121** | **98.73%**          | **0.156**       | Best — feature reuse + strong gradient flow    |
| EfficientNet    | ~92.7%              | 0.179           | Close second, slightly faster inference        |
| CNN (custom)    | ~92.5%              | 0.684           | Stable baseline, higher loss                   |
| LNN             | ~80.8%              | 0.482           | Lightweight; slower convergence, lower ceiling |

DenseNet's dense connectivity pattern lets it pick up subtle texture and color cues across plant growth stages without the gradient degradation seen in plain deep CNNs — which is why it pulled ahead on this dataset.

---

## ThingSpeak Dashboard

The ESP32 firmware uploads six fields:

| Field    | Quantity              | Unit    |
| -------- | --------------------- | ------- |
| field1   | Air temperature       | °C      |
| field2   | Humidity              | %       |
| field3   | Water temperature     | °C      |
| field4   | Turbidity             | NTU     |
| field5   | TDS                   | ppm     |
| field6   | Electrical conductivity | µS/cm |

Create a channel on [ThingSpeak](https://thingspeak.com/), copy the Write API Key into the sketch, and add chart widgets for each field. Set the channel to public if you want to share the live dashboard.

---

## Results

- **DenseNet121 reached 98.73% validation accuracy** on plant growth stage classification.
- End-to-end pipeline (sensor → cloud → DL inference → dashboard) ran continuously during testing without manual intervention.
- Closed-loop water reuse cut water consumption versus traditional soil farming significantly — a known aquaponics benefit, validated in our setup.

See the project report (capstone document) for full confusion matrices, training curves, and risk analysis.

---

## Future Work

- Add dissolved oxygen and nitrate-specific sensors
- Time-series forecasting for early anomaly detection (LSTM / Prophet on ThingSpeak history)
- Mobile app (Flutter / React Native) replacing the ThingSpeak web dashboard
- Solar power integration for off-grid deployment
- Edge inference on Raspberry Pi 5 or Jetson Nano so DL runs without internet
- Scale up to multi-tank commercial deployment

---

## Authors

| Name                  
| Manish Kumar          

**Mentor:** Dr. Anamika Sharma, Assistant Professor, CSED, TIET Patiala
**Department:** Computer Science and Engineering, Thapar Institute of Engineering and Technology, Patiala

---

## License

Released for academic and educational use. If you build on this work, please cite the project and credit the authors.

---

## Acknowledgements

- Dr. Anamika Sharma for mentorship throughout the capstone
- TIET CSED department for hardware and lab support
- Mendeley Data for the publicly available plant growth dataset
- The ThingSpeak, TensorFlow/Keras, and Arduino communities

---

*If this project helped you, consider giving the repo a ⭐ — it makes it easier for other students and aquaponics tinkerers to find.*

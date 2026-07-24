# TinyML-Resistor-Classifier


[![Board](https://img.shields.io/badge/Board-Seeed%20XIAO%20ESP32S3%20Sense-brightgreen)](https://www.seeedstudio.com/XIAO-ESP32S3-Sense-p-5639.html)
[![Framework](https://img.shields.io/badge/ML-TensorFlow%20Lite%20Micro-orange)](https://www.tensorflow.org/lite/microcontrollers)
[![Accuracy](https://img.shields.io/badge/INT8%20Accuracy-99.74%25-blue)]()
[![License](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)

An end-to-end TinyML edge vision system designed to automate resistor identification and sorting in electronics laboratories. Powered by an **XIAO ESP32S3 Sense**, **MobileNetV2 (α = 0.50)**, and **TFLite Micro**, the system performs real-time optical resistor classification directly on microcontrollers in **~1.2 seconds**.

---

## Demo

Working demonstration of the 10-class resistor classifier running an esp32

https://github.com/user-attachments/assets/1d65bf31-cd97-47a0-a1eb-eec64f69841b

---


## 📌 Problem & Context

In university and maker electronics labs, sorting loose 4-band resistors is a notoriously tedious process. Because manual verification using digital multimeters (DMM) takes significant time, components frequently end up mixed in wrong bins or discarded. Misidentified resistors lead to circuit malfunctioning or board damage.

Standard computer vision solutions often rely on high-resolution cloud processing or object-detection bounding boxes, which are computationally prohibitive on low-power edge microcontrollers. **Ohm Sweet Ohm** addresses this challenge with a compact, standalone, and cost-effective hardware assembly running a custom-quantized image classification model tailored for hardware-constrained micro-cameras.

---

## 🛠 Hardware Architecture & System Setup

| Component | Specification / Function |
| :--- | :--- |
| **Microcontroller & Camera** | Seeed Studio XIAO ESP32S3 Sense with OV3660 camera sensor |
| **Display** | Adafruit ST7735R 1.8" TFT SPI display (top-mounted for easy viewer visibility) |
| **Enclosure / Rig** | Custom standoffs calibrated to fix camera distance, balancing resolution and focus depth |
| **Firmware Stack** | C++ compiled via PlatformIO / VSCode, leveraging TFLite Micro C++ runtime |

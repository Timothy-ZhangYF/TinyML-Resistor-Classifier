 TinyML-Resistor-Classifier

[![Board](https://img.shields.io/badge/Board-Seeed%20XIAO%20ESP32S3%20Sense-brightgreen)](https://www.seeedstudio.com/XIAO-ESP32S3-Sense-p-5639.html)
[![Framework](https://img.shields.io/badge/ML-TensorFlow%20Lite%20Micro-orange)](https://www.tensorflow.org/lite/microcontrollers)
[![Model](https://img.shields.io/badge/Model-MobileNetV2%20%CE%B1%3D0.50-yellow)]()
[![Accuracy](https://img.shields.io/badge/INT8%20Accuracy-99.74%25-blue)]()


An end-to-end TinyML edge vision system designed to automate resistor identification and sorting in electronics laboratories. Powered by an **XIAO ESP32S3 Sense**, **MobileNetV2 (α = 0.50)**, and **TFLite Micro**, the system performs real-time optical resistor classification directly on microcontrollers in **~1.2 seconds**.

---

## 🎬 Hardware Demonstration

> **Real-time inference running on Seeed XIAO ESP32S3 Sense (~1.2s latency, 99.74% accuracy):**

https://github.com/user-attachments/assets/1d65bf31-cd97-47a0-a1eb-eec64f69841b

*Captured live with top-mounted SPI display outputting predicted resistance class and model confidence score (video playback is sped up 3x).* 

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

```
+-------------------------------------------------------------+
|                     Ohm Sweet Ohm Rig                       |
|                                                             |
|   +-----------------------------------------------------+   |
|   |         Adafruit ST7735R Display (SPI)             |   |
|   |         Outputs: Resistor Class + Confidence        |   |
|   +-----------------------------------------------------+   |
|                              ^                              |
|                              | (SPI Output)                 |
|   +-----------------------------------------------------+   |
|   |           XIAO ESP32S3 Sense Microcontroller        |   |
|   |   - Captures QVGA (320x240) image via OV3660        |   |
|   |   - Rescales & normalizes image tensor              |   |
|   |   - Runs INT8 Quantized TFLite Micro Model          |   |
|   +-----------------------------------------------------+   |
|                              |                              |
|             (Fixed Height Mounting Standoffs)               |
|                              v                              |
|                    [ Target Resistor ]                      |
+-------------------------------------------------------------+
```

---

## 📊 Dataset & Engineering Strategy

Because deployable micro-cameras yield noisy, out-of-focus, or lower-quality images compared to high-end benchmark datasets, we collected a custom **3,765-image dataset** matching the exact deployment setup:

* **Capture Environment:** Captured using the native **OV3660** sensor at `FRAMESIZE_QVGA` ($320 \times 240$), capturing identical focal distance, lighting, and shadow variations as inference time.
* **Class Breakdown:** Covers 9 distinct 4-band resistor classes (including $10\times$ multiplier pairs with 3 identical bands to test color granularity) plus 1 `Idle` (no resistor) state.
  * **Classes:** $47\Omega$, $220\Omega$, $390\Omega$, $1.5\text{k}\Omega$, $5.6\text{k}\Omega$, $6.8\text{k}\Omega$, $180\text{k}\Omega$, $560\text{k}\Omega$, $4.7\text{M}\Omega$, `Idle`
* **Data Augmentation & Diversity:** Automated script logged 100 images per run over Serial at 1s intervals across varying rotations, offsets, leg-bend configurations, and lighting setups.

---

## 🧠 Model Architecture & Edge Optimization

Rather than using frozen transfer learning weights (which bottlenecked post-quantization accuracy to ~77%), the model features a full end-to-end retraining of MobileNetV2 feature extraction layers adapted for micro-vision:

1. **Feature Extractor:** MobileNetV2 with width multiplier **$\alpha = 0.50$** (Input: $224 \times 224 \times 3$).
2. **Pooling & Regularization:** Global Average Pooling 2D + $0.35$ Dropout rate.
3. **Classification Layer:** Single Dense layer outputting 10 logits.
4. **Quantization:** Full integer 8-bit (`INT8`) quantization for microcontroller RAM/Flash efficiency.

```
Model Summary (MobileNetV2 α=0.50):
=================================================================
Layer (type)                 Output Shape              Param #   
=================================================================
mobilenetv2_0.50_224         (None, 7, 7, 1280)        706,224   
global_average_pooling2d     (None, 1280)              0         
dropout                      (None, 1280)              0         
dense                        (None, 10)                12,810    
=================================================================
Total params: 719,034 (2.74 MB)
Trainable params: 700,490 (2.67 MB)
Non-trainable params: 18,544 (72.44 KB)
```

### Trade-off Evaluation

| Architecture | Model Parameters | INT8 Quantized Accuracy | Hardware Inference Time | Trade-off Notes |
| :--- | :--- | :--- | :--- | :--- |
| **MobileNetV2 (α = 0.35)** | ~423K | 98.69% | **~0.9 s** | Faster, but lower confidence on difficult classes (e.g. $4.7\text{M}\Omega$) |
| **MobileNetV2 (α = 0.50)** | **~719K** | **99.74%** | **~1.2 s** | **Selected**: Superior confidence and robustness across all classes |

---

## 📈 Performance & Results

The finalized INT8 quantized model achieved **99.74% accuracy** on test datasets, demonstrating near-perfect class separation across identical-looking component packages.

```
TFLite INT8 Confusion Matrix (381 Test Samples):
       0   1   2   3   4   5   6   7   8   9  (Predicted)
 0   [40   0   0   0   0   0   0   0   0   0]  (47 Ω)
 1   [ 0  38   0   0   0   0   0   0   0   0]  (220 Ω)
 2   [ 0   0  35   0   0   0   0   0   0   0]  (390 Ω)
 3   [ 0   0   0  37   0   0   0   0   0   0]  (1.5 kΩ)
 4   [ 0   0   0   0  38   0   0   0   0   0]  (5.6 kΩ)
 5   [ 0   0   0   1   0  34   0   0   0   0]  (6.8 kΩ)
 6   [ 0   0   0   0   0   0  38   0   0   0]  (180 kΩ)
 7   [ 0   0   0   0   0   0   0  40   0   0]  (560 kΩ)
 8   [ 0   0   0   0   0   0   0   0  40   0]  (4.7 MΩ)
 9   [ 0   0   0   0   0   0   0   0   0  40]  (Idle)
```

---

## 🚀 Repository Structure

```
.
├── firmware/                   # PlatformIO C++ firmware project
│   ├── src/
│   │   ├── main.cpp            # Application logic & display SPI drivers
│   │   └── model.cc            # Exported TFLite Micro INT8 model byte array
│   └── platformio.ini          # ESP32S3 environment & lib dependencies
├── models/                     # Notebooks & exported models
│   ├── train_mobilenet.ipynb   # Dataset loader, full training & quantization pipeline
│   └── resistor_model_int8.tflite
├── dataset/                    # Sample images and structure
└── docs/                       # Assembly photos & schematic diagrams
```

---

## ⚡ Quickstart & Deployment

1. **Firmware Setup:**
   * Open the `firmware/` directory in **VSCode** with **PlatformIO** installed.
   * Build and flash the project onto your Seeed XIAO ESP32S3 Sense:
     ```bash
     pio run --target upload
     ```

2. **Model Training & Quantization:**
   * Launch `models/train_mobilenet.ipynb` in Google Colab or Jupyter.
   * Run all cells to train the MobileNetV2 backbone, evaluate accuracy, export INT8 `.tflite`, and generate C++ byte array headers (`model.cc`).

---

## 👥 Authors & Acknowledgments

* **Timothy Zhang** & **James Steeman** – Developed for *ESE3600: Tiny Machine Learning*.

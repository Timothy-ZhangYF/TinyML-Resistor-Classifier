//
// ESP32-S3 XIAO Camera — MobileNet 224×224×3 TFLite Inference
// Final Project Ohm Sweet Ohm
//

#include "esp_camera.h"
#include "esp_timer.h"
#include "img_converters.h"
#include "Arduino.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

#include "NeuralNetwork.h"
#define CAMERA_MODEL_XIAO_ESP32S3
#include "camera_pins.h"

#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>

#define LED_PIN 21    // built-in LED on XIAO
#define TARGET_W 224
#define TARGET_H 224

//SPI ST7735 TFT Display pins
#define TFT_RST D5
#define TFT_DC D6
#define TFT_CS D7
#define TFT_SCLK D8 
#define TFT_MOSI D10

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

NeuralNetwork *g_nn;

String labels[] = {"180K", "1.5K", "220", "390", "47", "4.7M", "560K", "5.6K", "6.8K", "IDLE "};

//
// Convert RGB565 → RGB888 float
//
static inline void unpack_rgb565(uint16_t p, uint8_t &r, uint8_t &g, uint8_t &b) {
    r = ((p >> 11) & 0x1F) << 3;
    g = ((p >> 5) & 0x3F) << 2;
    b = (p & 0x1F) << 3;
}

//
// Resize from QVGA → 224x224 using center-crop + nearest neighbor
//
void preprocess_to_tensor(camera_fb_t *fb, TfLiteTensor *input) {
    const int src_w = fb->width;
    const int src_h = fb->height;
    const int dst_w = TARGET_W;
    const int dst_h = TARGET_H;

    uint8_t *dst = input->data.uint8; 

    // We access the raw buffer as bytes to handle the Endianness manually
    uint8_t *src_bytes = (uint8_t *)fb->buf;

    int crop_y = (src_h - dst_h) / 2;
    int crop_x = (src_w - dst_w) / 2;

    for (int y = 0; y < dst_h; y++) {
        for (int x = 0; x < dst_w; x++) {
            
            // Calculate the index in the raw buffer (2 bytes per pixel)
            int src_idx = ((y + crop_y) * src_w + (x + crop_x)) * 2;

            // ESP32-S3 Camera usually sends Big Endian, but we read Little Endian.
            // We manually construct the 16-bit pixel.
            uint8_t b1 = src_bytes[src_idx];
            uint8_t b2 = src_bytes[src_idx+1];
            
            // Swap these if the colors look inverted/purple-green
            uint16_t p565 = (b1 << 8) | b2; 

            // Unpack RGB565 to RGB888
            uint8_t r = ((p565 >> 11) & 0x1F) << 3;
            uint8_t g = ((p565 >> 5) & 0x3F) << 2;
            uint8_t b = (p565 & 0x1F) << 3;

            int dst_i = (y * dst_w + x) * 3;

            // Scale is 1/255 and ZeroPoint is 0. 
            dst[dst_i + 0] = r;
            dst[dst_i + 1] = g;
            dst[dst_i + 2] = b;
        }
    }
}

void setup() {
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

    Serial.begin(115200);
    delay(2000);

    pinMode(LED_PIN, OUTPUT); 
    digitalWrite(LED_PIN, HIGH);

    // TFT setup
    tft.initR(INITR_BLACKTAB); 
    tft.fillScreen(ST7735_BLACK);
    tft.setTextSize(2);
    tft.setTextColor(ST7735_WHITE);

    // Camera config
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer   = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk     = XCLK_GPIO_NUM;
    config.pin_pclk     = PCLK_GPIO_NUM;
    config.pin_vsync    = VSYNC_GPIO_NUM;
    config.pin_href     = HREF_GPIO_NUM;
    config.pin_sccb_sda = SIOD_GPIO_NUM;
    config.pin_sccb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn     = PWDN_GPIO_NUM;
    config.pin_reset    = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;

    config.frame_size   = FRAMESIZE_QVGA;   // 320×240
    config.pixel_format = PIXFORMAT_RGB565;

    config.grab_mode    = CAMERA_GRAB_WHEN_EMPTY;
    config.fb_location  = CAMERA_FB_IN_PSRAM;
    config.jpeg_quality = 12;
    config.fb_count     = 1;

    if (esp_camera_init(&config) != ESP_OK) {
        Serial.println("Camera init FAILED!");
        while (1);
    }
    Serial.println("Camera init OK!");

    // Load your MobileNet model
    g_nn = new NeuralNetwork();
    Serial.println("--- MODEL INFO ---");
    if (g_nn->getInput()->type == kTfLiteInt8) Serial.println("Input Type: Int8 (Signed)");
    else if (g_nn->getInput()->type == kTfLiteUInt8) Serial.println("Input Type: UInt8 (Unsigned)");
    
    Serial.printf("Input Scale: %.5f\n", g_nn->getInput()->params.scale);
    Serial.printf("Input Zero: %d\n", g_nn->getInput()->params.zero_point);
    Serial.println("------------------");
    Serial.println("Model initialized.");

    
}

// ASCII Art characters from dark to light
const char *ascii_chars = " .:-=+*#%@";

void debug_view_image(TfLiteTensor *input) {
    int h = 224; 
    int w = 224; 
    int channels = 3;
    
    // We use uint8 because your logs confirmed type 3!
    uint8_t *data = input->data.uint8; 

    Serial.println("\n--- MODEL 'VISION' DEBUG ---");
    
    // Downsample (print every 5th row, 3rd col) so it fits in Serial Monitor
    for (int y = 0; y < h; y += 5) {
        for (int x = 0; x < w; x += 3) {
            int idx = (y * w + x) * channels;
            
            // Get RGB
            uint8_t r = data[idx];
            uint8_t g = data[idx+1];
            uint8_t b = data[idx+2];
            
            // Simple grayscale for ASCII
            int gray = (r + g + b) / 3;
            int char_idx = map(gray, 0, 255, 0, 9);
            
            Serial.print(ascii_chars[char_idx]);
        }
        Serial.println(); 
    }
    
    int c_idx = ((h/2) * w + (w/2)) * 3;
    Serial.printf("Center Pixel: R=%d  G=%d  B=%d\n", 
                  data[c_idx], data[c_idx+1], data[c_idx+2]);
    Serial.println("----------------------------");
}


void loop() {
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("CAMERA ERROR");
        delay(100);
        return;
    }

    if (fb->format != PIXFORMAT_RGB565) {
        esp_camera_fb_return(fb);
        return;
    }

    // 1. Preprocess
    uint64_t t0 = esp_timer_get_time();
    preprocess_to_tensor(fb, g_nn->getInput());
    uint64_t t_prep = (esp_timer_get_time() - t0) / 1000;

    // ... after preprocess_to_tensor ...
    preprocess_to_tensor(fb, g_nn->getInput());

    // 2. Predict
    uint64_t t1 = esp_timer_get_time();
    g_nn->predict();
    uint64_t t_inf = (esp_timer_get_time() - t1) / 1000;

    esp_camera_fb_return(fb);

    // 3. Process Output (UInt8)
    TfLiteTensor* output = g_nn->getOutput();
    int num_classes = output->dims->data[output->dims->size - 1];
    
    uint8_t *out_data = output->data.uint8;
    float scale = output->params.scale;
    int zero_point = output->params.zero_point;

    int best_class = -1;
    float best_prob = -1.0f;

    Serial.printf("Prep=%llums  Infer=%llums\n", t_prep, t_inf);
    Serial.print("Output: [ ");

    for (int i = 0; i < num_classes; i++) {
        // Dequantize: (Value - Zero) * Scale
        float prob = (out_data[i] - zero_point) * scale;
        Serial.printf("%.3f ", prob);
        
        if (prob > best_prob) {
            best_prob = prob;
            best_class = i;
        }
    }
    Serial.printf("] -> Prediction: Class %d\n", best_class);

    //print on TFT
    tft.fillScreen(ST7735_BLACK);
    tft.setCursor(0,40);
    tft.println("Resistor:");
    tft.print(labels[best_class]);
    if (best_class != 9) {
        tft.println(" Ohm");
    } else {
        tft.println();
    }
    tft.println();
    tft.println("Conf:");
    tft.println(best_prob);

    //delay(50);
}

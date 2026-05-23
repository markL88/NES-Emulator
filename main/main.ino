#include <esp_wifi.h>
#include <esp_task_wdt.h>
#include <FFat.h>
#include <SPIFFS.h>
#include <SD.h>
#include <SD_MMC.h>
#include <Arduino_GFX_Library.h>
#include "hw_config.h"
#include <Adafruit_NeoPixel.h>
#define RGB_PIN 48
Adafruit_NeoPixel rgb_led(1, RGB_PIN, NEO_GRB + NEO_KHZ800);
extern "C" void draw_image_from_sd(const char *filename, int x, int y, int width, int height);

#include <driver/i2s.h>

// Initialize the I2S driver for the Menu
void init_menu_audio() {
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = 44100,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 4,
        .dma_buf_len = 128,
        .use_apll = false,
        .tx_desc_auto_clear = true
    };
    
    i2s_pin_config_t pin_config = {
        .mck_io_num = I2S_PIN_NO_CHANGE, // Protects the S3
        .bck_io_num = 46,
        .ws_io_num = 47,
        .data_out_num = 45,
        .data_in_num = I2S_PIN_NO_CHANGE
    };
    

    // ==========================================
    // NEW DEBUG SECTION
    // ==========================================
    Serial.println("Attempting to start I2S...");
    
    esp_err_t err = i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    
    if (err == ESP_OK) {
        Serial.println("I2S Driver Installed Successfully!");
    } else {
        Serial.println("I2S DRIVER FAILED TO INSTALL!");
    }
    
    i2s_set_pin(I2S_NUM_0, &pin_config);
    i2s_zero_dma_buffer(I2S_NUM_0);
}

// Generates a crisp, 7-millisecond synthesized "Tick"
void play_menu_tick() {
    size_t bytes_written;
    int16_t sample = 0;
    
    // 300 samples at 44.1kHz is extremely fast
    for (int i = 0; i < 500; i++) {
        // Toggle every 15 samples to create a high-pitched click.
        // Amplitude is 600 (out of 32767) for a soft UI volume.
        sample = ((i / 100) % 2 == 0) ? 10000 : -10000; 
        uint32_t sample32 = ((uint32_t)(uint16_t)sample << 16) | (uint16_t)sample;
        i2s_write(I2S_NUM_0, &sample32, sizeof(sample32), &bytes_written, portMAX_DELAY);
    }
}

void play_select_tick() {
    size_t bytes_written;
    int16_t sample = 0;
    
    // Note 1: C5 (Low Note - Divisor 42)
    // 3000 samples = ~68 milliseconds
    for (int i = 0; i < 3000; i++) {
        sample = ((i / 42) % 2 == 0) ? 2000 : -2000; 
        uint32_t sample32 = ((uint32_t)(uint16_t)sample << 16) | (uint16_t)sample;
        i2s_write(I2S_NUM_0, &sample32, sizeof(sample32), &bytes_written, portMAX_DELAY);
    }

    // Note 2: G5 (Middle Note - Divisor 28)
    // 3000 samples = ~68 milliseconds
    for (int i = 0; i < 3000; i++) {
        sample = ((i / 28) % 2 == 0) ? 2000 : -2000; 
        uint32_t sample32 = ((uint32_t)(uint16_t)sample << 16) | (uint16_t)sample;
        i2s_write(I2S_NUM_0, &sample32, sizeof(sample32), &bytes_written, portMAX_DELAY);
    }

    // Note 3: C6 (High Octave - Divisor 21)
    // 6000 samples = ~136 milliseconds (Held twice as long for emphasis!)
    for (int i = 0; i < 6000; i++) {
        sample = ((i / 21) % 2 == 0) ? 2000 : -2000; 
        uint32_t sample32 = ((uint32_t)(uint16_t)sample << 16) | (uint16_t)sample;
        i2s_write(I2S_NUM_0, &sample32, sizeof(sample32), &bytes_written, portMAX_DELAY);
    }
}

extern "C" {
#include <nofrendo.h>
}

// 1. ONE GLOBAL DEFINITION (Remove any other 'extern FS' or 'FS &filesystem' lines)
FS &filesystem = SD; 

// 2. HEX COLORS (Core 3.x needs these)
#define NES_BLACK 0x0000
#define NES_GREEN 0x07E0
#define NES_RED   0xF800

int16_t bg_color;
extern Arduino_TFT *gfx;
extern void display_begin();

// ==========================================
// THE ROM MENU SYSTEM
// ==========================================

String show_game_menu() {
    String games[20]; 
    int gameCount = 0;

    // Use the filesystem defined in hw_config.h
    File root = filesystem.open("/");
    while (true) {
        File entry = root.openNextFile();
        if (!entry) break; 
        if (!entry.isDirectory()) {
            String filename = entry.name();
            String lowerFilename = filename;
            lowerFilename.toLowerCase();
            if (lowerFilename.endsWith(".nes")) { 
                games[gameCount] = filename;
                gameCount++;
                if (gameCount >= 20) break; 
            }
        }
        entry.close();
    }

    if (gameCount == 0) {
        gfx->setTextColor(NES_RED); // Red
        gfx->setCursor(10, 10);
        gfx->println("No .nes files found!");
        while(true); 
    }

    // ==========================================
    // 1. DRAW STATIC GRAPHICS ONCE
    // ==========================================
    gfx->fillScreen(NES_BLACK);   // Clear the screen to Black

    // --> NEW LED CODE HERE <--
    rgb_led.begin();
    rgb_led.setBrightness(1); // 15/255 = very dim, safe to look at
    rgb_led.setPixelColor(0, rgb_led.Color(0, 255, 20)); // Gameboy Green
    rgb_led.show(); // Push the color to the hardware
    
    // Draw the Title
    gfx->setTextColor(NES_GREEN, NES_BLACK); // Green on Black
    gfx->setCursor(10, 10);
    gfx->println("SELECT NES GAME");
    gfx->drawLine(10, 30, 200, 30, NES_GREEN);
    
    // Draw the Logo on the right side of the screen!
    draw_image_from_sd("/logo.bin", 210, 10, 100, 100); 
    gfx->drawRect(209, 9, 102, 102, NES_GREEN);

    int selectedIndex = 0;
    bool redraw = true;

    init_menu_audio(); //menu sound

    while (true) {
        // ==========================================
        // 2. ONLY REDRAW THE LIST WHEN SCROLLING
        // ==========================================
        if (redraw) {
            for (int i = 0; i < gameCount; i++) {
                gfx->setCursor(20, 40 + (i * 20));
                if (i == selectedIndex) {
                    gfx->setTextColor(NES_BLACK, NES_GREEN); 
                } else {
                    gfx->setTextColor(NES_GREEN, NES_BLACK);
                }
                
                // Add blank spaces to the end of the text. 
                // This forces the ESP32 to draw "black space" over the old highlight box!
                String displayName = games[i];
                while (displayName.length() < 15) {
                    displayName += " ";
                }
                gfx->println(displayName);
            }
            redraw = false;

            delay(100);
        }

        // ==========================================
        // 3. INPUT HANDLING
        // ==========================================


        if (digitalRead(HW_CONTROLLER_GPIO_DOWN) == LOW) {
            selectedIndex++;
            if (selectedIndex >= gameCount) selectedIndex = 0; 
            redraw = true;
            play_menu_tick(); // <-- NEW: Play the blip!
        }
        if (digitalRead(HW_CONTROLLER_GPIO_UP) == LOW) {
            selectedIndex--;
            if (selectedIndex < 0) selectedIndex = gameCount - 1; 
            redraw = true;
            play_menu_tick(); // <-- NEW: Play the blip!
        }
        if (digitalRead(HW_CONTROLLER_GPIO_A) == LOW) {
            gfx->fillScreen(NES_BLACK);
            gfx->setTextColor(NES_GREEN);
            gfx->setCursor(10, 10);
            gfx->println("Loading Game...");

            // --> NEW LED CODE HERE <--
            // Switch to a nice rich Blue (R:0, G:50, B:255)
            rgb_led.setPixelColor(0, rgb_led.Color(0, 50, 255));
            rgb_led.show();

            play_select_tick(); //confirmation tick
            delay(100);       // Wait for the sound to finish
            
            // --> CRITICAL NEW LINE <--
            // Hand the car keys back to the system so the emulator doesn't crash!
            i2s_driver_uninstall(I2S_NUM_0);
            
            return String(FSROOT) + "/" + games[selectedIndex]; 
        }
        delay(10);
    }
}

void setup()
{
    Serial.begin(115200);

    // turn off WiFi
    esp_wifi_deinit();

    // disable Core 0 WDT
    //TaskHandle_t idle_0 = xTaskGetIdleTaskHandleForCPU(0);
    //esp_task_wdt_delete(idle_0);

    // start display
    display_begin();

    // Ensure your button pins are set to inputs so the menu works
    pinMode(HW_CONTROLLER_GPIO_UP, INPUT_PULLUP);
    pinMode(HW_CONTROLLER_GPIO_DOWN, INPUT_PULLUP);
    pinMode(HW_CONTROLLER_GPIO_A, INPUT_PULLUP);

    // filesystem defined in hw_config.h
    FILESYSTEM_BEGIN

    File root = filesystem.open("/");
    char *argv[1];
    
    if (!root)
    {
        Serial.println("Filesystem mount failed! Please check hw_config.h settings.");
        gfx->setCursor(10, 10);
        gfx->println("Filesystem mount failed! Please check hw_config.h settings.");
    }
    else
    {
        // Run the custom menu!
        String selectedGame = show_game_menu();
        
        argv[0] = (char*)selectedGame.c_str();

        Serial.println("NoFrendo start!\n");
        // This specific port only takes 1 argument
        nofrendo_main(1, argv);
        Serial.println("NoFrendo end!\n");
    }
}

void loop()
{
}

#include <esp_wifi.h>
#include <esp_task_wdt.h>
#include <FFat.h>
#include <SPIFFS.h>
#include <SD.h>
#include <SD_MMC.h>
#include <Arduino_GFX_Library.h>
#include "hw_config.h"

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

    int selectedIndex = 0;
    bool redraw = true;

    while (true) {
        if (redraw) {
            gfx->fillScreen(NES_BLACK);   // Black
            gfx->setTextColor(NES_GREEN, NES_BLACK); // Green on Black
            gfx->setCursor(10, 10);
            gfx->println("SELECT NES GAME");
            gfx->drawLine(10, 30, 200, 30, NES_GREEN);

            for (int i = 0; i < gameCount; i++) {
                gfx->setCursor(20, 40 + (i * 20));
                if (i == selectedIndex) {
                    gfx->setTextColor(NES_BLACK, NES_GREEN); 
                } else {
                    gfx->setTextColor(NES_GREEN, NES_BLACK);
                }
                gfx->println(games[i]);
            }
            redraw = false;
        }

        // We read the pins exactly as you mapped them in hw_config.h
        if (digitalRead(HW_CONTROLLER_GPIO_DOWN) == LOW) {
            selectedIndex++;
            if (selectedIndex >= gameCount) selectedIndex = 0; 
            redraw = true;
            delay(150); 
        }
        if (digitalRead(HW_CONTROLLER_GPIO_UP) == LOW) {
            selectedIndex--;
            if (selectedIndex < 0) selectedIndex = gameCount - 1; 
            redraw = true;
            delay(150); 
        }
        if (digitalRead(HW_CONTROLLER_GPIO_A) == LOW) {
            gfx->fillScreen(NES_BLACK);
            gfx->setTextColor(NES_GREEN);
            gfx->setCursor(10, 10);
            gfx->println("Booting Nofrendo...");
            
            // Format the path exactly how this library expects it (/fs/game.nes)
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
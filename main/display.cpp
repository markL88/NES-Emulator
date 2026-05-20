extern "C"
{
#include <nes/nes.h>
}

#include "hw_config.h"
#include <Arduino_GFX_Library.h>
#include <SD.h>

// ====================================================
// YOUR CUSTOM HARDWARE CONFIGURATION
// ====================================================

#define TFT_BRIGHTNESS 255 /* 0 - 255 */
#define TFT_BL 8  // Your custom Backlight pin

// The SPI Bus configured exactly to your KiCad schematic
Arduino_DataBus *bus = new Arduino_ESP32SPI(
    5,      /* DC (Data/Command) */ 
    3,      /* CS (Chip Select) */ 
    7,      /* SCK */ 
    6,      /* MOSI */ 
    10      /* MISO (The screen only receives data, so we leave this blank) */
);

// CHOOSE YOUR SCREEN DRIVER:
// If you have a standard 320x240 ILI9341, leave this uncommented:
Arduino_ILI9341 *gfx = new Arduino_ILI9341(bus, 4 /* RST */, 3 /* rotation */);

// If you have a 320x240 ST7789, comment out the line above and uncomment this one:
// Arduino_ST7789 *gfx = new Arduino_ST7789(bus, 4 /* RST */, 3 /* rotation */, true /* IPS */, 320, 240);


// ====================================================
// CORE DISPLAY MATH (No changes needed below here)
// ====================================================

static int16_t w, h, frame_x, frame_y, frame_x_offset, frame_width, frame_height, frame_line_pixels;
extern int16_t bg_color;
extern uint16_t myPalette[];

extern void display_begin()
{
    gfx->begin(40000000);
    
    // Back to the original Dark Grey background
    bg_color = gfx->color565(24, 28, 24);
    gfx->fillScreen(bg_color);

#ifdef TFT_BL
    // Core 3.x Backlight syntax
    ledcAttach(TFT_BL, 12000, 8); 
    ledcWrite(TFT_BL, TFT_BRIGHTNESS); 
#endif
}


extern "C" void display_init()
{
    w = gfx->width();
    h = gfx->height();
    
    // Centers the 256x240 NES image on your 320x240 screen perfectly
    if (w > NES_SCREEN_WIDTH)
    {
        frame_x = (w - NES_SCREEN_WIDTH) / 2; // Should equal 32
        frame_x_offset = 0;
        frame_width = NES_SCREEN_WIDTH;
        frame_height = NES_SCREEN_HEIGHT;
        frame_line_pixels = frame_width;
    }
    else
    {
        frame_x = 0;
        frame_x_offset = (NES_SCREEN_WIDTH - w) / 2;
        frame_width = w;
        frame_height = NES_SCREEN_HEIGHT;
        frame_line_pixels = frame_width;
    }
    frame_y = (gfx->height() - NES_SCREEN_HEIGHT) / 2; // Should equal 0
}

extern "C" void display_write_frame(const uint8_t *data[])
{
    gfx->startWrite();
    gfx->writeAddrWindow(frame_x, frame_y, frame_width, frame_height);
    for (int32_t i = 0; i < NES_SCREEN_HEIGHT; i++)
    {
        // Pushes one full line of NES pixels to the screen using the internal palette
        gfx->writeIndexedPixels((uint8_t *)(data[i] + frame_x_offset), myPalette, frame_line_pixels);
    }
    gfx->endWrite();
}

extern "C" void display_clear()
{
    gfx->fillScreen(bg_color);
}

//custom main menu logo
extern "C" void draw_image_from_sd(const char *filename, int x, int y, int width, int height) {
    // 1. Open the file on the SD card
    File imgFile = SD.open(filename);
    if (!imgFile) {
        Serial.println("Could not find image on SD card!");
        return;
    }

    // 2. Create a tiny memory bucket just big enough for ONE horizontal row of pixels
    // (width * 2 because each RGB565 pixel is 2 bytes)
    uint16_t *row_buffer = (uint16_t *)malloc(width * 2);
    if (!row_buffer) {
        imgFile.close();
        return;
    }

    // 3. Read the file row-by-row and stamp it to the screen
    for (int row = 0; row < height; row++) {
        imgFile.read((uint8_t *)row_buffer, width * 2);
        gfx->draw16bitRGBBitmap(x, y + row, row_buffer, width, 1);
    }

    // 4. Clean up the memory and close the file
    free(row_buffer);
    imgFile.close();
}

// Add this to the bottom of display.cpp
//IN GAME OVERLAY

extern "C" void draw_overlay_menu(int cursor) {
    // 1. Draw the Menu Background (Centered for 320x240 screen)
    gfx->fillRect(80, 60, 160, 120, gfx->color565(20, 20, 20));
    gfx->drawRect(80, 60, 160, 120, gfx->color565(0, 255, 0));

    // 2. Menu Title (Centered inside the box)
    gfx->setCursor(127, 75);
    gfx->setTextColor(gfx->color565(0, 255, 0));
    gfx->setTextSize(1);
    gfx->println("SYSTEM MENU");

    // 3. Option 0: Resume Game
    gfx->setCursor(95, 105);
    if (cursor == 0) {
        gfx->setTextColor(gfx->color565(255, 255, 255)); // White if selected
        gfx->println("> Resume Game");
    } else {
        gfx->setTextColor(gfx->color565(0, 150, 0));     // Dark Green if idle
        gfx->println("  Resume Game");
    }

    // 4. Option 1: Exit to Launcher
    gfx->setCursor(95, 130);
    if (cursor == 1) {
        gfx->setTextColor(gfx->color565(255, 255, 255));
        gfx->println("> Quit to Menu");
    } else {
        gfx->setTextColor(gfx->color565(0, 150, 0));
        gfx->println("  Quit to Menu");
    }

    // 5. Option 2: Placeholder for your future audio module
    gfx->setCursor(95, 155);
    gfx->setTextColor(gfx->color565(80, 80, 80)); // Greyed out for now
    gfx->println("  Volume: [ N/A ]");
}

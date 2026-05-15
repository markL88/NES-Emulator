extern "C"
{
#include <nes/nes.h>
}

#include "hw_config.h"
#include <Arduino_GFX_Library.h>

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
    -1      /* MISO (The screen only receives data, so we leave this blank) */
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
    gfx->begin();
    
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
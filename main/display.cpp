extern "C"
{
#include <nes/nes.h>
}

#include "hw_config.h"
#include <Arduino_GFX_Library.h>
#include <SD.h>

extern bool is_overlay_active;
extern int menu_cursor;
extern volatile bool menu_needs_redraw; 
extern volatile int global_volume; 

extern "C" void draw_overlay_menu(int cursor); 

#define TFT_BRIGHTNESS 255 
#define TFT_BL 8  

Arduino_DataBus *bus = new Arduino_ESP32SPI(5, 3, 7, 6, 10);
Arduino_ILI9341 *gfx = new Arduino_ILI9341(bus, 4, 3);

static int16_t w, h, frame_x, frame_y, frame_x_offset, frame_width, frame_height, frame_line_pixels;
extern int16_t bg_color;
extern uint16_t myPalette[];

// NEW: Global flag to prevent the grey box from flickering
static bool force_base_redraw = false; 

extern void display_begin()
{
    gfx->begin(60000000); 
    bg_color = gfx->color565(24, 28, 24);
    gfx->fillScreen(bg_color);

#ifdef TFT_BL
    ledcAttach(TFT_BL, 12000, 8); 
    ledcWrite(TFT_BL, TFT_BRIGHTNESS); 
#endif
}

extern "C" void display_init()
{
    w = gfx->width();
    h = gfx->height();
    
    if (w > NES_SCREEN_WIDTH) {
        frame_x = (w - NES_SCREEN_WIDTH) / 2; 
        frame_x_offset = 0;
        frame_width = NES_SCREEN_WIDTH;
        frame_height = NES_SCREEN_HEIGHT;
        frame_line_pixels = frame_width;
    } else {
        frame_x = 0;
        frame_x_offset = (NES_SCREEN_WIDTH - w) / 2;
        frame_width = w;
        frame_height = NES_SCREEN_HEIGHT;
        frame_line_pixels = frame_width;
    }
    frame_y = (gfx->height() - NES_SCREEN_HEIGHT) / 2; 
}

extern "C" void display_write_frame(const uint8_t *data[])
{
    // ==========================================
    // 1. THE SMART BLINDFOLD
    // ==========================================
    static bool is_booting = true;

    if (is_booting) {
        bool is_blank = true;
        
        uint8_t bg_pixel = data[NES_SCREEN_HEIGHT / 2][(NES_SCREEN_WIDTH / 2) + frame_x_offset];

        for (int y = 30; y < 210; y += 4) {
            for (int x = 32; x < 224; x += 4) {
                if (data[y][x + frame_x_offset] != bg_pixel) {
                    is_blank = false; 
                    break; 
                }
            }
            if (!is_blank) break;
        }

        if (is_blank) {
            return; 
        } else {
            gfx->fillScreen(bg_color);
            is_booting = false; 
        }
    }

    // ==========================================
    // 2. NORMAL SCISSOR CLIP LOGIC
    // ==========================================
    static bool was_active_last_frame = false; // Tracks if the menu JUST opened

    if (!is_overlay_active) {
        was_active_last_frame = false;
        
        gfx->startWrite();
        gfx->writeAddrWindow(frame_x, frame_y, frame_width, frame_height);
        for (int32_t i = 0; i < NES_SCREEN_HEIGHT; i++) {
            gfx->writeIndexedPixels((uint8_t *)(data[i] + frame_x_offset), myPalette, frame_line_pixels);
        }
        gfx->endWrite();
        
    } else {
        
        if (!was_active_last_frame) {
            // The menu just opened! Force the base box to draw exactly once.
            force_base_redraw = true; 
            menu_needs_redraw = true; 
        }
        
        gfx->startWrite();
        
        gfx->writeAddrWindow(frame_x, 0, 256, 60);
        for (int32_t i = 0; i < 60; i++) {
            gfx->writeIndexedPixels((uint8_t *)(data[i] + frame_x_offset), myPalette, 256);
        }

        gfx->writeAddrWindow(frame_x, 180, 256, 60);
        for (int32_t i = 180; i < 240; i++) {
            gfx->writeIndexedPixels((uint8_t *)(data[i] + frame_x_offset), myPalette, 256);
        }

        gfx->writeAddrWindow(32, 60, 48, 120);
        for (int32_t i = 60; i < 180; i++) {
            gfx->writeIndexedPixels((uint8_t *)(data[i] + frame_x_offset), myPalette, 48);
        }

        gfx->writeAddrWindow(240, 60, 48, 120);
        for (int32_t i = 60; i < 180; i++) {
            gfx->writeIndexedPixels((uint8_t *)(data[i] + frame_x_offset + 208), myPalette, 48);
        }
        
        gfx->endWrite();

        if (menu_needs_redraw) {
            draw_overlay_menu(menu_cursor);
            menu_needs_redraw = false; 
        }
        
        was_active_last_frame = true;
    }
}

extern "C" void display_clear()
{
    //gfx->fillScreen(bg_color);
}

extern "C" void draw_image_from_sd(const char *filename, int x, int y, int width, int height) {
    File imgFile = SD.open(filename);
    if (!imgFile) return;

    uint16_t *row_buffer = (uint16_t *)malloc(width * 2);
    if (!row_buffer) {
        imgFile.close();
        return;
    }

    for (int row = 0; row < height; row++) {
        imgFile.read((uint8_t *)row_buffer, width * 2);
        gfx->draw16bitRGBBitmap(x, y + row, row_buffer, width, 1);
    }

    free(row_buffer);
    imgFile.close();
}

extern "C" void draw_overlay_menu(int cursor) {
    uint16_t menu_bg = gfx->color565(20, 20, 20);

    // 1. Only draw the giant grey box and green border when the menu FIRST opens
    if (force_base_redraw) {
        gfx->fillRect(80, 60, 160, 120, menu_bg);
        gfx->drawRect(80, 60, 160, 120, gfx->color565(0, 255, 0));

        gfx->setCursor(127, 75);
        gfx->setTextColor(gfx->color565(0, 255, 0)); // Static title doesn't need bg color
        gfx->setTextSize(1);
        gfx->println("SYSTEM MENU");
        
        force_base_redraw = false; 
    }

    // 2. The Text-Background Trick
    // Passing 'menu_bg' as the second argument forces the hardware to draw the 
    // dark grey background AT THE EXACT SAME TIME as the letters, eliminating flicker.
    
    gfx->setCursor(95, 105);
    if (cursor == 0) {
        gfx->setTextColor(gfx->color565(255, 255, 255), menu_bg); 
        gfx->println("> Resume Game  "); // Padded spaces physically overwrite old cursors
    } else {
        gfx->setTextColor(gfx->color565(0, 150, 0), menu_bg);     
        gfx->println("  Resume Game  ");
    }

    gfx->setCursor(95, 130);
    if (cursor == 1) {
        gfx->setTextColor(gfx->color565(255, 255, 255), menu_bg);
        gfx->println("> Quit to Menu ");
    } else {
        gfx->setTextColor(gfx->color565(0, 150, 0), menu_bg);
        gfx->println("  Quit to Menu ");
    }

    gfx->setCursor(95, 155);
    if (cursor == 2) {
        gfx->setTextColor(gfx->color565(255, 255, 255), menu_bg);
        gfx->print("> Volume: ( ");
        gfx->print(global_volume);
        gfx->println(" )  "); // Extra spaces wipe out the zero when turning down from 10
    } else {
        gfx->setTextColor(gfx->color565(0, 150, 0), menu_bg);
        gfx->print("  Volume: ( ");
        gfx->print(global_volume);
        gfx->println(" )  ");
    }
}

#include <Arduino.h>
#include "hw_config.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/i2s.h> 
//#include <SD.h> // Ensure SD library is included

extern "C" volatile int global_volume = 10;

void play_overlay_tick() {
    size_t bytes_written;
    int16_t sample = 0;
    for (int i = 0; i < 500; i++) {
        sample = ((i / 25) % 2 == 0) ? 60000 : -60000; 
        uint32_t sample32 = ((uint32_t)(uint16_t)sample << 16) | (uint16_t)sample;
        i2s_write(I2S_NUM_0, &sample32, sizeof(sample32), &bytes_written, portMAX_DELAY);
    }
}

void play_overlaymenu_tick() {
    size_t bytes_written;
    int16_t sample = 0;
    for (int i = 0; i < 500; i++) {
        sample = ((i / 100) % 2 == 0) ? 10000 : -10000; 
        uint32_t sample32 = ((uint32_t)(uint16_t)sample << 16) | (uint16_t)sample;
        i2s_write(I2S_NUM_0, &sample32, sizeof(sample32), &bytes_written, portMAX_DELAY);
    }

    // 2. THE BROOM: Force the sound out of the hardware
    uint32_t silence = 0;
    for (int i = 0; i < 1536; i++) {
        i2s_write(I2S_NUM_0, &silence, sizeof(silence), &bytes_written, portMAX_DELAY);
    }
}

void play_deselect_tick() {
    size_t bytes_written;
    int16_t sample = 0;
    for (int i = 0; i < 3000; i++) {
        sample = ((i / 21) % 2 == 0) ? 2000 : -2000; 
        uint32_t sample32 = ((uint32_t)(uint16_t)sample << 16) | (uint16_t)sample;
        i2s_write(I2S_NUM_0, &sample32, sizeof(sample32), &bytes_written, portMAX_DELAY);
    }
    for (int i = 0; i < 3000; i++) {
        sample = ((i / 28) % 2 == 0) ? 2000 : -2000; 
        uint32_t sample32 = ((uint32_t)(uint16_t)sample << 16) | (uint16_t)sample;
        i2s_write(I2S_NUM_0, &sample32, sizeof(sample32), &bytes_written, portMAX_DELAY);
    }
    for (int i = 0; i < 6000; i++) {
        sample = ((i / 42) % 2 == 0) ? 2000 : -2000; 
        uint32_t sample32 = ((uint32_t)(uint16_t)sample << 16) | (uint16_t)sample;
        i2s_write(I2S_NUM_0, &sample32, sizeof(sample32), &bytes_written, portMAX_DELAY);
    }
}

extern "C" void draw_overlay_menu(int cursor);

extern "C" void controller_init() {
    pinMode(HW_CONTROLLER_GPIO_UP, INPUT_PULLUP);
    pinMode(HW_CONTROLLER_GPIO_DOWN, INPUT_PULLUP);
    pinMode(HW_CONTROLLER_GPIO_LEFT, INPUT_PULLUP);
    pinMode(HW_CONTROLLER_GPIO_RIGHT, INPUT_PULLUP);
    pinMode(HW_CONTROLLER_GPIO_A, INPUT_PULLUP);
    pinMode(HW_CONTROLLER_GPIO_B, INPUT_PULLUP);
    pinMode(HW_CONTROLLER_GPIO_START, INPUT_PULLUP);
    pinMode(HW_CONTROLLER_GPIO_SELECT, INPUT_PULLUP);
}

bool is_overlay_active = false;
int menu_cursor = 0;
unsigned long last_input_time = 0; 

extern "C" {
    volatile bool mute_game_audio = false; 
    volatile bool menu_needs_redraw = false; 
    extern void force_sram_save_now(void); 
    
    extern "C" volatile int global_volume; 
}

extern "C" uint32_t controller_read_input() {
    bool start = (digitalRead(HW_CONTROLLER_GPIO_START) == LOW);
    bool select = (digitalRead(HW_CONTROLLER_GPIO_SELECT) == LOW);

    if (start && select && (millis() - last_input_time > 300)) {
        is_overlay_active = !is_overlay_active; 
        menu_cursor = 0;
        last_input_time = millis();

        if (is_overlay_active) {
            play_overlay_tick(); 
            mute_game_audio = true; 
            menu_needs_redraw = true; 
        } else {
            play_overlay_tick(); 
            mute_game_audio = false; 

        }
    }

    static int ghost_wipe_frames = 0; 

    if (is_overlay_active) {
        bool m_up = (digitalRead(HW_CONTROLLER_GPIO_UP) == LOW);
        bool m_down = (digitalRead(HW_CONTROLLER_GPIO_DOWN) == LOW);
        bool m_a = (digitalRead(HW_CONTROLLER_GPIO_A) == LOW);
        bool m_left = (digitalRead(HW_CONTROLLER_GPIO_LEFT) == LOW);   // NEW
        bool m_right = (digitalRead(HW_CONTROLLER_GPIO_RIGHT) == LOW); // NEW

        if (m_up && menu_cursor > 0 && (millis() - last_input_time > 200)) {
            menu_cursor--;
            play_overlaymenu_tick();
            menu_needs_redraw = true; 
            last_input_time = millis();
        }
        
        // FIX: Change 'menu_cursor < 1' to 'menu_cursor < 2' to allow reaching the 3rd slot!
        if (m_down && menu_cursor < 2 && (millis() - last_input_time > 200)) {
            menu_cursor++;
            play_overlaymenu_tick();
            menu_needs_redraw = true; 
            last_input_time = millis();
        }

        // NEW: Volume Control Logic
        if (menu_cursor == 2) {
            if (m_left && global_volume > 0 && (millis() - last_input_time > 150)) {
                global_volume--;
                menu_needs_redraw = true;
                last_input_time = millis();
            }
            if (m_right && global_volume < 10 && (millis() - last_input_time > 150)) {
                global_volume++;
                menu_needs_redraw = true;
                last_input_time = millis();
            }
        }

        if (m_a && (millis() - last_input_time > 200)) {
            if (menu_cursor == 0) {
                is_overlay_active = false; 
                mute_game_audio = false; 
                play_overlay_tick();
                ghost_wipe_frames = 5; 

            } else if (menu_cursor == 1) {
                play_deselect_tick();

                // 3. Lights Out
                pinMode(8, OUTPUT); 
                digitalWrite(8, LOW); 
                
                // 1. Force the physical SRAM dump to the SD card!
                force_sram_save_now(); 
                
                // 2. The 3-Second Lifeline
                // Core 1 now has all the time it needs to finish the frame, 
                // break the loop, open the SD card, write 8KB, and safely close the file.
                delay(100); 
                ESP.restart(); 
            }
            last_input_time = millis();
        }
        return 0xFFFF;
    }

    if (ghost_wipe_frames > 0) {
        ghost_wipe_frames--;
        return 0xFFFF; 
    }

    uint32_t b = 0xFFFF;
    bool up = (digitalRead(HW_CONTROLLER_GPIO_UP) == LOW);
    bool down = (digitalRead(HW_CONTROLLER_GPIO_DOWN) == LOW);
    bool left = (digitalRead(HW_CONTROLLER_GPIO_LEFT) == LOW);
    bool right = (digitalRead(HW_CONTROLLER_GPIO_RIGHT) == LOW);
    bool a = (digitalRead(HW_CONTROLLER_GPIO_A) == LOW);
    bool b_btn = (digitalRead(HW_CONTROLLER_GPIO_B) == LOW);
    
    start = (digitalRead(HW_CONTROLLER_GPIO_START) == LOW);
    select = (digitalRead(HW_CONTROLLER_GPIO_SELECT) == LOW);

    if (up)    b &= ~(1 << 0);
    if (down)  b &= ~(1 << 1);
    if (left)  b &= ~(1 << 2);
    if (right) b &= ~(1 << 3);
    if (a)     b &= ~(1 << 6);
    if (b_btn) b &= ~(1 << 7);
    if (start) b &= ~(1 << 5);
    if (select)b &= ~(1 << 4);

    return b;
}

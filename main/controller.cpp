#include <Arduino.h>
#include "hw_config.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

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

extern "C" uint32_t controller_read_input() {
    // 1. Read trigger pins BEFORE freezing
    bool start = (digitalRead(HW_CONTROLLER_GPIO_START) == LOW);
    bool select = (digitalRead(HW_CONTROLLER_GPIO_SELECT) == LOW);

    // 2. Check for the Menu Trigger (Start + Select)
    if (start && select && !is_overlay_active) {
        is_overlay_active = true;
        menu_cursor = 0;
        draw_overlay_menu(menu_cursor);
        delay(300); // Debounce entry
    }

    bool just_resumed = false;

    // 3. The "Time Freeze" Loop
    while (is_overlay_active) {
        bool m_up = (digitalRead(HW_CONTROLLER_GPIO_UP) == LOW);
        bool m_down = (digitalRead(HW_CONTROLLER_GPIO_DOWN) == LOW);
        bool m_a = (digitalRead(HW_CONTROLLER_GPIO_A) == LOW);

        if (m_up && menu_cursor > 0) {
            menu_cursor--;
            draw_overlay_menu(menu_cursor);
            delay(200);
        }
        
        if (m_down && menu_cursor < 1) {
            menu_cursor++;
            draw_overlay_menu(menu_cursor);
            delay(200);
        }

        if (m_a) {
            if (menu_cursor == 0) {
                is_overlay_active = false; 
                just_resumed = true; // Flag that we just escaped
                delay(300); // Give your finger time to lift off 'A'
            } else if (menu_cursor == 1) {
                // Quit to Menu: The "Lights Out" Protocol
                
                // 1. Force GPIO 8 to output mode
                pinMode(8, OUTPUT); 
                
                // 2. Kill the backlight
                digitalWrite(8, LOW); 
                
                // 3. Give the LED a fraction of a second to physically fade to black
                delay(100);
                ESP.restart(); 
            }
        }
        
        // Feed the watchdog to prevent crashes
        vTaskDelay(20 / portTICK_PERIOD_MS);
    }

    // ==========================================
    // 4. The Ghost Input Fix
    // ==========================================
    // If we just closed the menu, force a completely blank controller frame. 
    // This destroys the Start+Select ghosts from before the time freeze.
    if (just_resumed) {
        return 0xFFFF; 
    }

    // ==========================================
    // 5. Normal Game Logic (Re-read fresh pins!)
    // ==========================================
    uint32_t b = 0xFFFF;
    bool up = (digitalRead(HW_CONTROLLER_GPIO_UP) == LOW);
    bool down = (digitalRead(HW_CONTROLLER_GPIO_DOWN) == LOW);
    bool left = (digitalRead(HW_CONTROLLER_GPIO_LEFT) == LOW);
    bool right = (digitalRead(HW_CONTROLLER_GPIO_RIGHT) == LOW);
    bool a = (digitalRead(HW_CONTROLLER_GPIO_A) == LOW);
    bool b_btn = (digitalRead(HW_CONTROLLER_GPIO_B) == LOW);
    
    // Re-read Start and Select so they don't get stuck!
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

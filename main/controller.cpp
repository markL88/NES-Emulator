#include <Arduino.h>
#include "hw_config.h"

extern "C" void controller_init() {
    // Initialize your physical PCB pins with internal pull-up resistors
    pinMode(HW_CONTROLLER_GPIO_UP, INPUT_PULLUP);
    pinMode(HW_CONTROLLER_GPIO_DOWN, INPUT_PULLUP);
    pinMode(HW_CONTROLLER_GPIO_LEFT, INPUT_PULLUP);
    pinMode(HW_CONTROLLER_GPIO_RIGHT, INPUT_PULLUP);
    pinMode(HW_CONTROLLER_GPIO_A, INPUT_PULLUP);
    pinMode(HW_CONTROLLER_GPIO_B, INPUT_PULLUP);
}

extern "C" uint32_t controller_read_input() {
    // Nofrendo expects 0xFFFF (all 1s) when NO buttons are pressed
    uint32_t b = 0xFFFF;

    // Read the physical pins (LOW means the physical button is pressed)
    bool up = (digitalRead(HW_CONTROLLER_GPIO_UP) == LOW);
    bool down = (digitalRead(HW_CONTROLLER_GPIO_DOWN) == LOW);
    bool left = (digitalRead(HW_CONTROLLER_GPIO_LEFT) == LOW);
    bool right = (digitalRead(HW_CONTROLLER_GPIO_RIGHT) == LOW);
    bool a = (digitalRead(HW_CONTROLLER_GPIO_A) == LOW);
    bool b_btn = (digitalRead(HW_CONTROLLER_GPIO_B) == LOW);

    /* * NOFRENDO BITMAP:
     * Bit 0: UP
     * Bit 1: DOWN
     * Bit 2: LEFT
     * Bit 3: RIGHT
     * Bit 4: SELECT
     * Bit 5: START
     * Bit 6: A
     * Bit 7: B
     * * To tell Nofrendo a button is pressed, we force its specific bit to 0.
     */

    if (up)    b &= ~(1 << 0);
    if (down)  b &= ~(1 << 1);
    if (left)  b &= ~(1 << 2);
    if (right) b &= ~(1 << 3);
    
    if (a)     b &= ~(1 << 6);
    if (b_btn) b &= ~(1 << 7);

    // ==========================================
    // CUSTOM HARDWARE COMBOS
    // ==========================================

    // START Button Combo: Press A + B together
    if (a && b_btn) {
        b &= ~(1 << 5); // Forces the "Start" bit to 0
    }

    // SELECT Button Combo: Press DOWN + B together
    if (down && b_btn) {
        b &= ~(1 << 4); // Forces the "Select" bit to 0
    }

    return b;
}
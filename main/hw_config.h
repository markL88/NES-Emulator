#ifndef _HW_CONFIG_H_
#define _HW_CONFIG_H_

#define FSROOT "/fs"

/* M5Stack */
#if defined(ARDUINO_M5Stack_Core_ESP32) || defined(ARDUINO_M5STACK_FIRE)

// Uncomment one of below, M5Stack support SPIFFS and SD
// #define FILESYSTEM_BEGIN SPIFFS.begin(false, FSROOT); FS filesystem = SPIFFS;
// We removed "FS &filesystem = SD" from here because we moved it to the .ino file
#define FILESYSTEM_BEGIN SPI.begin(7, 10, 6, 9); SD.begin(9, SPI, FSROOT);

/* enable audio */
#define HW_AUDIO
#define HW_AUDIO_SAMPLERATE 22050

/* controller is I2C M5Stack CardKB */
#define HW_CONTROLLER_I2C_M5CARDKB

/* Odroid-Go */
#elif defined(ARDUINO_ODROID_ESP32)

// Uncomment one of below, ODROID support SPIFFS and SD
// #define FILESYSTEM_BEGIN SPIFFS.begin(false, FSROOT); FS filesystem = SPIFFS;
#define FILESYSTEM_BEGIN SD.begin(SS, SPI, FSROOT); FS filesystem = SD;

/* enable audio */
#define HW_AUDIO
#define HW_AUDIO_SAMPLERATE 22050

/* controller is GPIO */
#define HW_CONTROLLER_GPIO
#define HW_CONTROLLER_GPIO_ANALOG_JOYSTICK
#define HW_CONTROLLER_GPIO_UP_DOWN 35
#define HW_CONTROLLER_GPIO_LEFT_RIGHT 34
#define HW_CONTROLLER_GPIO_SELECT 27
#define HW_CONTROLLER_GPIO_START 39
#define HW_CONTROLLER_GPIO_A 32
#define HW_CONTROLLER_GPIO_B 33
#define HW_CONTROLLER_GPIO_X 13
#define HW_CONTROLLER_GPIO_Y 0

/* TTGO T-Watch */
#elif defined(ARDUINO_T) || defined(ARDUINO_TWATCH_BASE) || defined(ARDUINO_TWATCH_2020_V1) || defined(ARDUINO_TWATCH_2020_V2) // TTGO T-Watch

// TTGO T-watch with game module only support SPIFFS
#define FILESYSTEM_BEGIN SPIFFS.begin(false, FSROOT); FS filesystem = SPIFFS;

/* buzzer audio */
#define HW_AUDIO_BUZZER
#define HW_AUDIO_BUZZER_PIN 4
#define HW_AUDIO_SAMPLERATE 22050 // nofrendo minimum sample rate

/* controller is GPIO */
#define HW_CONTROLLER_GPIO
#define HW_CONTROLLER_GPIO_ANALOG_JOYSTICK
#define HW_CONTROLLER_GPIO_UP_DOWN 34
#define HW_CONTROLLER_GPIO_LEFT_RIGHT 33
#define HW_CONTROLLER_GPIO_SELECT 15
#define HW_CONTROLLER_GPIO_START 36
#define HW_CONTROLLER_GPIO_A 13
#define HW_CONTROLLER_GPIO_B 25
#define HW_CONTROLLER_GPIO_X 14
#define HW_CONTROLLER_GPIO_Y 26

/* custom hardware */
#else

// 1. SD CARD CONFIGURATION (Shared SPI Bus: SCK=7, MISO=10, MOSI=6, CS=9)
// We force the ESP32 to remap its SPI pins before mounting the SD card!
#define FILESYSTEM_BEGIN SPI.begin(7, 10, 6, 9); SD.begin(9, SPI, 20000000, FSROOT); FS filesystem = SD;

// 2. AUDIO (Commented out completely so the MAX module doesn't crash the ESP32)
// #define HW_AUDIO
// #define HW_AUDIO_EXTDAC
// #define HW_AUDIO_EXTDAC_WCLK 21
// #define HW_AUDIO_EXTDAC_BCLK 22
// #define HW_AUDIO_EXTDAC_DOUT 19
// #define HW_AUDIO_SAMPLERATE 22050

// 3. CONTROLLER CONFIGURATION
#define HW_CONTROLLER_GPIO

// IMPORTANT: We MUST comment out the analog joystick so it accepts your digital buttons
// #define HW_CONTROLLER_GPIO_ANALOG_JOYSTICK

// Your Digital D-Pad
#define HW_CONTROLLER_GPIO_UP 12
#define HW_CONTROLLER_GPIO_DOWN 13
#define HW_CONTROLLER_GPIO_LEFT 17
#define HW_CONTROLLER_GPIO_RIGHT 14

// Your A & B Buttons
#define HW_CONTROLLER_GPIO_A 18
#define HW_CONTROLLER_GPIO_B 21

// Your Start & Select Buttons (Commented out for now as requested)
 #define HW_CONTROLLER_GPIO_SELECT 41
 #define HW_CONTROLLER_GPIO_START 42

#endif /* custom hardware */

#endif /* _HW_CONFIG_H_ */

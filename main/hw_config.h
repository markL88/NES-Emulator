#ifndef _HW_CONFIG_H_
#define _HW_CONFIG_H_

#define FSROOT "/fs"

// ==========================================
// CUSTOM ESP32-S3 HARDWARE CONFIGURATION
// ==========================================

// 1. SD CARD CONFIGURATION (Shared SPI Bus: SCK=7, MISO=10, MOSI=6, CS=9)
// We force the ESP32 to remap its SPI pins before mounting the SD card!
#define FILESYSTEM_BEGIN SPI.begin(7, 10, 6, 9); SD.begin(9, SPI, 20000000, FSROOT); FS filesystem = SD;

// 2. AUDIO (Enabled for MAX98357A I2S Module)
#define HW_AUDIO
#define HW_AUDIO_EXTDAC
#define HW_AUDIO_EXTDAC_WCLK 47
#define HW_AUDIO_EXTDAC_BCLK 46
#define HW_AUDIO_EXTDAC_DOUT 45
#define HW_AUDIO_SAMPLERATE 44100

// 3. CONTROLLER CONFIGURATION
#define HW_CONTROLLER_GPIO

// Your Digital D-Pad
#define HW_CONTROLLER_GPIO_UP 12
#define HW_CONTROLLER_GPIO_DOWN 13
#define HW_CONTROLLER_GPIO_LEFT 17
#define HW_CONTROLLER_GPIO_RIGHT 14

// Your A & B Buttons
#define HW_CONTROLLER_GPIO_A 18
#define HW_CONTROLLER_GPIO_B 21

// Your Start & Select Buttons 
#define HW_CONTROLLER_GPIO_SELECT 41
#define HW_CONTROLLER_GPIO_START 42

#endif /* _HW_CONFIG_H_ */

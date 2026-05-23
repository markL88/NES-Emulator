#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <driver/i2s.h>
#include <nes/nes.h>
#include "hw_config.h"

// NEW: Connects to the valve switch in controller.cpp
extern volatile bool mute_game_audio;

#define DEFAULT_FRAGSIZE 64
static void (*audio_callback)(void *buffer, int length) = NULL;
static int16_t *audio_frame;

int osd_init_sound()
{
  audio_frame = NOFRENDO_MALLOC(4 * DEFAULT_FRAGSIZE);

  i2s_config_t cfg = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = 44100, 
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S, 
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 6,
    .dma_buf_len = 256,
    .use_apll = false,
    .tx_desc_auto_clear = true
  };

  esp_err_t err = i2s_driver_install(I2S_NUM_0, &cfg, 0, NULL);
  
  printf("\n====================================\n");
  printf("NOFRENDO I2S ENGINE STARTUP: %d (0 means SUCCESS)\n", err);
  printf("====================================\n\n");

  i2s_pin_config_t pins = {
    .mck_io_num = I2S_PIN_NO_CHANGE, 
    .bck_io_num = 46, 
    .ws_io_num = 47,  
    .data_out_num = 45, 
    .data_in_num = I2S_PIN_NO_CHANGE,
  };
  
  i2s_set_pin(I2S_NUM_0, &pins);
  i2s_zero_dma_buffer(I2S_NUM_0);

  audio_callback = NULL;
  return 0;
}

void osd_stopsound()
{
  audio_callback = NULL;
}

void do_audio_frame()
{
  int left = 44100 / NES_REFRESH_RATE; 
  while (left)
  {
    int n = DEFAULT_FRAGSIZE;
    if (n > left) n = left;
    
    if (audio_callback != NULL) {
        audio_callback(audio_frame, n); 
    }

    int16_t *mono_ptr = audio_frame + n;
    int16_t *stereo_ptr = audio_frame + n + n;
    int i = n;
    
    while (i--)
    {
      int32_t raw_volume = *(--mono_ptr);
      
      raw_volume = raw_volume; 
      
      if (raw_volume > 32700) raw_volume = 32700;
      if (raw_volume < -32700) raw_volume = -32700;
      
      *(--stereo_ptr) = (int16_t)raw_volume; 
      *(--stereo_ptr) = (int16_t)raw_volume; 
    }

    size_t i2s_bytes_write;
    
    // THE AUDIO VALVE
    if (!mute_game_audio) {
        // If the menu is closed, play the game audio
        i2s_write(I2S_NUM_0, (const char *)audio_frame, 4 * n, &i2s_bytes_write, portMAX_DELAY);
        left -= i2s_bytes_write / 4;
    } else {
        // If the menu is open, bypass the speaker entirely
        // Subtracting 'n' tricks the emulator into maintaining 60 FPS
        left -= n; 
    }
  }
}

void osd_setsound(void (*playfunc)(void *buffer, int length))
{
  audio_callback = playfunc;
}

void osd_getsoundinfo(sndinfo_t *info)
{
  info->sample_rate = 44100; 
  info->bps = 16;
}

/*
 * Simple byte player for the AZ-Delivery ESP32 Dev Kit.
 *
 * Audio data is compiled into audio_data.h.  The player repeats it and
 * writes the same mono sample to both I2S slots.  The `audio` jescore job
 * controls playback from the CLI:
 *
 *   jescore audio on
 *   jescore audio off
 *   jescore audio state
 *
 * I2S pins:
 *   DIN  = GPIO12
 *   BCLK = GPIO14
 *   LRC  = GPIO25
 */

#include <Arduino.h>
#include <driver/i2s.h>
#include <jescore.h>

#include "audio_data.h"

#define I2S_PORT I2S_NUM_0
#define I2S_DIN 12
#define I2S_BCLK 14
#define I2S_LRC 25
#define I2S_DMA_SAMPLES 256

static int16_t i2s_buffer[I2S_DMA_SAMPLES * 2];
static volatile bool audio_enabled = true;

static void i2s_setup() {
    const i2s_config_t config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = AUDIO_SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_I2S_MSB,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = I2S_DMA_SAMPLES,
        .use_apll = false,
        .tx_desc_auto_clear = true,
        .fixed_mclk = 0,
    };

    const i2s_pin_config_t pins = {
        .bck_io_num = I2S_BCLK,
        .ws_io_num = I2S_LRC,
        .data_out_num = I2S_DIN,
        .data_in_num = I2S_PIN_NO_CHANGE,
    };

    i2s_driver_install(I2S_PORT, &config, 0, nullptr);
    i2s_set_pin(I2S_PORT, &pins);
    i2s_zero_dma_buffer(I2S_PORT);
}

static void audio_player(void* p) {
    size_t sample = 0;

    while (true) {
        for (size_t i = 0; i < I2S_DMA_SAMPLES; i++) {
            const int16_t value = audio_enabled ? audio_data[sample] : 0;
            i2s_buffer[2 * i] = value;
            i2s_buffer[2 * i + 1] = value;

            if (++sample == AUDIO_NUM_SAMPLES) {
                sample = 0;
            }
        }

        size_t bytes_written = 0;
        i2s_write(I2S_PORT, i2s_buffer, sizeof(i2s_buffer), &bytes_written,
                  portMAX_DELAY);
    }
}

static void audio_command(void* p) {
    char* args = jes_job_get_args();

    if (!args || jes_job_is_arg(args, "state")) {
        jes_print("audio is %s\n\r", audio_enabled ? "on" : "off");
        return;
    }

    if (jes_job_is_arg(args, "on")) {
        audio_enabled = true;
        jes_print("audio on\n\r");
        return;
    }

    if (jes_job_is_arg(args, "off")) {
        audio_enabled = false;
        jes_print("audio off\n\r");
        return;
    }

    jes_print("Usage: audio <on|off|state>\n\r");
}

void setup() {
    i2s_setup();
    jes_init();
    jes_register_and_launch_job("_audio", 8192, 1, audio_player, 1, 1);
    jes_register_job("audio", 4096, 1, audio_command, 0, 1);
}

void loop() {
    // jescore handles the CLI asynchronously; audio_player runs forever.
}

#include <Arduino.h>
#include "M5Cardputer.h"
#include "audio_config.h"
#include "file_manager.h"
#include "ui_manager.h"
#include <utility/Keyboard/KeyboardReader/TCA8418.h>

TaskHandle_t handleUITask = NULL;
TaskHandle_t handleAudioTask = NULL;

void Task_TFT(void *pvParameters);
void Task_Audio(void *pvParameters);
void audio_eof_mp3(const char *info);

void setup() {
    auto cfg = M5.config();
    cfg.serial_baudrate = 115200;
    cfg.internal_mic = false;
    cfg.internal_spk = false;
    M5Cardputer.begin(cfg, true);

    Serial.println("Configuring TCA8418 keyboard driver");
    std::unique_ptr<KeyboardReader> reader(new TCA8418KeyboardReader());
    M5Cardputer.Keyboard.begin(std::move(reader));


    xTaskCreatePinnedToCore(Task_TFT, "Task_TFT", 20480, NULL, 2, &handleUITask, 0);
    xTaskCreatePinnedToCore(Task_Audio, "Task_Audio", 12288, NULL, 3, &handleAudioTask, 1);
}

void loop() {
    updateHeadphoneDetection();
    delay(100);
}

void Task_TFT(void *pvParameters) {
    initUI();
    initSDCard();
    scanDirectory(currentFolder);
    
    while (true) {
        M5Cardputer.update();
        if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
            Keyboard_Class::KeysState ks = M5Cardputer.Keyboard.keysState();
            for (auto ch : ks.word) handleKeyPress(ch);
            if (ks.enter) handleKeyPress('\n');
            if (ks.del) handleKeyPress('\b');
        }

        draw();
        vTaskDelay(40 / portTICK_PERIOD_MS);
    }
}

void Task_Audio(void *pvParameters) {
    if (!initES8311Codec()) {
        Serial.println("ERROR: Audio codec initialization failed!");
    }

    const TickType_t playDelay = pdMS_TO_TICKS(1);
    const TickType_t idleDelay = pdMS_TO_TICKS(20);
    unsigned long lastLog = 0;

    while (true) {
        if (nextTrackRequest && fileCount > 0) {
            audio.stopSong();
            trackStartMillis = millis();
            playbackTime = 0;

            const String &trackPath = audioFiles[currentFileIndex];
            Serial.printf("[Task_Media] Loading track %d: %s\n", currentFileIndex, trackPath.c_str());

            if (SD.exists(trackPath)) {
                if (codec_initialized) {
                    if (audio.connecttoFS(SD, trackPath.c_str())) {
                        Serial.println("[Task_Media] Track connected successfully.");
                        isPlaying = true;
                        isStoped = false;
                    } else {
                        Serial.println("ERROR: Failed to connect track to codec.");
                        isPlaying = false;
                        isStoped = true;
                    }
                } else {
                    Serial.println("WARNING: Codec not initialized, cannot play track.");
                    isPlaying = false;
                    isStoped = true;
                }
            } else {
                Serial.println("ERROR: Track file not found on SD.");
                isPlaying = false;
                isStoped = true;
            }
            nextTrackRequest = false;
        }

        if (currentUIState == UI_PLAYER && isPlaying && codec_initialized && !isStoped && fileCount > 0) {
            audio.loop();

            if (!audio.isRunning()) {
                Serial.printf("[Task_Media] Track %d ended, auto-advancing.\n", currentFileIndex);
                currentFileIndex++;
                if (currentFileIndex >= fileCount) currentFileIndex = 0;
                nextTrackRequest = true;
            }

            if (millis() - lastLog >= 5000) {
                Serial.printf("[Task_Media] Playing %d/%d, volume=%d, elapsed=%lu ms\n",
                              currentFileIndex + 1, fileCount, volume, millis() - trackStartMillis);
                lastLog = millis();
            }
            vTaskDelay(playDelay);
        } else {
            vTaskDelay(idleDelay);
        }
    }
}

void audio_eof_mp3(const char *info) {
    Serial.printf("eof_mp3: %s\n", info);
    if (currentUIState == UI_PLAYER && fileCount > 0) {
        currentFileIndex++;
        if (currentFileIndex >= fileCount) currentFileIndex = 0;
        Serial.printf("Auto-advancing to next: %s\n", audioFiles[currentFileIndex].c_str());
        nextTrackRequest = true;
    }
}

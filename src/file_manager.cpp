#include "file_manager.h"

SemaphoreHandle_t sdMutex = NULL;

String audioFiles[MAX_FILES];
uint8_t fileCount = 0;
uint8_t currentFileIndex = 0;
String currentFolder = "/";

String availableFolders[20];
uint8_t folderCount = 0;

bool isScanningFiles = false;
uint8_t scanProgress = 0;
uint8_t scanTotal = 0;

bool initSDCard() {
    SPI.begin(SD_SCK, SD_MISO, SD_MOSI);
    if (!SD.begin(SD_CS)) {
        Serial.println("ERROR: SD Mount Failed!");
        return false;
    }
    
    uint8_t cardType = SD.cardType();
    Serial.println("SD Card initialized successfully");
    Serial.printf("SD Card Type: %s\n", 
        cardType == CARD_MMC ? "MMC" :
        cardType == CARD_SD ? "SDSC" :
        cardType == CARD_SDHC ? "SDHC" : "UNKNOWN");
    Serial.printf("SD Card Size: %lluMB\n", SD.cardSize() / (1024 * 1024));
    
    return true;
}

void scanDirectory(const String& folder) {
    fileCount = 0;
    folderCount = 0;
    scanProgress = 0;
    scanTotal = 0;
    currentFolder = folder;
    
    Serial.printf("Scanning directory: %s\n", folder.c_str());

    File root = SD.open(folder);
    if (!root || !root.isDirectory()) {
        return;
    }

    File f = root.openNextFile();
    while (f && scanTotal < 255) {
        if (!f.isDirectory()) {
            String fname = String(f.name());
            fname.toLowerCase();
            int8_t dot = fname.lastIndexOf('.');
            if (dot >= 0) {
                String ext = fname.substring(dot + 1);
                if (ext == "mp3" || ext == "wav") scanTotal++;
            }
        }
        f.close();
        f = root.openNextFile();
        vTaskDelay(5 / portTICK_PERIOD_MS);
    }
    root.close();
    
    Serial.printf("Found %d audio files to process\n", scanTotal);

    root = SD.open(folder);
    f = root.openNextFile();
    
    while (f) {
        String fname = String(f.name());

        if (f.isDirectory()) {
            if (folderCount < 20) {
                if (!fname.startsWith("/")) {
                    fname = (folder == "/") ? "/" + fname : folder + "/" + fname;
                }
                availableFolders[folderCount++] = fname;
            }
        } else {
            if (!fname.startsWith("/")) {
                fname = (folder == "/") ? "/" + fname : folder + "/" + fname;
            }
            
            String lower = fname;
            lower.toLowerCase();
            int8_t dot = lower.lastIndexOf('.');

            if (dot >= 0) {
                String ext = lower.substring(dot + 1);
                if (ext == "mp3" || ext == "wav") {
                    if (fileCount < MAX_FILES) {
                        audioFiles[fileCount++] = fname;
                        scanProgress++;
                    } else break;
                }
            }
        }
        
        f.close();
        f = root.openNextFile();
        vTaskDelay(5 / portTICK_PERIOD_MS);
    }
    
    root.close();
}

String getFileName(uint8_t index) {
    if (index >= fileCount) return "";
    
    String fname = audioFiles[index];
    int8_t lastSlash = fname.lastIndexOf('/');
    if (lastSlash >= 0) fname = fname.substring(lastSlash + 1);
    
    int8_t lastDot = fname.lastIndexOf('.');
    if (lastDot > 0) fname = fname.substring(0, lastDot);
    
    return fname;
}
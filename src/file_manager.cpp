#include "file_manager.h"

String audioFiles[MAX_FILES];
int fileCount = 0;
int currentFileIndex = 0;
String currentFolder = "/";

String availableFolders[20];
int folderCount = 0;

bool isScanning = false;
bool isScanningFiles = false;
unsigned short int scanProgress = 0;
unsigned short int scanTotal = 0;

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

    Serial.printf("Free Heap: %d bytes\n", ESP.getFreeHeap());
    if (psramFound()) {
        Serial.printf("PSRAM available: %d bytes\n", ESP.getFreePsram());
    } else {
        Serial.println("PSRAM not found");
    }
    
    return true;
}

void scanFolders(const String& folder) {
    folderCount = 0;
    scanProgress = 0;
    scanTotal = 0;
    isScanning = true;

    Serial.printf("\n=== Scanning folders in: %s ===\n", folder.c_str());

    File root = SD.open(folder);
    if (!root || !root.isDirectory()) {
        Serial.println("ERROR: Failed to open folder");
        isScanning = false;
        return;
    }

    for (File f = root.openNextFile(); f; f = root.openNextFile()) {
        if (f.isDirectory()) {
            scanTotal++;
        }
        f.close();
        delay(1);
    }
    root.rewindDirectory();

    Serial.printf("Found %d folders to scan\n", scanTotal);

    for (File f = root.openNextFile(); f; f = root.openNextFile()) {
        if (f.isDirectory() && folderCount < 20) {
            String dirname = String(f.name());
            if (!dirname.startsWith("/")) {
                dirname = (folder == "/") ? "/" + dirname : folder + "/" + dirname;
            }
            availableFolders[folderCount++] = dirname;
            scanProgress++;
            Serial.printf("  [%d/%d] %s\n", scanProgress, scanTotal, dirname.c_str());
            delay(5);
        }
        f.close();
    }

    root.close();
    isScanning = false;  // Выключаем флаг
    
    Serial.printf("=== Scan complete: %d folders found ===\n", folderCount);
    Serial.printf("Free Heap after scan: %d bytes\n\n", ESP.getFreeHeap());
}

void listAudioFiles(const String& folder) {
    fileCount = 0;
    scanProgress = 0;
    scanTotal = 0;
    isScanningFiles = true;
    currentFolder = folder;
    
    Serial.printf("\n=== Listing audio files in: %s ===\n", folder.c_str());
    Serial.printf("MAX_FILES limit: %d\n", MAX_FILES);
    Serial.printf("Free Heap before listing: %d bytes\n", ESP.getFreeHeap());
    
    File root = SD.open(folder);
    if (!root) {
        Serial.println("ERROR: Failed to open folder");
        isScanningFiles = false;
        return;
    }
    
    if (!root.isDirectory()) {
        Serial.println("ERROR: Not a directory");
        root.close();
        isScanningFiles = false;
        return;
    }

    File file = root.openNextFile();
    while (file) {
        if (!file.isDirectory()) {
            String fname = String(file.name());
            String lower = fname;
            lower.toLowerCase();
            int dot = lower.lastIndexOf('.');
            
            if (dot >= 0) {
                String ext = lower.substring(dot + 1);
                if (ext == "mp3" || ext == "wav") {
                    scanTotal++;
                }
            }
        }
        file.close();
        file = root.openNextFile();
        delay(1);
    }
    
    root.close();
    root = SD.open(folder);
    
    Serial.printf("Found %d audio files\n", scanTotal);

    file = root.openNextFile();
    while (file && fileCount < MAX_FILES) {
        if (!file.isDirectory()) {
            String fname = String(file.name());

            if (!fname.startsWith("/")) {
                if (folder == "/") {
                    fname = "/" + fname;
                } else {
                    fname = folder + "/" + fname;
                }
            }

            String lower = fname;
            lower.toLowerCase();
            int dot = lower.lastIndexOf('.');
            
            if (dot >= 0) {
                String ext = lower.substring(dot + 1);
                if (ext == "mp3" || ext == "wav") {
                    audioFiles[fileCount] = fname;
                    fileCount++;
                    scanProgress++;
                    
                    Serial.printf("  [%d/%d] %s\n", scanProgress, scanTotal, fname.c_str());

                    if (fileCount % 10 == 0) {
                        uint32_t freeHeap = ESP.getFreeHeap();
                        Serial.printf("    -> Files loaded: %d, Free Heap: %d bytes\n", fileCount, freeHeap);

                        if (freeHeap < 10000) {
                            Serial.println("WARNING: Low memory! Stopping file scan.");
                            break;
                        }
                    }
                    
                    delay(5);
                }
            }
        }
        file.close();
        file = root.openNextFile();
    }
    
    root.close();
    isScanningFiles = false;
    
    if (fileCount >= MAX_FILES) {
        Serial.printf("\n!!! WARNING: Reached MAX_FILES limit (%d) !!!\n", MAX_FILES);
        Serial.println("!!! Some files were NOT loaded !!!");
        Serial.println("!!! Consider increasing MAX_FILES in file_manager.h !!!\n");
    }
    
    Serial.printf("=== Total audio files loaded: %d/%d ===\n", fileCount, MAX_FILES);
    Serial.printf("Free Heap after listing: %d bytes\n", ESP.getFreeHeap());

    if (ESP.getFreeHeap() < 20000) {
        Serial.println("\n!!! CRITICAL: Low memory detected !!!");
        Serial.println("!!! Device may become unstable !!!\n");
    }
}

String getFileName(int index) {
    if (index < 0 || index >= fileCount) return "";
    
    String fname = audioFiles[index];

    int lastSlash = fname.lastIndexOf('/');
    if (lastSlash >= 0) {
        fname = fname.substring(lastSlash + 1);
    }
    
    int lastDot = fname.lastIndexOf('.');
    if (lastDot > 0) {
        fname = fname.substring(0, lastDot);
    }
    
    return fname;
}
#include "file_manager.h"

String audioFiles[MAX_FILES];
int fileCount = 0;
int currentFileIndex = 0;
String currentFolder = "/";

String availableFolders[20];
int folderCount = 0;

bool initSDCard() {
    SPI.begin(SD_SCK, SD_MISO, SD_MOSI);
    if (!SD.begin(SD_CS)) {
        Serial.println("ERROR: SD Mount Failed!");
        return false;
    }
    Serial.println("SD Card initialized successfully");
    return true;
}

void scanAvailableFolders() {
    folderCount = 0;
    Serial.println("Scanning for folders with audio files...");

    availableFolders[folderCount++] = "/";

    File root = SD.open("/");
    if (!root || !root.isDirectory()) {
        Serial.println("Failed to open root directory");
        return;
    }
    
    File file = root.openNextFile();
    while (file && folderCount < 20) {
        if (file.isDirectory()) {
            String dirname = String(file.name());
            if (!dirname.startsWith("/")) {
                dirname = "/" + dirname;
            }
            Serial.printf("Found folder: %s\n", dirname.c_str());
            availableFolders[folderCount++] = dirname;
        }
        file = root.openNextFile();
    }
    root.close();
    
    Serial.printf("Total folders found: %d\n", folderCount);
}

void listAudioFiles(const String& folder) {
    fileCount = 0;
    currentFolder = folder;
    
    Serial.printf("Listing audio files in: %s\n", folder.c_str());
    
    File root = SD.open(folder);
    if (!root) {
        Serial.println("Failed to open folder");
        return;
    }
    
    if (!root.isDirectory()) {
        Serial.println("Not a directory");
        root.close();
        return;
    }
    
    File file = root.openNextFile();
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
                    Serial.printf("  [%d] %s\n", fileCount, fname.c_str());
                    fileCount++;
                }
            }
        }
        file = root.openNextFile();
    }
    
    root.close();
    
    if (fileCount >= MAX_FILES) {
        Serial.printf("WARNING: Reached MAX_FILES limit (%d). Some files were not loaded.\n", MAX_FILES);
    }
    
    Serial.printf("Total audio files found: %d\n", fileCount);
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
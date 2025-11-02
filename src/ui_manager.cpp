#include "ui_manager.h"
#include "font.h"
#include "battery.h"
#include "file_manager.h"
#include "audio_config.h"
#include <ESP32Time.h>

UIState currentUIState = UI_FOLDER_SELECT;
M5Canvas sprite(&M5Cardputer.Display);
M5Canvas spr(&M5Cardputer.Display);

#define KEY_ESC 27
#define KEY_DEL 127
#define KEY_ENTER 59
#define KEY_UP    0x80
#define KEY_DOWN  0x81

int bri = 2;
int brightness[5] = {60, 120, 180, 220, 255};
int sliderPos = 0;
int textPos = 90;
int graphSpeed = 0;
int g[14] = {0};
unsigned short grays[18];
unsigned short gray;
unsigned short light;

static int selectedFolderIndex = 0;
static int folderConfirmIndex = 0;
static ESP32Time rtc(0);

void initUI() {
    M5Cardputer.Display.setRotation(1);
    M5Cardputer.Display.setBrightness(brightness[bri]);
    sprite.createSprite(240, 135);
    spr.createSprite(86, 16);

    int co = 214;
    for (int i = 0; i < 18; i++) {
        grays[i] = M5Cardputer.Display.color565(co, co, co + 40);
        co = co - 13;
    }

    rtc.setTime(0, 0, 0, 17, 1, 2021);
    
    Serial.println("UI initialized");
}

void drawFolderSelect() {
    gray = grays[15];
    light = grays[11];
    
    sprite.fillRect(0, 0, 240, 135, gray);
    sprite.drawFastHLine(0, 0, 240, light);
    sprite.drawFastHLine(0, 134, 240, light);
    sprite.drawFastVLine(0, 0, 135, light);
    sprite.drawFastVLine(239, 0, 135, light);

    sprite.setTextFont(0);
    sprite.setTextDatum(4);
    sprite.setTextColor(ORANGE, gray);
    sprite.drawString("SELECT MUSIC FOLDER", 120, 15);

    sprite.setTextColor(grays[4], gray);
    sprite.setTextDatum(0);
    sprite.drawString("Use UP/DOWN arrows", 10, 100);
    sprite.drawString("ENTER to select", 10, 112);
    sprite.drawString("ESC to return here", 10, 124);

    sprite.setTextFont(0);
    int startY = 35;
    int lineHeight = 12;
    int maxVisible = 5;
    int startIdx = max(0, selectedFolderIndex - 2);
    
    for (int i = 0; i < maxVisible && (startIdx + i) < folderCount; i++) {
        int idx = startIdx + i;
        if (idx == selectedFolderIndex) {
            sprite.setTextColor(WHITE, BLUE);
            sprite.fillRect(8, startY + i * lineHeight - 1, 224, lineHeight, BLUE);
        } else {
            sprite.setTextColor(GREEN, gray);
        }
        
        String displayName = availableFolders[idx];
        if (displayName == "/") {
            displayName = "/ (Root)";
        }
        sprite.drawString(displayName.substring(0, 28), 10, startY + i * lineHeight);
    }
    
    sprite.pushSprite(0, 0);
}

void drawFolderConfirm() {
    gray = grays[15];
    light = grays[11];

    sprite.fillRect(0, 0, 240, 135, gray);
    sprite.setTextFont(0);
    sprite.setTextDatum(4);
    sprite.setTextColor(RED, gray);
    sprite.drawString("USE THIS FOLDER?", 120, 20);

    sprite.setTextDatum(0);
    sprite.setTextColor(GREEN, gray);
    sprite.drawString("Folder: " + availableFolders[folderConfirmIndex], 10, 60);
    sprite.drawString("ENTER - Yes", 10, 100);
    sprite.drawString("ESC - Back", 10, 112);

    sprite.pushSprite(0, 0);
}

void drawPlayer() {
    if (graphSpeed == 0) {
        gray = grays[15];
        light = grays[11];
        sprite.fillRect(0, 0, 240, 135, gray);
        sprite.fillRect(4, 8, 130, 122, BLACK);
        sprite.fillRect(129, 8, 5, 122, 0x0841);

        if (fileCount > 0) {
            sliderPos = map(currentFileIndex, 0, max(1, fileCount - 1), 8, 110);
        } else {
            sliderPos = 8;
        }
        sprite.fillRect(129, sliderPos, 5, 20, grays[2]);
        sprite.fillRect(131, sliderPos + 4, 1, 12, grays[16]);

        sprite.fillRect(4, 2, 50, 2, ORANGE);
        sprite.fillRect(84, 2, 50, 2, ORANGE);
        sprite.fillRect(190, 2, 45, 2, ORANGE);
        sprite.fillRect(190, 6, 45, 3, grays[4]);
        
        sprite.drawFastVLine(3, 9, 120, light);
        sprite.drawFastVLine(134, 9, 120, light);
        sprite.drawFastHLine(3, 129, 130, light);
        sprite.drawFastHLine(0, 0, 240, light);
        sprite.drawFastHLine(0, 134, 240, light);
        
        sprite.fillRect(139, 0, 3, 135, BLACK);
        sprite.fillRect(148, 14, 86, 42, BLACK);
        sprite.fillRect(148, 59, 86, 16, BLACK);

        sprite.fillTriangle(162, 18, 162, 26, 168, 22, GREEN);
        sprite.fillRect(162, 30, 6, 6, RED);
        
        sprite.drawFastVLine(143, 0, 135, light);
        sprite.drawFastVLine(238, 0, 135, light);
        sprite.drawFastVLine(138, 0, 135, light);
        sprite.drawFastVLine(148, 14, 42, light);
        sprite.drawFastHLine(148, 14, 86, light);

        for (int i = 0; i < 4; i++)
            sprite.fillRoundRect(148 + (i * 22), 94, 18, 18, 3, grays[4]);

        sprite.fillRect(220, 104, 8, 2, grays[13]);
        sprite.fillRect(220, 108, 8, 2, grays[13]);
        sprite.fillTriangle(228, 102, 228, 106, 231, 105, grays[13]);
        sprite.fillTriangle(220, 106, 220, 110, 217, 109, grays[13]);
        
        if (!stoped) {
            sprite.fillRect(152, 104, 3, 6, grays[13]);
            sprite.fillRect(157, 104, 3, 6, grays[13]);
        } else {
            sprite.fillTriangle(156, 102, 156, 110, 160, 106, grays[13]);
        }

        sprite.fillRoundRect(172, 82, 60, 3, 2, YELLOW);
        sprite.fillRoundRect(155 + ((volume / 5) * 17), 80, 10, 8, 2, grays[2]);
        sprite.fillRoundRect(157 + ((volume / 5) * 17), 82, 6, 4, 2, grays[10]);

        sprite.fillRoundRect(172, 124, 30, 3, 2, MAGENTA);
        sprite.fillRoundRect(172 + (bri * 5), 122, 10, 8, 2, grays[2]);
        sprite.fillRoundRect(174 + (bri * 5), 124, 6, 4, 2, grays[10]);

        sprite.drawRect(206, 119, 28, 12, GREEN);
        sprite.fillRect(234, 122, 3, 6, GREEN);

        for (int i = 0; i < 14; i++) {
            if (!stoped)
                g[i] = random(1, 5);
            for (int j = 0; j < g[i]; j++)
                sprite.fillRect(172 + (i * 4), 50 - j * 3, 3, 2, grays[4]);
        }

        sprite.setTextFont(0);
        sprite.setTextDatum(0);
        
        if (fileCount == 0) {
            sprite.setTextColor(RED, BLACK);
            sprite.drawString("No files found!", 8, 50);
        } else {
            int startIdx = max(0, currentFileIndex - 5);
            for (int i = 0; i < 10 && (startIdx + i) < fileCount; i++) {
                int idx = startIdx + i;
                if (idx == currentFileIndex) {
                    sprite.setTextColor(WHITE, BLACK);
                } else {
                    sprite.setTextColor(GREEN, BLACK);
                }
                sprite.drawString(getFileName(idx).substring(0, 20), 8, 10 + (i * 12));
            }
        }

        sprite.setTextColor(grays[1], gray);
        sprite.drawString("WINAMP", 150, 4);
        sprite.setTextColor(grays[2], gray);
        sprite.drawString("LIST", 58, 0);
        sprite.setTextColor(grays[4], gray);
        sprite.drawString("VOL", 150, 80);
        sprite.drawString("LIG", 150, 122);

        if (isPlaying) {
            sprite.setTextColor(grays[8], BLACK);
            sprite.drawString("P", 152, 18);
            sprite.drawString("L", 152, 27);
            sprite.drawString("A", 152, 36);
            sprite.drawString("Y", 152, 45);
        } else {
            sprite.setTextColor(grays[8], BLACK);
            sprite.drawString("S", 152, 18);
            sprite.drawString("T", 152, 27);
            sprite.drawString("O", 152, 36);
            sprite.drawString("P", 152, 45);
        }

        sprite.setTextColor(GREEN, BLACK);
        sprite.setFont(&DSEG7_Classic_Mini_Regular_16);
        if (!stoped)
            sprite.drawString(rtc.getTime().substring(3, 8), 172, 18);
        sprite.setTextFont(0);

        int percent = getBatteryPercent();
        sprite.setTextDatum(3);
        sprite.drawString(String(percent) + "%", 220, 121);

        sprite.setTextColor(BLACK, grays[4]);
        sprite.drawString("B", 220, 96);
        sprite.drawString("N", 198, 96);
        sprite.drawString("P", 176, 96);
        sprite.drawString("A", 154, 96);
        sprite.setTextColor(BLACK, grays[5]);
        sprite.drawString(">>", 202, 103);
        sprite.drawString("<<", 180, 103);

        spr.fillSprite(BLACK);
        spr.setTextColor(GREEN, BLACK);
        if (!stoped && fileCount > 0) {
            spr.drawString(getFileName(currentFileIndex), textPos, 4);
        }
        textPos -= 2;
        if (textPos < -300) textPos = 90;
        
        spr.pushSprite(&sprite, 148, 59);
        sprite.pushSprite(0, 0);
    }
    
    graphSpeed++;
    if (graphSpeed == 4) graphSpeed = 0;
}

void draw() {
        if (currentUIState == UI_FOLDER_SELECT) {
            drawFolderSelect();
        } else if (currentUIState == UI_FOLDER_CONFIRM) {
            drawFolderConfirm();
        } else {
            drawPlayer();
        }
    }

void handleKeyPress(char key) {
    if (currentUIState == UI_FOLDER_SELECT) {
        if (key == ';' || key == KEY_UP) {
            selectedFolderIndex--;
            if (selectedFolderIndex < 0) selectedFolderIndex = folderCount - 1;
        } else if (key == '.' || key == KEY_DOWN) {
            selectedFolderIndex++;
            if (selectedFolderIndex >= folderCount) selectedFolderIndex = 0;
        } else if (key == KEY_ENTER) {
            folderConfirmIndex = selectedFolderIndex;
            currentUIState = UI_FOLDER_CONFIRM;
        }
    }
    else if (currentUIState == UI_FOLDER_CONFIRM) {
        if (key == KEY_ENTER) {
            listAudioFiles(availableFolders[folderConfirmIndex]);
            currentFileIndex = 0;
            currentUIState = UI_PLAYER;
            textPos = 90;
            rtc.setTime(0, 0, 0, 17, 1, 2021);
        } else if (key == KEY_ESC || key == KEY_DEL) {
            currentUIState = UI_FOLDER_SELECT;
        }
    } 
    else {
        if (key == KEY_ESC || key == KEY_DEL) {
            audio.stopSong();
            isPlaying = false;
            stoped = true;
            currentUIState = UI_FOLDER_SELECT;
            selectedFolderIndex = 0;
            scanAvailableFolders();
        } else if (key == 'a') {
            isPlaying = !isPlaying;
            stoped = !stoped;
        } else if (key == 'v') {
            volume += 5;
            if (volume > 20) volume = 5;
            audio.setVolume(volume);
        } else if (key == 'l') {
            bri++;
            if (bri == 5) bri = 0;
            M5Cardputer.Display.setBrightness(brightness[bri]);
        } else if (key == 'n') {
            rtc.setTime(0, 0, 0, 17, 1, 2021);
            textPos = 90;
            currentFileIndex++;
            if (currentFileIndex >= fileCount) currentFileIndex = 0;
        } else if (key == 'p') {
            rtc.setTime(0, 0, 0, 17, 1, 2021);
            textPos = 90;
            currentFileIndex--;
            if (currentFileIndex < 0) currentFileIndex = fileCount - 1;
        } else if (key == ';') {
            currentFileIndex--;
            if (currentFileIndex < 0) currentFileIndex = fileCount - 1;
        } else if (key == '.') {
            currentFileIndex++;
            if (currentFileIndex >= fileCount) currentFileIndex = 0;
        } else if (key == 'b') {
            rtc.setTime(0, 0, 0, 17, 1, 2021);
            textPos = 90;
            currentFileIndex = random(0, fileCount);
        }
    }
}
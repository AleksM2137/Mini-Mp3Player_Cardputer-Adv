#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include <Arduino.h>
#include "M5Cardputer.h"

enum UIState {
    UI_FOLDER_SELECT,
    UI_FOLDER_CONFIRM,
    UI_PLAYER
};

extern UIState currentUIState;
extern M5Canvas sprite;
extern M5Canvas spr;
extern int bri;
extern int brightness[5];
extern int sliderPos;
extern int textPos;
extern int graphSpeed;
extern int g[14];
extern unsigned short grays[18];
extern unsigned short gray;
extern unsigned short light;

void initUI();
void drawFolderSelect();
void drawPlayer();
void draw();
void handleKeyPress(char key);

#endif
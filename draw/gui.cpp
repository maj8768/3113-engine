#include <string>
#include "raylib.h"
#include "../util.h"
#include "gui.h"



void guiDrawStartMenu(float p1X, float p1Y, float width, float height, Color color, int ballsSelected) {
    DrawRectangle(p1X, p1Y, width, height+ 20, color);
    DrawText("Lunar Lander", p1X+50, p1Y+25, 38, BLACK);
    DrawText("Press ENTER to start!", p1X+50, p1Y+100, 20, BLACK);
}

void guiDrawStartPopup(float p1X, float p1Y, float width, float height, Color color) {
//     DrawRectangle(p1X, p1Y, width, height, color);
    DrawRectangle(p1X, p1Y, width, height+ 20, color);
    DrawText("WASD to move", p1X +60, p1Y+25, 30, BLACK);
    DrawText("SPACE to +thrust", p1X +30, p1Y+75, 30, BLACK);
    DrawText("This is really difficult-- I'm sorry :( When the \nlight turns red, and you hear the sound\n you should start holding space to not blow up", p1X +10, p1Y+125, 15, BLACK);
}

void guiDrawEndPopup(float p1X, float p1Y, float width, float height, Color color, int player) {
    DrawText("Game Over!", p1X + 20, p1Y - 140, 20, BLACK);
    std::string mes = "The ";
    mes += (player == 1) ? "left" : "right";
    mes += " player has died in battle";
    DrawText(mes.c_str(), p1X - 40, p1Y - 110, 20, BLACK);
}

void guiDrawFailure(float p1X, float p1Y, float width, float height, Color color) {
    DrawRectangle(p1X, p1Y, width, height+ 20, color);
    DrawText("Lunar Lander", p1X+50, p1Y+25, 38, BLACK);
    DrawText("You went to fast on reentry, you are dead", p1X+10, p1Y+100, 20, BLACK);
}

void guiDrawSuccess(float p1X, float p1Y, float width, float height, Color color) {
    DrawRectangle(p1X, p1Y, width, height+ 20, color);
    DrawText("Lunar Lander", p1X+50, p1Y+25, 38, BLACK);
    DrawText("You landed the lander!", p1X+50, p1Y+100, 20, BLACK);
}

void guiDrawText(float p1X, float p1Y, const char* text, int fontSize, Color color) {
    DrawText(text, p1X, p1Y, fontSize, color);
}

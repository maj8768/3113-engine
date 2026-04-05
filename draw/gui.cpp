#include <string>
#include <iostream>
#include "raylib.h"
#include "../util.h"
#include "gui.h"



void guiDrawStartMenu(float p1X, float p1Y, float width, float height, Color color, int ballsSelected) {
    DrawRectangle(p1X, p1Y, width, height+ 20, color);
    DrawText("The Game", p1X+50, p1Y+25, 38, BLACK);
    DrawText("Press ENTER to start!", p1X+50, p1Y+100, 20, BLACK);
}

// void guiDrawHUD(float p1X, float p1Y, Color color, gameData gData, meshedObject cheese) {
// //    std::cout << "JAMAJ" << std::endl;
//     DrawText((std::string("alt: ") + std::to_string(gData.alt)).c_str(), p1X, p1Y-50, 30, color);
//     DrawText((std::string("vel: ") + std::to_string(-cheese.pEntity.magnitude.y)).c_str(), p1X, p1Y-25, 30, color); // lol negative because cheese is moving not you.
// }

void guiDrawStartPopup(float p1X, float p1Y, float width, float height, Color color) {
//     DrawRectangle(p1X, p1Y, width, height, color);
    DrawRectangle(p1X, p1Y, width, height+ 20, color);
    DrawText("WASD to move", p1X +60, p1Y+25, 30, BLACK);
    DrawText("SPACE to JUMP", p1X +30, p1Y+75, 30, BLACK);
    DrawText("This is really difficult-- I'm sorry :(\nhave fun :)))", p1X +10, p1Y+125, 15, BLACK);
}

void guiDrawEndPopup(float p1X, float p1Y, float width, float height, Color color, int player) {
    DrawText("Game Over!", p1X + 20, p1Y - 140, 20, BLACK);
    std::string mes = "The ";
    mes += (player == 1) ? "left" : "right";
    mes += " player has died in battle";
    DrawText(mes.c_str(), p1X - 40, p1Y - 110, 20, BLACK);
}

void guiDrawFailure(float p1X, float p1Y, float width, float height, Color color) {
    DrawRectangle(p1X, p1Y, width, height+ 45, color);
    DrawText("The Game", p1X+50, p1Y+25, 38, BLACK);
    DrawText("You ran out of lives :(", p1X+10, p1Y+100, 20, BLACK);
    DrawText("Press R to restart", p1X+10, p1Y+125, 20, BLACK);
}

void guiDrawSuccess(float p1X, float p1Y, float width, float height, Color color) {
    DrawRectangle(p1X, p1Y, width, height+ 45, color);
    DrawText("The Game", p1X+50, p1Y+25, 38, BLACK);
    DrawText("Congratulations! You won!", p1X+50, p1Y+100, 15, BLACK);
    DrawText("Press R to restart", p1X+10, p1Y+125, 20, BLACK);
}

void guiDrawText(float p1X, float p1Y, const char* text, int fontSize, Color color) {
    DrawText(text, p1X, p1Y, fontSize, color);
}

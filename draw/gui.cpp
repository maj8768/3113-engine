#include <string>
#include <iostream>
#include "raylib.h"
#include "../util.h"
#include "gui.h"

buyState bs = START;

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

void guiBuyMenu(float screen_width, float screen_height, player& player, int buy_guy) {
    float tleftx = screen_width/2-200;
    float tlefty = screen_height/2-200;
    DrawRectangle(tleftx-1, tlefty-1, 402, 402, WHITE);
    DrawRectangle(tleftx, tlefty, 400, 400, BLACK);
    DrawText("Buy Menu", tleftx + 15, tlefty + 15, 32, WHITE);
    
    switch(bs) {
        case(START):
            if(player.pState.hasDrank) {
                DrawText("Buy Drank $12.00", tleftx + 50, tlefty + 150, 24, WHITE);
            }
            else if (player.pState.hasCig) {
                DrawText("Buy Cigs $14.00", tleftx + 50, tlefty + 150, 24, WHITE);
            }
            DrawText("Goodbye $0.00 :)", tleftx + 50, tlefty + 175, 24, WHITE);
            break;
        default:
            DrawText("Goodbye $0.00 :)", tleftx + 50, tlefty + 175, 24, WHITE);
            break;
    }
}

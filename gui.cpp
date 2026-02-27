#include <string.h>
#include "raylib.h"
#include "util.h"
#include "gui.h"

void guiDrawStartMenu(float p1X, float p1Y, float width, float height, Color color, int ballsSelected) {
    DrawRectangle(p1X, p1Y, width, height+ 20, color);

    if (ballsSelected == 1) {
        DrawRectangle (p1X + width / 3 - 32, p1Y + height / 2 + 18, 34, 34, BLACK);
    }
    else if (ballsSelected == 2) {
        DrawRectangle (p1X + width / 3 + 13, p1Y + height / 2 + 18, 34, 34, BLACK);
    }
    else if (ballsSelected == 3) {
        DrawRectangle (p1X + width / 3 + 58, p1Y + height / 2 + 18, 34, 34, BLACK);
    }

    DrawText("How many balls?", p1X + 20, p1Y + height / 2 - 10, 20, BLACK);
    DrawRectangle (p1X + width / 3 - 30, p1Y + height / 2 + 20, 30, 30, GRAY);
    DrawText("1", p1X + width / 3 - 20, p1Y + height / 2 + 25, 20, BLACK);
    DrawRectangle (p1X + width / 3 + 15, p1Y + height / 2   + 20, 30, 30, GRAY);
    DrawText("2", p1X + width / 3 + 25, p1Y + height / 2 + 25, 20, BLACK);
    DrawRectangle (p1X + width / 3 + 60, p1Y + height / 2 + 20, 30, 30, GRAY);
    DrawText("3", p1X + width / 3 + 70, p1Y + height / 2 + 25, 20, BLACK);
    DrawText("Use number keys to select balls", p1X - 10, p1Y + height / 2 + 75, 15, BLACK);
    DrawText("Press ENTER to select", p1X - 20, p1Y + height / 2 + 90, 20, BLACK);
}

void guiDrawStartPopup(float p1X, float p1Y, float width, float height, Color color) {
    // DrawRectangle(p1X, p1Y, width, height, color);
    DrawText("Press ENTER to Start", p1X - 40, p1Y + height / 2 - 10, 20, BLACK);
}

void guiDrawEndPopup(float p1X, float p1Y, float width, float height, Color color, int player) {
    DrawText("Game Over!", p1X - 20, p1Y + height / 2 - 10, 20, BLACK);
    // char endMessageTooBigLol[200] = "Player: ";
    // char playerNum[4];
    // playerNum[0] = '0' + player;
    // playerNum[1] = '\0';
    // strcat(endMessageTooBigLol, playerNum);
    // strcat(endMessageTooBigLol, " has died in battle");
    // endMessageTooBigLol[8] += '0' + player;
    // endMessageTooBigLol[9] += "has died in battle";
    // DrawText("Game Over!", p1X - 20, p1Y + height / 2 - 10, 20, BLACK);
    // DrawText((const char*)endMessageTooBigLol, p1X - 40, p1Y + height / 2 + 20, 20, BLACK);
}
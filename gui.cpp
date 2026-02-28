#include <string>
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
    DrawText("Press ENTER to Start 2 Player(ARROW+WASD)", p1X - 120, p1Y - 100, 20, BLACK);
    DrawText("Press T (only if 1 ball) to Start 1 Player (WASD)", p1X - 150, p1Y - 140, 20, BLACK);
}

void guiDrawEndPopup(float p1X, float p1Y, float width, float height, Color color, int player) {
    DrawText("Game Over!", p1X + 20, p1Y - 140, 20, BLACK);
    std::string mes = "The ";
    mes += (player == 1) ? "left" : "right";
    mes += " player has died in battle";
    DrawText(mes.c_str(), p1X - 40, p1Y - 110, 20, BLACK);
}
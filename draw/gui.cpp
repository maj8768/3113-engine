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

void guiBuyMenu(float screen_width, float screen_height, player& player, int buy_guy) {
    // std::cout << (bs.selection == buySelection::ONE) << std::endl;
    // std::cout << (bs.selection == buySelection::TWO) << std::endl;
    // std::cout << (bs.selection == buySelection::THREE) << std::endl;

    static Texture2D shopGuy = { 0 };
    if (shopGuy.id == 0)
        shopGuy = LoadTexture("resources/levels/testing/shopguy.PNG");

    // Menu is 600px wide: 400px left panel + 200px right panel
    float tleftx = screen_width/2 - 300;
    float tlefty = screen_height/2 - 200;
    DrawRectangle(tleftx-1, tlefty-1, 602, 402, WHITE);
    DrawRectangle(tleftx, tlefty, 600, 400, BLACK);

    // Divider
    DrawRectangle(tleftx + 400, tlefty, 1, 400, WHITE);

    // Right panel — shopguy image (150x150) centered in the 200px column
    float imgX = tleftx + 400 + 25;   // 25px left padding
    float imgY = tlefty + 30;
    if (shopGuy.id != 0) {
        Rectangle src = { 0, 0, (float)shopGuy.width, (float)shopGuy.height };
        Rectangle dst = { imgX, imgY, 150, 150 };
        DrawTexturePro(shopGuy, src, dst, { 0, 0 }, 0, WHITE);
    }
    const char* shopName = "Dragan Nikolic";
    int nameW = MeasureText(shopName, 16);
    DrawText(shopName, imgX + (150 - nameW) / 2, imgY + 158, 16, WHITE);
    DrawText("Buy Menu", tleftx + 15, tlefty + 15, 32, WHITE);
    switch(bs.selection) {
        case buySelection::ONE:
            DrawText(">", tleftx + 15, tlefty + 150, 24, WHITE);
            break;
        case buySelection::TWO:
            DrawText(">", tleftx + 15, tlefty + 190, 24, WHITE);
            break;
        case buySelection::THREE:
            DrawText(">", tleftx + 15, tlefty + 230, 24, WHITE);
            break;
    }
    switch(bs.state) {
        case(buyState::START):
            if(player.pState.hasDrank) {
                if (player.pState.canDrinkDrank) {
                    DrawText("\"You already bought drank.\"", tleftx + 15, tlefty + 75, 24, WHITE);
                    DrawText("Goodbye", tleftx + 50, tlefty + 190, 24, WHITE);
                }
                else {
                    DrawText("\"You tryna buy drank?\"", tleftx + 15, tlefty + 75, 24, WHITE);
                    DrawText("Buy Drank $12.00", tleftx + 50, tlefty + 150, 24, WHITE);
                }
            }
            else if (player.pState.hasCig) {
                if (player.pState.canSmokeCig) {
                    DrawText("\"You already bought cigs.\"", tleftx + 15, tlefty + 75, 24, WHITE);
                    DrawText("Goodbye", tleftx + 50, tlefty + 190, 24, WHITE);
                }
                else {
                    DrawText("\"You tryna buy cigs?\"", tleftx + 15, tlefty + 75, 24, WHITE);
                    DrawText("Buy Cigs $14.00", tleftx + 50, tlefty + 150, 24, WHITE);
                }
            }
            DrawText("Goodbye", tleftx + 50, tlefty + 190, 24, WHITE);
            break;
        case(buyState::DRANK_SELECT):
            DrawText("\"Are you sure?\"", tleftx + 15, tlefty + 75, 24, WHITE);
            DrawText("Yes", tleftx + 50, tlefty + 150, 24, WHITE);
            DrawText("No", tleftx + 50, tlefty + 190, 24, WHITE);
            break;
        case(buyState::CIG_SELECT):
            DrawText("\"Are you sure?\"", tleftx + 15, tlefty + 75, 24, WHITE);
            DrawText("Yes", tleftx + 50, tlefty + 150, 24, WHITE);
            DrawText("No", tleftx + 50, tlefty + 190, 24, WHITE);
            break;
        case(buyState::POOR):
            DrawText("\"You don't have enough\nmoney.\"", tleftx + 15, tlefty + 75, 24, WHITE);
            DrawText("Goodbye", tleftx + 50, tlefty + 190, 24, WHITE);
            break;
        case(buyState::GOODBYE):
            DrawText("\"Go away now\"", tleftx + 50, tlefty + 75, 24, WHITE);
            DrawText("Goodbye", tleftx + 50, tlefty + 190, 24, WHITE);
            break;
        case(buyState::THANKS):
            DrawText("\"You're loyalty will not \ngo unrewarded.\"", tleftx + 15, tlefty + 75, 24, WHITE);
            DrawText("Goodbye", tleftx + 50, tlefty + 190, 24, WHITE);
            break;
        default:
            DrawText("you shouldnt be here", tleftx + 50, tlefty + 190, 24, WHITE);
            break;
    }
}

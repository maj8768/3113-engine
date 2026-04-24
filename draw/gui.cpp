#include <string>
#include <iostream>
#include <cstdio>
#include <algorithm>
#include "raylib.h"
#include "../util.h"
#include "gui.h"

void guiUberPickup(float sw, float sh, const char* passengerName, const char* destName, int selection, Texture2D& portrait, float reward) {
    float tleftx = sw/2 - 300;
    float tlefty = sh/2 - 200;
    DrawRectangle(tleftx-1, tlefty-1, 602, 402, WHITE);
    DrawRectangle(tleftx, tlefty, 600, 400, BLACK);
    DrawRectangle(tleftx + 400, tlefty, 1, 400, WHITE);

    float imgX = tleftx + 400 + 25;
    float imgY = tlefty + 30;
    if (portrait.id != 0) {
        Rectangle src = {0, 0, (float)portrait.width, (float)portrait.height};
        Rectangle dst = {imgX, imgY, 150, 150};
        DrawTexturePro(portrait, src, dst, {0,0}, 0, WHITE);
    }
    int nameW = MeasureText(passengerName, 13);
    DrawText(passengerName, (int)(imgX + (150 - nameW)/2), (int)(imgY + 158), 13, WHITE);
    DrawText("Ride Request", tleftx + 15, tlefty + 15, 32, WHITE);

    char dialogue[96];
    snprintf(dialogue, sizeof(dialogue), "\"Take me to %s?\"", destName);
    DrawText(dialogue, tleftx + 15, tlefty + 75, 20, WHITE);
    char acceptLine[32];
    snprintf(acceptLine, sizeof(acceptLine), "Accept  +$%.0f.00", reward);
    DrawText(acceptLine, tleftx + 50, tlefty + 140, 24, WHITE);
    DrawText("Goodbye", tleftx + 50, tlefty + 190, 24, WHITE);

    float cursorY = (selection == 0) ? tlefty + 140 : tlefty + 190;
    DrawText(">", tleftx + 15, cursorY, 24, WHITE);
}

void guiUberArrived(float sw, float sh, float reward) {
    int bw = 380, bh = 140;
    int bx = (sw - bw) / 2;
    int by = sh / 2 - 160;
    DrawRectangle(bx-1, by-1, bw+2, bh+2, WHITE);
    DrawRectangle(bx, by, bw, bh, BLACK);
    char rewardStr[24];
    snprintf(rewardStr, sizeof(rewardStr), "+$%.0f.00", reward);
    const char* thanks = "Thank you for the ride!";
    const char* prompt = "Press Enter";
    int tw = MeasureText(thanks, 24);
    int rw = MeasureText(rewardStr, 30);
    int pw = MeasureText(prompt, 18);
    DrawText(thanks, bx + (bw-tw)/2, by + 15, 24, WHITE);
    DrawText(rewardStr, bx + (bw-rw)/2, by + 50, 30, GREEN);
    DrawText(prompt, bx + (bw-pw)/2, by + 105, 18, GRAY);
}

void guiDrawEscapeMenu() {
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, {0, 0, 0, 160});

    const char* line1 = "Press R to Restart";
    const char* line2 = "Escape to go back to game";
    int w1 = MeasureText(line1, 36);
    int w2 = MeasureText(line2, 24);
    DrawText(line1, (SCREEN_WIDTH - w1) / 2, SCREEN_HEIGHT / 2 - 30, 36, WHITE);
    DrawText(line2, (SCREEN_WIDTH - w2) / 2, SCREEN_HEIGHT / 2 + 20, 24, GRAY);
}

void guiDrawStartMenu(float p1X, float p1Y, float width, float height, Color color, int ballsSelected) {
    const char* title = "The Game";
    const char* prompt = "Press Enter";
    int titleW = MeasureText(title, 64);
    int promptW = MeasureText(prompt, 28);
    DrawText(title, (SCREEN_WIDTH - titleW) / 2, SCREEN_HEIGHT / 2 - 60, 64, WHITE);
    DrawText(prompt, (SCREEN_WIDTH - promptW) / 2, SCREEN_HEIGHT / 2 + 20, 28, GRAY);
}

void guiDrawStartPopup(float p1X, float p1Y, float width, float height, Color color) {
    static Texture2D mapTex = {0};
    if (mapTex.id == 0)
        mapTex = LoadTexture("resources/levels/testing/map.png");

    int mapW = SCREEN_WIDTH / 2;
    if (mapTex.id != 0) {
        float scaleX = (float)mapW / mapTex.width;
        float scaleY = (float)SCREEN_HEIGHT / mapTex.height;
        float scale  = std::min(scaleX, scaleY);
        float dstW   = mapTex.width  * scale;
        float dstH   = mapTex.height * scale;
        float dstX   = (mapW - dstW) / 2.f;
        float dstY   = (SCREEN_HEIGHT - dstH) / 2.f;
        Rectangle src = {0, 0, (float)mapTex.width, (float)mapTex.height};
        Rectangle dst = {dstX, dstY, dstW, dstH};
        DrawTexturePro(mapTex, src, dst, {0, 0}, 0, WHITE);
    } else {
        DrawRectangle(0, 0, mapW, SCREEN_HEIGHT, {20, 20, 20, 255});
    }
    DrawRectangle(mapW, 0, 2, SCREEN_HEIGHT, {80, 80, 80, 255});


    int rx  = mapW + 20;
    int rw  = SCREEN_WIDTH - mapW - 40;
    int pad = 10;

    DrawText("Instructions", rx, 20, 26, WHITE);
    DrawRectangle(rx, 52, rw, SCREEN_HEIGHT / 2 - 72, {25, 25, 25, 220});


    const char* instructions[] = {
        "REMEMBER AT THE MAP!",
        "",
        "You will die if you don't drink every 2 minutes 50 seconds",
        "You need money, drive uber rides to earn cash",
        "Don't run out of fuel, fill up at the gas station",
        "Press F to interact with things",
        "Survive 3 days to win!",
    };
    int instrCount = (int)(sizeof(instructions) / sizeof(instructions[0]));
    for (int i = 0; i < instrCount; i++)
        DrawText(instructions[i], rx + pad, 62 + i * 22, 18, LIGHTGRAY);

    // Controls box
    int ctrlY = SCREEN_HEIGHT / 2 + 10;
    DrawText("Controls", rx, ctrlY, 26, WHITE);
    DrawRectangle(rx, ctrlY + 32, rw, SCREEN_HEIGHT / 2 - 72, {25, 25, 25, 220});


    const char* controls[] = {
        "WASD        Move / steer car",
        "E / Q        Shift up / down",
        "R              Reverse",
        "F              Interact",
        "G              Drop drink",
        "N              Noclip (debug)",
        "ESC          Pause menu",
    };
    int ctrlCount = (int)(sizeof(controls) / sizeof(controls[0]));
    for (int i = 0; i < ctrlCount; i++)
        DrawText(controls[i], rx + pad, ctrlY + 42 + i * 22, 18, LIGHTGRAY);

    // Press Enter (bottom right)
    const char* prompt = "Press Enter";
    int pw = MeasureText(prompt, 22);
    DrawText(prompt, SCREEN_WIDTH - pw - 20, SCREEN_HEIGHT - 34, 22, GRAY);
}

void guiDrawEndPopup(float p1X, float p1Y, float width, float height, Color color, int player) {
    DrawText("Game Over!", p1X + 20, p1Y - 140, 20, BLACK);
    std::string mes = "The ";
    mes += (player == 1) ? "left" : "right";
    mes += " player has died in battle";
    DrawText(mes.c_str(), p1X - 40, p1Y - 110, 20, BLACK);
}

void guiDrawFailure(float p1X, float p1Y, float width, float height, Color color) {
    const char* title = "You Failed";
    const char* sub = "Press Enter to Restart";
    int titleW = MeasureText(title, 64);
    int subW = MeasureText(sub, 28);
    DrawText(title, (SCREEN_WIDTH - titleW) / 2, SCREEN_HEIGHT / 2 - 60, 64, WHITE);
    DrawText(sub, (SCREEN_WIDTH - subW) / 2, SCREEN_HEIGHT / 2 + 20, 28, GRAY);
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

void guiGasMenu(float screen_width, float screen_height, player& player, int selection, Texture2D& portrait) {
    float tleftx = screen_width/2 - 300;
    float tlefty = screen_height/2 - 200;
    DrawRectangle(tleftx-1, tlefty-1, 602, 402, WHITE);
    DrawRectangle(tleftx, tlefty, 600, 400, BLACK);
    DrawRectangle(tleftx + 400, tlefty, 1, 400, WHITE);

    float imgX = tleftx + 400 + 25;
    float imgY = tlefty + 30;
    if (portrait.id != 0) {
        Rectangle src = {0, 0, (float)portrait.width, (float)portrait.height};
        Rectangle dst = {imgX, imgY, 150, 150};
        DrawTexturePro(portrait, src, dst, {0, 0}, 0, WHITE);
    }
    const char* name = "Friendly Gas Guy";
    int nameW = MeasureText(name, 16);
    DrawText(name, imgX + (150 - nameW) / 2, imgY + 158, 16, WHITE);
    DrawText("Gas Station", tleftx + 15, tlefty + 15, 32, WHITE);

    float currentGallons = player.pState.carFuel / 4.0f;
    float gallonsNeeded = (100.0f - player.pState.carFuel) / 4.0f;
    float maxAfford = std::min(gallonsNeeded, player.pState.money / 2.5f);
    float affordCost = maxAfford * 2.5f;
    bool full = player.pState.carFuel >= 100.f;
    bool broke = player.pState.money <= 0.0f;

    char fuelLine[32];
    snprintf(fuelLine, sizeof(fuelLine), "%.1fg / 25g", currentGallons);
    int fuelW = MeasureText(fuelLine, 18);
    DrawText(fuelLine, (int)(tleftx + 395 - fuelW), (int)(tlefty + 370), 18, RED);

    if (broke) {
        DrawText("\"You're broke, man.\"", tleftx + 15, tlefty + 75, 24, WHITE);
        DrawText(">", tleftx + 15, tlefty + 190, 24, WHITE);
        DrawText("Close  [F]", tleftx + 50, tlefty + 190, 24, WHITE);
    } else if (full) {
        DrawText("\"Tank is full!\"", tleftx + 15, tlefty + 75, 24, WHITE);
        DrawText(">", tleftx + 15, tlefty + 190, 24, WHITE);
        DrawText("Close  [F]", tleftx + 50, tlefty + 190, 24, WHITE);
    } else {
        DrawText("\"Need some gas?\"", tleftx + 15, tlefty + 75, 24, WHITE);

        char fillLine[64];
        snprintf(fillLine, sizeof(fillLine), "Fill (%.1fg)  $%.2f", maxAfford, affordCost);
        char plusLine[32];
        snprintf(plusLine, sizeof(plusLine), "+%.1fg", maxAfford);

        float fillY = tlefty + 140;
        float closeY = tlefty + 200;
        float cursorY = (selection == 0) ? fillY : closeY;
        DrawText(">", tleftx + 15, cursorY, 24, WHITE);
        DrawText(fillLine, tleftx + 50, fillY, 24, WHITE);
        DrawText(plusLine, tleftx + 50, fillY + 28, 18, GREEN);
        DrawText("Close  [F]", tleftx + 50, closeY, 24, WHITE);
    }
}

void guiBuyMenu(float screen_width, float screen_height, player& player, int buy_guy) {
    static Texture2D shopGuy = {0};
    if (shopGuy.id == 0)
    shopGuy = LoadTexture("resources/levels/testing/shopguy.PNG");
    static Texture2D gasGuy = {0};
    if (gasGuy.id == 0)
    gasGuy = LoadTexture("resources/levels/testing/gaspfp.PNG");

    float tleftx = screen_width/2 - 300;
    float tlefty = screen_height/2 - 200;
    DrawRectangle(tleftx-1, tlefty-1, 602, 402, WHITE);
    DrawRectangle(tleftx, tlefty, 600, 400, BLACK);
    DrawRectangle(tleftx + 400, tlefty, 1, 400, WHITE);

    float imgX = tleftx + 400 + 25;
    float imgY = tlefty + 30;
    Texture2D& portrait = (buy_guy == 2) ? gasGuy : shopGuy;
    if (portrait.id != 0) {
        Rectangle src = {0, 0, (float)portrait.width, (float)portrait.height};
        Rectangle dst = {imgX, imgY, 150, 150};
        DrawTexturePro(portrait, src, dst, {0, 0}, 0, WHITE);
    }
    const char* shopName = (buy_guy == 2) ? "Friendly Gas Guy" : "Dragan Nikolic";
    int nameW = MeasureText(shopName, 16);
    DrawText(shopName, imgX + (150 - nameW) / 2, imgY + 158, 16, WHITE);
    DrawText((buy_guy == 2) ? "Gas Station" : "Buy Menu", tleftx + 15, tlefty + 15, 32, WHITE);
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
        case(buyState::GAS_1_SELECT):
            if (buy_guy == 2) {
            DrawText("\"Are you sure?\"", tleftx + 15, tlefty + 75, 24, WHITE);
            DrawText("Yes", tleftx + 50, tlefty + 150, 24, WHITE);
            DrawText("No", tleftx + 50, tlefty + 190, 24, WHITE);
        }
        break;
        default:
            if (buy_guy == 2) {
            float gallonsNeeded = (100.0f - player.pState.carFuel) / 4.0f;
            float cost = gallonsNeeded * 2.5f;
            char fillLine[64];
            snprintf(fillLine, sizeof(fillLine), "Fill Up (%.1fg) - $%.2f", gallonsNeeded, cost);
            switch (bs.state) {
                case buyState::START:
                    DrawText((player.pState.carFuel >= 100.f) ? "\"Tank is full!\"" : "\"Need some gas?\"", tleftx + 15, tlefty + 75, 24, WHITE);
                if (player.pState.carFuel < 100.f) DrawText(fillLine, tleftx + 50, tlefty + 150, 24, WHITE);
                DrawText("Goodbye", tleftx + 50, tlefty + 190, 24, WHITE);
                break;
                case buyState::THANKS:
                    DrawText("\"Enjoy the ride!\"", tleftx + 15, tlefty + 75, 24, WHITE);
                DrawText("Goodbye", tleftx + 50, tlefty + 190, 24, WHITE);
                break;
                case buyState::POOR:
                    DrawText("\"You're broke, man.\"", tleftx + 15, tlefty + 75, 24, WHITE);
                DrawText("Goodbye", tleftx + 50, tlefty + 190, 24, WHITE);
                break;
                case buyState::GOODBYE:
                    DrawText("\"Come back soon!\"", tleftx + 15, tlefty + 75, 24, WHITE);
                DrawText("Goodbye", tleftx + 50, tlefty + 190, 24, WHITE);
                break;
                default:
                    DrawText("you shouldnt be here", tleftx + 50, tlefty + 190, 24, WHITE);
                break;
            }
        } else {
            DrawText("you shouldnt be here", tleftx + 50, tlefty + 190, 24, WHITE);
        }
        break;
    }
}

void guiDrawStartMenu(float p1X, float p1Y, float width, float height, Color color, int ballsSelected);
void guiDrawEscapeMenu();
void guiUberPickup(float sw, float sh, const char* passengerName, const char* destName, int selection, Texture2D& portrait, float reward);
void guiUberArrived(float sw, float sh, float reward);
void guiDrawStartPopup(float p1X, float p1Y, float width, float height, Color color);
void guiDrawEndPopup(float p1X, float p1Y, float width, float height, Color color, int player);
void guiDrawText(float p1X, float p1Y, const char* text, int fontSize, Color color);

void guiDrawSuccess(float p1X, float p1Y, float width, float height, Color color);
void guiDrawFailure(float p1X, float p1Y, float width, float height, Color color);
void guiDrawHUD(float p1X, float p1Y, Color color, gameData gData, meshedObject cheese);
void guiGasMenu(float screen_width, float screen_height, player& player, int selection, Texture2D& portrait);
void guiBuyMenu(float screen_width, float screen_height, player& player, int buy_guy);

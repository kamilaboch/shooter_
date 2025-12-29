#include "raylib.h"
#include <string>

using namespace std;

// Stany aplikacji
enum GameState { MENU, GAME };

int main() {
    const int W = 1280, H = 720;

    InitWindow(W, H, "Flanki Shooter");
    SetTargetFPS(60);

    GameState state = MENU;

    float playerX = W / 2.0f;
    float playerY = H - 60.0f;
    float speed = 400.0f;

    string nick = "";

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        // ===== MENU =====
        if (state == MENU) {

            // Wpisywanie nicku
            for (int key = GetCharPressed(); key > 0; key = GetCharPressed())
                if (key >= 32 && key <= 125 && nick.length() < 12)
                    nick += (char)key;

            // Usuwanie znaku
            if (IsKeyPressed(KEY_BACKSPACE) && !nick.empty())
                nick.pop_back();

            // Start gry tylko gdy podano nick
            if (IsKeyPressed(KEY_ENTER) && !nick.empty())
                state = GAME;

            BeginDrawing();
            ClearBackground(BLACK);

            DrawText("FLANKI SHOOTER", W / 2 - 220, 250, 50, RAYWHITE);
            DrawText("Nick:", W / 2 - 150, 320, 30, GRAY);
            DrawRectangle(W / 2 - 150, 360, 300, 40, DARKGRAY);
            DrawText(nick.c_str(), W / 2 - 140, 370, 25, RAYWHITE);

            EndDrawing();
            continue;
        }

        // ===== GRA =====
        if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))
            playerX -= speed * dt;
        if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT))
            playerX += speed * dt;

        if (playerX < 20) playerX = 20;
        if (playerX > W - 20) playerX = W - 20;

        if (IsKeyPressed(KEY_ESCAPE))
            state = MENU;

        BeginDrawing();
        ClearBackground(BLACK);

        DrawCircle((int)playerX, (int)playerY, 18, RAYWHITE);
        DrawText(TextFormat("Gracz: %s", nick.c_str()), 20, 20, 20, GRAY);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}

#include "raylib.h"
#include <fstream>


enum GameState {
    MENU,
    GAME
};

int main() {
    const int W = 1280;
    const int H = 720;

    InitWindow(W, H, "Flanki Shooter");
    SetTargetFPS(60);

    GameState state = MENU;

    float playerX = W / 2.0f;
    float playerY = H - 60.0f;
    float speed = 400.0f;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        // ================= MENU =================
        if (state == MENU) {
            if (IsKeyPressed(KEY_ENTER)) {
                playerX = W / 2.0f;
                state = GAME;
            }

            BeginDrawing();
            ClearBackground(BLACK);

            DrawText("FLANKI SHOOTER", W / 2 - 220, 250, 50, RAYWHITE);
            DrawText("ENTER - Start gry", W / 2 - 150, 330, 30, GRAY);
            DrawText("ESC - Wyjscie", W / 2 - 120, 370, 25, DARKGRAY);

            EndDrawing();
            continue;
        }

        if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))
            playerX -= speed * dt;
        if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT))
            playerX += speed * dt;

        if (playerX < 20) playerX = 20;
        if (playerX > W - 20) playerX = W - 20;

        if (IsKeyPressed(KEY_ESCAPE)) {
            state = MENU;
        }

        BeginDrawing();
        ClearBackground(BLACK);

        DrawCircle((int)playerX, (int)playerY, 18, RAYWHITE);
        DrawText("A/D lub strzalki - ruch", 20, 20, 20, GRAY);
        DrawText("ESC - menu", 20, 50, 20, GRAY);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
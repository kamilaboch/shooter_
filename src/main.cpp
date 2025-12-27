#include "raylib.h"

int main() {
    const int W = 1280;
    const int H = 720;

    InitWindow(W, H, "Flanki Shooter");
    SetTargetFPS(60);

    float playerX = W / 2.0f;
    float playerY = H - 60.0f;
    float speed = 400.0f;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))
            playerX -= speed * dt;
        if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT))
            playerX += speed * dt;

        if (playerX < 20) playerX = 20;
        if (playerX > W - 20) playerX = W - 20;

        BeginDrawing();
        ClearBackground(BLACK);

        DrawCircle((int)playerX, (int)playerY, 18, RAYWHITE);
        DrawText("A/D lub strzalki - ruch", 20, 20, 20, GRAY);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}

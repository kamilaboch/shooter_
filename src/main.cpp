
#include "raylib.h"

int main() {
    InitWindow(1280, 720, "Flanki Shooter");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BLACK);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}

#include "raylib.h"
#include <string>
#include <vector>
#include <fstream>

using namespace std;

// Stany aplikacji
enum GameState { MENU, GAME };

// PROFIL
struct Profile {
    string nick;
};

// ZARZ¥DZANIE PROFILAMI 
class ProfileManager {
public:
    vector<Profile> profiles;
    int selected = 0;

    void load() {
        ifstream in("profiles.dat");
        profiles.clear();
        Profile p;
        while (in >> p.nick)
            profiles.push_back(p);
    }

    void save() {
        ofstream out("profiles.dat");
        for (auto& p : profiles)
            out << p.nick <<"\n";
    }

    void addProfile(const string& nick) {
        profiles.push_back({ nick});
        selected = (int)profiles.size() - 1;
        save();
    }

    Profile& current() {
        return profiles[selected];
    }
};

int main() {
    const int W = 1280, H = 720;
    InitWindow(W, H, "Flanki Shooter");
    SetTargetFPS(60);

    GameState state = MENU;

    float playerX = W / 2.0f;
    float playerY = H - 60.0f;
    float speed = 400.0f;

    ProfileManager pm;
    pm.load();

    bool newProfileMode = false;
    string newNick = "";

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        // MENU
        if (state == MENU) {

            // NOWY PROFIL
            if (newProfileMode) {

                for (int key = GetCharPressed(); key > 0; key = GetCharPressed())
                    if (key >= 32 && key <= 125 && newNick.length() < 12)
                        newNick += (char)key;

                if (IsKeyPressed(KEY_BACKSPACE) && !newNick.empty())
                    newNick.pop_back();

                if (IsKeyPressed(KEY_ENTER) && !newNick.empty()) {
                    pm.addProfile(newNick);
                    newNick.clear();
                    newProfileMode = false;
                }

            }
            // WYBÓR PROFILU 
            else if (!pm.profiles.empty()) {
                int count = (int)pm.profiles.size();
                if (IsKeyPressed(KEY_UP))
                    pm.selected = (pm.selected - 1 + count) % count;

                if (IsKeyPressed(KEY_DOWN))
                    pm.selected = (pm.selected + 1) % count;

                if (IsKeyPressed(KEY_ENTER)) {
                    playerX = W / 2.0f;
                    state = GAME;
                }
            }

            if (IsKeyPressed(KEY_N)) {
                newProfileMode = true;
                newNick.clear();
            }

            //  RYSOWANIE MENU
            BeginDrawing();
            ClearBackground(BLACK);

            DrawText("FLANKI SHOOTER", W / 2 - 220, 120, 50, RAYWHITE);
            DrawText("Profile:", W / 2 - 200, 200, 30, GRAY);

            for (int i = 0; i < pm.profiles.size(); i++) {
                Color c = (i == pm.selected) ? YELLOW : GRAY;
                DrawText(pm.profiles[i].nick.c_str(),
                    W / 2 - 200, 240 + i * 30, 22, c);
            }

            DrawText("ENTER - wybierz", W / 2 - 200, H - 180, 20, DARKGRAY);
            DrawText("N - nowy profil", W / 2 - 200, H, 20, DARKGRAY);
            DrawText("ESC - wyjscie", W / 2 -200, H - 20, 20, DARKGRAY);
            if (newProfileMode) {
                DrawText("Nowy nick:", W / 2 - 150, 470, 25, GRAY);
                DrawRectangle(W / 2 - 150, 500, 300, 40, DARKGRAY);
                DrawText(newNick.c_str(), W / 2 - 140, 510, 25, RAYWHITE);
            }

            EndDrawing();
            continue;
        }

        // GRA 
        if (!pm.profiles.empty()) {
            if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))
                playerX -= speed * dt;
            if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT))
                playerX += speed * dt;

            if (playerX < 20) playerX = 20;
            if (playerX > W - 20) playerX = W - 20;

            if (IsKeyPressed(KEY_ESCAPE)) {
                state = MENU;
            }
        }
        BeginDrawing();
        ClearBackground(BLACK);


        DrawCircle((int)playerX, (int)playerY, 18, RAYWHITE);

        if (!pm.profiles.empty()) {
            DrawText(TextFormat("Gracz: %s", pm.current().nick.c_str()),
                20, 20, 20, GRAY);
        }
        else {
            DrawText("Brak profili - wcisnij N w menu", 20, 20, 20, GRAY);
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}

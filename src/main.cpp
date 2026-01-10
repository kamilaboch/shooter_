#include "raylib.h"
#include <string>
#include <vector>
#include <fstream>
#include <cmath>
#include <cstdlib>

using namespace std;

// Stany aplikacji
enum GameState { MENU, GAME, LEVEL_TRANSITION, WIN_SCREEN };

// --- STRUKTURY OSOBY 1 (Gameplay) ---
struct Bullet {
    float x, y;
    float speedY; float speedX;
    bool isEnemy;
    Color color;
    int damage;
};

struct Enemy {
    float x, y;
    float width, height;
    int hp; int maxHp;
    bool isBoss;
    float formationX, formationY;
    bool inPosition;
    Color baseColor;
};

struct PowerUp {
    float x, y; int type; bool active;
};

// Funkcje pomocnicze Osoby 1
Color GetRandomBottleColor() {
    int r = GetRandomValue(0, 3);
    switch (r) {
    case 0: return GREEN; break;
    case 1: return SKYBLUE; break;
    case 2: return BEIGE; break;
    case 3: return YELLOW; break;
    default: return ORANGE;
    }
}

Enemy CreateBasicEnemy(float formX, float formY, int hpBonus, float startOffsetY) {
    Enemy e;
    e.width = 40; e.height = 40;
    e.hp = 20 + hpBonus; e.maxHp = e.hp;
    e.isBoss = false;
    e.formationX = formX;
    e.formationY = formY;
    e.x = e.formationX;
    e.y = -100.0f - startOffsetY;
    e.inPosition = false;
    e.baseColor = GetRandomBottleColor();
    return e;
}

void SpawnLevel(vector<Enemy>& enemies, int level, int formationType) {
    int hpBonus = level * 15;
    float spacingX = 50.0f;
    float spacingY = 50.0f;
    float centerX = 1280 / 2.0f;
    float startY = 100.0f;

    switch (formationType) {
    case 0: // SIATKA
    {
        int rows = 3 + level / 2;
        int cols = 6 + (level % 3);
        float startX = (1280 - (cols * spacingX)) / 2.0f + spacingX / 2.0f;
        for (int row = 0; row < rows; ++row) {
            for (int col = 0; col < cols; ++col) {
                enemies.push_back(CreateBasicEnemy(startX + col * spacingX, startY + row * spacingY, hpBonus, row * 100.0f));
            }
        }
    }
    break;
    case 1: // V-KSZTAŁTNA
    {
        int count = 7 + level;
        for (int i = 0; i < count; ++i) {
            enemies.push_back(CreateBasicEnemy(centerX - (i * spacingX), startY + (i * spacingY), hpBonus, i * 80.0f));
            if (i > 0) enemies.push_back(CreateBasicEnemy(centerX + (i * spacingX), startY + (i * spacingY), hpBonus, i * 80.0f));
        }
    }
    break;
    case 2: // OKRĄG
    {
        int count = 12 + level * 2;
        float radius = 180.0f;
        float circleCenterY = 250.0f;
        for (int i = 0; i < count; ++i) {
            float angle = (float)i / count * 2.0f * PI;
            enemies.push_back(CreateBasicEnemy(centerX + cos(angle) * radius, circleCenterY + sin(angle) * radius, hpBonus, i * 50.0f));
        }
    }
    break;
    case 3: // BLOK
    {
        int rows = 5 + level / 2;
        int cols = 10 + level / 2;
        float tightSpacing = 42.0f;
        float startX = (1280 - (cols * tightSpacing)) / 2.0f + tightSpacing / 2.0f;
        for (int row = 0; row < rows; ++row) {
            for (int col = 0; col < cols; ++col) {
                enemies.push_back(CreateBasicEnemy(startX + col * tightSpacing, startY + row * tightSpacing, hpBonus, row * 80.0f));
            }
        }
    }
    break;
    }
}

// --- KOD OSOBY 3 (NIENARUSZONY) ---

// PROFIL
struct Profile {
    string nick;
};

// ZARZĄDZANIE PROFILAMI 
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
            out << p.nick << "\n";
    }

    void addProfile(const string& nick) {
        profiles.push_back({ nick });
        selected = (int)profiles.size() - 1;
        save();
    }

    Profile& current() {
        return profiles[selected];
    }
};

int main() {
    const int W = 1280, H = 720;
    InitWindow(W, H, "Flanki Shooter: Prepare to Battle");
    SetTargetFPS(60);

    GameState state = MENU;

    float playerX = W / 2.0f;
    float playerY = H - 60.0f;
    float speed = 400.0f;

    ProfileManager pm;
    pm.load();

    bool newProfileMode = false;
    string newNick = "";

    // ZMIENNE GAMEPLAY (OSOBA 1)
    int playerHP = 100; int score = 0; int level = 0; bool bossActive = false;
    int weaponLevel = 1; bool hasShield = false; bool fireAmmo = false; bool rapidFire = false; float shootTimer = 0.0f;
    float formationOffset = 0.0f; float formationDir = 1.0f;
    float transitionTimer = 0.0f;
    vector<Bullet> bullets; vector<Enemy> enemies; vector<PowerUp> powerups;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        // MENU (KOD OSOBY 3)
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
                    // RESET STANU GRY (OSOBA 1)
                    playerX = W / 2.0f; playerY = H - 60.0f; playerHP = 100; score = 0; level = 0; bossActive = false;
                    weaponLevel = 1; hasShield = false; fireAmmo = false; rapidFire = false;
                    bullets.clear(); enemies.clear(); powerups.clear();

                    // ZMIANA: Zamiast do GAME, idziemy do ekranu przygotowania
                    state = LEVEL_TRANSITION;
                    transitionTimer = 3.0f;
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
            DrawText("N - nowy profil", W / 2 - 200, H - 150, 20, DARKGRAY);
            DrawText("ESC - wyjscie", W / 2 - 200, H - 20, 20, DARKGRAY);
            if (newProfileMode) {
                DrawText("Nowy nick:", W / 2 - 150, 470, 25, GRAY);
                DrawRectangle(W / 2 - 150, 500, 300, 40, DARKGRAY);
                DrawText(newNick.c_str(), W / 2 - 140, 510, 25, RAYWHITE);
            }

            EndDrawing();
            continue;
        }

        // --- WIN SCREEN ---
        if (state == WIN_SCREEN) {
            if (IsKeyPressed(KEY_ENTER)) state = MENU;
            BeginDrawing(); ClearBackground(RAYWHITE); DrawText("GRATULACJE!", W / 2 - 200, H / 2 - 50, 60, GOLD); DrawText("Pokonales Bossa!", W / 2 - 250, H / 2 + 20, 30, DARKGRAY); DrawText(TextFormat("Wynik: %d", score), W / 2 - 150, H / 2 + 60, 30, BLACK); EndDrawing(); continue;
        }

        // --- LEVEL TRANSITION (Ekrany przejścia / Startu) ---
        if (state == LEVEL_TRANSITION) {
            transitionTimer -= dt;
            if (transitionTimer <= 0) {
                level++; // Zwiekszamy level (z 0 na 1 przy starcie)
                int bossLevel = 6;
                if (level >= bossLevel) {
                    bossActive = true;
                    enemies.push_back({ (float)W / 2 - 100, -200, 200, 150, 5000, 5000, true, 0, 0, true, MAROON });
                }
                else {
                    int formationType = (level - 1) % 4;
                    SpawnLevel(enemies, level, formationType);
                    formationOffset = 0.0f;
                }
                state = GAME;
            }

            BeginDrawing();
            ClearBackground(BLACK);

            // Logika wyswietlania tekstu zalezna od levelu
            if (level == 0) {
                DrawText("PRZYGOTUJ SIE!", W / 2 - 150, H / 2 - 50, 40, ORANGE);
                DrawText("NADCHODZI FALA 1...", W / 2 - 200, H / 2 + 20, 30, WHITE);
            }
            else {
                DrawText(TextFormat("LEVEL %d UKONCZONY!", level), W / 2 - 200, H / 2 - 50, 40, GREEN);
                DrawText(TextFormat("Przygotuj sie na Level %d...", level + 1), W / 2 - 220, H / 2 + 20, 30, WHITE);
            }
            EndDrawing();
            continue;
        }

        // --- GAMEPLAY (OSOBA 1) ---
        if (!pm.profiles.empty() && state == GAME) {
            // STEROWANIE
            if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) playerX -= speed * dt; if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) playerX += speed * dt;
            if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) playerY -= speed * dt; if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) playerY += speed * dt;
            if (playerX < 20) playerX = 20; if (playerX > W - 20) playerX = W - 20;
            if (playerY > H - 20) playerY = H - 20; if (playerY < 400) playerY = 400; // Limit gorny
            if (IsKeyPressed(KEY_ESCAPE)) state = MENU;

            // STRZELANIE
            if (shootTimer > 0) shootTimer -= dt;
            bool shootInput = rapidFire ? IsKeyDown(KEY_SPACE) : IsKeyPressed(KEY_SPACE);
            float fireRate = rapidFire ? 0.15f : 0.0f;
            if (shootInput && shootTimer <= 0) {
                shootTimer = fireRate; int dmg = fireAmmo ? 20 : 10; Color bulletColor = fireAmmo ? ORANGE : GREEN; float bSpeed = -600.0f;
                if (weaponLevel == 1) bullets.push_back({ playerX, playerY - 20, bSpeed, 0, false, bulletColor, dmg });
                else if (weaponLevel == 2) { bullets.push_back({ playerX - 10, playerY - 20, bSpeed, 0, false, bulletColor, dmg }); bullets.push_back({ playerX + 10, playerY - 20, bSpeed, 0, false, bulletColor, dmg }); }
                else if (weaponLevel == 3) { bullets.push_back({ playerX, playerY - 25, bSpeed, 0, false, bulletColor, dmg }); bullets.push_back({ playerX - 15, playerY - 15, bSpeed, -100, false, bulletColor, dmg }); bullets.push_back({ playerX + 15, playerY - 15, bSpeed, 100, false, bulletColor, dmg }); }
                else if (weaponLevel == 4) { bullets.push_back({ playerX - 10, playerY - 20, bSpeed, 0, false, bulletColor, dmg }); bullets.push_back({ playerX + 10, playerY - 20, bSpeed, 0, false, bulletColor, dmg }); bullets.push_back({ playerX - 25, playerY - 10, bSpeed, -150, false, bulletColor, dmg }); bullets.push_back({ playerX + 25, playerY - 10, bSpeed, 150, false, bulletColor, dmg }); }
                else if (weaponLevel >= 5) { bullets.push_back({ playerX, playerY - 25, bSpeed, 0, false, bulletColor, dmg }); bullets.push_back({ playerX - 15, playerY - 20, bSpeed, -50, false, bulletColor, dmg }); bullets.push_back({ playerX + 15, playerY - 20, bSpeed, 50, false, bulletColor, dmg }); bullets.push_back({ playerX - 30, playerY - 10, bSpeed, -200, false, bulletColor, dmg }); bullets.push_back({ playerX + 30, playerY - 10, bSpeed, 200, false, bulletColor, dmg }); }
            }

            // LOGIKA PRZEJŚCIA PO UKOŃCZENIU FALI
            if (!bossActive && enemies.empty()) {
                state = LEVEL_TRANSITION;
                transitionTimer = 3.0f;
            }

            // RUCH I LOGIKA WROGÓW
            if (!bossActive && !enemies.empty()) { formationOffset += formationDir * 150.0f * dt; if (formationOffset > 200) formationDir = -1.0f; if (formationOffset < -200) formationDir = 1.0f; }
            for (auto& e : enemies) {
                if (e.isBoss) {
                    if (e.y < 50) e.y += 100 * dt; else e.x += sin(GetTime()) * 300 * dt;
                    if (GetRandomValue(0, 100) < 8) { bullets.push_back({ e.x + e.width / 2, e.y + e.height, 400, 0, true, RED, 10 }); bullets.push_back({ e.x, e.y + e.height, 350, -200, true, RED, 10 }); bullets.push_back({ e.x + e.width, e.y + e.height, 350, 200, true, RED, 10 }); }
                }
                else {
                    if (!e.inPosition) { e.y += 500 * dt; e.x = e.formationX + formationOffset; if (e.y >= e.formationY) { e.y = e.formationY; e.inPosition = true; } }
                    else { e.x = e.formationX + formationOffset; }
                    if (GetRandomValue(0, 3000) < 5) bullets.push_back({ e.x + e.width / 2, e.y + e.height, 300, 0, true, e.baseColor, 10 });
                }
            }

            // POCISKI, KOLIZJE
            for (int i = 0; i < bullets.size(); i++) { bullets[i].y += bullets[i].speedY * dt; bullets[i].x += bullets[i].speedX * dt; if (bullets[i].y < -50 || bullets[i].y > H + 50 || bullets[i].x < -50 || bullets[i].x > W + 50) { bullets.erase(bullets.begin() + i); i--; } }
            Rectangle playerRect = { playerX - 18, playerY - 18, 36, 36 };
            for (int i = 0; i < bullets.size(); i++) { if (bullets[i].isEnemy) { if (CheckCollisionCircleRec({ bullets[i].x, bullets[i].y }, 5, playerRect)) { if (hasShield) { hasShield = false; bullets.erase(bullets.begin() + i); i--; } else { playerHP -= 10; bullets.erase(bullets.begin() + i); i--; if (playerHP <= 0) state = MENU; } } } }
            for (int i = 0; i < bullets.size(); i++) { if (!bullets[i].isEnemy) { bool hit = false; for (int j = 0; j < enemies.size(); j++) { Rectangle enemyRect = { enemies[j].x, enemies[j].y, enemies[j].width, enemies[j].height }; if (CheckCollisionCircleRec({ bullets[i].x, bullets[i].y }, 3, enemyRect)) { enemies[j].hp -= bullets[i].damage; hit = true; if (enemies[j].hp <= 0) { score += (enemies[j].isBoss ? 5000 : 50); if (!enemies[j].isBoss && GetRandomValue(0, 100) < 20) { int pType = GetRandomValue(0, 4); powerups.push_back({ enemies[j].x, enemies[j].y, pType, true }); } if (enemies[j].isBoss) { enemies.erase(enemies.begin() + j); state = WIN_SCREEN; break; } enemies.erase(enemies.begin() + j); } break; } } if (hit) { bullets.erase(bullets.begin() + i); i--; } } }
            for (int i = 0; i < powerups.size(); i++) { powerups[i].y += 150 * dt; if (CheckCollisionRecs(playerRect, { powerups[i].x, powerups[i].y, 20, 20 })) { int t = powerups[i].type; if (t == 0) playerHP = (playerHP + 30 > 100) ? 100 : playerHP + 30; if (t == 1) { if (weaponLevel < 5) weaponLevel++; } if (t == 2) hasShield = true; if (t == 3) fireAmmo = true; if (t == 4) rapidFire = true; powerups.erase(powerups.begin() + i); i--; } else if (powerups[i].y > H) { powerups.erase(powerups.begin() + i); i--; } }
        }

        // RYSOWANIE GRY (OSOBA 1)
        BeginDrawing();
        ClearBackground(BLACK);
        if (!pm.profiles.empty() && state == GAME) {
            DrawCircle((int)playerX, (int)playerY, 18, RAYWHITE); if (hasShield) DrawRing({ playerX, playerY }, 22, 25, 0, 360, 0, SKYBLUE);
            for (auto& e : enemies) { Color c = e.isBoss ? e.baseColor : (e.inPosition ? e.baseColor : RED); DrawRectangle((int)e.x, (int)e.y, (int)e.width, (int)e.height, c); if (e.isBoss) { DrawRectangle(e.x, e.y - 15, e.width, 10, DARKGRAY); DrawRectangle(e.x, e.y - 15, e.width * ((float)e.hp / e.maxHp), 10, GREEN); } }
            for (auto& b : bullets) DrawCircle((int)b.x, (int)b.y, b.isEnemy ? 5 : (fireAmmo ? 5 : 3), b.color);
            for (auto& p : powerups) { Color pc = WHITE; const char* txt = "?"; if (p.type == 0) { pc = GREEN; txt = "+"; } if (p.type == 1) { pc = YELLOW; txt = "W"; } if (p.type == 2) { pc = BLUE; txt = "S"; } if (p.type == 3) { pc = ORANGE; txt = "F"; } if (p.type == 4) { pc = PURPLE; txt = "R"; } DrawRectangle((int)p.x, (int)p.y, 20, 20, pc); DrawText(txt, (int)p.x + 5, (int)p.y + 2, 10, BLACK); }
            DrawText(TextFormat("Gracz: %s", pm.current().nick.c_str()), 10, 10, 20, WHITE); DrawText(TextFormat("HP: %d", playerHP), 10, 40, 20, GREEN); DrawText(TextFormat("Score: %d", score), 10, 70, 20, YELLOW);
            DrawText("Active bonuses:", 10, 110, 15, GRAY); if (fireAmmo) DrawText("FIRE AMMO", 10, 130, 15, ORANGE); if (rapidFire) DrawText("RAPID FIRE", 10, 150, 15, PURPLE); DrawText(TextFormat("Weapon Lvl: %d/5", weaponLevel), 10, 170, 15, YELLOW);
            if (bossActive) DrawText("FINAL BOSS!", W / 2 - 100, 50, 30, RED); else DrawText(TextFormat("LEVEL: %d", level), W - 150, 10, 20, BLUE);
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}

#include "raylib.h"
#include <string>
#include <vector>
#include <fstream>
#include <cmath>
#include <cstdlib>
#include <cstdint>
#include <algorithm>
#include <filesystem>

using namespace std;

// STANY APLIKACJI
enum GameState {
    MENU,              // menu główne + profile
    GAME,              // aktywna rozgrywka
    LEVEL_TRANSITION,  // ekran przejścia między poziomami
    WIN_SCREEN         // ekran zwycięstwa (boss)
};
float Clamp(float value, float min, float max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}


// STRUKTURY ROZGRYWKI

// Pojedynczy pocisk (gracza lub wroga)
struct Bullet {
    float x, y;
    float speedY;
    float speedX;
    bool isEnemy;
    Color color;
    int damage;
};

// Przeciwnik (zwykły lub boss)
struct Enemy {
    float x, y;
    float width, height;
    int hp, maxHp;
    bool isBoss;

    // Pozycja w formacji
    float formationX, formationY;
    bool inPosition;

    Color baseColor;
    int textureIndex;
};

// Bonus wypadający z przeciwnika
struct PowerUp {
    float x, y;
    int type;
    bool active;
};

// FUNKCJE POMOCNICZE
// Losowy kolor przeciwnika
Color GetRandomBottleColor() {
    int r = GetRandomValue(0, 3);
    switch (r) {
    case 0: return GREEN;
    case 1: return SKYBLUE;
    case 2: return BEIGE;
    case 3: return YELLOW;
    default: return ORANGE;
    }
}

// Tworzy standardowego przeciwnika
Enemy CreateBasicEnemy(
    float formX,
    float formY,
    int hpBonus,
    float startOffsetY,
    int textureIndex
) {
    Enemy e;
    e.width = 56;
    e.height = 56;

    e.hp = 20 + hpBonus;
    e.maxHp = e.hp;
    e.isBoss = false;

    e.formationX = formX;
    e.formationY = formY;

    // Startuje nad ekranem
    e.x = e.formationX;
    e.y = -100.0f - startOffsetY;

    e.inPosition = false;
    e.baseColor = GetRandomBottleColor();
    e.textureIndex = textureIndex;
    return e;
}

int gEnemyTextureCount = 0;

// Losowy indeks tekstury przeciwnika
int GetRandomEnemyTextureIndex() {
    if (gEnemyTextureCount <= 0) return -1;
    return GetRandomValue(0, gEnemyTextureCount - 1);
}

// GENEROWANIE FALI PRZECIWNIKÓW
void SpawnLevel(vector<Enemy>& enemies, int level, int formationType) {
    int hpBonus = level * 15;

    float spacingX = 60.0f;
    float spacingY = 60.0f;
    float centerX = 1280 / 2.0f;
    float startY = 100.0f;

    switch (formationType) {

        // --- SIATKA ---
    case 0: {
        int rows = 3 + level / 2;
        int cols = 6 + (level % 3);

        float startX =
            (1280 - (cols * spacingX)) / 2.0f + spacingX / 2.0f;

        for (int r = 0; r < rows; r++) {
            for (int c = 0; c < cols; c++) {
                enemies.push_back(
                    CreateBasicEnemy(
                        startX + c * spacingX,
                        startY + r * spacingY,
                        hpBonus,
                        r * 100.0f,
                        GetRandomEnemyTextureIndex()
                    )
                );
            }
        }
    } break;

          // --- FORMACJA V ---
    case 1: {
        int count = 7 + level;
        for (int i = 0; i < count; i++) {
            enemies.push_back(CreateBasicEnemy(
                centerX - i * spacingX,
                startY + i * spacingY,
                hpBonus,
                i * 80.0f,
                GetRandomEnemyTextureIndex()
            ));
            if (i > 0) {
                enemies.push_back(CreateBasicEnemy(
                    centerX + i * spacingX,
                    startY + i * spacingY,
                    hpBonus,
                    i * 80.0f,
                    GetRandomEnemyTextureIndex()
                ));
            }
        }
    } break;

          // --- OKRĄG ---
    case 2: {
        int count = 12 + level * 2;
        float radius = 180.0f;
        float centerY = 250.0f;

        for (int i = 0; i < count; i++) {
            float angle = (float)i / count * 2.0f * PI;
            enemies.push_back(CreateBasicEnemy(
                centerX + cos(angle) * radius,
                centerY + sin(angle) * radius,
                hpBonus,
                i * 50.0f,
                GetRandomEnemyTextureIndex()
            ));
        }
    } break;

          // --- BLOK ---
    case 3: {
        int rows = 5 + level / 2;
        int cols = 10 + level / 2;
        float spacing = 56.0f;

        float startX =
            (1280 - (cols * spacing)) / 2.0f + spacing / 2.0f;

        for (int r = 0; r < rows; r++) {
            for (int c = 0; c < cols; c++) {
                enemies.push_back(CreateBasicEnemy(
                    startX + c * spacing,
                    startY + r * spacing,
                    hpBonus,
                    r * 80.0f,
                    GetRandomEnemyTextureIndex()
                ));
            }
        }
    } break;
    }
}

// SZYFR 
// Proste szyfrowanie tekstu (ASCII 32–126)
string caesarEncrypt(const string& text, int shift) {
    string result = text;
    for (char& c : result) {
        if (c >= 32 && c <= 126) {
            c = (char)(32 + (c - 32 + shift) % 95);
        }
    }
    return result;
}

// Odszyfrowywanie tekstu
string caesarDecrypt(const string& text, int shift) {
    string result = text;
    for (char& c : result) {
        if (c >= 32 && c <= 126) {
            c = (char)(32 + (c - 32 - shift + 95) % 95);
        }
    }
    return result;
}
string ResolveAssetPath(const char* filename) {
    namespace fs = std::filesystem;
    const vector<fs::path> prefixes = {
        "assets",
        "..\\assets",
        "..\\..\\assets",
        "..\\..\\..\\assets",
        "..\\..\\..\\..\\assets",
        "..\\..\\..\\..\\..\\assets"
    };
    vector<fs::path> bases;
    bases.push_back(fs::current_path());
    bases.push_back(fs::path(GetWorkingDirectory()));
    bases.push_back(fs::path(GetApplicationDirectory()));

    for (const auto& base : bases) {
        for (const auto& prefix : prefixes) {
            fs::path candidate = base / prefix / filename;
            if (fs::exists(candidate)) {
                return candidate.string();
            }
        }
    }
    return (fs::path("assets") / filename).string();
}

void MakeNearWhiteTransparent(Image* img, unsigned char threshold) {
    if (img == nullptr || img->data == nullptr) {
        return;
    }
    ImageFormat(img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    unsigned char* data = (unsigned char*)img->data;
    const int pixelCount = img->width * img->height;
    for (int i = 0; i < pixelCount; ++i) {
        unsigned char* px = data + (i * 4);
        if (px[0] >= threshold && px[1] >= threshold && px[2] >= threshold) {
            px[3] = 0;
        }
    }
}


// PROFIL GRACZA
struct Profile {
    string nick;      // nazwa gracza
    int bestScore = 0;
};


// MENEDŻER PROFILI

class ProfileManager {
public:
    vector<Profile> profiles; // lista profili
    int selected = 0;         // aktualnie wybrany profil

    // Wczytywanie profili z pliku
    void load() {
        profiles.clear();
        ifstream file("profiles.dat");
        const int SHIFT = 3;

        if (!file) return;

        string encNick;
        int score;

        while (file >> encNick >> score) {
            profiles.push_back({
                caesarDecrypt(encNick, SHIFT),
                score
                });
        }
    }

    // Zapis profili do pliku
    void save() {
        ofstream out("profiles.dat", ios::trunc);
        const int SHIFT = 3;

        for (auto& p : profiles) {
            out << caesarEncrypt(p.nick, SHIFT)
                << " "
                << p.bestScore
                << "\n";
        }
    }

    // Dodanie nowego profilu
    void addProfile(const string& nick) {
        profiles.push_back({ nick, 0 });
        selected = (int)profiles.size() - 1;
        save();
    }

    // Zwraca aktualnie wybrany profil
    Profile& current() {
        return profiles[selected];
    }
};
int main() {
    const int W = 1280;
    const int H = 720;

    // OKNO + AUDIO
    InitWindow(W, H, "Flanki Shooter: Prepare to Battle");
    ToggleFullscreen();
    SetTargetFPS(60);
    InitAudioDevice();

    GameState state = MENU;

    // ZMIENNE GRACZA
    float playerX = W / 2.0f;
    float playerY = H - 60.0f;
    float speed = 400.0f;

    // PROFILE
    ProfileManager pm;
    pm.load();

    bool newProfileMode = false;
    string newNick;

    // ZMIENNE ROZGRYWKI
    int playerHP = 100;
    int score = 0;
    int level = 0;
    bool bossActive = false;

    int weaponLevel = 1;
    bool hasShield = false;
    bool fireAmmo = false;
    bool rapidFire = false;

    float shootTimer = 0.0f;
    float rapidFireTimer = 0.0f;

    float drunkLevel = 0.0f;
    const float maxDrunk = 100.0f;

    float formationOffset = 0.0f;
    float formationDir = 1.0f;
    float transitionTimer = 0.0f;

    vector<Bullet> bullets;
    vector<Enemy> enemies;
    vector<PowerUp> powerups;

    // ===============================
    // ŁADOWANIE ASSETÓW – GRAFIKA
    // ============================

    // Gracz
    Image playerImg = LoadImage(ResolveAssetPath("student3.png").c_str());
    Texture2D texPlayer = {};
    if (playerImg.data) {
        MakeNearWhiteTransparent(&playerImg, 230);
        texPlayer = LoadTextureFromImage(playerImg);
        UnloadImage(playerImg);
    }

    // Boss i inne obiekty
    Texture2D texBoss = LoadTexture(ResolveAssetPath("boss.png").c_str());
    Texture2D texCoffee = LoadTexture(ResolveAssetPath("kawa.png").c_str());

    // Pocisk wroga
    Image enemyBulletImg = LoadImage(ResolveAssetPath("enemy.png").c_str());
    Texture2D texEnemyBullet = {};
    if (enemyBulletImg.data) {
        MakeNearWhiteTransparent(&enemyBulletImg, 230);
        texEnemyBullet = LoadTextureFromImage(enemyBulletImg);
        UnloadImage(enemyBulletImg);
    }

    // ===============================
    // DŹWIĘKI
    // ===============================
    Sound sfxLevelUp = LoadSound(ResolveAssetPath("levelup.mp3").c_str());
    Sound sfxShoot = LoadSound(ResolveAssetPath("strzal.mp3").c_str());
    Sound sfxPower = LoadSound(ResolveAssetPath("power.mp3").c_str());

    // ===============================
    // POWER-UPY – TEKSTURY
    // ===============================
    Image powerHpImg = LoadImage(ResolveAssetPath("hp.png").c_str());
    Texture2D texPowerHp = {};
    if (powerHpImg.data) {
        MakeNearWhiteTransparent(&powerHpImg, 230);
        texPowerHp = LoadTextureFromImage(powerHpImg);
        UnloadImage(powerHpImg);
    }

    Texture2D texPowerWeapon = LoadTexture(ResolveAssetPath("weapon.png").c_str());
    Texture2D texPowerShield = LoadTexture(ResolveAssetPath("shield.png").c_str());
    Texture2D texPowerFire = LoadTexture(ResolveAssetPath("fire.png").c_str());

    Image powerRapidImg = LoadImage(ResolveAssetPath("rapid.png").c_str());
    Texture2D texPowerRapid = {};
    if (powerRapidImg.data) {
        MakeNearWhiteTransparent(&powerRapidImg, 230);
        texPowerRapid = LoadTextureFromImage(powerRapidImg);
        UnloadImage(powerRapidImg);
    }

    // ===============================
    // TEKSTURY PRZECIWNIKÓW
    // ===============================
    vector<Texture2D> enemyTextures;
    const vector<string> enemyFiles = {
        "tyskie2.png",
        "warka.png",
        "tyskie2.png",
        "zubr.png"
    };

    for (const auto& file : enemyFiles) {
        Image img = LoadImage(ResolveAssetPath(file.c_str()).c_str());
        if (img.data) {
            MakeNearWhiteTransparent(&img, 230);
            Texture2D tex = LoadTextureFromImage(img);
            UnloadImage(img);
            if (tex.id != 0)
                enemyTextures.push_back(tex);
        }
    }

    // Fallback – jedna tekstura, jeśli inne się nie załadują
    if (enemyTextures.empty()) {
        Image img = LoadImage(ResolveAssetPath("enemy.png").c_str());
        if (img.data) {
            MakeNearWhiteTransparent(&img, 230);
            Texture2D tex = LoadTextureFromImage(img);
            UnloadImage(img);
            if (tex.id != 0)
                enemyTextures.push_back(tex);
        }
    }

    gEnemyTextureCount = (int)enemyTextures.size();
    // ===============================
    // GŁÓWNA PĘTLA GRY
    // ===============================
    while (!WindowShouldClose()) {
        float dt = GetFrameTime(); // delta time

        // ===============================
        // MENU
        // ===============================
        if (state == MENU) {

            // --- tryb tworzenia nowego profilu ---
            if (newProfileMode) {

                // wpisywanie znaków
                for (int key = GetCharPressed(); key > 0; key = GetCharPressed()) {
                    if (key >= 32 && key <= 125 && newNick.length() < 12)
                        newNick += (char)key;
                }

                if (IsKeyPressed(KEY_BACKSPACE) && !newNick.empty())
                    newNick.pop_back();

                if (IsKeyPressed(KEY_ENTER) && !newNick.empty()) {
                    pm.addProfile(newNick);
                    newNick.clear();
                    newProfileMode = false;
                }
            }
            // --- wybór istniejącego profilu ---
            else if (!pm.profiles.empty()) {

                int count = (int)pm.profiles.size();

                if (IsKeyPressed(KEY_UP))
                    pm.selected = (pm.selected - 1 + count) % count;

                if (IsKeyPressed(KEY_DOWN))
                    pm.selected = (pm.selected + 1) % count;

                if (IsKeyPressed(KEY_ENTER)) {
                    // reset stanu gry
                    playerX = W / 2.0f;
                    playerY = H - 60.0f;
                    playerHP = 100;
                    score = 0;
                    level = 0;
                    bossActive = false;

                    weaponLevel = 1;
                    hasShield = false;
                    fireAmmo = false;
                    rapidFire = false;
                    rapidFireTimer = 0.0f;
                    drunkLevel = 0.0f;

                    bullets.clear();
                    enemies.clear();
                    powerups.clear();

                    // przejście do ekranu startowego poziomu
                    state = LEVEL_TRANSITION;
                    transitionTimer = 3.0f;
                }
            }

            // aktywacja trybu nowego profilu
            if (IsKeyPressed(KEY_N)) {
                newProfileMode = true;
                newNick.clear();
            }

            // --- rysowanie menu ---
            BeginDrawing();
            ClearBackground(BLACK);

            DrawText("FLANKI SHOOTER", W / 2 - 220, 120, 50, RAYWHITE);
            DrawText("Profile:", W / 2 - 200, 200, 20, GRAY);

            // lista profili
            for (int i = 0; i < pm.profiles.size(); i++) {
                Color c = (i == pm.selected) ? YELLOW : GRAY;
                DrawText(
                    TextFormat("%s  |  Best: %d",
                        pm.profiles[i].nick.c_str(),
                        pm.profiles[i].bestScore),
                    W / 2 - 200,
                    240 + i * 30,
                    22,
                    c
                );
            }

            int menuY = 240 + pm.profiles.size() * 30 + 30;

            DrawText("ENTER - wybierz", W / 2 - 200, menuY, 20, DARKGRAY);
            DrawText("N - nowy profil", W / 2 - 200, menuY + 30, 20, DARKGRAY);

            // pole edycji nicku
            if (newProfileMode) {
                DrawText("Nowy nick:", W / 2 - 200, menuY + 70, 22, GRAY);
                DrawRectangle(W / 2 - 200, menuY + 100, 300, 40, DARKGRAY);
                DrawText(newNick.c_str(), W / 2 - 190, menuY + 110, 22, RAYWHITE);
            }

            DrawText("ESC - wyjscie", W / 2 - 200, menuY + 160, 20, DARKGRAY);

            EndDrawing();
            continue;
        }

        // ===============================
        // EKRAN PRZEJŚCIA POZIOMU
        // ===============================
        if (state == LEVEL_TRANSITION) {

            transitionTimer -= dt;

            if (transitionTimer <= 0.0f) {

                level++; // nowy poziom

                int bossLevel = 6;
                if (level >= bossLevel) {
                    // spawn bossa
                    bossActive = true;
                    enemies.push_back({
                        (float)W / 2 - 160,
                        -240,
                        320,
                        240,
                        5000,
                        5000,
                        true,
                        0, 0,
                        true,
                        MAROON,
                        -1
                        });
                }
                else {
                    // spawn fali przeciwników
                    SpawnLevel(enemies, level, (level - 1) % 4);
                    formationOffset = 0.0f;
                }

                if (IsSoundReady(sfxLevelUp))
                    PlaySound(sfxLevelUp);

                state = GAME;
            }

            // --- rysowanie ekranu przejścia ---
            BeginDrawing();
            ClearBackground(BLACK);

            if (level == 0) {
                DrawText("PRZYGOTUJ SIE!", W / 2 - 150, H / 2 - 50, 40, ORANGE);
                DrawText("NADCHODZI FALA 1...", W / 2 - 200, H / 2 + 20, 30, WHITE);
            }
            else {
                DrawText(
                    TextFormat("LEVEL %d UKONCZONY!", level),
                    W / 2 - 200,
                    H / 2 - 50,
                    40,
                    GREEN
                );
                DrawText(
                    TextFormat("Przygotuj sie na Level %d...", level + 1),
                    W / 2 - 220,
                    H / 2 + 20,
                    30,
                    WHITE
                );
            }

            EndDrawing();
            continue;
        }
        // ===============================
        // GAMEPLAY – LOGIKA
        // ===============================
        if (!pm.profiles.empty() && state == GAME) {

            // --- timery bonusów ---
            if (rapidFireTimer > 0.0f) {
                rapidFireTimer -= dt;
                if (rapidFireTimer <= 0.0f) {
                    rapidFireTimer = 0.0f;
                    rapidFire = false;
                }
            }

            // --- efekt "upicia" ---
            if (drunkLevel > 0.0f)
                drunkLevel = max(0.0f, drunkLevel - 6.0f * dt);

            float drunkFactor = drunkLevel / maxDrunk;
            float moveSpeed = speed * (1.0f - 0.4f * drunkFactor);
            float wobble = sin(GetTime() * 2.5f) * 60.0f * drunkFactor;

            // --- sterowanie graczem ---
            if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))
                playerX -= moveSpeed * dt;
            if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT))
                playerX += moveSpeed * dt;
            if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))
                playerY -= moveSpeed * dt;
            if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))
                playerY += moveSpeed * dt;

            playerX += wobble * dt;

            // ograniczenia ekranu
            playerX = Clamp(playerX, 32, W - 32);
            playerY = Clamp(playerY, 400, H - 32);

            if (IsKeyPressed(KEY_ESCAPE))
                state = MENU;

            // ===============================
            // STRZELANIE
            // ===============================
            if (shootTimer > 0)
                shootTimer -= dt;

            bool shootDown =
                IsKeyDown(KEY_SPACE) || IsMouseButtonDown(MOUSE_LEFT_BUTTON);
            bool shootPressed =
                IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON);

            bool shootInput = rapidFire ? shootDown : shootPressed;
            float fireRate = rapidFire ? 0.15f : 0.0f;

            if (shootInput && shootTimer <= 0.0f) {
                shootTimer = fireRate;

                int dmg = fireAmmo ? 20 : 10;
                Color bulletColor = fireAmmo ? ORANGE : GREEN;
                float bulletSpeed = -600.0f;

                if (IsSoundReady(sfxShoot))
                    PlaySound(sfxShoot);

                // poziomy broni
                if (weaponLevel == 1) {
                    bullets.push_back({ playerX, playerY - 32, bulletSpeed, 0, false, bulletColor, dmg });
                }
                else if (weaponLevel == 2) {
                    bullets.push_back({ playerX - 10, playerY - 32, bulletSpeed, 0, false, bulletColor, dmg });
                    bullets.push_back({ playerX + 10, playerY - 32, bulletSpeed, 0, false, bulletColor, dmg });
                }
                else if (weaponLevel == 3) {
                    bullets.push_back({ playerX, playerY - 36, bulletSpeed, 0, false, bulletColor, dmg });
                    bullets.push_back({ playerX - 15, playerY - 26, bulletSpeed, -100, false, bulletColor, dmg });
                    bullets.push_back({ playerX + 15, playerY - 26, bulletSpeed, 100, false, bulletColor, dmg });
                }
                else if (weaponLevel == 4) {
                    bullets.push_back({ playerX - 10, playerY - 32, bulletSpeed, 0, false, bulletColor, dmg });
                    bullets.push_back({ playerX + 10, playerY - 32, bulletSpeed, 0, false, bulletColor, dmg });
                    bullets.push_back({ playerX - 25, playerY - 22, bulletSpeed, -150, false, bulletColor, dmg });
                    bullets.push_back({ playerX + 25, playerY - 22, bulletSpeed, 150, false, bulletColor, dmg });
                }
                else {
                    bullets.push_back({ playerX, playerY - 36, bulletSpeed, 0, false, bulletColor, dmg });
                    bullets.push_back({ playerX - 15, playerY - 30, bulletSpeed, -50, false, bulletColor, dmg });
                    bullets.push_back({ playerX + 15, playerY - 30, bulletSpeed, 50, false, bulletColor, dmg });
                    bullets.push_back({ playerX - 30, playerY - 20, bulletSpeed, -200, false, bulletColor, dmg });
                    bullets.push_back({ playerX + 30, playerY - 20, bulletSpeed, 200, false, bulletColor, dmg });
                }
            }

            // ===============================
            // PRZEJŚCIE PO FALI
            // ===============================
            if (!bossActive && enemies.empty()) {
                state = LEVEL_TRANSITION;
                transitionTimer = 3.0f;
            }

            // ===============================
            // RUCH WROGÓW
            // ===============================
            if (!bossActive && !enemies.empty()) {
                formationOffset += formationDir * 150.0f * dt;
                if (formationOffset > 200) formationDir = -1.0f;
                if (formationOffset < -200) formationDir = 1.0f;
            }

            for (auto& e : enemies) {

                // --- boss ---
                if (e.isBoss) {
                    if (e.y < 50)
                        e.y += 100 * dt;
                    else
                        e.x += sin(GetTime()) * 300 * dt;

                    if (GetRandomValue(0, 100) < 8) {
                        bullets.push_back({ e.x + e.width / 2, e.y + e.height, 400, 0, true, RED, 10 });
                        bullets.push_back({ e.x, e.y + e.height, 350, -200, true, RED, 10 });
                        bullets.push_back({ e.x + e.width, e.y + e.height, 350, 200, true, RED, 10 });
                    }
                }
                // --- zwykły wróg ---
                else {
                    if (!e.inPosition) {
                        e.y += 500 * dt;
                        e.x = e.formationX + formationOffset;
                        if (e.y >= e.formationY) {
                            e.y = e.formationY;
                            e.inPosition = true;
                        }
                    }
                    else {
                        e.x = e.formationX + formationOffset;
                    }

                    if (GetRandomValue(0, 3000) < 5) {
                        bullets.push_back({ e.x + e.width / 2, e.y + e.height, 300, 0, true, e.baseColor, 10 });
                    }
                }
            }

            // ===============================
            // POCISKI
            // ===============================
            for (int i = 0; i < bullets.size(); i++) {
                bullets[i].x += bullets[i].speedX * dt;
                bullets[i].y += bullets[i].speedY * dt;

                if (bullets[i].y < -50 || bullets[i].y > H + 50 ||
                    bullets[i].x < -50 || bullets[i].x > W + 50) {
                    bullets.erase(bullets.begin() + i);
                    i--;
                }
            }

            Rectangle playerRect = { playerX - 32, playerY - 32, 64, 64 };

            // --- trafienia gracza ---
            for (int i = 0; i < bullets.size(); i++) {
                if (bullets[i].isEnemy &&
                    CheckCollisionCircleRec({ bullets[i].x, bullets[i].y }, 5, playerRect)) {

                    if (hasShield) {
                        hasShield = false;
                    }
                    else {
                        playerHP -= 10;
                        drunkLevel = min(maxDrunk, drunkLevel + 10.0f);

                        if (playerHP <= 0) {
                            if (score > pm.current().bestScore) {
                                pm.current().bestScore = score;
                                pm.save();
                            }
                            state = MENU;
                        }
                    }
                    bullets.erase(bullets.begin() + i);
                    i--;
                }
            }

            // --- trafienia przeciwników ---
            for (int i = 0; i < bullets.size(); i++) {
                if (!bullets[i].isEnemy) {
                    for (int j = 0; j < enemies.size(); j++) {
                        Rectangle enemyRect = {
                            enemies[j].x, enemies[j].y,
                            enemies[j].width, enemies[j].height
                        };

                        if (CheckCollisionCircleRec({ bullets[i].x, bullets[i].y }, 3, enemyRect)) {

                            enemies[j].hp -= bullets[i].damage;

                            if (enemies[j].hp <= 0) {
                                score += enemies[j].isBoss ? 5000 : 50;

                                if (!enemies[j].isBoss &&
                                    GetRandomValue(0, 100) < 20) {
                                    powerups.push_back({
                                        enemies[j].x,
                                        enemies[j].y,
                                        GetRandomValue(0, 5),
                                        true
                                        });
                                }

                                if (enemies[j].isBoss) {
                                    if (IsSoundReady(sfxPower))
                                        PlaySound(sfxPower);

                                    enemies.erase(enemies.begin() + j);

                                    if (score > pm.current().bestScore) {
                                        pm.current().bestScore = score;
                                        pm.save();
                                    }
                                    state = WIN_SCREEN;
                                    break;
                                }

                                enemies.erase(enemies.begin() + j);
                            }

                            bullets.erase(bullets.begin() + i);
                            i--;
                            break;
                        }
                    }
                }
            }

            // ===============================
            // POWER-UPY
            // ===============================
            for (int i = 0; i < powerups.size(); i++) {
                powerups[i].y += 150 * dt;

                float size =
                    (powerups[i].type == 0 ||
                        powerups[i].type == 2 ||
                        powerups[i].type == 5) ? 36.0f : 28.0f;

                if (CheckCollisionRecs(
                    playerRect,
                    { powerups[i].x, powerups[i].y, size, size })) {

                    if (IsSoundReady(sfxPower))
                        PlaySound(sfxPower);

                    int t = powerups[i].type;

                    if (t == 0) playerHP = min(100, playerHP + 30);
                    if (t == 1 && weaponLevel < 5) weaponLevel++;
                    if (t == 2) hasShield = true;
                    if (t == 3) fireAmmo = true;
                    if (t == 4) { rapidFire = true; rapidFireTimer = max(rapidFireTimer, 8.0f); }
                    if (t == 5) {
                        playerHP = min(100, playerHP + 20);
                        drunkLevel = max(0.0f, drunkLevel - 40.0f);
                        rapidFire = true;
                        rapidFireTimer = max(rapidFireTimer, 6.0f);
                    }

                    powerups.erase(powerups.begin() + i);
                    i--;
                }
                else if (powerups[i].y > H) {
                    powerups.erase(powerups.begin() + i);
                    i--;
                }
            }
        }
        // ===============================
        // RYSOWANIE
        // ===============================
        BeginDrawing();
        ClearBackground(BLACK);

        // --- RYSOWANIE ROZGRYWKI ---
        if (!pm.profiles.empty() && state == GAME) {

            // --- gracz ---
            if (texPlayer.id != 0) {
                Rectangle src = { 0, 0, (float)texPlayer.width, (float)texPlayer.height };
                Rectangle dst = { playerX - 32, playerY - 32, 64, 64 };
                DrawTexturePro(texPlayer, src, dst, { 0, 0 }, 0.0f, WHITE);
            }
            else {
                DrawCircle((int)playerX, (int)playerY, 18, RAYWHITE);
            }

            // tarcza
            if (hasShield)
                DrawRing({ playerX, playerY }, 32, 36, 0, 360, 0, SKYBLUE);

            // --- przeciwnicy ---
            for (auto& e : enemies) {

                if (e.isBoss && texBoss.id != 0) {
                    Rectangle src = { 0, 0, (float)texBoss.width, (float)texBoss.height };
                    Rectangle dst = { e.x, e.y, e.width, e.height };
                    DrawTexturePro(texBoss, src, dst, { 0, 0 }, 0.0f, WHITE);
                }
                else if (!e.isBoss &&
                    e.textureIndex >= 0 &&
                    e.textureIndex < enemyTextures.size() &&
                    enemyTextures[e.textureIndex].id != 0) {

                    Texture2D& tex = enemyTextures[e.textureIndex];
                    Rectangle src = { 0, 0, (float)tex.width, (float)tex.height };
                    Rectangle dst = { e.x, e.y, e.width, e.height };
                    DrawTexturePro(tex, src, dst, { 0, 0 }, 0.0f, WHITE);
                }
                else {
                    DrawRectangle(e.x, e.y, e.width, e.height, e.baseColor);
                }

                // pasek HP bossa
                if (e.isBoss) {
                    DrawRectangle(e.x, e.y - 15, e.width, 10, DARKGRAY);
                    DrawRectangle(
                        e.x,
                        e.y - 15,
                        e.width * ((float)e.hp / e.maxHp),
                        10,
                        GREEN
                    );
                }
            }

            // --- pociski ---
            for (auto& b : bullets) {
                if (b.isEnemy && texEnemyBullet.id != 0) {
                    Rectangle src = { 0, 0, (float)texEnemyBullet.width, (float)texEnemyBullet.height };
                    Rectangle dst = { b.x - 14, b.y - 14, 28, 28 };
                    DrawTexturePro(texEnemyBullet, src, dst, { 0, 0 }, 0.0f, WHITE);
                }
                else {
                    DrawCircle(
                        (int)b.x,
                        (int)b.y,
                        b.isEnemy ? 5 : (fireAmmo ? 5 : 3),
                        b.color
                    );
                }
            }

            // --- power-upy ---
            for (auto& p : powerups) {

                Texture2D* tex = nullptr;
                float size = 28.0f;

                if (p.type == 0 && texPowerHp.id != 0) {
                    tex = &texPowerHp; size = 36;
                }
                if (p.type == 1 && texPowerWeapon.id != 0) {
                    tex = &texPowerWeapon;
                }
                if (p.type == 2 && texPowerShield.id != 0) {
                    tex = &texPowerShield; size = 36;
                }
                if (p.type == 3 && texPowerFire.id != 0) {
                    tex = &texPowerFire;
                }
                if (p.type == 4 && texPowerRapid.id != 0) {
                    tex = &texPowerRapid; size = 40;
                }
                if (p.type == 5 && texCoffee.id != 0) {
                    tex = &texCoffee; size = 36;
                }

                if (tex && tex->id != 0) {
                    Rectangle src = { 0, 0, (float)tex->width, (float)tex->height };
                    Rectangle dst = { p.x, p.y, size, size };
                    DrawTexturePro(*tex, src, dst, { 0, 0 }, 0.0f, WHITE);
                }
                else {
                    DrawRectangle(p.x, p.y, size, size, WHITE);
                }
            }

            // --- HUD ---
            DrawText(TextFormat("Gracz: %s", pm.current().nick.c_str()), 10, 10, 20, WHITE);
            DrawText(TextFormat("HP: %d", playerHP), 10, 40, 20, GREEN);
            DrawText(TextFormat("Score: %d", score), 10, 70, 20, YELLOW);
            DrawText(TextFormat("Best: %d", pm.current().bestScore), 10, 100, 20, GOLD);

            DrawText("Active bonuses:", 10, 130, 15, GRAY);
            if (fireAmmo)  DrawText("FIRE AMMO", 10, 150, 15, ORANGE);
            if (rapidFire) DrawText("RAPID FIRE", 10, 170, 15, PURPLE);

            DrawText(
                TextFormat("Weapon Lvl: %d/5", weaponLevel),
                10, 190, 15, YELLOW
            );

            DrawText(
                TextFormat("Upicie: %d%%", (int)drunkLevel),
                10, 210, 15, LIGHTGRAY
            );

            if (bossActive)
                DrawText("FINAL BOSS!", W / 2 - 100, 50, 30, RED);
            else
                DrawText(TextFormat("LEVEL: %d", level), W - 150, 10, 20, BLUE);
        }

        EndDrawing();
    }

    // ===============================
    // SPRZĄTANIE ZASOBÓW
    // ===============================
    UnloadTexture(texPlayer);
    UnloadTexture(texBoss);
    UnloadTexture(texCoffee);
    UnloadTexture(texEnemyBullet);
    UnloadTexture(texPowerHp);
    UnloadTexture(texPowerWeapon);
    UnloadTexture(texPowerShield);
    UnloadTexture(texPowerFire);
    UnloadTexture(texPowerRapid);

    for (auto& tex : enemyTextures)
        UnloadTexture(tex);

    UnloadSound(sfxLevelUp);
    UnloadSound(sfxShoot);
    UnloadSound(sfxPower);

    CloseAudioDevice();
    CloseWindow();

    return 0;
}

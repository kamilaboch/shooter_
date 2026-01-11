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
    int textureIndex;
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

Enemy CreateBasicEnemy(float formX, float formY, int hpBonus, float startOffsetY, int textureIndex) {
    Enemy e;
    e.width = 56; e.height = 56;
    e.hp = 20 + hpBonus; e.maxHp = e.hp;
    e.isBoss = false;
    e.formationX = formX;
    e.formationY = formY;
    e.x = e.formationX;
    e.y = -100.0f - startOffsetY;
    e.inPosition = false;
    e.baseColor = GetRandomBottleColor();
    e.textureIndex = textureIndex;
    return e;
}

int gEnemyTextureCount = 0;

int GetRandomEnemyTextureIndex() {
    if (gEnemyTextureCount <= 0) {
        return -1;
    }
    return GetRandomValue(0, gEnemyTextureCount - 1);
}

void SpawnLevel(vector<Enemy>& enemies, int level, int formationType) {
    int hpBonus = level * 15;
    float spacingX = 60.0f;
    float spacingY = 60.0f;
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
                enemies.push_back(CreateBasicEnemy(startX + col * spacingX, startY + row * spacingY, hpBonus, row * 100.0f, GetRandomEnemyTextureIndex()));
            }
        }
    }
    break;
    case 1: // V-KSZTAŁTNA
    {
        int count = 7 + level;
        for (int i = 0; i < count; ++i) {
            enemies.push_back(CreateBasicEnemy(centerX - (i * spacingX), startY + (i * spacingY), hpBonus, i * 80.0f, GetRandomEnemyTextureIndex()));
            if (i > 0) enemies.push_back(CreateBasicEnemy(centerX + (i * spacingX), startY + (i * spacingY), hpBonus, i * 80.0f, GetRandomEnemyTextureIndex()));
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
            enemies.push_back(CreateBasicEnemy(centerX + cos(angle) * radius, circleCenterY + sin(angle) * radius, hpBonus, i * 50.0f, GetRandomEnemyTextureIndex()));
        }
    }
    break;
    case 3: // BLOK
    {
        int rows = 5 + level / 2;
        int cols = 10 + level / 2;
        float tightSpacing = 56.0f;
        float startX = (1280 - (cols * tightSpacing)) / 2.0f + tightSpacing / 2.0f;
        for (int row = 0; row < rows; ++row) {
            for (int col = 0; col < cols; ++col) {
                enemies.push_back(CreateBasicEnemy(startX + col * tightSpacing, startY + row * tightSpacing, hpBonus, row * 80.0f, GetRandomEnemyTextureIndex()));
            }
        }
    }
    break;
    }
}
// Szyfrowanie cezara (legacy)
string caesarEncrypt(const string& text, int shift) {
    string result = text;
    for (char& c : result) {
        if (c >= 32 && c <= 126) { // znaki drukowalne ASCII
            c = (char)(32 + (c - 32 + shift) % 95);
        }
    }
    return result;
}

string caesarDecrypt(const string& text, int shift) {
    string result = text;
    for (char& c : result) {
        if (c >= 32 && c <= 126) {
            c = (char)(32 + (c - 32 - shift + 95) % 95);
        }
    }
    return result;
}

string xorTransform(const string& text, unsigned char key) {
    string result = text;
    for (char& c : result) {
        c = (char)(c ^ key);
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

// --- KOD OSOBY 3 (NIENARUSZONY) ---

// PROFIL
struct Profile {
    string nick;
    int bestScore = 0;
};

// ZARZĄDZANIE PROFILAMI 
class ProfileManager {
public:
    vector<Profile> profiles;
    int selected = 0;

    void load() {
        const char* kProfileFile = "profile.dat";
        const char* kLegacyProfileFile = "profiles.dat";
        const unsigned char kXorKey = 0x5A;

        profiles.clear();

        ifstream in(kProfileFile, ios::binary);
        if (in) {
            uint32_t count = 0;
            in.read(reinterpret_cast<char*>(&count), sizeof(count));
            if (!in.fail() && count < 1000) {
                for (uint32_t i = 0; i < count; ++i) {
                    uint32_t nameLen = 0;
                    int32_t score = 0;
                    in.read(reinterpret_cast<char*>(&nameLen), sizeof(nameLen));
                    in.read(reinterpret_cast<char*>(&score), sizeof(score));
                    if (in.fail() || nameLen > 256) {
                        break;
                    }
                    string enc(nameLen, '\0');
                    in.read(&enc[0], nameLen);
                    if (in.fail()) {
                        break;
                    }
                    string nick = xorTransform(enc, kXorKey);
                    profiles.push_back({ nick, score });
                }
                if (!profiles.empty()) {
                    return;
                }
            }
        }

        ifstream legacy(kLegacyProfileFile);
        if (legacy) {
            string nick;
            int score;
            const int SHIFT = 3;

            while (legacy >> nick >> score) {
                nick = caesarDecrypt(nick, SHIFT);
                profiles.push_back({ nick, score });
            }
            if (!profiles.empty()) {
                save();
            }
        }
    }


    void save() {
        const char* kProfileFile = "profile.dat";
        const unsigned char kXorKey = 0x5A;
        ofstream out(kProfileFile, ios::binary | ios::trunc);

        uint32_t count = (uint32_t)profiles.size();
        out.write(reinterpret_cast<const char*>(&count), sizeof(count));
        for (auto& p : profiles) {
            string enc = xorTransform(p.nick, kXorKey);
            uint32_t nameLen = (uint32_t)enc.size();
            int32_t score = (int32_t)p.bestScore;
            out.write(reinterpret_cast<const char*>(&nameLen), sizeof(nameLen));
            out.write(reinterpret_cast<const char*>(&score), sizeof(score));
            if (nameLen > 0) {
                out.write(enc.data(), nameLen);
            }
        }
    }


    void addProfile(const string& nick) {
        profiles.push_back({ nick, 0 });
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
    ToggleFullscreen();
    SetTargetFPS(60);
    InitAudioDevice();

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
    float rapidFireTimer = 0.0f;
    float drunkLevel = 0.0f;
    const float maxDrunk = 100.0f;
    float formationOffset = 0.0f; float formationDir = 1.0f;
    float transitionTimer = 0.0f;
    vector<Bullet> bullets; vector<Enemy> enemies; vector<PowerUp> powerups;

    Image playerImg = LoadImage(ResolveAssetPath("student3.png").c_str());
    Texture2D texPlayer = {};
    if (playerImg.data != nullptr) {
        MakeNearWhiteTransparent(&playerImg, 230);
        texPlayer = LoadTextureFromImage(playerImg);
        UnloadImage(playerImg);
    }
    Texture2D texBoss = LoadTexture(ResolveAssetPath("boss.png").c_str());
    Texture2D texCoffee = LoadTexture(ResolveAssetPath("kawa.png").c_str());
    Image enemyBulletImg = LoadImage(ResolveAssetPath("enemy.png").c_str());
    Texture2D texEnemyBullet = {};
    if (enemyBulletImg.data != nullptr) {
        MakeNearWhiteTransparent(&enemyBulletImg, 230);
        texEnemyBullet = LoadTextureFromImage(enemyBulletImg);
        UnloadImage(enemyBulletImg);
    }
    Sound sfxLevelUp = LoadSound(ResolveAssetPath("levelup.mp3").c_str());
    Sound sfxShoot = LoadSound(ResolveAssetPath("strzal.mp3").c_str());
    Sound sfxPower = LoadSound(ResolveAssetPath("power.mp3").c_str());
    Image powerHpImg = LoadImage(ResolveAssetPath("hp.png").c_str());
    Texture2D texPowerHp = {};
    if (powerHpImg.data != nullptr) {
        MakeNearWhiteTransparent(&powerHpImg, 230);
        texPowerHp = LoadTextureFromImage(powerHpImg);
        UnloadImage(powerHpImg);
    }
    Texture2D texPowerWeapon = LoadTexture(ResolveAssetPath("weapon.png").c_str());
    Texture2D texPowerShield = LoadTexture(ResolveAssetPath("shield.png").c_str());
    Texture2D texPowerFire = LoadTexture(ResolveAssetPath("fire.png").c_str());
    Image powerRapidImg = LoadImage(ResolveAssetPath("rapid.png").c_str());
    Texture2D texPowerRapid = {};
    if (powerRapidImg.data != nullptr) {
        MakeNearWhiteTransparent(&powerRapidImg, 230);
        texPowerRapid = LoadTextureFromImage(powerRapidImg);
        UnloadImage(powerRapidImg);
    }
    vector<Texture2D> enemyTextures;
    const vector<string> enemyFiles = { "tyskie2.png", "warka.png", "tyskie2.png", "zubr.png" };
    for (const auto& file : enemyFiles) {
        string path = ResolveAssetPath(file.c_str());
        Image img = LoadImage(path.c_str());
        if (img.data != nullptr) {
            MakeNearWhiteTransparent(&img, 230);
            Texture2D tex = LoadTextureFromImage(img);
            UnloadImage(img);
            if (tex.id != 0) {
                enemyTextures.push_back(tex);
            }
        }
    }
    if (enemyTextures.empty()) {
        string fallbackPath = ResolveAssetPath("enemy.png");
        Image img = LoadImage(fallbackPath.c_str());
        if (img.data != nullptr) {
            MakeNearWhiteTransparent(&img, 230);
            Texture2D tex = LoadTextureFromImage(img);
            UnloadImage(img);
            if (tex.id != 0) {
                enemyTextures.push_back(tex);
            }
        }
    }
    gEnemyTextureCount = (int)enemyTextures.size();

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
                    rapidFireTimer = 0.0f; drunkLevel = 0.0f;
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
            //  RYSOWANIE MENU
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
                    W / 2 - 200, 240 + i * 30, 22, c
                );
            }

            // ===== BLOK MENU (jedna baza Y) =====
            int menuY = 240 + pm.profiles.size() * 30 + 30;

            DrawText("ENTER - wybierz", W / 2 - 200, menuY, 20, DARKGRAY);
            DrawText("N - nowy profil", W / 2 - 200, menuY + 30, 20, DARKGRAY);

            // POLE NOWEGO NICKU POD "N - nowy profil"
            if (newProfileMode) {
                DrawText("Nowy nick:", W / 2 - 200, menuY + 70, 22, GRAY);
                DrawRectangle(W / 2 - 200, menuY + 100, 300, 40, DARKGRAY);
                DrawText(newNick.c_str(), W / 2 - 190, menuY + 110, 22, RAYWHITE);
            }

            // ESC na samym dole
            DrawText("ESC - wyjscie", W / 2 - 200, menuY + 160, 20, DARKGRAY);

            EndDrawing();

            continue;
        }

        // --- WIN SCREEN ---
        if (state == WIN_SCREEN) {
            if (IsKeyPressed(KEY_ENTER)) state = MENU;
            BeginDrawing(); ClearBackground(RAYWHITE);
            DrawText("GRATULACJE!", W / 2 - 200, H / 2 - 50, 60, GOLD);
            DrawText("Pokonales Bossa!", W / 2 - 250, H / 2 + 20, 30, DARKGRAY);
            DrawText(TextFormat("Wynik: %d", score), W / 2 - 150, H / 2 + 60, 30, BLACK);
            EndDrawing();
            continue;
        }

        // --- LEVEL TRANSITION (Ekrany przejścia / Startu) ---
        if (state == LEVEL_TRANSITION) {
            transitionTimer -= dt;
            if (transitionTimer <= 0) {
                level++; // Zwiekszamy level (z 0 na 1 przy starcie)
                int bossLevel = 6;
                if (level >= bossLevel) {
                    bossActive = true;
                    enemies.push_back({ (float)W / 2 - 160, -240, 320, 240, 5000, 5000, true, 0, 0, true, MAROON, -1 });
                }
                else {
                    int formationType = (level - 1) % 4;
                    SpawnLevel(enemies, level, formationType);
                    formationOffset = 0.0f;
                }
                if (IsSoundReady(sfxLevelUp)) {
                    PlaySound(sfxLevelUp);
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
            if (rapidFireTimer > 0.0f) {
                rapidFireTimer -= dt;
                if (rapidFireTimer <= 0.0f) {
                    rapidFireTimer = 0.0f;
                    rapidFire = false;
                }
            }

            if (drunkLevel > 0.0f) {
                drunkLevel = max(0.0f, drunkLevel - 6.0f * dt);
            }
            float drunkFactor = drunkLevel / maxDrunk;
            float moveSpeed = speed * (1.0f - 0.4f * drunkFactor);
            float wobble = sin(GetTime() * 2.5f) * 60.0f * drunkFactor;

            // STEROWANIE
            if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) playerX -= moveSpeed * dt; if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) playerX += moveSpeed * dt;
            if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) playerY -= moveSpeed * dt; if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) playerY += moveSpeed * dt;
            playerX += wobble * dt;
            if (playerX < 32) playerX = 32; if (playerX > W - 32) playerX = W - 32;
            if (playerY > H - 32) playerY = H - 32; if (playerY < 400) playerY = 400; // Limit gorny
            if (IsKeyPressed(KEY_ESCAPE)) state = MENU;

            // STRZELANIE
            if (shootTimer > 0) shootTimer -= dt;
            bool shootDown = IsKeyDown(KEY_SPACE) || IsMouseButtonDown(MOUSE_LEFT_BUTTON);
            bool shootPressed = IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
            bool shootInput = rapidFire ? shootDown : shootPressed;
            float fireRate = rapidFire ? 0.15f : 0.0f;
            if (shootInput && shootTimer <= 0) {
                shootTimer = fireRate; int dmg = fireAmmo ? 20 : 10; Color bulletColor = fireAmmo ? ORANGE : GREEN; float bSpeed = -600.0f;
                if (IsSoundReady(sfxShoot)) {
                    PlaySound(sfxShoot);
                }
                if (weaponLevel == 1) bullets.push_back({ playerX, playerY - 32, bSpeed, 0, false, bulletColor, dmg });
                else if (weaponLevel == 2) { bullets.push_back({ playerX - 10, playerY - 32, bSpeed, 0, false, bulletColor, dmg }); bullets.push_back({ playerX + 10, playerY - 32, bSpeed, 0, false, bulletColor, dmg }); }
                else if (weaponLevel == 3) { bullets.push_back({ playerX, playerY - 36, bSpeed, 0, false, bulletColor, dmg }); bullets.push_back({ playerX - 15, playerY - 26, bSpeed, -100, false, bulletColor, dmg }); bullets.push_back({ playerX + 15, playerY - 26, bSpeed, 100, false, bulletColor, dmg }); }
                else if (weaponLevel == 4) { bullets.push_back({ playerX - 10, playerY - 32, bSpeed, 0, false, bulletColor, dmg }); bullets.push_back({ playerX + 10, playerY - 32, bSpeed, 0, false, bulletColor, dmg }); bullets.push_back({ playerX - 25, playerY - 22, bSpeed, -150, false, bulletColor, dmg }); bullets.push_back({ playerX + 25, playerY - 22, bSpeed, 150, false, bulletColor, dmg }); }
                else if (weaponLevel >= 5) { bullets.push_back({ playerX, playerY - 36, bSpeed, 0, false, bulletColor, dmg }); bullets.push_back({ playerX - 15, playerY - 30, bSpeed, -50, false, bulletColor, dmg }); bullets.push_back({ playerX + 15, playerY - 30, bSpeed, 50, false, bulletColor, dmg }); bullets.push_back({ playerX - 30, playerY - 20, bSpeed, -200, false, bulletColor, dmg }); bullets.push_back({ playerX + 30, playerY - 20, bSpeed, 200, false, bulletColor, dmg }); }
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
                    if (e.y < 50) e.y += 100 * dt;
                    else e.x += sin(GetTime()) * 300 * dt;
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
            Rectangle playerRect = { playerX - 32, playerY - 32, 64, 64 };
            for (int i = 0; i < bullets.size(); i++) {
                if (bullets[i].isEnemy) {
                    if (CheckCollisionCircleRec({ bullets[i].x, bullets[i].y }, 5, playerRect)) {
                        if (hasShield) { hasShield = false; bullets.erase(bullets.begin() + i); i--; }

                        else {
                            playerHP -= 10; bullets.erase(bullets.begin() + i); i--;
                            drunkLevel = min(maxDrunk, drunkLevel + 10.0f);
                            if (playerHP <= 0)
                            {
                                if (score > pm.current().bestScore) {
                                    pm.current().bestScore = score;
                                    pm.save();
                                } state = MENU;
                            }
                        }
                    }
                }
            }
            for (int i = 0; i < bullets.size(); i++) {
                if (!bullets[i].isEnemy) {
                    bool hit = false; for (int j = 0; j < enemies.size(); j++) {
                        Rectangle enemyRect = { enemies[j].x, enemies[j].y, enemies[j].width, enemies[j].height }; if (CheckCollisionCircleRec({ bullets[i].x, bullets[i].y }, 3, enemyRect)) {
                            enemies[j].hp -= bullets[i].damage; hit = true; if (enemies[j].hp <= 0) {
                                score += (enemies[j].isBoss ? 5000 : 50); if (!enemies[j].isBoss && GetRandomValue(0, 100) < 20) { int pType = GetRandomValue(0, 5); powerups.push_back({ enemies[j].x, enemies[j].y, pType, true }); } if (enemies[j].isBoss) {
                                    if (IsSoundReady(sfxPower)) {
                                        PlaySound(sfxPower);
                                    }
                                    enemies.erase(enemies.begin() + j); if (score > pm.current().bestScore) {
                                        pm.current().bestScore = score;
                                        pm.save();
                                    } state = WIN_SCREEN; break;
                                } enemies.erase(enemies.begin() + j);
                            } break;
                        }
                    } if (hit) { bullets.erase(bullets.begin() + i); i--; }
                }
            }
            for (int i = 0; i < powerups.size(); i++) {
                powerups[i].y += 150 * dt;
                int t = powerups[i].type;
                float size = (t == 0 || t == 2 || t == 5) ? 36.0f : 28.0f;
                if (CheckCollisionRecs(playerRect, { powerups[i].x, powerups[i].y, size, size })) {
                    if (IsSoundReady(sfxPower)) {
                        PlaySound(sfxPower);
                    }
                    if (t == 0) playerHP = (playerHP + 30 > 100) ? 100 : playerHP + 30;
                    if (t == 1) { if (weaponLevel < 5) weaponLevel++; }
                    if (t == 2) hasShield = true;
                    if (t == 3) fireAmmo = true;
                    if (t == 4) { rapidFire = true; rapidFireTimer = max(rapidFireTimer, 8.0f); }
                    if (t == 5) { playerHP = (playerHP + 20 > 100) ? 100 : playerHP + 20; drunkLevel = max(0.0f, drunkLevel - 40.0f); rapidFire = true; rapidFireTimer = max(rapidFireTimer, 6.0f); }
                    powerups.erase(powerups.begin() + i);
                    i--;
                } else if (powerups[i].y > H) {
                    powerups.erase(powerups.begin() + i);
                    i--;
                }
            }
        }

        // RYSOWANIE GRY (OSOBA 1)
        BeginDrawing();
        ClearBackground(BLACK);
        if (!pm.profiles.empty() && state == GAME) {
            if (texPlayer.id != 0) {
                Rectangle srcPlayer = { 0, 0, (float)texPlayer.width, (float)texPlayer.height };
                Rectangle dstPlayer = { playerX - 32, playerY - 32, 64, 64 };
                DrawTexturePro(texPlayer, srcPlayer, dstPlayer, { 0, 0 }, 0.0f, WHITE);
            } else {
                DrawCircle((int)playerX, (int)playerY, 18, RAYWHITE);
            }
            if (hasShield) DrawRing({ playerX, playerY }, 32, 36, 0, 360, 0, SKYBLUE);
            for (auto& e : enemies) {
                Color c = e.isBoss ? e.baseColor : (e.inPosition ? e.baseColor : RED);
                if (e.isBoss && texBoss.id != 0) {
                    Rectangle srcBoss = { 0, 0, (float)texBoss.width, (float)texBoss.height };
                    Rectangle dstBoss = { e.x, e.y, e.width, e.height };
                    DrawTexturePro(texBoss, srcBoss, dstBoss, { 0, 0 }, 0.0f, WHITE);
                } else if (!e.isBoss && e.textureIndex >= 0 && e.textureIndex < (int)enemyTextures.size() && enemyTextures[e.textureIndex].id != 0) {
                    Texture2D& tex = enemyTextures[e.textureIndex];
                    Rectangle srcEnemy = { 0, 0, (float)tex.width, (float)tex.height };
                    Rectangle dstEnemy = { e.x, e.y, e.width, e.height };
                    DrawTexturePro(tex, srcEnemy, dstEnemy, { 0, 0 }, 0.0f, WHITE);
                } else {
                    DrawRectangle((int)e.x, (int)e.y, (int)e.width, (int)e.height, c);
                }
                if (e.isBoss) {
                    DrawRectangle(e.x, e.y - 15, e.width, 10, DARKGRAY);
                    DrawRectangle(e.x, e.y - 15, e.width * ((float)e.hp / e.maxHp), 10, GREEN);
                }
            }
            for (auto& b : bullets) {
                if (b.isEnemy && texEnemyBullet.id != 0) {
                    Rectangle srcBullet = { 0, 0, (float)texEnemyBullet.width, (float)texEnemyBullet.height };
                    Rectangle dstBullet = { b.x - 14, b.y - 14, 28, 28 };
                    DrawTexturePro(texEnemyBullet, srcBullet, dstBullet, { 0, 0 }, 0.0f, WHITE);
                } else if (!b.isEnemy && texPowerWeapon.id != 0) {
                    Rectangle srcBullet = { 0, 0, (float)texPowerWeapon.width, (float)texPowerWeapon.height };
                    Rectangle dstBullet = { b.x - 8, b.y - 8, 16, 16 };
                    DrawTexturePro(texPowerWeapon, srcBullet, dstBullet, { 0, 0 }, 0.0f, WHITE);
                } else {
                    DrawCircle((int)b.x, (int)b.y, b.isEnemy ? 5 : (fireAmmo ? 5 : 3), b.color);
                }
            }
            for (auto& p : powerups) {
                Color pc = WHITE;
                const char* txt = "?";
                Texture2D* tex = nullptr;
                if (p.type == 0) { pc = GREEN; txt = "+"; if (texPowerHp.id != 0) tex = &texPowerHp; }
                if (p.type == 1) { pc = YELLOW; txt = "W"; if (texPowerWeapon.id != 0) tex = &texPowerWeapon; }
                if (p.type == 2) { pc = BLUE; txt = "S"; if (texPowerShield.id != 0) tex = &texPowerShield; }
                if (p.type == 3) { pc = ORANGE; txt = "F"; if (texPowerFire.id != 0) tex = &texPowerFire; }
                if (p.type == 4) { pc = PURPLE; txt = "R"; if (texPowerRapid.id != 0) tex = &texPowerRapid; }
                if (p.type == 5 && texCoffee.id != 0) { tex = &texCoffee; }
                float size = (p.type == 0 || p.type == 2 || p.type == 5) ? 36.0f : 28.0f;
                if (p.type == 4) size = 40.0f;
                if (tex != nullptr && tex->id != 0) {
                    Rectangle src = { 0, 0, (float)tex->width, (float)tex->height };
                    Rectangle dst = { p.x, p.y, size, size };
                    DrawTexturePro(*tex, src, dst, { 0, 0 }, 0.0f, WHITE);
                } else {
                    DrawRectangle((int)p.x, (int)p.y, (int)size, (int)size, pc);
                    DrawText(txt, (int)p.x + 7, (int)p.y + 4, 12, BLACK);
                }
            }
            DrawText(TextFormat("Gracz: %s", pm.current().nick.c_str()), 10, 10, 20, WHITE); DrawText(TextFormat("HP: %d", playerHP), 10, 40, 20, GREEN); DrawText(TextFormat("Score: %d", score), 10, 70, 20, YELLOW);
            DrawText(
                TextFormat("Best: %d", pm.current().bestScore),
                10, 100, 20, GOLD);
            DrawText("Active bonuses:", 10, 110, 15, GRAY); if (fireAmmo) DrawText("FIRE AMMO", 10, 130, 15, ORANGE); if (rapidFire) DrawText("RAPID FIRE", 10, 150, 15, PURPLE); DrawText(TextFormat("Weapon Lvl: %d/5", weaponLevel), 10, 170, 15, YELLOW);
            DrawText(TextFormat("Upicie: %d%%", (int)drunkLevel), 10, 190, 15, LIGHTGRAY);
            if (bossActive) DrawText("FINAL BOSS!", W / 2 - 100, 50, 30, RED); else DrawText(TextFormat("LEVEL: %d", level), W - 150, 10, 20, BLUE);
        }

        EndDrawing();
    }

    UnloadTexture(texPlayer);
    UnloadTexture(texBoss);
    UnloadTexture(texCoffee);
    UnloadTexture(texEnemyBullet);
    UnloadTexture(texPowerHp);
    UnloadTexture(texPowerWeapon);
    UnloadTexture(texPowerShield);
    UnloadTexture(texPowerFire);
    UnloadTexture(texPowerRapid);
    for (auto& tex : enemyTextures) {
        UnloadTexture(tex);
    }
    UnloadSound(sfxLevelUp);
    UnloadSound(sfxShoot);
    UnloadSound(sfxPower);
    CloseAudioDevice();

    CloseWindow();
    return 0;
}

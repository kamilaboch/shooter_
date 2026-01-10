#include "raylib.h"
#include <string>
#include <vector>
#include <fstream>
#include <algorithm>
#include <cmath>

using namespace std;

// ---------------------------------------------------------
// BAZA DANYCH PIW
// ---------------------------------------------------------
struct BeerType {
    string name;
    Color color;
    int maxHp;
    int scoreValue;
};

const BeerType BEER_DB[10] = {
    { "KUFLOWE MOCNE (BOSS)", RED,        50, 5000 },
    { "Perla Export",         GOLD,       9,  500 },
    { "Heineken",             GREEN,      8,  400 },
    { "Harnas",               ORANGE,     7,  300 },
    { "Lech",                 LIME,       6,  250 },
    { "Zywiec",               BLUE,       5,  200 },
    { "EB",                   YELLOW,     4,  150 },
    { "Zubr",                 DARKGREEN,  3,  100 },
    { "Tyskie",               MAROON,     2,  50 },
    { "Debowe Mocne",         BROWN,      1,  10 },
};

// ---------------------------------------------------------
// ENUMY I STRUKTURY
// ---------------------------------------------------------
enum DropType {
    DROP_SIP,
    DROP_CURE,
    DROP_MULTISHOT,
    DROP_DMG,
    DROP_RAPID
};

struct PowerUp {
    float x, y;
    DropType type;
    bool active;
    Color color;
    char symbol;
};

struct Bullet {
    float x, y;
    float speedX;
    bool active;
    bool piercing;
    int damage;
};

struct Enemy { float x, y, width, height; int hp, maxHp, beerIndex; bool active; };
struct Particle { float x, y, speedX, speedY, life; bool active; Color color; };

// GRACZ
struct Player {
    Vector2 pos;
    Vector2 velocity;
    float intoxication; // 0.0 - 100.0

    // Statystyki
    float baseSpeed;
    float baseAccel;

    // Buffy
    int bonusBullets;
    bool buffRapid;
    bool buffDmg;
    bool buffPierce;
};

// ---------------------------------------------------------
// PROFIL MANAGER
// ---------------------------------------------------------
struct Profile { string nick; };
class ProfileManager {
public:
    vector<Profile> profiles;
    int selected = 0;
    void load() {
        ifstream in("profiles.dat"); profiles.clear(); Profile p;
        while (in >> p.nick) profiles.push_back(p);
    }
    void save() {
        ofstream out("profiles.dat"); for (auto& p : profiles) out << p.nick << "\n";
    }
    void addProfile(const string& nick) {
        profiles.push_back({ nick }); selected = (int)profiles.size() - 1; save();
    }
};

// ---------------------------------------------------------
// LEVEL GENERATOR
// ---------------------------------------------------------
enum GameState { MENU, GAME, CHUG_CHALLENGE, WIN_SCREEN, LOSE_SCREEN };

int GetMaxWaves(int level) {
    if (level == 10) return 1;
    if (level < 3) return 1;
    return 2;
}

void SpawnEnemy(vector<Enemy>& enemies, float x, float y, int beerIdx) {
    Enemy e;
    float scale = 0.8f + (beerIdx * 0.08f);
    e.width = 90 * scale;
    e.height = 140 * scale;
    e.x = x - e.width / 2.0f;
    e.y = y;
    e.beerIndex = beerIdx;
    e.hp = BEER_DB[beerIdx].maxHp;
    e.maxHp = e.hp;
    e.active = true;
    enemies.push_back(e);
}

void ClearBoard(vector<Bullet>& bullets, vector<Particle>& particles, vector<PowerUp>& powerups) {
    bullets.clear();
    particles.clear();
    powerups.clear();
}

void LoadLevel(int level, int wave, vector<Enemy>& enemies, int screenW) {
    enemies.clear();

    if (level == 10) {
        Enemy boss;
        boss.width = 800; boss.height = 900;
        boss.x = screenW / 2.0f - boss.width / 2.0f; boss.y = 50;
        boss.beerIndex = 0; boss.hp = BEER_DB[0].maxHp; boss.maxHp = boss.hp;
        boss.active = true;
        enemies.push_back(boss);
        return;
    }

    int strongestAllowed = 9 - (level - 1);
    if (wave == 2) strongestAllowed--;
    if (strongestAllowed < 1) strongestAllowed = 1;

    int cols = 9; int rows = 5;
    float gapX = 140; float gapY = 160;
    float startX = (screenW - ((cols - 1) * gapX)) / 2.0f;
    float startY = 80;

    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            bool spawn = false;
            int beerType = strongestAllowed + r;
            if (beerType > 9) beerType = 9;

            if (level == 1) { if (r < 3) { spawn = true; beerType = 9; } }
            else if (level == 2) { int center = cols / 2; if (c >= center - r && c <= center + r) spawn = true; }
            else if (level == 3) { if (c == r || c == (cols - 1 - r)) spawn = true; if (r > 1 && (c == r - 2 || c == (cols - 1 - (r - 2)))) spawn = true; beerType = strongestAllowed + (r % 2); }
            else if (level == 4) { if (r < 4 && (r + c) % 2 == 0) spawn = true; beerType = ((r + c) % 3 == 0) ? strongestAllowed : strongestAllowed + 1; }
            else if (level == 5) { int center = cols / 2; int midRow = rows / 2; if (abs(c - center) + abs(r - midRow) <= 2) spawn = true; }
            else if (level == 6) { if (c < 2 || (c > 3 && c < 5) || c > 6) spawn = true; }
            else if (level == 7) { int center = cols / 2; if (r == 0 || r == 1) spawn = true; if (r > 1 && abs(c - center) <= (rows - r)) spawn = true; }
            else if (level == 8) { if (abs(c - r) <= 1 || abs(c - (cols - 1 - r)) <= 1) { spawn = true; beerType = GetRandomValue(strongestAllowed, 9); } }
            else if (level == 9) { if (GetRandomValue(0, 10) > 1) { spawn = true; beerType = GetRandomValue(1, 9); } }

            if (spawn) SpawnEnemy(enemies, startX + c * gapX, startY + r * gapY, beerType);
        }
    }
}

// ---------------------------------------------------------
// MAIN
// ---------------------------------------------------------
int main() {
    // --- POPRAWA INICJALIZACJI EKRANU ---
    // Najpierw pobieramy rozmiar monitora, potem ustawiamy okno.
    // To zapobiega rozci¹ganiu i rozmywaniu grafiki.
    InitWindow(800, 600, "Flanki Simulator: Student Edition"); // Tymczasowe okno
    int monitor = GetCurrentMonitor();
    int W = GetMonitorWidth(monitor);
    int H = GetMonitorHeight(monitor);
    SetWindowSize(W, H);
    ToggleFullscreen();
    SetTargetFPS(60);

    // TEKSTURY
    Texture2D beerTextures[10];
    beerTextures[0] = LoadTexture("assets/kuflowe.png");
    beerTextures[1] = LoadTexture("assets/perla.png");
    beerTextures[2] = LoadTexture("assets/heineken.png");
    beerTextures[3] = LoadTexture("assets/harnas.png");
    beerTextures[4] = LoadTexture("assets/lech.png");
    beerTextures[5] = LoadTexture("assets/zywiec.png");
    beerTextures[6] = LoadTexture("assets/eb.png");
    beerTextures[7] = LoadTexture("assets/zubr.png");
    beerTextures[8] = LoadTexture("assets/tyskie.png");
    beerTextures[9] = LoadTexture("assets/debowe.png");

    // FILTER_POINT = Ostra grafika (Pixel Perfect), brak rozmycia
    for (int i = 0; i < 10; i++) SetTextureFilter(beerTextures[i], TEXTURE_FILTER_POINT);

    Texture2D playerTexture = LoadTexture("assets/gracz.png");
    SetTextureFilter(playerTexture, TEXTURE_FILTER_POINT);

    GameState state = MENU;
    ProfileManager pm; pm.load();

    Player player;
    player.pos = { (float)W / 2.0f, (float)H - 100.0f };
    player.velocity = { 0, 0 };
    player.intoxication = 0.0f;
    player.baseSpeed = 1200.0f;
    player.baseAccel = 5000.0f;
    player.bonusBullets = 0;
    player.buffDmg = false;
    player.buffRapid = false;
    player.buffPierce = false;

    int score = 0;
    int currentLevel = 1;
    int currentWave = 1;
    int startLevel = 1;

    vector<Bullet> bullets;
    vector<Enemy> enemies;
    vector<Particle> particles;
    vector<PowerUp> powerups;

    float shootTimer = 0.0f;
    float chugProgress = 0.0f;
    float chugTimeLeft = 8.0f;
    int enemyDirection = 1;
    bool newProfileMode = false; string newNick = "";

    // ---------------------------------------------------------
    // G£ÓWNA PÊTLA GRY
    // ---------------------------------------------------------
    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        // --- UPDATE (LOGIKA) ---
        if (state == MENU) {
            if (newProfileMode) {
                int key = GetCharPressed();
                while (key > 0) { if (key >= 32 && key <= 125 && newNick.length() < 12) newNick += (char)key; key = GetCharPressed(); }
                if (IsKeyPressed(KEY_BACKSPACE) && !newNick.empty()) newNick.pop_back();
                if (IsKeyPressed(KEY_ENTER) && !newNick.empty()) { pm.addProfile(newNick); newNick.clear(); newProfileMode = false; }
            }
            else if (!pm.profiles.empty()) {
                if (IsKeyPressed(KEY_UP)) pm.selected = (pm.selected - 1 + pm.profiles.size()) % pm.profiles.size();
                if (IsKeyPressed(KEY_DOWN)) pm.selected = (pm.selected + 1) % pm.profiles.size();
                if (IsKeyPressed(KEY_RIGHT)) { startLevel++; if (startLevel > 10) startLevel = 1; }
                if (IsKeyPressed(KEY_LEFT)) { startLevel--; if (startLevel < 1) startLevel = 10; }
                if (IsKeyPressed(KEY_ENTER)) {
                    // RESET
                    player.pos = { (float)W / 2.0f, (float)H - 100.0f };
                    player.velocity = { 0,0 };
                    player.intoxication = 0.0f;
                    player.bonusBullets = 0;
                    player.buffDmg = false;
                    player.buffRapid = false;
                    ClearBoard(bullets, particles, powerups);
                    score = 0;
                    currentLevel = startLevel;
                    currentWave = 1;
                    LoadLevel(currentLevel, currentWave, enemies, W);
                    state = GAME;
                }
            }
            if (IsKeyPressed(KEY_N)) { newProfileMode = true; newNick.clear(); }
        }
        else if (state == GAME) {
            // Fizyka
            float soberFactor = 1.0f - (player.intoxication / 100.0f);
            if (soberFactor < 0.2f) soberFactor = 0.2f;
            float currentMaxSpeed = player.baseSpeed * soberFactor;
            float currentAccel = player.baseAccel * soberFactor;
            float currentFriction = 0.82f + (player.intoxication / 100.0f) * 0.16f;

            Vector2 input = { 0, 0 };
            if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) input.x -= 1;
            if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) input.x += 1;
            if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) input.y -= 1;
            if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) input.y += 1;

            player.velocity.x += input.x * currentAccel * dt;
            player.velocity.y += input.y * currentAccel * dt;

            float speed = sqrt(player.velocity.x * player.velocity.x + player.velocity.y * player.velocity.y);
            if (speed > currentMaxSpeed) {
                float scale = currentMaxSpeed / speed;
                player.velocity.x *= scale;
                player.velocity.y *= scale;
            }
            player.velocity.x *= currentFriction;
            player.velocity.y *= currentFriction;
            player.pos.x += player.velocity.x * dt;
            player.pos.y += player.velocity.y * dt;

            // Odbicia
            if (player.pos.x < 30) { player.pos.x = 30; player.velocity.x *= -0.5; }
            if (player.pos.x > W - 30) { player.pos.x = W - 30; player.velocity.x *= -0.5; }
            if (player.pos.y < 30) { player.pos.y = 30; player.velocity.y *= -0.5; }
            if (player.pos.y > H - 30) { player.pos.y = H - 30; player.velocity.y *= -0.5; }

            // Strzelanie
            shootTimer += dt;
            float fireRate = player.buffRapid ? 0.15f : 0.35f;
            if (IsKeyDown(KEY_SPACE) && shootTimer >= fireRate) {
                int dmg = player.buffDmg ? 3 : 1;
                int totalBullets = 1 + player.bonusBullets;
                for (int i = 0; i < totalBullets; i++) {
                    float spreadX = 0;
                    if (totalBullets > 1) {
                        float range = 300.0f;
                        float step = range / (totalBullets - 1);
                        spreadX = -range / 2.0f + (step * i);
                    }
                    bullets.push_back({ player.pos.x, player.pos.y - 30, spreadX, true, player.buffPierce, dmg });
                }
                shootTimer = 0.0f;
            }

            // Wrogowie
            bool touchEdge = false;
            float moveSpeed = 80.0f + (currentLevel * 10.0f) + (currentWave * 20.0f);
            if (currentLevel == 10) moveSpeed = 200.0f;

            for (auto& e : enemies) {
                if (!e.active) continue;
                e.x += moveSpeed * dt * enemyDirection;
                if ((e.x <= 10 && enemyDirection == -1) || (e.x + e.width >= W - 10 && enemyDirection == 1)) touchEdge = true;
            }
            if (touchEdge) { enemyDirection *= -1; for (auto& e : enemies) e.y += 20; }

            // Kolizje
            for (auto& b : bullets) {
                if (!b.active) continue;
                b.y -= 1100.0f * dt; b.x += b.speedX * dt;
                if (b.y < 0) b.active = false;

                for (auto& e : enemies) {
                    if (!e.active) continue;
                    if (CheckCollisionCircleRec({ b.x, b.y }, 8, { e.x, e.y, e.width, e.height })) {
                        if (!b.piercing) b.active = false;
                        e.hp -= b.damage;
                        for (int i = 0; i < 3; i++) particles.push_back({ b.x, b.y, (float)GetRandomValue(-100,100), (float)GetRandomValue(-100,100), 0.5f, true, BEER_DB[e.beerIndex].color });

                        if (e.hp <= 0) {
                            e.active = false; score += BEER_DB[e.beerIndex].scoreValue;
                            int rand = GetRandomValue(0, 100);
                            if (rand < 20) { PowerUp p; p.x = e.x + e.width / 2; p.y = e.y + e.height / 2; p.active = true; p.type = DROP_SIP; p.color = GOLD; p.symbol = '%'; powerups.push_back(p); }
                            else if (rand < 25) { PowerUp p; p.x = e.x + e.width / 2; p.y = e.y + e.height / 2; p.active = true; p.type = DROP_CURE; p.color = GREEN; p.symbol = '+'; powerups.push_back(p); }
                            else if (rand < 50) {
                                PowerUp p; p.x = e.x + e.width / 2; p.y = e.y + e.height / 2; p.active = true;
                                int w = GetRandomValue(0, 2);
                                if (w == 0) { p.type = DROP_MULTISHOT; p.color = SKYBLUE; p.symbol = 'M'; }
                                if (w == 1) { p.type = DROP_DMG; p.color = RED; p.symbol = 'D'; }
                                if (w == 2) { p.type = DROP_RAPID; p.color = ORANGE; p.symbol = 'R'; }
                                powerups.push_back(p);
                            }
                        }
                        if (!b.piercing) break;
                    }
                }
            }

            for (auto& p : powerups) {
                if (!p.active) continue;
                p.y += 300.0f * dt;
                if (p.y > H) p.active = false;

                if (CheckCollisionCircleRec({ p.x, p.y }, 30, { player.pos.x - 32, player.pos.y - 32, 64, 64 })) {
                    p.active = false;
                    if (p.type == DROP_SIP) { player.intoxication += 5.0f; score += 100; }
                    else if (p.type == DROP_CURE) { player.intoxication -= 15.0f; if (player.intoxication < 0) player.intoxication = 0; }
                    else if (p.type == DROP_MULTISHOT) { player.bonusBullets++; }
                    else if (p.type == DROP_DMG) { player.buffDmg = true; }
                    else if (p.type == DROP_RAPID) { player.buffRapid = true; }
                }
            }

            if (player.intoxication >= 100.0f) state = LOSE_SCREEN;

            bool allDead = true;
            for (auto& e : enemies) {
                if (e.active) {
                    allDead = false;
                    if (CheckCollisionRecs({ player.pos.x - 25, player.pos.y - 25, 50, 50 }, { e.x, e.y, e.width, e.height })) {
                        player.intoxication += 15.0f; e.active = false;
                    }
                    if (e.y + e.height >= H) { player.intoxication += 10.0f; e.active = false; }
                }
            }

            if (allDead) {
                int maxWaves = GetMaxWaves(currentLevel);
                if (currentWave < maxWaves) {
                    currentWave++;
                    ClearBoard(bullets, particles, powerups);
                    LoadLevel(currentLevel, currentWave, enemies, W);
                }
                else {
                    if (currentLevel == 3 || currentLevel == 6 || currentLevel == 9) {
                        state = CHUG_CHALLENGE;
                        chugProgress = 0.0f;
                        chugTimeLeft = 8.0f;
                        ClearBoard(bullets, particles, powerups);
                    }
                    else {
                        currentLevel++;
                        currentWave = 1;
                        player.bonusBullets = 0; player.buffDmg = false; player.buffRapid = false;
                        if (currentLevel > 10) state = WIN_SCREEN;
                        else {
                            ClearBoard(bullets, particles, powerups);
                            LoadLevel(currentLevel, currentWave, enemies, W);
                        }
                    }
                }
            }

            for (auto& p : particles) { if (p.active) { p.x += p.speedX * dt; p.y += p.speedY * dt; p.life -= dt; if (p.life <= 0)p.active = false; } }
        }
        else if (state == CHUG_CHALLENGE) {
            chugTimeLeft -= dt;
            if (IsKeyPressed(KEY_SPACE)) chugProgress += 8.0f;
            chugProgress -= 15.0f * dt;
            if (chugProgress < 0) chugProgress = 0;

            if (chugProgress >= 100.0f) {
                player.intoxication += 20.0f;
                currentLevel++; currentWave = 1;
                player.bonusBullets = 0; player.buffDmg = false; player.buffRapid = false;

                ClearBoard(bullets, particles, powerups);
                LoadLevel(currentLevel, currentWave, enemies, W);
                state = GAME;
            }
            else if (chugTimeLeft <= 0) {
                currentWave = 1; player.bonusBullets = 0; player.buffDmg = false; player.buffRapid = false;

                ClearBoard(bullets, particles, powerups);
                LoadLevel(currentLevel, currentWave, enemies, W);
                state = GAME;
            }
        }
        else if (state == WIN_SCREEN || state == LOSE_SCREEN) {
            if (IsKeyPressed(KEY_ENTER)) state = MENU;
        }

        // --- RYSOWANIE (ODDZIELONE!) ---
        BeginDrawing();

        // !!!!!!!!! KLUCZOWE !!!!!!!!!
        // ClearBackground jest TERAZ tutaj, na samym pocz¹tku, 
        // niezale¿nie od if/else. To naprawia "rozmywanie" i "dziwny efekt".
        ClearBackground(BLACK);

        if (state == MENU) {
            DrawText("FLANKI SIMULATOR: STUDENT EDITION", W / 2 - 400, 100, 40, ORANGE);
            if (!pm.profiles.empty() && !newProfileMode) {
                for (int i = 0; i < pm.profiles.size(); i++) DrawText(pm.profiles[i].nick.c_str(), W / 2 - 50, 250 + i * 40, 30, (i == pm.selected) ? GREEN : GRAY);
                DrawText(TextFormat("< START LEVEL: %d >", startLevel), W / 2 - 120, H - 150, 30, YELLOW);
            }
            if (newProfileMode) DrawText(newNick.c_str(), W / 2 - 50, 400, 30, WHITE);
        }
        else if (state == GAME) {
            for (int i = 0; i < 80; i++) DrawPixel(GetRandomValue(0, W), GetRandomValue(0, H), DARKGRAY);

            if (playerTexture.id > 0) DrawTexturePro(playerTexture, { 0,0,(float)playerTexture.width,(float)playerTexture.height }, { player.pos.x - 32, player.pos.y - 32, 64,64 }, { 0,0 }, 0.0f, WHITE);
            else DrawCircle((int)player.pos.x, (int)player.pos.y, 20, BLUE);

            for (auto& p : powerups) {
                if (p.active) {
                    DrawCircle((int)p.x, (int)p.y, 20, p.color);
                    DrawCircleLines((int)p.x, (int)p.y, 20, WHITE);
                    DrawText(TextFormat("%c", p.symbol), (int)p.x - 8, (int)p.y - 10, 24, WHITE);
                }
            }

            for (auto& b : bullets) if (b.active) DrawCircle((int)b.x, (int)b.y, 6, YELLOW);

            for (auto& e : enemies) {
                if (e.active) {
                    Texture2D tex = beerTextures[e.beerIndex];
                    if (tex.id > 0) DrawTexturePro(tex, { 0,0,(float)tex.width,(float)tex.height }, { e.x,e.y,e.width,e.height }, { 0,0 }, 0.0f, WHITE);
                    else DrawRectangle(e.x, e.y, e.width, e.height, BEER_DB[e.beerIndex].color);
                    if (e.maxHp > 1) {
                        float hpP = (float)e.hp / e.maxHp;
                        DrawRectangle(e.x, e.y - 8, e.width, 5, RED); DrawRectangle(e.x, e.y - 8, e.width * hpP, 5, GREEN);
                    }
                }
            }
            for (auto& p : particles) if (p.active) DrawRectangle(p.x, p.y, 4, 4, p.color);

            DrawText(TextFormat("PKT: %d", score), 20, 20, 30, WHITE);
            DrawText(TextFormat("LVL: %d", currentLevel), W / 2 - 50, 20, 30, WHITE);

            Color drunkColor = GREEN;
            if (player.intoxication > 30) drunkColor = YELLOW;
            if (player.intoxication > 60) drunkColor = ORANGE;
            if (player.intoxication > 85) drunkColor = RED;
            DrawText("UPOJENIE:", W - 250, 20, 20, drunkColor);
            DrawRectangle(W - 250, 50, 200, 30, DARKGRAY);
            DrawRectangle(W - 250, 50, (int)(player.intoxication * 2), 30, drunkColor);
            DrawText(TextFormat("%.0f%%", player.intoxication), W - 250 + 80, 55, 20, BLACK);

            int buffY = H - 50;
            if (player.bonusBullets > 0) { DrawText(TextFormat("MULTI x%d", player.bonusBullets + 1), 20, buffY, 20, SKYBLUE); buffY -= 25; }
            if (player.buffDmg) { DrawText("DMG UP", 20, buffY, 20, RED); buffY -= 25; }
            if (player.buffRapid) { DrawText("RAPID", 20, buffY, 20, ORANGE); buffY -= 25; }
        }
        else if (state == CHUG_CHALLENGE) {
            DrawText("BONUS LEVEL: ZERUJ PIWKO!", W / 2 - 300, 100, 50, YELLOW);
            DrawText("SPACJA! SPACJA! SPACJA!", W / 2 - 250, 200, 30, WHITE);

            Texture2D mug = beerTextures[0];
            float mugW = mug.width;
            float mugH = mug.height;
            float mugX = W / 2 - mugW / 2;
            float mugY = H / 2 - mugH / 2 + 50;

            // Rysujemy Pe³ny Kufel
            DrawTexture(mug, mugX, mugY, WHITE);

            // Rysujemy ciemny prostok¹t ZAS£ANIAJ¥CY piwo od góry
            float drankPercent = chugProgress / 100.0f; // 0.0 -> 1.0
            if (drankPercent > 1.0f) drankPercent = 1.0f;

            // Zas³aniamy górê piwa kolorem t³a (BLACK)
            // Jeœli wypite 10% (0.1), to zas³aniamy górne 10% piwa
            DrawRectangle(mugX, mugY, mugW, (int)(mugH * drankPercent), BLACK);

            DrawRectangleLines(mugX, mugY, mugW, mugH, WHITE);
            DrawText(TextFormat("CZAS: %.1f", chugTimeLeft), W / 2 - 80, H - 100, 40, RED);
        }
        else if (state == WIN_SCREEN || state == LOSE_SCREEN) {
            if (state == WIN_SCREEN) DrawText("WYGRANA! ZALICZONA SESJA!", W / 2 - 300, H / 2, 50, GREEN);
            else DrawText("ZGON... (100% UPOJENIA)", W / 2 - 300, H / 2, 50, RED);
            DrawText("ENTER - MENU", W / 2 - 100, H / 2 + 100, 30, GRAY);
        }

        EndDrawing();
    }

    for (int i = 0; i < 10; i++) UnloadTexture(beerTextures[i]);
    UnloadTexture(playerTexture);
    CloseWindow();
    return 0;
}
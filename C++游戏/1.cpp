#include <graphics.h>
#include <windows.h>
#include <vector>
#include <algorithm>
#include <cmath>
#include <time.h>
#include <mmsystem.h>

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "MSIMG32.LIB")

#define WIN_WIDTH   800
#define WIN_HEIGHT  600

// ------------------- 音乐控制 -------------------
void PlayMusic(const char* file, const char* alias, bool loop = false) {
    char cmd[256];
    sprintf_s(cmd, "close %s", alias);
    mciSendString(cmd, NULL, 0, NULL);
    sprintf_s(cmd, "open \"%s\" alias %s", file, alias);
    mciSendString(cmd, NULL, 0, NULL);
    sprintf_s(cmd, "play %s %s", alias, loop ? "repeat" : "");
    mciSendString(cmd, NULL, 0, NULL);
}
void StopMusic(const char* alias) {
    char cmd[256];
    sprintf_s(cmd, "stop %s", alias);
    mciSendString(cmd, NULL, 0, NULL);
    sprintf_s(cmd, "close %s", alias);
    mciSendString(cmd, NULL, 0, NULL);
}
void PauseMusic(const char* alias) {
    char cmd[256];
    sprintf_s(cmd, "pause %s", alias);
    mciSendString(cmd, NULL, 0, NULL);
}
void ResumeMusic(const char* alias) {
    char cmd[256];
    sprintf_s(cmd, "resume %s", alias);
    mciSendString(cmd, NULL, 0, NULL);
}

// ------------------- 核心游戏常量 -------------------
#define PLAYER_W    50
#define PLAYER_H    50
#define ARROW_W     30
#define ARROW_H     10
#define ENEMY_W     70
#define ENEMY_H     70
#define BALL_SIZE   10
#define BALL1_SISE   20
const int ROAD_Y[3] = { 280, 340, 400 };
#define PLAYER_SPEED_X   375.0f
#define ARROW_SPEED      200.0f
#define ENEMY_SPEED      30.0f

#define BALL_SPEED       100.0f
#define SMALL_SHOOT_INTERVAL 7.0f
#define MEDIUM_SHOOT_INTERVAL 3.0f
#define ATTACK_RANGE     400.0f

#define ATK_BTN_RADIUS   40
#define ATK_BTN_X        (WIN_WIDTH - ATK_BTN_RADIUS - 20)
#define ATK_BTN_Y        (WIN_HEIGHT - ATK_BTN_RADIUS - 20)

#define SKILL_RADIUS     28
#define SKILL_LEFT_OFFSET_X   -100
#define SKILL_LEFT_OFFSET_Y   0
#define SKILL_LEFTUP_OFFSET_X -70
#define SKILL_LEFTUP_OFFSET_Y -70
#define SKILL_UP_OFFSET_X     0
#define SKILL_UP_OFFSET_Y     -90

#define SMALL_BTN_RADIUS 25
#define SMALL_BTN_START_X (20 + SMALL_BTN_RADIUS)
#define SMALL_BTN_START_Y (WIN_HEIGHT - 2 * SMALL_BTN_RADIUS - 80)
#define SMALL_BTN_GAP_X   (2 * SMALL_BTN_RADIUS + 10)
#define SMALL_BTN_GAP_Y   (2 * SMALL_BTN_RADIUS + 10)

#define KEY_DOWN(key) ((GetAsyncKeyState(key) & 0x8000) ? 1 : 0)

// 核心游戏图片
IMAGE g_bgGame;
IMAGE g_playerImg;
IMAGE g_arrowImg;
IMAGE g_arrowImg2;
IMAGE g_smallImgs[3];
IMAGE g_mediumImgs[2];
IMAGE g_rageEffect;      // 狂暴.png
IMAGE g_redeemEffect;    // 救赎.png
IMAGE g_fireBallImg;     // 火球.png (中怪子弹)

int g_game_playerX, g_game_playerY, g_game_curRoad;
int g_game_playerHP;
int g_game_score;
enum GameState { GAME_IDLE, GAME_RUNNING, GAME_PAUSED, GAME_OVER };
GameState g_gameState = GAME_IDLE;

struct Arrow { float x, y; int w, h; int damage; };
std::vector<Arrow> g_arrows;
struct Ball {
    float x, y;
    int size;
    int damage;
    bool isFire;   // true=火球(中怪发射), false=普通子弹(小怪)
};
std::vector<Ball> g_balls;
struct Monster {
    float x, y; int w, h; int hp; int type; int imgIdx; int road; bool active; float shootTimer;
};
std::vector<Monster> g_monsters;

bool g_showRange = false;
float g_rangeEndTime = 0;
int g_targetHP = 0;
bool g_hasTarget = false;

RECT g_pauseBtn = { WIN_WIDTH / 2 - 50, WIN_HEIGHT - 60, WIN_WIDTH / 2 + 50, WIN_HEIGHT - 20 };
RECT g_restartBtn = { WIN_WIDTH / 2 - 150, WIN_HEIGHT - 60, WIN_WIDTH / 2 - 60, WIN_HEIGHT - 20 };
RECT g_endBtn = { WIN_WIDTH / 2 + 60, WIN_HEIGHT - 60, WIN_WIDTH / 2 + 150, WIN_HEIGHT - 20 };
RECT g_startBtn = { WIN_WIDTH / 2 - 80, WIN_HEIGHT / 2 - 25, WIN_WIDTH / 2 + 80, WIN_HEIGHT / 2 + 25 };

// 结算页面按钮
RECT g_backRoleBtn = { WIN_WIDTH / 2 - 180, WIN_HEIGHT - 80, WIN_WIDTH / 2 - 20, WIN_HEIGHT - 20 };
RECT g_backDiffBtn = { WIN_WIDTH / 2 + 20, WIN_HEIGHT - 80, WIN_WIDTH / 2 + 180, WIN_HEIGHT - 20 };

// 难度选择页面左上角返回按钮
RECT g_backToRoleBtn = { 20, 20, 70, 70 };  // 半径25的圆形

// ---------- 原有技能（键盘Q/E/R）----------
float g_skillLeftEffectTime = 0;     // 范围加成剩余时间
float g_skillLeftUpEffectTime = 0;   // 攻击力加成剩余时间
float g_skillUpEffectTime = 0;       // 大招沉默剩余时间
float g_skillLeftCooldown = 0;
float g_skillLeftUpCooldown = 0;
float g_skillUpCooldown = 0;
int g_rangeBonus = 0;
int g_attackBonus = 0;               // 临时攻击力加成（E技能）
bool g_ultLockMove = false;

int g_skillLeftX, g_skillLeftY;
int g_skillLeftUpX, g_skillLeftUpY;
int g_skillUpX, g_skillUpY;

// ---------- 新增小技能（键盘1/2/3/4）----------
float g_smallCooldown[4] = { 0,0,0,0 };
float g_rageEffectTime = 0;          // 狂暴剩余时间（秒）
float g_redeemEffectTime = 0;        // 救赎特效剩余时间
float g_invincibleTime = 0;          // 无敌剩余时间
bool g_reviveAvailable = false;      // 是否拥有复活被动
bool g_bottomYellow = false;          // 购买复活后底部变色

int g_smallBtnX[4], g_smallBtnY[4];

// 键盘防连发
bool lastQ = false, lastE = false, lastR = false;
bool lastDigit[4] = { false,false,false,false };

// ---------- 对局计时 ----------
float g_gameTimer = 0.0f;
bool g_timerRunning = false;

// ---------- 函数声明 ----------
void putimage_alpha(int x, int y, IMAGE* img);
void Game_SpawnWave();
void Game_Init();
void Game_Start();
void Game_Update(float delta);
void Game_HandleKeyboard();
void Game_HandleMouse(float currentTime);
void Game_Draw();
void InitIntroImages();
void InitButtons();
void InitSkillButtons();
void InitSmallButtons();
void DrawCurrentState();
void HandleIntroMouseClicks();

// ---------- 实现 ----------
void putimage_alpha(int x, int y, IMAGE* img) {
    int w = img->getwidth(), h = img->getheight();
    AlphaBlend(GetImageHDC(NULL), x, y, w, h, GetImageHDC(img), 0, 0, w, h, { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA });
}

void Game_SpawnWave() {
    Monster m;
    m.type = 1; m.w = ENEMY_W; m.h = ENEMY_H; m.hp = 200; m.road = 1;
    m.y = (float)(ROAD_Y[1] - ENEMY_H); m.x = (float)WIN_WIDTH;
    m.imgIdx = rand() % 2; m.active = true; m.shootTimer = 0;
    g_monsters.push_back(m);
    for (int i = 0; i < 3; i++) {
        Monster s;
        s.type = 0; s.w = ENEMY_W; s.h = ENEMY_H; s.hp = 100; s.road = 0;
        s.y = (float)(ROAD_Y[0] - ENEMY_H); s.x = (float)(WIN_WIDTH + i * 60);
        s.imgIdx = rand() % 3; s.active = true; s.shootTimer = 0;
        g_monsters.push_back(s);
    }
    for (int i = 0; i < 3; i++) {
        Monster s;
        s.type = 0; s.w = ENEMY_W; s.h = ENEMY_H; s.hp = 100; s.road = 2;
        s.y = (float)(ROAD_Y[2] - ENEMY_H); s.x = (float)(WIN_WIDTH + i * 60);
        s.imgIdx = rand() % 3; s.active = true; s.shootTimer = 0;
        g_monsters.push_back(s);
    }
}

void Game_Init() {
    loadimage(&g_bgGame, "05.jpg", WIN_WIDTH, WIN_HEIGHT);
    loadimage(&g_playerImg, "01.png", PLAYER_W, PLAYER_H);
    loadimage(&g_arrowImg, "箭矢.png", ARROW_W, ARROW_H);
    loadimage(&g_arrowImg2, "箭矢02.png", ARROW_W, ARROW_H);
    loadimage(&g_smallImgs[0], "怪小怪01.png", ENEMY_W, ENEMY_H);
    loadimage(&g_smallImgs[1], "怪小怪02.png", ENEMY_W, ENEMY_H);
    loadimage(&g_smallImgs[2], "怪小怪03.png", ENEMY_W, ENEMY_H);
    loadimage(&g_mediumImgs[0], "怪中怪01.png", ENEMY_W, ENEMY_H);
    loadimage(&g_mediumImgs[1], "怪中怪02.png", ENEMY_W, ENEMY_H);
    loadimage(&g_rageEffect, "狂暴.png", PLAYER_W, PLAYER_H);
    loadimage(&g_redeemEffect, "救赎.png", PLAYER_W, PLAYER_H);
    loadimage(&g_fireBallImg, "火球.png", BALL1_SISE, BALL1_SISE);

    g_monsters.clear();
    g_arrows.clear();
    g_balls.clear();
    g_game_score = 0;
    g_game_playerHP = 500;
    g_game_playerX = 0;
    g_game_curRoad = 0;
    g_game_playerY = ROAD_Y[0] - PLAYER_H;
    g_hasTarget = false;
    g_targetHP = 0;
    g_showRange = false;
    g_gameState = GAME_IDLE;

    // 重置所有技能
    g_skillLeftEffectTime = 0; g_skillLeftUpEffectTime = 0; g_skillUpEffectTime = 0;
    g_skillLeftCooldown = 0; g_skillLeftUpCooldown = 0; g_skillUpCooldown = 0;
    g_rangeBonus = 0; g_attackBonus = 0;
    g_ultLockMove = false;
    for (int i = 0; i < 4; i++) g_smallCooldown[i] = 0;
    g_rageEffectTime = 0;
    g_redeemEffectTime = 0;
    g_invincibleTime = 0;
    g_reviveAvailable = false;
    g_bottomYellow = false;

    g_gameTimer = 0.0f;
    g_timerRunning = false;
}

void Game_Start() {
    g_gameState = GAME_RUNNING;
    g_monsters.clear();
    g_arrows.clear();
    g_balls.clear();
    g_game_score = 0;
    g_game_playerHP = 500;
    g_game_playerX = 0;
    g_game_curRoad = 0;
    g_game_playerY = ROAD_Y[0] - PLAYER_H;
    g_hasTarget = false;
    g_targetHP = 0;
    g_showRange = false;
    // 重置技能
    g_skillLeftEffectTime = 0; g_skillLeftUpEffectTime = 0; g_skillUpEffectTime = 0;
    g_skillLeftCooldown = 0; g_skillLeftUpCooldown = 0; g_skillUpCooldown = 0;
    g_rangeBonus = 0; g_attackBonus = 0;
    g_ultLockMove = false;
    for (int i = 0; i < 4; i++) g_smallCooldown[i] = 0;
    g_rageEffectTime = 0;
    g_redeemEffectTime = 0;
    g_invincibleTime = 0;
    g_reviveAvailable = false;
    g_bottomYellow = false;

    // 重置计时器
    g_gameTimer = 0.0f;
    g_timerRunning = true;

    Game_SpawnWave();
    StopMusic("intro_music");
    PlayMusic("01.mp3", "game_music", true);
}

void Game_Update(float delta) {
    if (g_gameState != GAME_RUNNING) return;

    // 计时器更新
    if (g_timerRunning) {
        g_gameTimer += delta;
    }

    // 技能持续时间更新
    if (g_skillLeftEffectTime > 0) {
        g_skillLeftEffectTime -= delta;
        if (g_skillLeftEffectTime <= 0) { g_skillLeftEffectTime = 0; g_rangeBonus = 0; }
    }
    if (g_skillLeftUpEffectTime > 0) {
        g_skillLeftUpEffectTime -= delta;
        if (g_skillLeftUpEffectTime <= 0) { g_skillLeftUpEffectTime = 0; g_attackBonus = 0; }
    }
    if (g_skillUpEffectTime > 0) {
        g_skillUpEffectTime -= delta;
        if (g_skillUpEffectTime <= 0) { g_skillUpEffectTime = 0; g_ultLockMove = false; }
        else { g_ultLockMove = true; }
    }
    if (g_rageEffectTime > 0) {
        g_rageEffectTime -= delta;
    }
    if (g_redeemEffectTime > 0) {
        g_redeemEffectTime -= delta;
    }
    if (g_invincibleTime > 0) {
        g_invincibleTime -= delta;
        if (g_invincibleTime < 0) g_invincibleTime = 0;
    }

    // 冷却更新
    if (g_skillLeftCooldown > 0) g_skillLeftCooldown -= delta;
    if (g_skillLeftUpCooldown > 0) g_skillLeftUpCooldown -= delta;
    if (g_skillUpCooldown > 0) g_skillUpCooldown -= delta;
    for (int i = 0; i < 4; i++) if (g_smallCooldown[i] > 0) g_smallCooldown[i] -= delta;
    if (g_skillLeftCooldown < 0) g_skillLeftCooldown = 0;
    if (g_skillLeftUpCooldown < 0) g_skillLeftUpCooldown = 0;
    if (g_skillUpCooldown < 0) g_skillUpCooldown = 0;
    for (int i = 0; i < 4; i++) if (g_smallCooldown[i] < 0) g_smallCooldown[i] = 0;

    // 玩家移动
    if (!g_ultLockMove) {
        if (KEY_DOWN('A')) { g_game_playerX -= (int)(PLAYER_SPEED_X * delta); if (g_game_playerX < 0) g_game_playerX = 0; }
        if (KEY_DOWN('D')) { g_game_playerX += (int)(PLAYER_SPEED_X * delta); int maxX = WIN_WIDTH - PLAYER_W; if (g_game_playerX > maxX) g_game_playerX = maxX; }
    }
    static int lastW = 0, lastS = 0;
    if (KEY_DOWN('W') && !lastW && g_game_curRoad > 0) { g_game_curRoad--; g_game_playerY = ROAD_Y[g_game_curRoad] - PLAYER_H; }
    if (KEY_DOWN('S') && !lastS && g_game_curRoad < 2) { g_game_curRoad++; g_game_playerY = ROAD_Y[g_game_curRoad] - PLAYER_H; }
    lastW = KEY_DOWN('W'); lastS = KEY_DOWN('S');

    // 怪物移动
    float step = ENEMY_SPEED * delta;
    for (auto& mon : g_monsters) { if (!mon.active) continue; mon.x -= step; if (mon.x + mon.w < 0) mon.active = false; }
    g_monsters.erase(std::remove_if(g_monsters.begin(), g_monsters.end(), [](const Monster& m) { return !m.active; }), g_monsters.end());

    bool hasMedium = false;
    for (auto& mon : g_monsters) if (mon.type == 1) { hasMedium = true; break; }
    if (!hasMedium && g_gameState == GAME_RUNNING) Game_SpawnWave();

    // 怪物射击（大招沉默）
    for (auto& mon : g_monsters) {
        if (!mon.active) continue;
        bool silenced = false;
        if (g_skillUpEffectTime > 0 && mon.road == g_game_curRoad) {
            float px = g_game_playerX + PLAYER_W / 2.0f;
            float ex = mon.x + mon.w / 2.0f;
            if (fabs(px - ex) <= 300.0f) silenced = true;
        }
        if (!silenced) {
            float interval = (mon.type == 0) ? SMALL_SHOOT_INTERVAL : MEDIUM_SHOOT_INTERVAL;
            mon.shootTimer += delta;
            if (mon.shootTimer >= interval) {
                mon.shootTimer = 0;
                Ball b;
                b.size = BALL_SIZE;
                b.x = mon.x + mon.w / 2 - BALL_SIZE / 2;
                b.y = mon.y + mon.h / 2 - BALL_SIZE / 2;
                b.damage = (mon.type == 0) ? 20 : 30;
                b.isFire = (mon.type == 1);
                g_balls.push_back(b);
            }
        }
    }

    // 子弹移动和碰撞
    float ballStep = BALL_SPEED * delta;
    for (auto& ball : g_balls) ball.x -= ballStep;
    g_balls.erase(std::remove_if(g_balls.begin(), g_balls.end(), [](const Ball& b) { return b.x + BALL_SIZE<0 || b.x>WIN_WIDTH; }), g_balls.end());
    for (size_t i = 0; i < g_balls.size(); ) {
        Ball& b = g_balls[i];
        if (b.x < g_game_playerX + PLAYER_W && b.x + BALL_SIZE > g_game_playerX &&
            b.y < g_game_playerY + PLAYER_H && b.y + BALL_SIZE > g_game_playerY) {
            if (g_invincibleTime <= 0) {
                g_game_playerHP -= b.damage;
                PlayMusic("hit.mp3", "hit", false);
                if (g_game_playerHP <= 0) {
                    if (g_reviveAvailable) {
                        g_game_playerHP = 300;
                        g_reviveAvailable = false;
                        g_bottomYellow = false;
                        PlayMusic("revive.mp3", "revive", false);
                    }
                    else {
                        g_gameState = GAME_OVER;
                        g_timerRunning = false;
                        StopMusic("game_music");
                    }
                }
            }
            g_balls.erase(g_balls.begin() + i);
            continue;
        }
        ++i;
    }

    // 箭矢移动和碰撞
    float arrowStep = ARROW_SPEED * delta;
    for (auto& arr : g_arrows) arr.x += arrowStep;
    g_arrows.erase(std::remove_if(g_arrows.begin(), g_arrows.end(), [](const Arrow& a) { return a.x + a.w<0 || a.x>WIN_WIDTH; }), g_arrows.end());
    for (size_t i = 0; i < g_arrows.size(); ) {
        Arrow& arr = g_arrows[i];
        bool hit = false;
        for (auto& mon : g_monsters) {
            if (!mon.active) continue;
            if (arr.x < mon.x + mon.w && arr.x + ARROW_W > mon.x && arr.y < mon.y + mon.h && arr.y + ARROW_H > mon.y) {
                mon.hp -= arr.damage;
                hit = true;
                g_hasTarget = true;
                g_targetHP = mon.hp;
                if (mon.hp <= 0) {
                    mon.active = false;
                    if (mon.type == 0) g_game_score += 20;
                    else g_game_score += 50;
                    g_hasTarget = false;
                }
                break;
            }
        }
        if (hit) g_arrows.erase(g_arrows.begin() + i);
        else ++i;
    }
}

void Game_HandleKeyboard() {
    if (g_gameState != GAME_RUNNING) return;
    // Q键：范围技能
    bool qNow = KEY_DOWN('Q');
    if (qNow && !lastQ && g_skillLeftCooldown <= 0) {
        g_skillLeftEffectTime = 3.0f;
        g_rangeBonus = 100;
        g_skillLeftCooldown = 5.0f;
        PlayMusic("skill.mp3", "skill", false);
    }
    lastQ = qNow;

    // E键：攻击力技能
    bool eNow = KEY_DOWN('E');
    if (eNow && !lastE && g_skillLeftUpCooldown <= 0) {
        g_skillLeftUpEffectTime = 3.0f;
        g_attackBonus = 10;
        g_skillLeftUpCooldown = 5.0f;
        PlayMusic("skill.mp3", "skill", false);
    }
    lastE = eNow;

    // R键：大招
    bool rNow = KEY_DOWN('R');
    if (rNow && !lastR && g_skillUpCooldown <= 0) {
        g_skillUpEffectTime = 10.0f;
        g_ultLockMove = true;
        g_skillUpCooldown = 20.0f;
        PlayMusic("ultimate.mp3", "ultimate", false);
    }
    lastR = rNow;

    // 数字键1-4
    for (int i = 0; i < 4; i++) {
        int vk = '1' + i;
        bool now = KEY_DOWN(vk);
        if (now && !lastDigit[i]) {
            bool canBuy = false;
            int cost = 0;
            if (i == 0) { cost = 100; canBuy = (g_smallCooldown[i] <= 0 && g_game_score >= cost); }
            else if (i == 1) { cost = 100; canBuy = (g_smallCooldown[i] <= 0 && g_game_score >= cost); }
            else if (i == 2) { cost = 200; canBuy = (g_smallCooldown[i] <= 0 && g_game_score >= cost); }
            else if (i == 3) { cost = 500; canBuy = (!g_reviveAvailable && g_smallCooldown[i] <= 0 && g_game_score >= cost); }

            if (canBuy) {
                g_game_score -= cost;
                if (i == 0) {
                    g_rageEffectTime = 10.0f;
                    g_smallCooldown[i] = 30.0f;
                }
                else if (i == 1) {
                    g_game_playerHP += 100;
                    g_redeemEffectTime = 2.0f;
                    g_smallCooldown[i] = 30.0f;
                }
                else if (i == 2) {
                    g_invincibleTime = 5.0f;
                    g_smallCooldown[i] = 30.0f;
                }
                else if (i == 3) {
                    g_reviveAvailable = true;
                    g_bottomYellow = true;
                    g_smallCooldown[i] = 99999.0f;
                }
                PlayMusic("buy.mp3", "buy", false);
            }
        }
        lastDigit[i] = now;
    }
}enum AppState {
    STATE_INTRO1, STATE_INTRO2, STATE_INTRO3,
    STATE_ROLE_SELECT, STATE_CHARACTER_SHOW, STATE_DIFF_SELECT,
    STATE_GAME, STATE_GAME_OVER1, STATE_SCORE
};
AppState g_appState = STATE_INTRO1;
void Game_HandleMouse(float currentTime) {
    if (g_gameState == GAME_OVER) return;
    static bool lastLeft = false;
    if (MouseHit()) {
        MOUSEMSG msg = GetMouseMsg();
        if (msg.uMsg == WM_LBUTTONDOWN && !lastLeft) {
            // 开始按钮
            if (g_gameState == GAME_IDLE &&
                msg.x >= g_startBtn.left && msg.x <= g_startBtn.right &&
                msg.y >= g_startBtn.top && msg.y <= g_startBtn.bottom) {
                Game_Start();
                return;
            }
            // 重新挑战按钮
            if ((g_gameState == GAME_RUNNING || g_gameState == GAME_PAUSED) &&
                msg.x >= g_restartBtn.left && msg.x <= g_restartBtn.right &&
                msg.y >= g_restartBtn.top && msg.y <= g_restartBtn.bottom) {
                StopMusic("game_music");
                Game_Start();
                return;
            }
            // 结束游戏按钮 - 返回难度选择
            if ((g_gameState == GAME_RUNNING || g_gameState == GAME_PAUSED) &&
                msg.x >= g_endBtn.left && msg.x <= g_endBtn.right &&
                msg.y >= g_endBtn.top && msg.y <= g_endBtn.bottom) {
                StopMusic("game_music");
                // 清空游戏数据
                g_monsters.clear();
                g_arrows.clear();
                g_balls.clear();
                g_gameState = GAME_IDLE;           // 重置游戏状态
                g_appState = STATE_DIFF_SELECT;    // 跳转到难度选择界面
                PlayMusic("05.mp3", "intro_music", false);
                return;
            }
            // 暂停/继续按钮
            if ((g_gameState == GAME_RUNNING || g_gameState == GAME_PAUSED) &&
                msg.x >= g_pauseBtn.left && msg.x <= g_pauseBtn.right &&
                msg.y >= g_pauseBtn.top && msg.y <= g_pauseBtn.bottom) {
                if (g_gameState == GAME_RUNNING) {
                    g_gameState = GAME_PAUSED;
                    g_timerRunning = false;
                    PauseMusic("game_music");
                }
                else {
                    g_gameState = GAME_RUNNING;
                    g_timerRunning = true;
                    ResumeMusic("game_music");
                }
                return;
            }
            // 攻击按钮
            if (g_gameState == GAME_RUNNING) {
                int dx = msg.x - ATK_BTN_X, dy = msg.y - ATK_BTN_Y;
                if (dx * dx + dy * dy <= ATK_BTN_RADIUS * ATK_BTN_RADIUS) {
                    g_showRange = true;
                    g_rangeEndTime = currentTime + 0.2f;
                    float px = g_game_playerX + PLAYER_W / 2.0f, py = g_game_playerY + PLAYER_H / 2.0f;
                    float minDist = ATTACK_RANGE + g_rangeBonus + 1;
                    for (auto& mon : g_monsters) {
                        float ex = mon.x + mon.w / 2.0f, ey = mon.y + mon.h / 2.0f;
                        float d = sqrtf((px - ex) * (px - ex) + (py - ey) * (py - ey));
                        if (d < minDist) minDist = d;
                    }
                    if (minDist <= ATTACK_RANGE + g_rangeBonus) {
                        Arrow arr;
                        arr.w = ARROW_W; arr.h = ARROW_H;
                        arr.x = g_game_playerX + PLAYER_W;
                        arr.y = g_game_playerY + PLAYER_H / 2 - ARROW_H / 2;
                        int extra = g_attackBonus;
                        if (g_rageEffectTime > 0) extra += 20;
                        arr.damage = 20 + extra;
                        g_arrows.push_back(arr);
                        PlayMusic("02.mp3", "attack", false);
                    }
                }
            }
        }
        lastLeft = (msg.uMsg == WM_LBUTTONDOWN);
    }
    else {
        lastLeft = false;
    }
}

void DrawSkillButton(int x, int y, float cooldown, const char* text) {
    COLORREF fillColor = (cooldown > 0) ? RGB(100, 100, 100) : RGB(0, 200, 200);
    setfillcolor(fillColor);
    setlinecolor(WHITE);
    setlinestyle(PS_SOLID, 2);
    fillcircle(x, y, SKILL_RADIUS);
    setbkmode(TRANSPARENT);
    settextcolor(WHITE);
    settextstyle(14, 0, "微软雅黑");
    if (cooldown > 0) {
        char cdText[8];
        sprintf_s(cdText, "%.0f", ceil(cooldown));
        outtextxy(x - 8, y - 8, cdText);
    }
    else {
        outtextxy(x - 10, y - 7, text);
    }
}

void DrawSmallButton(int idx, int x, int y, float cooldown) {
    COLORREF fillColor = (cooldown > 0) ? RGB(80, 80, 80) : RGB(100, 200, 100);
    setfillcolor(fillColor);
    setlinecolor(WHITE);
    setlinestyle(PS_SOLID, 2);
    fillcircle(x, y, SMALL_BTN_RADIUS);
    setbkmode(TRANSPARENT);
    settextcolor(WHITE);
    settextstyle(12, 0, "微软雅黑");
    char text[16];
    if (cooldown > 0 && cooldown < 1000) {
        sprintf_s(text, "%d\n%.0f", idx + 1, ceil(cooldown));
        outtextxy(x - 12, y - 12, text);
    }
    else if (cooldown >= 99999) {
        outtextxy(x - 8, y - 8, "∞");
    }
    else {
        sprintf_s(text, "%d", idx + 1);
        outtextxy(x - 6, y - 8, text);
        int cost = (idx == 0 || idx == 1) ? 100 : ((idx == 2) ? 200 : 500);
        char costText[16];
        sprintf_s(costText, "%d", cost);
        settextstyle(10, 0, "微软雅黑");
        outtextxy(x - 10, y + 8, costText);
    }
}

void Game_Draw() {
    putimage(0, 0, &g_bgGame);
    // 玩家绘制（无敌闪烁）
    if (g_invincibleTime > 0 && ((int)(GetTickCount() / 100) % 2 == 0)) {
        // 闪烁跳过
    }
    else {
        putimage_alpha(g_game_playerX, g_game_playerY, &g_playerImg);
    }
    if (g_rageEffectTime > 0) putimage_alpha(g_game_playerX, g_game_playerY, &g_rageEffect);
    if (g_redeemEffectTime > 0) putimage_alpha(g_game_playerX, g_game_playerY, &g_redeemEffect);

    setlinecolor(g_bottomYellow ? RGB(255, 255, 0) : RGB(255, 0, 0));
    setlinestyle(PS_SOLID, 2);
    line(g_game_playerX, g_game_playerY + PLAYER_H, g_game_playerX + PLAYER_W, g_game_playerY + PLAYER_H);

    if (g_gameState != GAME_IDLE) {
        for (auto& mon : g_monsters) {
            if (!mon.active) continue;
            IMAGE* img = (mon.type == 0) ? &g_smallImgs[mon.imgIdx] : &g_mediumImgs[mon.imgIdx];
            putimage_alpha((int)mon.x, (int)mon.y, img);
        }
    }
    for (auto& arr : g_arrows) {
        bool hasAnyBonus = (g_attackBonus > 0 || g_rageEffectTime > 0);
        IMAGE* curArrow = hasAnyBonus ? &g_arrowImg2 : &g_arrowImg;
        putimage_alpha((int)arr.x, (int)arr.y, curArrow);
    }

    for (auto& ball : g_balls) {
        if (ball.isFire) {
            putimage_alpha((int)ball.x, (int)ball.y, &g_fireBallImg);
        }
        else {
            setfillcolor(RGB(180, 0, 180));
            solidcircle((int)(ball.x + BALL_SIZE / 2), (int)(ball.y + BALL_SIZE / 2), BALL_SIZE / 2);
        }
    }

    if (g_showRange) {
        int bx = g_game_playerX + PLAYER_W + (int)(ATTACK_RANGE + g_rangeBonus);
        int cy = g_game_playerY + PLAYER_H / 2;
        setlinecolor(RGB(0, 100, 255)); setlinestyle(PS_DASH, 1);
        line(bx, cy - 5, bx, cy + 5);
        setlinestyle(PS_SOLID, 1);
    }
    // 攻击按钮
    setfillcolor(RGB(200, 80, 80)); setlinecolor(RGB(255, 255, 255)); setlinestyle(PS_SOLID, 2);
    fillcircle(ATK_BTN_X, ATK_BTN_Y, ATK_BTN_RADIUS);
    setbkmode(TRANSPARENT); settextcolor(WHITE); settextstyle(20, 0, "微软雅黑");
    outtextxy(ATK_BTN_X - 20, ATK_BTN_Y - 10, "攻击");
    // 原有技能按钮
    DrawSkillButton(g_skillLeftX, g_skillLeftY, g_skillLeftCooldown, "范围");
    DrawSkillButton(g_skillLeftUpX, g_skillLeftUpY, g_skillLeftUpCooldown, "攻击");
    DrawSkillButton(g_skillUpX, g_skillUpY, g_skillUpCooldown, "大招");
    // 左边小按钮
    for (int i = 0; i < 4; i++) {
        DrawSmallButton(i, g_smallBtnX[i], g_smallBtnY[i], g_smallCooldown[i]);
    }

    // 重新挑战
    setfillcolor(RGB(255, 165, 0));
    setlinecolor(BLACK);
    fillrectangle(g_restartBtn.left, g_restartBtn.top, g_restartBtn.right, g_restartBtn.bottom);
    setbkmode(TRANSPARENT);
    settextcolor(BLACK);
    settextstyle(18, 0, "微软雅黑");
    const char* restartText = "重新挑战";
    int tw_restart = textwidth(restartText), th_restart = textheight(restartText);
    outtextxy(g_restartBtn.left + (g_restartBtn.right - g_restartBtn.left - tw_restart) / 2,
        g_restartBtn.top + (g_restartBtn.bottom - g_restartBtn.top - th_restart) / 2,
        restartText);

    // 结束游戏
    setfillcolor(RGB(200, 50, 50));
    setlinecolor(BLACK);
    fillrectangle(g_endBtn.left, g_endBtn.top, g_endBtn.right, g_endBtn.bottom);
    setbkmode(TRANSPARENT);
    settextcolor(WHITE);
    settextstyle(18, 0, "微软雅黑");
    const char* endText = "结束游戏";
    int tw_end = textwidth(endText), th_end = textheight(endText);
    outtextxy(g_endBtn.left + (g_endBtn.right - g_endBtn.left - tw_end) / 2,
        g_endBtn.top + (g_endBtn.bottom - g_endBtn.top - th_end) / 2,
        endText);

    // 暂停按钮
    if (g_gameState == GAME_RUNNING || g_gameState == GAME_PAUSED) {
        setfillcolor(RGB(200, 200, 0)); setlinecolor(BLACK);
        fillrectangle(g_pauseBtn.left, g_pauseBtn.top, g_pauseBtn.right, g_pauseBtn.bottom);
        setbkmode(TRANSPARENT); settextcolor(BLACK); settextstyle(20, 0, "微软雅黑");
        const char* pauseText = (g_gameState == GAME_RUNNING) ? "暂停" : "继续";
        int tw2 = textwidth(pauseText), th2 = textheight(pauseText);
        outtextxy(g_pauseBtn.left + (g_pauseBtn.right - g_pauseBtn.left - tw2) / 2,
            g_pauseBtn.top + (g_pauseBtn.bottom - g_pauseBtn.top - th2) / 2, pauseText);
    }

    // UI文字
    settextcolor(BLACK);
    settextstyle(20, 0, "微软雅黑");
    char buf[64];
    sprintf_s(buf, "分数: %d", g_game_score);
    outtextxy(10, 10, buf);
    sprintf_s(buf, "血量: %d", g_game_playerHP);
    outtextxy(WIN_WIDTH - 280, 10, buf);
    if (g_hasTarget) {
        sprintf_s(buf, "目标血量: %d", g_targetHP);
        outtextxy(WIN_WIDTH - 280, 50, buf);
    }

    int minutes = (int)(g_gameTimer / 60);
    int seconds = (int)(g_gameTimer) % 60;
    char timeBuf[32];
    sprintf_s(timeBuf, "对局时长: %02d:%02d", minutes, seconds);
    int tw = textwidth(timeBuf);
    outtextxy((WIN_WIDTH - tw) / 2, 10, timeBuf);

    if (g_gameState == GAME_IDLE) {
        setfillcolor(RGB(100, 200, 100)); setlinecolor(BLACK);
        fillrectangle(g_startBtn.left, g_startBtn.top, g_startBtn.right, g_startBtn.bottom);
        setbkmode(TRANSPARENT); settextcolor(BLACK); settextstyle(25, 0, "微软雅黑");
        outtextxy(g_startBtn.left + 20, g_startBtn.top + 10, "开始游戏");
    }
}

// ------------------- 开场及流程 -------------------
IMAGE g_intro1, g_intro2, g_intro3, g_bgRole;
IMAGE g_gameOver1, g_gameOver2;
IMAGE g_characterImg;

struct Button {
    RECT rect;
    const char* text;
};
std::vector<Button> g_roleButtons;
std::vector<Button> g_diffButtons;



DWORD g_stateStartTime = 0;
int g_finalScore;
float g_finalGameTime = 0;

void InitIntroImages() {
    loadimage(&g_intro1, "01.jpg", WIN_WIDTH, WIN_HEIGHT);
    loadimage(&g_intro2, "02.jpg", WIN_WIDTH, WIN_HEIGHT);
    loadimage(&g_intro3, "03.jpg", WIN_WIDTH, WIN_HEIGHT);
    loadimage(&g_bgRole, "04.jpg", WIN_WIDTH, WIN_HEIGHT);
    loadimage(&g_gameOver1, "06.jpg", WIN_WIDTH, WIN_HEIGHT);
    loadimage(&g_gameOver2, "07.jpg", WIN_WIDTH, WIN_HEIGHT);
    loadimage(&g_characterImg, "伽罗.jpg", WIN_WIDTH, WIN_HEIGHT);
}
void InitButtons() {
    int btnW = 200, btnH = 50, gap = 20, startY = 150;
    for (int i = 0; i < 5; i++) {
        Button btn;
        btn.rect.left = WIN_WIDTH / 2 - btnW / 2;
        btn.rect.right = WIN_WIDTH / 2 + btnW / 2;
        btn.rect.top = startY + i * (btnH + gap);
        btn.rect.bottom = btn.rect.top + btnH;
        const char* names[] = { "对抗路", "打野", "中路", "发育路", "辅助" };
        btn.text = names[i];
        g_roleButtons.push_back(btn);
    }
    int diffStartY = 200;
    for (int i = 0; i < 3; i++) {
        Button btn;
        btn.rect.left = WIN_WIDTH / 2 - 80;
        btn.rect.right = WIN_WIDTH / 2 + 80;
        btn.rect.top = diffStartY + i * 70;
        btn.rect.bottom = btn.rect.top + 50;
        const char* names[] = { "简单", "中等", "困难" };
        btn.text = names[i];
        g_diffButtons.push_back(btn);
    }
}
void InitSkillButtons() {
    g_skillLeftX = ATK_BTN_X + SKILL_LEFT_OFFSET_X;
    g_skillLeftY = ATK_BTN_Y + SKILL_LEFT_OFFSET_Y;
    g_skillLeftUpX = ATK_BTN_X + SKILL_LEFTUP_OFFSET_X;
    g_skillLeftUpY = ATK_BTN_Y + SKILL_LEFTUP_OFFSET_Y;
    g_skillUpX = ATK_BTN_X + SKILL_UP_OFFSET_X;
    g_skillUpY = ATK_BTN_Y + SKILL_UP_OFFSET_Y;
}
void InitSmallButtons() {
    int baseX = SMALL_BTN_START_X;
    int baseY = SMALL_BTN_START_Y;
    g_smallBtnX[0] = baseX;
    g_smallBtnY[0] = baseY;
    g_smallBtnX[1] = baseX + SMALL_BTN_GAP_X;
    g_smallBtnY[1] = baseY;
    g_smallBtnX[2] = baseX;
    g_smallBtnY[2] = baseY + SMALL_BTN_GAP_Y;
    g_smallBtnX[3] = baseX + SMALL_BTN_GAP_X;
    g_smallBtnY[3] = baseY + SMALL_BTN_GAP_Y;
}
void DrawCurrentState() {
    switch (g_appState) {
    case STATE_INTRO1: putimage(0, 0, &g_intro1); break;
    case STATE_INTRO2: putimage(0, 0, &g_intro2); break;
    case STATE_INTRO3: putimage(0, 0, &g_intro3); break;
    case STATE_ROLE_SELECT:
    case STATE_DIFF_SELECT:
        putimage(0, 0, &g_bgRole);
        break;
    case STATE_CHARACTER_SHOW:
        putimage(0, 0, &g_characterImg);
        break;
    case STATE_GAME:
        Game_Draw();
        return;
    case STATE_GAME_OVER1:
        putimage(0, 0, &g_gameOver1);
        break;
    case STATE_SCORE:
        putimage(0, 0, &g_gameOver2);
        setbkmode(TRANSPARENT);
        settextcolor(RGB(255, 215, 0));
        settextstyle(28, 0, "微软雅黑");
        char scoreText[128];
        sprintf_s(scoreText, "玩家本局游戏得分为：%d", g_finalScore);
        outtextxy(150, 250, scoreText);
        int minutes = (int)(g_finalGameTime / 60);
        int seconds = (int)(g_finalGameTime) % 60;
        char timeText[128];
        sprintf_s(timeText, "本局对局时长为：%d分%02d秒", minutes, seconds);
        outtextxy(150, 280, timeText);
        const char* title;
        if (g_finalScore <= 100) title = "菜鸟";
        else if (g_finalScore <= 200) title = "NPC";
        else if (g_finalScore <= 400) title = "铜牌发育路";
        else if (g_finalScore <= 600) title = "银牌发育路";
        else if (g_finalScore <= 800) title = "金牌发育路";
        else title = "顶级发育路";
        char titleText[128];
        sprintf_s(titleText, "恭喜获得称号：%s", title);
        outtextxy(150, 320, titleText);

        // 结算页面按钮
        setfillcolor(RGB(100, 150, 200));
        setlinecolor(BLACK);
        fillrectangle(g_backRoleBtn.left, g_backRoleBtn.top, g_backRoleBtn.right, g_backRoleBtn.bottom);
        setbkmode(TRANSPARENT);
        settextcolor(WHITE);
        settextstyle(20, 0, "微软雅黑");
        const char* roleText = "返回分路";
        int twr = textwidth(roleText), thr = textheight(roleText);
        outtextxy(g_backRoleBtn.left + (g_backRoleBtn.right - g_backRoleBtn.left - twr) / 2,
            g_backRoleBtn.top + (g_backRoleBtn.bottom - g_backRoleBtn.top - thr) / 2, roleText);

        setfillcolor(RGB(100, 200, 100));
        fillrectangle(g_backDiffBtn.left, g_backDiffBtn.top, g_backDiffBtn.right, g_backDiffBtn.bottom);
        const char* diffText = "返回难度";
        int twd = textwidth(diffText), thd = textheight(diffText);
        outtextxy(g_backDiffBtn.left + (g_backDiffBtn.right - g_backDiffBtn.left - twd) / 2,
            g_backDiffBtn.top + (g_backDiffBtn.bottom - g_backDiffBtn.top - thd) / 2, diffText);
        break;
    }
    // 绘制难度选择页面左上角返回按钮
    if (g_appState == STATE_DIFF_SELECT) {
        setfillcolor(RGB(200, 200, 200));
        setlinecolor(BLACK);
        setlinestyle(PS_SOLID, 2);
        fillcircle(g_backToRoleBtn.left + 25, g_backToRoleBtn.top + 25, 25);
        setbkmode(TRANSPARENT);
        settextcolor(BLACK);
        settextstyle(20, 0, "微软雅黑");
        outtextxy(g_backToRoleBtn.left + 18, g_backToRoleBtn.top + 18, "←");
    }

    if (g_appState == STATE_ROLE_SELECT) {
        for (auto& btn : g_roleButtons) {
            setfillcolor(RGB(0, 160, 200)); setlinecolor(BLACK); setlinestyle(PS_SOLID, 2);
            fillrectangle(btn.rect.left, btn.rect.top, btn.rect.right, btn.rect.bottom);
            setbkmode(TRANSPARENT); settextcolor(WHITE); settextstyle(25, 0, "微软雅黑");
            int tw = textwidth(btn.text), th = textheight(btn.text);
            outtextxy(btn.rect.left + (btn.rect.right - btn.rect.left - tw) / 2,
                btn.rect.top + (btn.rect.bottom - btn.rect.top - th) / 2, btn.text);
        }
    }
    else if (g_appState == STATE_DIFF_SELECT) {
        for (auto& btn : g_diffButtons) {
            setfillcolor(RGB(200, 150, 0)); setlinecolor(BLACK); setlinestyle(PS_SOLID, 2);
            fillrectangle(btn.rect.left, btn.rect.top, btn.rect.right, btn.rect.bottom);
            setbkmode(TRANSPARENT); settextcolor(WHITE); settextstyle(25, 0, "微软雅黑");
            int tw = textwidth(btn.text), th = textheight(btn.text);
            outtextxy(btn.rect.left + (btn.rect.right - btn.rect.left - tw) / 2,
                btn.rect.top + (btn.rect.bottom - btn.rect.top - th) / 2, btn.text);
        }
    }
}
void HandleIntroMouseClicks() {
    static bool lastLeft = false;
    if (MouseHit()) {
        MOUSEMSG msg = GetMouseMsg();
        if (msg.uMsg == WM_LBUTTONDOWN && !lastLeft) {
            // 难度选择页面返回按钮
            if (g_appState == STATE_DIFF_SELECT) {
                int dx = msg.x - (g_backToRoleBtn.left + 25);
                int dy = msg.y - (g_backToRoleBtn.top + 25);
                if (dx * dx + dy * dy <= 25 * 25) {
                    g_appState = STATE_ROLE_SELECT;
                    return;
                }
            }
            // 结算页面按钮
            if (g_appState == STATE_SCORE) {
                if (msg.x >= g_backRoleBtn.left && msg.x <= g_backRoleBtn.right &&
                    msg.y >= g_backRoleBtn.top && msg.y <= g_backRoleBtn.bottom) {
                    g_appState = STATE_ROLE_SELECT;
                    return;
                }
                if (msg.x >= g_backDiffBtn.left && msg.x <= g_backDiffBtn.right &&
                    msg.y >= g_backDiffBtn.top && msg.y <= g_backDiffBtn.bottom) {
                    g_appState = STATE_DIFF_SELECT;
                    return;
                }
                return;
            }

            if (g_appState == STATE_INTRO3) {
                g_appState = STATE_ROLE_SELECT;
                g_stateStartTime = GetTickCount();
            }
            else if (g_appState == STATE_ROLE_SELECT) {
                for (size_t i = 0; i < g_roleButtons.size(); i++) {
                    if (msg.x >= g_roleButtons[i].rect.left && msg.x <= g_roleButtons[i].rect.right &&
                        msg.y >= g_roleButtons[i].rect.top && msg.y <= g_roleButtons[i].rect.bottom) {
                        if (i == 3) {
                            g_appState = STATE_CHARACTER_SHOW;
                            g_stateStartTime = GetTickCount();
                        }
                        break;
                    }
                }
            }
            else if (g_appState == STATE_DIFF_SELECT) {
                for (size_t i = 0; i < g_diffButtons.size(); i++) {
                    if (msg.x >= g_diffButtons[i].rect.left && msg.x <= g_diffButtons[i].rect.right &&
                        msg.y >= g_diffButtons[i].rect.top && msg.y <= g_diffButtons[i].rect.bottom) {
                        if (i == 0) {
                            g_appState = STATE_GAME;
                            Game_Init();
                            StopMusic("intro_music");
                        }
                        break;
                    }
                }
            }
            else if (g_appState == STATE_GAME_OVER1) {
                g_appState = STATE_SCORE;
            }
        }
        lastLeft = (msg.uMsg == WM_LBUTTONDOWN);
    }
    else {
        lastLeft = false;
    }
}

// ------------------- main -------------------
int main() {
    srand((unsigned)time(NULL));
    initgraph(WIN_WIDTH, WIN_HEIGHT);
    InitIntroImages();
    InitButtons();
    InitSkillButtons();
    InitSmallButtons();

    PlayMusic("03.mp3", "intro_music", false);
    g_appState = STATE_INTRO1;
    g_stateStartTime = GetTickCount();

    float lastFrameTime = (float)GetTickCount() / 1000.0f;
    bool running = true;
    while (running) {
        float now = (float)GetTickCount() / 1000.0f;
        float delta = now - lastFrameTime;
        if (delta > 0.05f) delta = 0.05f;
        lastFrameTime = now;

        // 自动切换开场和角色展示
        if (g_appState == STATE_INTRO1 && GetTickCount() - g_stateStartTime >= 5000) {
            g_appState = STATE_INTRO2;
            g_stateStartTime = GetTickCount();
            PlayMusic("04.mp3", "intro_music", false);
        }
        else if (g_appState == STATE_INTRO2 && GetTickCount() - g_stateStartTime >= 5000) {
            g_appState = STATE_INTRO3;
            g_stateStartTime = GetTickCount();
            PlayMusic("05.mp3", "intro_music", false);
        }
        else if (g_appState == STATE_CHARACTER_SHOW && GetTickCount() - g_stateStartTime >= 2000) {
            g_appState = STATE_DIFF_SELECT;
            g_stateStartTime = GetTickCount();
        }

        if (g_appState == STATE_GAME) {
            Game_Update(delta);
            Game_HandleKeyboard();
            Game_HandleMouse(now);
            if (g_showRange && now >= g_rangeEndTime) g_showRange = false;
            if (g_gameState == GAME_OVER) {
                g_finalScore = g_game_score;
                g_finalGameTime = g_gameTimer;
                StopMusic("game_music");
                g_appState = STATE_GAME_OVER1;
                g_monsters.clear(); g_arrows.clear(); g_balls.clear();
            }
            BeginBatchDraw();
            Game_Draw();
            EndBatchDraw();
        }
        else {
            HandleIntroMouseClicks();
            BeginBatchDraw();
            DrawCurrentState();
            EndBatchDraw();
        }

        if (KEY_DOWN(VK_ESCAPE)) running = false;
        Sleep(1);
    }

    StopMusic("intro_music");
    StopMusic("game_music");
    StopMusic("attack");
    StopMusic("hit");
    StopMusic("skill");
    StopMusic("ultimate");
    StopMusic("buy");
    StopMusic("revive");
    closegraph();
    return 0;
}
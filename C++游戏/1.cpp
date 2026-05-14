#include <graphics.h>
#include <windows.h>
#include <vector>
#include <algorithm>
#include <cmath>
#include <time.h>
#include <mmsystem.h>

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "MSIMG32.LIB")

#define WIN_WIDTH   1520
#define WIN_HEIGHT  853

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
bool g_welcomePlayed = false;
// ------------------- 核心游戏常量 -------------------
#define PLAYER_W    95
#define PLAYER_H    95
#define ARROW_W     57
#define ARROW_H     14
#define ENEMY_W     95
#define ENEMY_H     95
#define BOSS_W      180
#define BOSS_H      280
#define BOSS_SMALL_W 95
#define BOSS_SMALL_H 95
#define BALL_SIZE   25
const int ROAD_Y[3] = { 427, 503, 587 };
#define PLAYER_SPEED_X   712.5f
#define ARROW_SPEED      380.0f
#define BALL_SPEED       190.0f
#define ATTACK_RANGE     760.0f

#define ATK_BTN_RADIUS   90
#define ATK_BTN_X        (WIN_WIDTH - ATK_BTN_RADIUS - 38)
#define ATK_BTN_Y        (WIN_HEIGHT - ATK_BTN_RADIUS)

#define SKILL_RADIUS     50
#define SKILL_LEFT_OFFSET_X   -120
#define SKILL_LEFT_OFFSET_Y   0
#define SKILL_LEFTUP_OFFSET_X -90
#define SKILL_LEFTUP_OFFSET_Y -90
#define SKILL_UP_OFFSET_X     0
#define SKILL_UP_OFFSET_Y     -120

#define SMALL_BTN_RADIUS 47
#define SMALL_BTN_START_X (38 + SMALL_BTN_RADIUS)
#define SMALL_BTN_START_Y (WIN_HEIGHT - 2 * SMALL_BTN_RADIUS - 112)
#define SMALL_BTN_GAP_X   (2 * SMALL_BTN_RADIUS + 19)
#define SMALL_BTN_GAP_Y   (2 * SMALL_BTN_RADIUS + 19)

#define KEY_DOWN(key) ((GetAsyncKeyState(key) & 0x8000) ? 1 : 0)

// ------------------- 图片对象 -------------------
IMAGE g_bgGame;
IMAGE g_playerImg;
IMAGE g_playerImgLeft;
IMAGE g_arrowImg;
IMAGE g_arrowImg2;
IMAGE g_arrowImgLeft;
IMAGE g_arrowImg2Left;
IMAGE g_smallImgs[4];
IMAGE g_mediumImgs[4];
IMAGE g_bigMonsterImgs[4];
IMAGE g_rageEffect;
IMAGE g_redeemEffect;
IMAGE g_fireBallImg;
IMAGE g_planetImg;   // 星球.png
// ------------------- 普通模式全局变量 -------------------
float g_game_playerX;
int g_game_playerY, g_game_curRoad;
int g_game_playerHP;
int g_game_score;
enum GameState { GAME_IDLE, GAME_RUNNING, GAME_PAUSED, GAME_OVER };
GameState g_gameState = GAME_IDLE;

struct Arrow { float x, y; int w, h; int damage; float dir; };
std::vector<Arrow> g_arrows;
struct Ball { float x, y; int size; int damage; bool isFire; };
std::vector<Ball> g_balls;
struct Monster {
    float x, y; int w, h; int hp; int type; int imgIdx; int road; bool active; float shootTimer;
};
std::vector<Monster> g_monsters;

bool g_showRange = false;
float g_rangeEndTime = 0;
int g_targetHP = 0;
bool g_hasTarget = false;

RECT g_pauseBtn = { WIN_WIDTH / 2 - 95, WIN_HEIGHT - 114, WIN_WIDTH / 2 + 95, WIN_HEIGHT - 38 };
RECT g_restartBtn = { WIN_WIDTH / 2 - 285, WIN_HEIGHT - 114, WIN_WIDTH / 2 - 114, WIN_HEIGHT - 38 };
RECT g_endBtn = { WIN_WIDTH / 2 + 114, WIN_HEIGHT - 114, WIN_WIDTH / 2 + 285, WIN_HEIGHT - 38 };
RECT g_startBtn = { WIN_WIDTH / 2 - 152, WIN_HEIGHT / 2 - 47, WIN_WIDTH / 2 + 152, WIN_HEIGHT / 2 + 47 };

float g_skillLeftEffectTime = 0;
float g_skillLeftUpEffectTime = 0;
float g_skillUpEffectTime = 0;
float g_skillLeftCooldown = 0;
float g_skillLeftUpCooldown = 0;
float g_skillUpCooldown = 0;
int g_rangeBonus = 0;
int g_attackBonus = 0;
bool g_ultLockMove = false;

int g_skillLeftX, g_skillLeftY;
int g_skillLeftUpX, g_skillLeftUpY;
int g_skillUpX, g_skillUpY;

float g_smallCooldown[4] = { 0,0,0,0 };
float g_rageEffectTime = 0;
float g_redeemEffectTime = 0;
float g_invincibleTime = 0;
bool g_reviveAvailable = false;
bool g_bottomYellow = false;
int g_smallBtnX[4], g_smallBtnY[4];

bool lastQ = false, lastE = false, lastR = false;
bool lastDigit[4] = { false,false,false,false };
float g_gameTimer = 0.0f;
bool g_timerRunning = false;
int g_difficulty = 0;

// ------------------- 困难模式全局变量 -------------------
#define TOTAL_STAGES 4
IMAGE g_bgMap[4];
IMAGE g_bossImgs[3];
IMAGE g_chestImg, g_chestOpenImg;
IMAGE g_rewardImgs[3][2];
IMAGE g_gameOverImg, g_gameWinImg;
IMAGE g_victoryBg, g_defeatBg;
bool g_showReward = false;
int g_rewardStage, g_rewardIdx;

IMAGE g_rewardBgImgsFull[6];
IMAGE g_rewardSmallImgs[6];
int g_obtainedRewards[3];
int g_obtainedRewardCount;
bool g_rewardSkillAvailable[3];
RECT g_rewardSkillBtns[3];

struct HardMonster {
    int type; int road; int hp, maxHp; int imgIdx; float x, y; float shootTimer; bool active;
};
std::vector<HardMonster> g_hardMonsters;
int g_hardMapMonsterCount = 0;
bool g_mapCleared = false;
bool g_chestClicked = false;
bool g_bossDefeated = false;

struct RewardButton {
    int stage, rewardIdx; IMAGE* img; bool active; RECT rect;
};
std::vector<RewardButton> g_rewardButtons;
int g_rewardIndex = 0;

enum HardState {
    HARD_IDLE, HARD_PLAYING, HARD_VICTORY, HARD_DEFEAT, HARD_WAIT_NEXT, HARD_WAIT_RETRY
};
HardState g_hardState = HARD_IDLE;
int g_currentStage = 1, g_currentMap = 0;
int g_hardScore = 1000;
int g_timeLeft = 15 * 60;
float g_lastSecond = 0;
bool g_chestAnimation = false;
float g_chestAnimTimer = 0.0f;
float g_chestScale = 1.0f;

bool g_hardPaused = false;
bool g_showTalentIntro = false;
int g_talentCooldown = 0;
IMAGE g_talentSmall, g_talentIntro;
bool g_talentLoaded = false, g_talentIntroLoaded = false;

RECT g_talentBtn;
RECT g_pauseHardBtn = { WIN_WIDTH / 2 - 285, WIN_HEIGHT - 114, WIN_WIDTH / 2 - 95, WIN_HEIGHT - 38 };
RECT g_exitHardBtn = { WIN_WIDTH / 2 + 95, WIN_HEIGHT - 114, WIN_WIDTH / 2 + 285, WIN_HEIGHT - 38 };
RECT g_startHardBtn = { WIN_WIDTH / 2 - 190, WIN_HEIGHT / 2 - 47, WIN_WIDTH / 2 + 190, WIN_HEIGHT / 2 + 47 };

RECT g_retryBtn = { WIN_WIDTH / 2 - 190, WIN_HEIGHT / 2 + 95, WIN_WIDTH / 2 - 38, WIN_HEIGHT / 2 + 190 };
RECT g_giveUpBtn = { WIN_WIDTH / 2 + 38, WIN_HEIGHT / 2 + 95, WIN_WIDTH / 2 + 190, WIN_HEIGHT / 2 + 190 };
IMAGE g_doorImg;
bool g_doorActive = false;

float g_reward2EffectTime = 0;
bool g_reward2Active = false;
int g_arrowShotCount = 0;
float g_currentPlayerSpeed = PLAYER_SPEED_X;
IMAGE g_reward2PlayerImg;
IMAGE g_waveImg;

struct Wave { float x, y; int w, h; int damage; float speedX; };
std::vector<Wave> g_waves;

struct SpiritPet { int type; float x, y; int hp; float shootTimer; bool active; };
std::vector<SpiritPet> g_spiritPets;
struct SpiritStone { float x, y; int damage; float speedX, speedY; bool active; };
std::vector<SpiritStone> g_spiritStones;
IMAGE g_spiritPetImg, g_spiritStoneImg;

struct IceBall { float x, y; int damage; float speedX; bool active; };
std::vector<IceBall> g_iceBalls;
IMAGE g_iceBallImg;

struct MeleeMinion {
    int road; float x, y; int hp; float moveSpeed; float attackTimer; int imgIdx; bool active; float riseOffset;
};
std::vector<MeleeMinion> g_meleeMinions;
IMAGE g_meleeImgs[3];
IMAGE g_slashImg;
struct Slash { float x, y; int damage; float speedX; float traveledDist; bool active; };
std::vector<Slash> g_slashes;

struct IceMinion {
    int road; float x, y; int hp; float moveSpeed; float attackTimer; int imgIdx; bool active; bool facingRight;
};
std::vector<IceMinion> g_iceMinions;
IMAGE g_iceMinionLeft[3], g_iceMinionRight[3];
IMAGE g_iceClawLeft, g_iceClawRight;
float g_iceMinionSummonTimer = 0.0f;
const float ICE_ATTACK_INTERVAL = 1.0f;

struct IceClaw { float x, y; int damage; float speedX; float traveledDist; bool active; };
std::vector<IceClaw> g_iceClaws;

float g_slowEffectTime = 0.0f;
bool g_isSlowed = false;
IMAGE g_frozenPlayerImg;
IMAGE g_portalImg;

float g_talentSilenceTime = 0.0f;
int g_talentAttackBonus = 0;
float g_summonCooldown = 0.0f;
float g_summonTimer = 0.0f;

// 怪大怪03相关
struct FlameArea { int road; float x, y; float width, height; bool active; };
std::vector<FlameArea> g_flameAreas;
float g_flameSwitchTimer = 0.0f;
float g_flameDamageAcc = 0.0f;
float g_flameDamageTotal = 0.0f;
bool g_isInFlame = false;

struct Hand { float x, y; float moveDir; float moveRange[2]; bool active; };
Hand g_hand;
struct FireBall { float x, y; int type; float targetX, targetY; float speedX, speedY; bool active; float life; };
std::vector<FireBall> g_fireBalls;
struct Explosion { float x, y; int type; float timer; bool active; };
std::vector<Explosion> g_explosions;
float g_fireBallCooldown = 0.0f;
int g_fireBallIndex = 0;
float g_fireDamageAcc = 0.0f;
float g_fireDamageTotal = 0.0f;
bool g_isInFire = false;

IMAGE g_fireImg, g_handImg, g_smallFireBallImg, g_midFireBallImg, g_bigFireBallImg;
IMAGE g_smallExplodeImg, g_midExplodeImg, g_bigExplodeImg;

// ---------- boss01 技能相关 ----------
struct ThunderCloud {
    float x, y; float speedX; bool active;
};
std::vector<ThunderCloud> g_thunderClouds;

struct Mark {
    int road;   // 0,1,2
    float x, y;
    float life;
    bool active;
};
std::vector<Mark> g_marks;

struct Spear {
    float x, y;
    float targetX, targetY;
    float speedX, speedY;
    float travelTimeLeft;
    bool active;
    bool isFalling;
};
std::vector<Spear> g_spears;

struct SpearLand {
    float x, y;
    float timer;
    bool active;
};
std::vector<SpearLand> g_spearLands;

enum BossSkillState {
    BOSS_IDLE,
    BOSS_SKILL1,
    BOSS_SKILL1_WAIT,
    BOSS_SKILL2,
    BOSS_SKILL2_WAIT,
    BOSS_SKILL3,         // 新增
    BOSS_SKILL3_WAIT     // 新增
}; BossSkillState g_bossSkillState = BOSS_IDLE;
float g_bossSkillTimer = 0.0f;
int g_bossSkill2Phase = 0;
float g_bossSkill2MoveTimer = 0.0f;
bool g_bossReducedDamage = false;
bool g_bossInvincible = false;
const int g_bossDamageReduction = 20;

struct BossClone {
    int road;
    float x, y;
    bool hasLeftSpear;
    bool hasRightSpear;
    bool active;
};
std::vector<BossClone> g_bossClones;
HardMonster* g_bossMonster = nullptr;

IMAGE g_bossSmallImg;
IMAGE g_spearLeftImg, g_spearRightImg, g_spearFallingImg, g_spearDownImg;
IMAGE g_thunderCloudImg, g_markImg;
// boss01 技能3相关
bool g_skill3Active = false;           // 技能3是否进行中
float g_skill3Timer = 0.0f;            // 技能3计时器（5秒）
bool g_playerControlDisabled = false;   // 玩家控制是否被禁用
float g_skill3DamageAcc = 0.0f;        // 扣血累加器（每秒扣50）

struct Boss02 {
    float x, y;
    bool active;
} g_boss02;

struct Lamp {
    float x, y;
    bool active;
};
std::vector<Lamp> g_lamps;

// 图片资源
IMAGE g_boss02Img;
IMAGE g_lampImg;
IMAGE g_suppressImg;
// 奖励技能相关
float g_reward01Timer = 0;
float g_reward02Timer = 0;
float g_reward03Timer = 0;
float g_reward04Timer = 0;
float g_reward05Timer = 0;
bool g_reward06Used = false;
bool g_reward04Active = false;
float g_reward04HealAcc = 0;
int g_bigSwordCount = 0;
float g_bigSwordTimer = 0;
int g_lightningCount = 0;
float g_lightningTimer = 0;

struct SwordRain {
    float x, y; float timer; int swordsLeft; bool active;
};
std::vector<SwordRain> g_swordRains;
struct BigSword {
    float x, y; int damage; float speedX; float traveledDist; bool active;
};
std::vector<BigSword> g_bigSwords;
struct Lightning {
    float x, y;
    float speedX;
    int damage;
    bool active;
};
std::vector<Lightning> g_lightnings;

IMAGE g_faZhenImg, g_swordImg, g_bigSwordLeftImg, g_bigSwordRightImg, g_lightningImg;

IMAGE g_smallBtnImgs[4];
IMAGE g_skillBtnImgs[4];

bool g_playerFacingLeft = false;

// ------------------- 应用状态枚举（新增玩法介绍页） -------------------
enum AppState {
    STATE_INTRO1, STATE_INTRO2, STATE_INTRO3, STATE_INTRO4,  // 新增玩法介绍
    STATE_ROLE_SELECT, STATE_CHARACTER_SHOW, STATE_DIFF_SELECT,
    STATE_GAME, STATE_GAME_OVER1, STATE_SCORE,
    STATE_HARD_GAME
};
AppState g_appState = STATE_INTRO1;
DWORD g_stateStartTime = 0;
int g_finalScore;
float g_finalGameTime = 0;

IMAGE g_intro1, g_intro2, g_intro3, g_intro4;  // 新增玩法介绍图片
IMAGE g_bgRole;
IMAGE g_gameOver1, g_gameOver2;
IMAGE g_characterImg;

struct Button { RECT rect; const char* text; };
std::vector<Button> g_roleButtons, g_diffButtons;

RECT g_backRoleBtn = { WIN_WIDTH / 2 - 342, WIN_HEIGHT - 152, WIN_WIDTH / 2 - 38, WIN_HEIGHT - 38 };
RECT g_backDiffBtn = { WIN_WIDTH / 2 + 38, WIN_HEIGHT - 152, WIN_WIDTH / 2 + 342, WIN_HEIGHT - 38 };

// 困难模式胜利/失败页面新增按钮
RECT g_hardRestartBtn = { WIN_WIDTH / 2 - 285, WIN_HEIGHT - 114, WIN_WIDTH / 2 - 114, WIN_HEIGHT - 38 };
RECT g_hardExitBtn = { WIN_WIDTH / 2 + 114, WIN_HEIGHT - 114, WIN_WIDTH / 2 + 285, WIN_HEIGHT - 38 };

// ------------------- 音频播放标志 -------------------
bool g_talentMusicPlayed = false;
bool g_scoreMusicPlayed = false;
bool g_victoryMusicPlayed = false;
bool g_defeatMusicPlayed = false;
float g_bossVoiceTimer = 0.0f;      // boss 语音计时器

// ------------------- 函数声明 -------------------
void putimage_alpha(int x, int y, IMAGE* img);
void Game_SpawnWave();
void Game_Init();
void Game_Start();
void Game_Update(float delta);
void Game_HandleKeyboard();
void Game_HandleMouse(float currentTime);
void Game_Draw();
void DrawSkillButton(int x, int y, float cooldown, const char* text, int imgIdx);
void DrawSmallButton(int idx, int x, int y, float cooldown);
void DrawEffect(IMAGE* img, float& timer);

void Hard_Init();
void Hard_Update(float delta);
void Hard_HandleKeyboard();
void Hard_HandleMouse(float currentTime);
void Hard_Draw();
void Hard_LoadMap(int stage);
void Hard_CheckMapClear();
void Hard_StartChestAnimation();
void Hard_UpdateChestAnimation(float delta);
void Hard_DrawChestAnimation();
void Hard_AdvanceToNextStage();
void Hard_TimeOut();
void Hard_PlayerDeath();
void Hard_ResetCurrentMap();
void Hard_StartGame();
void Hard_ClearAllProjectiles();
void InitRewardPool();

void InitIntroImages();
void InitButtons();
void InitSkillButtons();
void InitSmallButtons();
void DrawCurrentState();
void HandleIntroMouseClicks();
void LoadSmallButtonImages();
void LoadSkillButtonImages();

// 新增boss技能函数
void Boss_StartSkill1();
void Boss_StartSkill2();
void Boss_ResetToNormal();
void Boss_UpdateSkill1(float delta);
void Boss_UpdateSkill2(float delta);
void Boss_Update(float delta);
void Boss_TryStartSkill();

// ------------------- 通用工具函数 -------------------
void putimage_alpha(int x, int y, IMAGE* img) {
    int w = img->getwidth(), h = img->getheight();
    AlphaBlend(GetImageHDC(NULL), x, y, w, h, GetImageHDC(img), 0, 0, w, h, { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA });
}// ==================== 普通模式函数实现 ====================
void Game_SpawnWave() {
    int smallCount = (g_difficulty == 0) ? 4 : 6;
    Monster m;
    m.type = 1; m.w = ENEMY_W; m.h = ENEMY_H; m.hp = 200; m.road = 1;
    m.y = (float)(ROAD_Y[1] - ENEMY_H); m.x = (float)WIN_WIDTH;
    m.imgIdx = (g_difficulty == 0) ? rand() % 2 : rand() % 3;
    m.active = true; m.shootTimer = 0;
    g_monsters.push_back(m);

    int half = smallCount / 2;
    for (int i = 0; i < half; i++) {
        Monster s;
        s.type = 0; s.w = ENEMY_W; s.h = ENEMY_H; s.hp = 100; s.road = 0;
        s.y = (float)(ROAD_Y[0] - ENEMY_H); s.x = (float)(WIN_WIDTH + i * 160);
        s.imgIdx = rand() % 3; s.active = true; s.shootTimer = 0;
        g_monsters.push_back(s);
    }
    for (int i = 0; i < half; i++) {
        Monster s;
        s.type = 0; s.w = ENEMY_W; s.h = ENEMY_H; s.hp = 100; s.road = 2;
        s.y = (float)(ROAD_Y[2] - ENEMY_H); s.x = (float)(WIN_WIDTH + i * 160);
        s.imgIdx = rand() % 3; s.active = true; s.shootTimer = 0;
        g_monsters.push_back(s);
    }
}

void Game_Init() {
    if (g_difficulty == 0) loadimage(&g_bgGame, "05.jpg", WIN_WIDTH, WIN_HEIGHT);
    else if (g_difficulty == 1) loadimage(&g_bgGame, "10.jpg", WIN_WIDTH, WIN_HEIGHT);
    else loadimage(&g_bgGame, "05.jpg", WIN_WIDTH, WIN_HEIGHT);
    loadimage(&g_playerImg, "01.png", PLAYER_W, PLAYER_H);
    loadimage(&g_arrowImg, "箭矢.png", ARROW_W, ARROW_H);
    loadimage(&g_arrowImg2, "箭矢02.png", ARROW_W, ARROW_H);
    loadimage(&g_smallImgs[0], "怪小怪01.png", ENEMY_W, ENEMY_H);
    loadimage(&g_smallImgs[1], "怪小怪02.png", ENEMY_W, ENEMY_H);
    loadimage(&g_smallImgs[2], "怪小怪03.png", ENEMY_W, ENEMY_H);
    loadimage(&g_smallImgs[3], "怪小怪04.png", ENEMY_W, ENEMY_H);
    loadimage(&g_mediumImgs[0], "怪中怪01.png", ENEMY_W, ENEMY_H);
    loadimage(&g_mediumImgs[1], "怪中怪02.png", ENEMY_W, ENEMY_H);
    loadimage(&g_mediumImgs[2], "怪中怪03.png", ENEMY_W, ENEMY_H);
    loadimage(&g_mediumImgs[3], "怪中怪04.png", ENEMY_W, ENEMY_H);
    loadimage(&g_bigMonsterImgs[0], "怪大怪01.png", ENEMY_W, ENEMY_H);
    loadimage(&g_bigMonsterImgs[1], "怪大怪02.png", ENEMY_W, ENEMY_H);
    loadimage(&g_bigMonsterImgs[2], "怪大怪03.png", ENEMY_W, ENEMY_H);
    loadimage(&g_bigMonsterImgs[3], "怪大怪04.png", ENEMY_W, ENEMY_H);
    loadimage(&g_rageEffect, "狂暴.png", PLAYER_W, PLAYER_H);
    loadimage(&g_redeemEffect, "救赎.png", PLAYER_W, PLAYER_H);
    loadimage(&g_fireBallImg, "火球.png", BALL_SIZE, BALL_SIZE);
    loadimage(&g_planetImg, "星球.png", BALL_SIZE, BALL_SIZE);
    LoadSmallButtonImages();
    LoadSkillButtonImages();

    g_monsters.clear(); g_arrows.clear(); g_balls.clear();
    g_game_score = 0; g_game_playerHP = 500;
    g_game_playerX = 0; g_game_curRoad = 0;
    g_game_playerY = ROAD_Y[0] - PLAYER_H;
    g_hasTarget = false; g_targetHP = 0; g_showRange = false;
    g_gameState = GAME_IDLE;

    g_skillLeftEffectTime = 0; g_skillLeftUpEffectTime = 0; g_skillUpEffectTime = 0;
    g_skillLeftCooldown = 0; g_skillLeftUpCooldown = 0; g_skillUpCooldown = 0;
    g_rangeBonus = 0; g_attackBonus = 0; g_ultLockMove = false;
    for (int i = 0; i < 4; i++) g_smallCooldown[i] = 0;
    g_rageEffectTime = 0; g_redeemEffectTime = 0; g_invincibleTime = 0;
    g_reviveAvailable = false; g_bottomYellow = false;
    g_gameTimer = 0.0f; g_timerRunning = false;
}

void Game_Start() {
    if (g_difficulty == 0) loadimage(&g_bgGame, "05.jpg", WIN_WIDTH, WIN_HEIGHT);
    else if (g_difficulty == 1) loadimage(&g_bgGame, "10.jpg", WIN_WIDTH, WIN_HEIGHT);
    g_gameState = GAME_RUNNING;
    g_monsters.clear(); g_arrows.clear(); g_balls.clear();
    g_game_score = 0; g_game_playerHP = 500;
    g_game_playerX = 0; g_game_curRoad = 0;
    g_game_playerY = ROAD_Y[0] - PLAYER_H;
    g_hasTarget = false; g_targetHP = 0; g_showRange = false;
    g_skillLeftEffectTime = 0; g_skillLeftUpEffectTime = 0; g_skillUpEffectTime = 0;
    g_skillLeftCooldown = 0; g_skillLeftUpCooldown = 0; g_skillUpCooldown = 0;
    g_rangeBonus = 0; g_attackBonus = 0; g_ultLockMove = false;
    for (int i = 0; i < 4; i++) g_smallCooldown[i] = 0;
    g_rageEffectTime = 0; g_redeemEffectTime = 0; g_invincibleTime = 0;
    g_reviveAvailable = false; g_bottomYellow = false;
    g_gameTimer = 0.0f; g_timerRunning = true;
    Game_SpawnWave();
    StopMusic("intro_music");
    // 根据难度播放背景音乐
    if (g_difficulty == 0) PlayMusic("简单模式.mp3", "game_music", true);
    else PlayMusic("中等模式.mp3", "game_music", true);
}

void Game_Update(float delta) {
    if (g_gameState != GAME_RUNNING) return;
    if (g_timerRunning) g_gameTimer += delta;

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
        else g_ultLockMove = true;
    }
    if (g_rageEffectTime > 0) g_rageEffectTime -= delta;
    if (g_redeemEffectTime > 0) g_redeemEffectTime -= delta;
    if (g_invincibleTime > 0) {
        g_invincibleTime -= delta;
        if (g_invincibleTime < 0) g_invincibleTime = 0;
    }

    if (g_skillLeftCooldown > 0) g_skillLeftCooldown -= delta;
    if (g_skillLeftUpCooldown > 0) g_skillLeftUpCooldown -= delta;
    if (g_skillUpCooldown > 0) g_skillUpCooldown -= delta;
    for (int i = 0; i < 4; i++) if (g_smallCooldown[i] > 0) g_smallCooldown[i] -= delta;
    if (g_skillLeftCooldown < 0) g_skillLeftCooldown = 0;
    if (g_skillLeftUpCooldown < 0) g_skillLeftUpCooldown = 0;
    if (g_skillUpCooldown < 0) g_skillUpCooldown = 0;
    for (int i = 0; i < 4; i++) if (g_smallCooldown[i] < 0) g_smallCooldown[i] = 0;

    if (!g_ultLockMove) {
        if (KEY_DOWN('A')) { g_game_playerX -= PLAYER_SPEED_X * delta; if (g_game_playerX < 0) g_game_playerX = 0; }
        if (KEY_DOWN('D')) { g_game_playerX += PLAYER_SPEED_X * delta; int maxX = WIN_WIDTH - PLAYER_W; if (g_game_playerX > maxX) g_game_playerX = maxX; }
    }
    static int lastW = 0, lastS = 0;
    if (KEY_DOWN('W') && !lastW && g_game_curRoad > 0) { g_game_curRoad--; g_game_playerY = ROAD_Y[g_game_curRoad] - PLAYER_H; }
    if (KEY_DOWN('S') && !lastS && g_game_curRoad < 2) { g_game_curRoad++; g_game_playerY = ROAD_Y[g_game_curRoad] - PLAYER_H; }
    lastW = KEY_DOWN('W'); lastS = KEY_DOWN('S');

    float step = 30.0f;
    float mediumSpeed = (g_difficulty == 0) ? 20.0f : 30.0f;
    for (auto& mon : g_monsters) {
        if (!mon.active) continue;
        float sp = (mon.type == 1 && g_difficulty == 0) ? mediumSpeed : step;
        mon.x -= sp * delta;
        if (mon.x + mon.w < 0) mon.active = false;
    }
    g_monsters.erase(std::remove_if(g_monsters.begin(), g_monsters.end(), [](const Monster& m) { return !m.active; }), g_monsters.end());

    bool hasMedium = false;
    for (auto& mon : g_monsters) if (mon.type == 1) { hasMedium = true; break; }
    if (!hasMedium && g_gameState == GAME_RUNNING) Game_SpawnWave();

    for (auto& mon : g_monsters) {
        if (!mon.active) continue;
        bool silenced = false;
        if (g_skillUpEffectTime > 0 && mon.road == g_game_curRoad) {
            float px = g_game_playerX + PLAYER_W / 2.0f;
            float ex = mon.x + mon.w / 2.0f;
            if (fabs(px - ex) <= 300.0f) silenced = true;
        }
        if (!silenced) {
            float interval; int damage;
            if (mon.type == 0) {
                if (g_difficulty == 0) { damage = 20; interval = 7.0f; }
                else { damage = 30; interval = 4.0f; }
            }
            else {
                if (g_difficulty == 0) { damage = 30; interval = 4.0f; }
                else { damage = 50; interval = 7.0f; }
            }
            mon.shootTimer += delta;
            if (mon.shootTimer >= interval) {
                mon.shootTimer = 0;
                Ball b;
                b.size = BALL_SIZE;
                b.x = mon.x + mon.w / 2 - BALL_SIZE / 2;
                b.y = mon.y + mon.h / 2 - BALL_SIZE / 2;
                b.damage = damage;
                b.isFire = (mon.type == 1);
                g_balls.push_back(b);
            }
        }
    }

    float ballStep = BALL_SPEED * delta;
    for (auto& ball : g_balls) ball.x -= ballStep;
    g_balls.erase(std::remove_if(g_balls.begin(), g_balls.end(), [](const Ball& b) { return b.x + BALL_SIZE < 0 || b.x > WIN_WIDTH; }), g_balls.end());
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

    float arrowStep = ARROW_SPEED * delta;
    for (auto& arr : g_arrows) arr.x += arr.dir * arrowStep;
    g_arrows.erase(std::remove_if(g_arrows.begin(), g_arrows.end(), [](const Arrow& a) { return a.x + a.w < 0 || a.x > WIN_WIDTH; }), g_arrows.end());
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
    bool qNow = KEY_DOWN('Q');
    if (qNow && !lastQ && g_skillLeftCooldown <= 0) {
        g_skillLeftEffectTime = 3.0f; g_rangeBonus = 100; g_skillLeftCooldown = 5.0f;
        PlayMusic("技能1.mp3", "skill", false);
    }
    lastQ = qNow;
    bool eNow = KEY_DOWN('E');
    if (eNow && !lastE && g_skillLeftUpCooldown <= 0) {
        g_skillLeftUpEffectTime = 3.0f; g_attackBonus = 10; g_skillLeftUpCooldown = 5.0f;
        PlayMusic("技能2.mp3", "skill", false);
    }
    lastE = eNow;
    bool rNow = KEY_DOWN('R');
    if (rNow && !lastR && g_skillUpCooldown <= 0) {
        g_skillUpEffectTime = 10.0f; g_ultLockMove = true; g_skillUpCooldown = 20.0f;
        PlayMusic("大招.mp3", "ultimate", false);
    }
    lastR = rNow;

    for (int i = 0; i < 4; i++) {
        int vk = '1' + i;
        bool now = KEY_DOWN(vk);
        if (now && !lastDigit[i]) {
            int cost = (i == 0 || i == 1) ? 100 : (i == 2 ? 200 : 500);
            if (g_smallCooldown[i] <= 0 && g_game_score >= cost) {
                g_game_score -= cost;
                if (i == 0) { g_rageEffectTime = 10.0f; g_smallCooldown[i] = 30.0f; }
                else if (i == 1) { g_game_playerHP += 100; g_redeemEffectTime = 2.0f; g_smallCooldown[i] = 30.0f; }
                else if (i == 2) { g_invincibleTime = 5.0f; g_smallCooldown[i] = 30.0f; }
                else if (i == 3) {
                    if (!g_reviveAvailable) {
                        g_reviveAvailable = true;
                        g_bottomYellow = true;
                        g_smallCooldown[i] = 30.0f;
                    }
                }
                PlayMusic("buy.mp3", "buy", false);
            }
        }
        lastDigit[i] = now;
    }
}

void Game_HandleMouse(float currentTime) {
    if (g_gameState == GAME_OVER) return;
    static bool lastLeft = false;
    if (MouseHit()) {
        MOUSEMSG msg = GetMouseMsg();
        if (msg.uMsg == WM_LBUTTONDOWN && !lastLeft) {
            if (g_gameState == GAME_IDLE &&
                msg.x >= g_startBtn.left && msg.x <= g_startBtn.right &&
                msg.y >= g_startBtn.top && msg.y <= g_startBtn.bottom) {
                Game_Start(); return;
            }
            if ((g_gameState == GAME_RUNNING || g_gameState == GAME_PAUSED) &&
                msg.x >= g_restartBtn.left && msg.x <= g_restartBtn.right &&
                msg.y >= g_restartBtn.top && msg.y <= g_restartBtn.bottom) {
                StopMusic("game_music"); Game_Start(); return;
            }
            if ((g_gameState == GAME_RUNNING || g_gameState == GAME_PAUSED) &&
                msg.x >= g_endBtn.left && msg.x <= g_endBtn.right &&
                msg.y >= g_endBtn.top && msg.y <= g_endBtn.bottom) {
                StopMusic("game_music");
                g_monsters.clear(); g_arrows.clear(); g_balls.clear();
                g_gameState = GAME_IDLE;
                g_appState = STATE_DIFF_SELECT;
                PlayMusic("05.mp3", "intro_music", true);
                return;
            }
            if ((g_gameState == GAME_RUNNING || g_gameState == GAME_PAUSED) &&
                msg.x >= g_pauseBtn.left && msg.x <= g_pauseBtn.right &&
                msg.y >= g_pauseBtn.top && msg.y <= g_pauseBtn.bottom) {
                if (g_gameState == GAME_RUNNING) {
                    g_gameState = GAME_PAUSED; g_timerRunning = false; PauseMusic("game_music");
                }
                else {
                    g_gameState = GAME_RUNNING; g_timerRunning = true; ResumeMusic("game_music");
                }
                return;
            }
            if (g_gameState == GAME_RUNNING) {
                int dx = msg.x - ATK_BTN_X, dy = msg.y - ATK_BTN_Y;
                if (dx * dx + dy * dy <= ATK_BTN_RADIUS * ATK_BTN_RADIUS) {
                    g_showRange = true; g_rangeEndTime = currentTime + 0.2f;
                    float px = g_game_playerX + PLAYER_W / 2.0f, py = g_game_playerY + PLAYER_H / 2.0f;
                    float minDist = ATTACK_RANGE + g_rangeBonus + 1;
                    for (auto& mon : g_monsters) {
                        float ex = mon.x + mon.w / 2.0f, ey = mon.y + mon.h / 2.0f;
                        float d = sqrtf((px - ex) * (px - ex) + (py - ey) * (py - ey));
                        if (d < minDist) minDist = d;
                    }
                    if (minDist <= ATTACK_RANGE + g_rangeBonus) {
                        Arrow arr; arr.w = ARROW_W; arr.h = ARROW_H;
                        arr.x = g_game_playerX + PLAYER_W;
                        arr.y = g_game_playerY + PLAYER_H / 2 - ARROW_H / 2;
                        int extra = g_attackBonus + (g_rageEffectTime > 0 ? 20 : 0);
                        arr.damage = 20 + extra;
                        arr.dir = 1;
                        g_arrows.push_back(arr);
                        PlayMusic("普攻.mp3", "attack", false);
                    }
                }
            }
        }
        lastLeft = (msg.uMsg == WM_LBUTTONDOWN);
    }
    else { lastLeft = false; }
}

void DrawSkillButton(int x, int y, float cooldown, const char* text, int imgIdx) {
    int r = SKILL_RADIUS;
    if (g_skillBtnImgs[imgIdx].getwidth() > 0) {
        putimage_alpha(x - r, y - r, &g_skillBtnImgs[imgIdx]);
    }
    else {
        COLORREF fillColor = (cooldown > 0) ? RGB(100, 100, 100) : RGB(0, 200, 200);
        setfillcolor(fillColor);
        setlinecolor(WHITE);
        fillcircle(x, y, r);
        setbkmode(TRANSPARENT);
        settextcolor(WHITE);
        settextstyle(14, 0, "微软雅黑");
        outtextxy(x - 10, y - 7, text);
    }
    if (cooldown > 0) {
        setfillcolor(RGB(100, 100, 100));
        setlinecolor(RGB(100, 100, 100));
        fillcircle(x, y, r);
        settextcolor(WHITE);
        settextstyle(20, 0, "微软雅黑");
        char cd[16];
        sprintf_s(cd, "%.0f", ceil(cooldown));
        int tw = textwidth(cd), th = textheight(cd);
        outtextxy(x - tw / 2, y - th / 2, cd);
    }
}

void DrawSmallButton(int idx, int x, int y, float cooldown) {
    int r = SMALL_BTN_RADIUS;
    int diameter = 2 * r;
    int cx = x + r;
    int cy = y + r;
    if (g_smallBtnImgs[idx].getwidth() > 0) {
        putimage_alpha(x, y, &g_smallBtnImgs[idx]);
    }
    else {
        setfillcolor(RGB(80, 80, 80));
        solidrectangle(x, y, x + diameter, y + diameter);
    }
    if (cooldown > 0) {
        setfillcolor(RGB(100, 100, 100));
        setlinecolor(RGB(100, 100, 100));
        fillcircle(cx, cy, r);
        settextcolor(WHITE);
        settextstyle(20, 0, "微软雅黑");
        char cd[16];
        sprintf_s(cd, "%d", (int)ceil(cooldown));
        int tw = textwidth(cd), th = textheight(cd);
        outtextxy(cx - tw / 2, cy - th / 2, cd);
    }
}

void DrawEffect(IMAGE* img, float& timer) {
    if (timer > 0) putimage_alpha(g_game_playerX, g_game_playerY, img);
}

void Game_Draw() {
    putimage(0, 0, &g_bgGame);
    IMAGE* playerImg = &g_playerImg;
    if (g_isSlowed && g_slowEffectTime > 0) playerImg = &g_frozenPlayerImg;
    else if (g_reward2Active) playerImg = &g_reward2PlayerImg;
    if (g_invincibleTime > 0 && ((int)(GetTickCount() / 100) % 2 == 0)) {}
    else putimage_alpha((int)g_game_playerX, g_game_playerY, playerImg);
    DrawEffect(&g_rageEffect, g_rageEffectTime);
    DrawEffect(&g_redeemEffect, g_redeemEffectTime);
    setlinecolor(g_bottomYellow ? RGB(255, 255, 0) : RGB(255, 0, 0));
    setlinestyle(PS_SOLID, 2);
    line((int)g_game_playerX, g_game_playerY + PLAYER_H, (int)g_game_playerX + PLAYER_W, g_game_playerY + PLAYER_H);

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
            putimage_alpha((int)ball.x, (int)ball.y, &g_planetImg);
        }
    }
    if (g_showRange) {
        int bx = (int)g_game_playerX + PLAYER_W + (int)(ATTACK_RANGE + g_rangeBonus);
        int cy = g_game_playerY + PLAYER_H / 2;
        setlinecolor(RGB(0, 100, 255)); setlinestyle(PS_DASH, 1);
        line(bx, cy - 10, bx, cy + 10);
        setlinestyle(PS_SOLID, 1);
    }

    // 攻击按钮
    if (g_skillBtnImgs[3].getwidth() > 0) {
        int r = ATK_BTN_RADIUS;
        putimage_alpha(ATK_BTN_X - r, ATK_BTN_Y - r, &g_skillBtnImgs[3]);
    }
    else {
        setfillcolor(RGB(200, 80, 80));
        setlinecolor(RGB(255, 255, 255));
        fillcircle(ATK_BTN_X, ATK_BTN_Y, ATK_BTN_RADIUS);
    }
    setbkmode(TRANSPARENT);
    settextcolor(WHITE);
    settextstyle(38, 0, "微软雅黑");
    outtextxy(ATK_BTN_X - 38, ATK_BTN_Y - 19, "攻击");

    DrawSkillButton(g_skillLeftX, g_skillLeftY, g_skillLeftCooldown, "范围", 0);
    DrawSkillButton(g_skillLeftUpX, g_skillLeftUpY, g_skillLeftUpCooldown, "攻击", 1);
    DrawSkillButton(g_skillUpX, g_skillUpY, g_skillUpCooldown, "大招", 2);
    for (int i = 0; i < 4; i++) DrawSmallButton(i, g_smallBtnX[i], g_smallBtnY[i], g_smallCooldown[i]);

    setfillcolor(RGB(255, 165, 0)); setlinecolor(BLACK);
    fillrectangle(g_restartBtn.left, g_restartBtn.top, g_restartBtn.right, g_restartBtn.bottom);
    setbkmode(TRANSPARENT); settextcolor(BLACK); settextstyle(34, 0, "微软雅黑");
    const char* restartText = "重新挑战";
    int tw_restart = textwidth(restartText), th_restart = textheight(restartText);
    outtextxy(g_restartBtn.left + (g_restartBtn.right - g_restartBtn.left - tw_restart) / 2,
        g_restartBtn.top + (g_restartBtn.bottom - g_restartBtn.top - th_restart) / 2, restartText);

    setfillcolor(RGB(200, 50, 50)); setlinecolor(BLACK);
    fillrectangle(g_endBtn.left, g_endBtn.top, g_endBtn.right, g_endBtn.bottom);
    setbkmode(TRANSPARENT); settextcolor(WHITE); settextstyle(34, 0, "微软雅黑");
    const char* endText = "结束游戏";
    int tw_end = textwidth(endText), th_end = textheight(endText);
    outtextxy(g_endBtn.left + (g_endBtn.right - g_endBtn.left - tw_end) / 2,
        g_endBtn.top + (g_endBtn.bottom - g_endBtn.top - th_end) / 2, endText);

    if (g_gameState == GAME_RUNNING || g_gameState == GAME_PAUSED) {
        setfillcolor(RGB(200, 200, 0)); setlinecolor(BLACK);
        fillrectangle(g_pauseBtn.left, g_pauseBtn.top, g_pauseBtn.right, g_pauseBtn.bottom);
        setbkmode(TRANSPARENT); settextcolor(BLACK); settextstyle(38, 0, "微软雅黑");
        const char* pauseText = (g_gameState == GAME_RUNNING) ? "暂停" : "继续";
        int tw2 = textwidth(pauseText), th2 = textheight(pauseText);
        outtextxy(g_pauseBtn.left + (g_pauseBtn.right - g_pauseBtn.left - tw2) / 2,
            g_pauseBtn.top + (g_pauseBtn.bottom - g_pauseBtn.top - th2) / 2, pauseText);
    }

    settextcolor(BLACK); settextstyle(38, 0, "微软雅黑");
    char buf[64];
    sprintf_s(buf, "分数: %d", g_game_score); outtextxy(10, 10, buf);
    sprintf_s(buf, "血量: %d", g_game_playerHP); outtextxy(WIN_WIDTH - 380, 10, buf);
    if (g_hasTarget) { sprintf_s(buf, "目标血量: %d", g_targetHP); outtextxy(WIN_WIDTH - 380, 60, buf); }
    int minutes = (int)(g_gameTimer / 60), seconds = (int)g_gameTimer % 60;
    sprintf_s(buf, "对局时长: %02d:%02d", minutes, seconds);
    int tw = textwidth(buf); outtextxy((WIN_WIDTH - tw) / 2, 10, buf);

    if (g_gameState == GAME_IDLE) {
        setfillcolor(RGB(100, 200, 100)); setlinecolor(BLACK);
        fillrectangle(g_startBtn.left, g_startBtn.top, g_startBtn.right, g_startBtn.bottom);
        setbkmode(TRANSPARENT); settextcolor(BLACK); settextstyle(47, 0, "微软雅黑");
        outtextxy(g_startBtn.left + 38, g_startBtn.top + 19, "开始游戏");
    }
}

// ==================== 困难模式函数实现 ====================
void Hard_LoadMap(int stage) {
    g_hardMonsters.clear();
    int offsetLeft = 228;

    auto addSmall = [&](int road, int imgIdx, float extraOffset = 0) {
        HardMonster m;
        m.type = 0; m.road = road; m.hp = 100; m.maxHp = 100; m.imgIdx = imgIdx;
        m.x = WIN_WIDTH - ENEMY_W - offsetLeft - extraOffset;
        m.y = ROAD_Y[road] - ENEMY_H; m.shootTimer = 0; m.active = true;
        g_hardMonsters.push_back(m);
        };
    auto addMedium = [&](int road, int imgIdx, float extraOffset = 0) {
        HardMonster m;
        m.type = 3; m.road = road; m.hp = 200; m.maxHp = 200; m.imgIdx = imgIdx;
        m.x = WIN_WIDTH - ENEMY_W - offsetLeft - extraOffset;
        m.y = ROAD_Y[road] - ENEMY_H; m.shootTimer = 0; m.active = true;
        g_hardMonsters.push_back(m);
        };
    auto addBig = [&](int road, int imgIdx, float extraOffset = 0) {
        HardMonster m;
        m.type = 1; m.road = road; m.hp = 500; m.maxHp = 500; m.imgIdx = imgIdx;
        m.x = WIN_WIDTH - ENEMY_W - offsetLeft - extraOffset;
        m.y = ROAD_Y[road] - ENEMY_H; m.shootTimer = 0; m.active = true;
        g_hardMonsters.push_back(m);
        };
    auto addBoss = [&](int bossIdx) {
        HardMonster m;
        m.type = 2; m.road = 1; m.hp = 1000 + (bossIdx - 1) * 500; m.maxHp = m.hp;
        m.imgIdx = bossIdx - 1; m.x = 1280.0f; m.y = 280.0f; m.shootTimer = 0; m.active = true;
        g_hardMonsters.push_back(m);
        };

    if (stage == 1) {
        addBig(1, 0, 0);
        addSmall(0, 0, 0); addSmall(0, 1, 57);
        addSmall(2, 0, 0); addSmall(2, 1, 57);
    }
    else if (stage == 2) {
        addBig(1, 1, 0);
        addMedium(0, 0, 0); addMedium(0, 1, 57);
        addMedium(2, 0, 0); addMedium(2, 1, 57);
    }
    else if (stage == 3) {
        addBig(1, 2, 0);
        addMedium(0, 2, 0); addMedium(0, 3, 57);
        addMedium(2, 2, 0); addMedium(2, 3, 57);
    }
    else if (stage == 4) {
        addBoss(1);
    }

    // 保存boss指针
    if (stage == 4) {
        for (auto& mon : g_hardMonsters) {
            if (mon.type == 2) {
                g_bossMonster = &mon;
                break;
            }
        }
        // 重置技能状态机
        g_bossSkillState = BOSS_IDLE;
        g_bossReducedDamage = false;
        g_bossInvincible = false;
        g_thunderClouds.clear();
        g_marks.clear();
        g_spears.clear();
        g_spearLands.clear();
        g_bossClones.clear();
    }
    else {
        g_bossMonster = nullptr;
    }

    g_mapCleared = false;
    g_chestClicked = false;
    g_bossDefeated = false;
    g_hardMapMonsterCount = (int)g_hardMonsters.size();

    // 重置其他小怪相关状态
    g_flameAreas.clear();
    g_hand.active = false;
    g_fireBalls.clear();
    g_explosions.clear();
    g_flameSwitchTimer = 0.0f;
    g_fireBallCooldown = 0.0f;
    g_fireBallIndex = 0;
    g_isInFlame = false;
    g_flameDamageAcc = 0.0f;
    g_flameDamageTotal = 0.0f;
    g_fireDamageAcc = 0.0f;
    g_fireDamageTotal = 0.0f;
    g_isInFire = false;

    // 根据关卡切换背景音乐
    if (stage == 4) {
        StopMusic("game_music");
        PlayMusic("困难boss.mp3", "game_music", true);
    }
    else if (stage >= 1 && stage <= 3) {
        StopMusic("game_music");
        PlayMusic("简单模式.mp3", "game_music", true);
    }
}

void Hard_CheckMapClear() {
    if (g_hardState != HARD_PLAYING) return;
    bool allDead = true;
    for (auto& m : g_hardMonsters) if (m.hp > 0) { allDead = false; break; }
    if (!allDead) return;
    for (auto& m : g_meleeMinions) if (m.active && m.hp > 0) { allDead = false; break; }
    if (!allDead) return;
    for (auto& ice : g_iceMinions) if (ice.active && ice.hp > 0) { allDead = false; break; }
    if (allDead && !g_mapCleared) {
        g_mapCleared = true;
        if (g_currentStage <= 3) {
            // 宝箱地图，等待玩家点击宝箱
        }
        else {
            g_bossDefeated = true;
            g_hardState = HARD_VICTORY;
        }
    }
}

void Hard_StartGame() {
    g_hardState = HARD_PLAYING;
    g_hardPaused = false;
    g_timeLeft = 15 * 60;
    g_lastSecond = (float)GetTickCount() / 1000.0f;
    g_game_playerHP = 500;
    g_game_playerX = 0;
    g_game_curRoad = 0;
    g_game_playerY = ROAD_Y[0] - PLAYER_H;
    g_arrows.clear();
    for (int i = 0; i < 3; i++) g_smallCooldown[i] = 0;
    g_rageEffectTime = g_redeemEffectTime = g_invincibleTime = 0;
    g_summonTimer = 10.0f;
    g_summonCooldown = 0.0f;
    g_playerFacingLeft = false;
    g_iceMinionSummonTimer = 0.0f;
    g_obtainedRewardCount = 0;
    for (int i = 0; i < 3; i++) g_rewardSkillAvailable[i] = false;

    // 重置 boss 技能状态
    if (g_bossMonster) {
        g_bossMonster->active = true;
        g_bossMonster->hp = g_bossMonster->maxHp;
    }
    g_bossSkillState = BOSS_IDLE;
    g_bossReducedDamage = false;
    g_bossInvincible = false;
    g_thunderClouds.clear();
    g_marks.clear();
    g_spears.clear();
    g_spearLands.clear();
    g_bossClones.clear();

    g_flameAreas.clear();
    g_hand.active = false;
    g_fireBalls.clear();
    g_explosions.clear();
    g_flameSwitchTimer = 0.0f;
    g_fireBallCooldown = 0.0f;
    g_fireBallIndex = 0;
    g_isInFlame = false;
    g_flameDamageAcc = 0.0f;
    g_flameDamageTotal = 0.0f;
    g_fireDamageAcc = 0.0f;
    g_fireDamageTotal = 0.0f;
    g_isInFire = false;

    // 重置音频标志
    g_talentMusicPlayed = false;
    g_scoreMusicPlayed = false;
    g_victoryMusicPlayed = false;
    g_defeatMusicPlayed = false;
}

void InitRewardPool() {
    int arr[6] = { 0,1,2,3,4,5 };
    for (int i = 0; i < 6; i++) {
        int j = rand() % 6;
        int t = arr[i]; arr[i] = arr[j]; arr[j] = t;
    }
    for (int i = 0; i < 3; i++) {
        g_obtainedRewards[i] = arr[i];
    }
    g_obtainedRewardCount = 0;
}

void Hard_Init() {
    loadimage(&g_playerImg, "01.png", PLAYER_W, PLAYER_H);
    loadimage(&g_playerImgLeft, "01左.png", PLAYER_W, PLAYER_H);
    loadimage(&g_arrowImg, "箭矢.png", ARROW_W, ARROW_H);
    loadimage(&g_arrowImg2, "箭矢02.png", ARROW_W, ARROW_H);
    loadimage(&g_arrowImgLeft, "箭矢左.png", ARROW_W, ARROW_H);
    loadimage(&g_arrowImg2Left, "箭矢02左.png", ARROW_W, ARROW_H);
    for (int i = 0; i < 4; i++) {
        char name[64];
        sprintf_s(name, "怪小怪%02d.png", i + 1);
        loadimage(&g_smallImgs[i], name, ENEMY_W, ENEMY_H);
        sprintf_s(name, "怪中怪%02d.png", i + 1);
        loadimage(&g_mediumImgs[i], name, ENEMY_W, ENEMY_H);
        sprintf_s(name, "怪大怪%02d.png", i + 1);
        loadimage(&g_bigMonsterImgs[i], name, ENEMY_W, ENEMY_H);
    }
    loadimage(&g_rageEffect, "狂暴.png", PLAYER_W, PLAYER_H);
    loadimage(&g_redeemEffect, "救赎.png", PLAYER_W, PLAYER_H);
    loadimage(&g_fireBallImg, "火球.png", BALL_SIZE, BALL_SIZE);
    loadimage(&g_doorImg, "门.png", 133, 152);
    loadimage(&g_victoryBg, "06.jpg", WIN_WIDTH, WIN_HEIGHT);
    loadimage(&g_defeatBg, "08.jpg", WIN_WIDTH, WIN_HEIGHT);

    for (int s = 1; s <= 4; s++) {
        char name[64];
        sprintf_s(name, "第%d关.jpg", s);
        loadimage(&g_bgMap[s - 1], name, WIN_WIDTH, WIN_HEIGHT);
        for (int r = 0; r < 2; r++) {
            sprintf_s(name, "第%d关奖励%02d.png", s, r + 1);
            loadimage(&g_rewardImgs[s - 1][r], name, SMALL_BTN_RADIUS * 2, SMALL_BTN_RADIUS * 2);
        }
    }
    loadimage(&g_chestImg, "宝箱.png", 95, 95);
    loadimage(&g_chestOpenImg, "宝箱开启.png", 95, 95);
    loadimage(&g_gameOverImg, "08.jpg", WIN_WIDTH, WIN_HEIGHT);
    loadimage(&g_gameWinImg, "06.jpg", WIN_WIDTH, WIN_HEIGHT);
    if (!g_talentLoaded) {
        loadimage(&g_talentSmall, "本命天赋缩小.jpg", 190, 285);
        g_talentLoaded = true;
    }
    if (!g_talentIntroLoaded) {
        loadimage(&g_talentIntro, "本命天赋.jpg", WIN_WIDTH, WIN_HEIGHT);
        g_talentIntroLoaded = true;
    }

    loadimage(&g_reward2PlayerImg, "骑鲨伽罗.png", PLAYER_W, PLAYER_H);
    loadimage(&g_waveImg, "海浪.png", 152, 76);
    loadimage(&g_spiritPetImg, "灵宝.png", 76, 76);
    loadimage(&g_spiritStoneImg, "灵石.png", 38, 38);
    loadimage(&g_iceBallImg, "冰球.png", 40, 40);
    for (int i = 0; i < 3; i++) {
        char name[64];
        sprintf_s(name, "近战士%02d.png", i + 1);
        loadimage(&g_meleeImgs[i], name, 76, 76);
    }
    loadimage(&g_slashImg, "斩击.png", 57, 57);
    for (int i = 0; i < 6; i++) {
        char name[64];
        sprintf_s(name, "奖励%02d.jpg", i + 1);
        loadimage(&g_rewardBgImgsFull[i], name, WIN_WIDTH, WIN_HEIGHT);
        sprintf_s(name, "奖励缩小%02d.jpg", i + 1);
        loadimage(&g_rewardSmallImgs[i], name, 190, 285);
    }
    for (int i = 0; i < 3; i++) {
        char leftName[64], rightName[64];
        sprintf_s(leftName, "冰怪%02d.png", i + 1);
        sprintf_s(rightName, "冰怪右%02d.png", i + 1);
        loadimage(&g_iceMinionLeft[i], leftName, 76, 76);
        loadimage(&g_iceMinionRight[i], rightName, 76, 76);
    }
    loadimage(&g_iceClawLeft, "冰爪.png", 57, 57);
    loadimage(&g_iceClawRight, "冰爪右.png", 57, 57);
    loadimage(&g_frozenPlayerImg, "冰冻伽罗.png", PLAYER_W, PLAYER_H);
    loadimage(&g_portalImg, "传送门.png", 76, 76);

    loadimage(&g_fireImg, "火焰.png", 300, ENEMY_H);
    loadimage(&g_handImg, "手.png", 70, 70);
    loadimage(&g_smallFireBallImg, "小火球.png", 40, 40);
    loadimage(&g_midFireBallImg, "中火球.png", 60, 60);
    loadimage(&g_bigFireBallImg, "大火球.png", 80, 80);
    loadimage(&g_smallExplodeImg, "小爆.png", 60, 60);
    loadimage(&g_midExplodeImg, "中爆.png", 80, 80);
    loadimage(&g_bigExplodeImg, "大爆.png", 100, 100);
    loadimage(&g_planetImg, "星球.png", BALL_SIZE, BALL_SIZE);
    // boss01 技能图片
    loadimage(&g_thunderCloudImg, "雷云.png", 200, 120);
    loadimage(&g_markImg, "标记.png", ENEMY_W, 50);
    loadimage(&g_spearFallingImg, "长枪.png", 30, 90);
    loadimage(&g_spearDownImg, "长枪落下.png", 40, 90);
    loadimage(&g_spearLeftImg, "长枪左.png", 80, 10);
    loadimage(&g_spearRightImg, "长枪右.png", 80, 10);
    loadimage(&g_bossSmallImg, "boss01.png", BOSS_SMALL_W, BOSS_SMALL_H);
    loadimage(&g_bossImgs[0], "boss01.png", BOSS_W, BOSS_H);
    loadimage(&g_faZhenImg, "法阵.png", 80, 50);
    loadimage(&g_swordImg, "剑.png", 20, 40);
    loadimage(&g_bigSwordLeftImg, "剑1.png", 50, 30);
    loadimage(&g_bigSwordRightImg, "剑2.png", 50, 30);
    loadimage(&g_lightningImg, "闪电.png", 80, 80);
    loadimage(&g_boss02Img, "boss02.png", PLAYER_W, PLAYER_H);
    loadimage(&g_lampImg, "灯.png", 40, 40);
    loadimage(&g_suppressImg, "压制.png", PLAYER_W, PLAYER_H);
    LoadSmallButtonImages();
    LoadSkillButtonImages();

    g_currentStage = 1; g_currentMap = 0;
    g_hardScore = 1000; g_timeLeft = 15 * 60;
    g_lastSecond = (float)GetTickCount() / 1000.0f;
    g_hardState = HARD_IDLE;
    g_hardPaused = false;
    g_showTalentIntro = true;
    g_rewardButtons.clear(); g_rewardIndex = 0;
    g_bossDefeated = false; g_chestAnimation = false;
    g_game_playerHP = 500; g_game_playerX = 0; g_game_curRoad = 0;
    g_game_playerY = ROAD_Y[0] - PLAYER_H;
    for (int i = 0; i < 3; i++) g_smallCooldown[i] = 0;
    g_rageEffectTime = g_redeemEffectTime = g_invincibleTime = 0;
    g_talentCooldown = 0;
    g_reward2Active = false; g_reward2EffectTime = 0; g_arrowShotCount = 0;
    g_currentPlayerSpeed = PLAYER_SPEED_X;
    g_spiritPets.clear(); g_spiritStones.clear(); g_waves.clear();
    g_iceBalls.clear(); g_meleeMinions.clear(); g_slashes.clear();
    g_talentSilenceTime = 0; g_talentAttackBonus = 0;
    g_summonCooldown = 0;
    g_iceMinionSummonTimer = 0.0f;
    g_iceMinions.clear();
    g_iceClaws.clear();
    g_slowEffectTime = 0; g_isSlowed = false;
    InitRewardPool();
    Hard_LoadMap(1);

    int talentX = 304;
    int talentY = 600;
    int talentW = 190;
    int talentH = 285;
    if (g_talentLoaded) {
        int imgW = g_talentSmall.getwidth();
        int imgH = g_talentSmall.getheight();
        talentW = imgW; talentH = imgH;
        g_talentBtn = { talentX, talentY, talentX + imgW, talentY + imgH };
        int btnX = talentX + imgW + 30;
        int btnY = talentY;
        for (int i = 0; i < 3; i++) {
            g_rewardSkillBtns[i] = { btnX + i * (imgW + 20), btnY, btnX + i * (imgW + 20) + imgW, btnY + imgH };
            g_rewardSkillAvailable[i] = false;
        }
    }
    else {
        g_talentBtn = { talentX, talentY, talentX + talentW, talentY + talentH };
        int btnX = talentX + talentW + 30;
        int btnY = talentY;
        for (int i = 0; i < 3; i++) {
            g_rewardSkillBtns[i] = { btnX + i * (talentW + 20), btnY, btnX + i * (talentW + 20) + talentW, btnY + talentH };
            g_rewardSkillAvailable[i] = false;
        }
    }
}

void Hard_ClearAllProjectiles() {
    g_balls.clear();
    g_arrows.clear();
    g_waves.clear();
    g_spiritStones.clear();
    g_iceBalls.clear();
    g_slashes.clear();
    g_iceClaws.clear();
    g_fireBalls.clear();
    g_explosions.clear();
    g_spears.clear();
    g_spearLands.clear();
}

void CheckPlayerDeath() {
    if (g_game_playerHP <= 0) {
        if (g_reviveAvailable) {
            g_game_playerHP = 300;
            g_reviveAvailable = false;
            g_bottomYellow = false;
            PlayMusic("revive.mp3", "revive", false);
        }
        else {
            g_hardState = HARD_DEFEAT;
        }
    }
}

// ------------------- Boss技能新实现 -------------------
void Boss_StartSkill1() {
    if (g_bossMonster) g_bossMonster->active = true;
    if (!g_bossMonster || g_bossMonster->hp <= 0) return;
    g_bossSkillState = BOSS_SKILL1;
    g_bossSkillTimer = 10.0f;
    g_bossReducedDamage = true;
    g_thunderClouds.clear();
    g_marks.clear();
    g_spears.clear();
    g_spearLands.clear();
    float cloudY = ROAD_Y[0] - 150 - 60; // 云高60，使云中心在道路上150px处
    ThunderCloud cloud1, cloud2;
    cloud1.active = true;
    cloud1.x = 200;
    cloud1.y = cloudY;
    cloud1.speedX = 100;
    cloud2.active = true;
    cloud2.x = WIN_WIDTH - 200;
    cloud2.y = cloudY;
    cloud2.speedX = -100;
    g_thunderClouds.push_back(cloud1);
    g_thunderClouds.push_back(cloud2);
}
void Boss_StartSkill2() {
    if (!g_bossMonster || g_bossMonster->hp <= 0) return;
    g_bossSkillState = BOSS_SKILL2;
    g_bossInvincible = true;
    g_bossReducedDamage = false;
    g_bossMonster->active = false;
    g_bossClones.clear();
    int roads[2];
    roads[0] = rand() % 3;
    do { roads[1] = rand() % 3; } while (roads[1] == roads[0]);
    for (int i = 0; i < 2; i++) {
        BossClone clone;
        clone.road = roads[i];
        clone.x = WIN_WIDTH - BOSS_SMALL_W;
        clone.y = ROAD_Y[clone.road] - BOSS_SMALL_H;
        clone.hasLeftSpear = true;
        clone.hasRightSpear = false;
        clone.active = true;
        g_bossClones.push_back(clone);
    }
    g_bossSkill2Phase = 0;
    g_bossSkill2MoveTimer = 0.0f;
}
void Boss_StartSkill3() {
    PlayMusic("boss07.mp3", "boss_skill3", false);
    if (!g_bossMonster || g_bossMonster->hp <= 0) return;
    g_bossSkillState = BOSS_SKILL3;
    g_skill3Active = true;
    g_skill3Timer = 5.0f;
    g_skill3DamageAcc = 0.0f;
    g_playerControlDisabled = true;   // 禁用玩家控制

    // 确保 boss01 可见且静止
    g_bossMonster->active = true;

    // 生成 boss02：玩家右侧 50 像素，与玩家同道路
    g_boss02.active = true;
    g_boss02.x = g_game_playerX + PLAYER_W + 50;
    g_boss02.y = g_game_playerY;
    if (g_boss02.x + PLAYER_W > WIN_WIDTH) g_boss02.x = WIN_WIDTH - PLAYER_W;

    // 生成三个灯：玩家左侧20、上方20、下方20
    g_lamps.clear();
    Lamp lampLeft, lampUp, lampDown;
    lampLeft.active = true;
    lampLeft.x = g_game_playerX - 40;
    lampLeft.y = g_game_playerY + PLAYER_H / 2 - 40;
    lampUp.active = true;
    lampUp.x = g_game_playerX + PLAYER_W / 2 - 40;
    lampUp.y = g_game_playerY - 40;
    lampDown.active = true;
    lampDown.x = g_game_playerX + PLAYER_W / 2 - 40;
    lampDown.y = g_game_playerY + PLAYER_H;
    g_lamps.push_back(lampLeft);
    g_lamps.push_back(lampUp);
    g_lamps.push_back(lampDown);
}

void Boss_ResetToNormal() {
    if (!g_bossMonster) return;
    g_bossMonster->active = true;
    g_bossMonster->x = 1280.0f;
    g_bossMonster->y = 280.0f;
    g_bossClones.clear();
    g_bossInvincible = false;
    g_bossReducedDamage = false;
    g_bossSkillState = BOSS_IDLE;
    g_thunderClouds.clear();
    g_marks.clear();
    g_spears.clear();
    g_spearLands.clear();
}

void Boss_UpdateSkill1(float delta) {
    if (g_bossSkillState != BOSS_SKILL1) return;
    g_bossSkillTimer -= delta;
    if (g_bossSkillTimer <= 0.0f) {
        g_bossSkillState = BOSS_SKILL1_WAIT;
        g_bossSkillTimer = 5.0f;
        g_bossReducedDamage = false;
        g_thunderClouds.clear();
        g_marks.clear();
        g_spears.clear();
        g_spearLands.clear();
        return;
    }

    for (auto& cloud : g_thunderClouds) {
        cloud.x += cloud.speedX * delta;
    }

    static float markGenTimer = 0.0f;
    markGenTimer += delta;
    if (markGenTimer >= 0.5f) {
        markGenTimer = 0.0f;
        Mark m;
        m.road = rand() % 3;
        float markX = 300 + (rand() % (WIN_WIDTH - 600));
        m.x = markX;
        m.y = ROAD_Y[m.road] - 25;
        m.life = 1.0f;
        m.active = true;
        g_marks.push_back(m);
    }

    for (size_t i = 0; i < g_marks.size(); ) {
        Mark& mk = g_marks[i];
        mk.life -= delta;
        if (mk.life <= 0.0f) {
            if (!g_thunderClouds.empty()) {
                ThunderCloud* nearest = &g_thunderClouds[0];
                float minDist = fabs(nearest->x - mk.x);
                for (auto& cloud : g_thunderClouds) {
                    float d = fabs(cloud.x - mk.x);
                    if (d < minDist) { minDist = d; nearest = &cloud; }
                }
                Spear spear;
                spear.active = true;
                spear.isFalling = true;
                spear.x = nearest->x + 50;
                spear.y = nearest->y + 30;
                spear.targetX = mk.x;
                spear.targetY = mk.y;
                float travelTime = 1.0f;
                spear.speedX = (spear.targetX - spear.x) / travelTime;
                spear.speedY = (spear.targetY - spear.y) / travelTime;
                spear.travelTimeLeft = travelTime;
                g_spears.push_back(spear);
            }
            mk.active = false;
            g_marks.erase(g_marks.begin() + i);
            continue;
        }
        ++i;
    }

    for (size_t i = 0; i < g_spears.size(); ) {
        Spear& sp = g_spears[i];
        if (sp.isFalling) {
            sp.x += sp.speedX * delta;
            sp.y += sp.speedY * delta;
            sp.travelTimeLeft -= delta;
            if (sp.travelTimeLeft <= 0.0f) {
                sp.isFalling = false;
                SpearLand land;
                land.active = true;
                land.x = sp.targetX - 10;
                land.y = sp.targetY - 80;
                land.timer = 2.0f;
                g_spearLands.push_back(land);
                float playerCenterX = g_game_playerX + PLAYER_W / 2;
                if (fabs(playerCenterX - sp.targetX) <= 50.0f) {
                    if (g_invincibleTime <= 0) {
                        g_game_playerHP -= 200;
                        PlayMusic("hit.mp3", "hit", false);
                        CheckPlayerDeath();
                    }
                }
                g_spears.erase(g_spears.begin() + i);
                continue;
            }
        }
        ++i;
    }

    for (size_t i = 0; i < g_spearLands.size(); ) {
        g_spearLands[i].timer -= delta;
        if (g_spearLands[i].timer <= 0.0f) {
            g_spearLands.erase(g_spearLands.begin() + i);
        }
        else ++i;
    }
}

void Boss_UpdateSkill2(float delta) {
    if (g_bossSkillState != BOSS_SKILL2) return;
    const float moveSpeed = 1000.0f;
    static bool phaseDamageDone = false;

    if (g_bossClones.empty()) {
        Boss_ResetToNormal();
        g_bossSkillState = BOSS_SKILL2_WAIT;
        g_bossSkillTimer = 5.0f;
        return;
    }

    if (!phaseDamageDone) {
        for (auto& clone : g_bossClones) {
            if (!clone.active) continue;
            if (clone.hasLeftSpear) {
                float spearX = clone.x - 80;
                float spearY = clone.y + BOSS_SMALL_H / 2 - 5;
                if (g_game_playerX + PLAYER_W > spearX && g_game_playerX < spearX + 80 &&
                    g_game_playerY + PLAYER_H > spearY && g_game_playerY < spearY + 10) {
                    if (g_invincibleTime <= 0) {
                        g_game_playerHP -= 200;
                        PlayMusic("hit.mp3", "hit", false);
                        CheckPlayerDeath();
                    }
                    phaseDamageDone = true;
                    break;
                }
            }
            if (clone.hasRightSpear) {
                float spearX = clone.x + BOSS_SMALL_W;
                float spearY = clone.y + BOSS_SMALL_H / 2 - 5;
                if (g_game_playerX + PLAYER_W > spearX && g_game_playerX < spearX + 80 &&
                    g_game_playerY + PLAYER_H > spearY && g_game_playerY < spearY + 10) {
                    if (g_invincibleTime <= 0) {
                        g_game_playerHP -= 200;
                        PlayMusic("hit.mp3", "hit", false);
                        CheckPlayerDeath();
                    }
                    phaseDamageDone = true;
                    break;
                }
            }
        }
    }

    bool allReached = true;
    float step = moveSpeed * delta;
    float targetX;
    if (g_bossSkill2Phase == 0 || g_bossSkill2Phase == 2)
        targetX = 0;
    else
        targetX = WIN_WIDTH - BOSS_SMALL_W;

    for (auto& clone : g_bossClones) {
        if (!clone.active) continue;
        if (g_bossSkill2Phase == 0 || g_bossSkill2Phase == 2) {
            clone.x -= step;
            if (clone.x > targetX) allReached = false;
            if (clone.x < targetX) clone.x = targetX;
        }
        else {
            clone.x += step;
            if (clone.x < targetX) allReached = false;
            if (clone.x > targetX) clone.x = targetX;
        }
        clone.y = ROAD_Y[clone.road] - BOSS_SMALL_H;
    }

    if (allReached) {
        g_bossSkill2Phase++;
        phaseDamageDone = false;
        if (g_bossSkill2Phase == 1 || g_bossSkill2Phase == 3) {
            for (auto& clone : g_bossClones) {
                clone.hasLeftSpear = false;
                clone.hasRightSpear = true;
                int newRoad;
                do { newRoad = rand() % 3; } while (newRoad == clone.road);
                clone.road = newRoad;
            }
        }
        else if (g_bossSkill2Phase == 2) {
            for (auto& clone : g_bossClones) {
                clone.hasLeftSpear = true;
                clone.hasRightSpear = false;
                int newRoad;
                do { newRoad = rand() % 3; } while (newRoad == clone.road);
                clone.road = newRoad;
            }
        }
        else if (g_bossSkill2Phase == 4) {
            Boss_ResetToNormal();
            g_bossSkillState = BOSS_SKILL2_WAIT;
            g_bossSkillTimer = 5.0f;
            return;
        }
    }
}
void Boss_UpdateSkill3(float delta) {
    if (g_bossSkillState != BOSS_SKILL3) return;

    g_skill3Timer -= delta;
    // 每秒扣血50点
    g_skill3DamageAcc += delta;
    if (g_skill3DamageAcc >= 1.0f) {
        g_skill3DamageAcc = 0.0f;
        g_game_playerHP -= 50;
        PlayMusic("hit.mp3", "hit", false);
        CheckPlayerDeath();
    }

    // 技能结束
    if (g_skill3Timer <= 0.0f) {
        // 清除 boss02 和灯
        g_boss02.active = false;
        g_lamps.clear();
        g_skill3Active = false;
        g_playerControlDisabled = false;   // 恢复玩家控制

        // 进入等待10秒状态
        g_bossSkillState = BOSS_SKILL3_WAIT;
        g_bossSkillTimer = 10.0f;
    }
}

void Boss_Update(float delta) {
    if (!g_bossMonster || g_bossMonster->hp <= 0) {
        if (g_bossSkillState != BOSS_IDLE) {
            g_bossSkillState = BOSS_IDLE;
            g_bossReducedDamage = false;
            g_bossInvincible = false;
            g_thunderClouds.clear();
            g_marks.clear();
            g_spears.clear();
            g_spearLands.clear();
            g_bossClones.clear();
        }
        return;
    }

    switch (g_bossSkillState) {
    case BOSS_SKILL1: Boss_UpdateSkill1(delta); break;
    case BOSS_SKILL1_WAIT:
        g_bossSkillTimer -= delta;
        if (g_bossSkillTimer <= 0.0f) Boss_StartSkill2();
        break;
    case BOSS_SKILL2: Boss_UpdateSkill2(delta); break;
    case BOSS_SKILL2_WAIT:
        g_bossSkillTimer -= delta;
        if (g_bossSkillTimer <= 0.0f) Boss_StartSkill3();
        break;
    case BOSS_SKILL3: Boss_UpdateSkill3(delta); break;
    case BOSS_SKILL3_WAIT:
        g_bossSkillTimer -= delta;
        if (g_bossSkillTimer <= 0.0f) Boss_StartSkill1();
        break;
    default: break;
    }
}

void Boss_TryStartSkill() {
    if (!g_bossMonster || g_bossMonster->hp <= 0) return;
    if (g_bossSkillState == BOSS_IDLE) {
        Boss_StartSkill1();
    }
}

// ------------------- Hard_Update 核心修改 -------------------
void Hard_Update(float delta) {
    // 技能效果持续时间更新
    if (g_skillLeftEffectTime > 0) {
        g_skillLeftEffectTime -= delta;
        if (g_skillLeftEffectTime <= 0) { g_skillLeftEffectTime = 0; g_rangeBonus = 0; }
    }
    if (g_skillLeftUpEffectTime > 0) {
        g_skillLeftUpEffectTime -= delta;
        if (g_skillLeftUpEffectTime <= 0) { g_skillLeftUpEffectTime = 0; g_attackBonus = 0; }
    }
    if (g_rageEffectTime > 0) g_rageEffectTime -= delta;
    if (g_redeemEffectTime > 0) g_redeemEffectTime -= delta;
    if (g_invincibleTime > 0) {
        g_invincibleTime -= delta;
        if (g_invincibleTime < 0) g_invincibleTime = 0;
    }
    if (g_talentSilenceTime > 0) {
        g_talentSilenceTime -= delta;
        if (g_talentSilenceTime <= 0) {
            g_talentSilenceTime = 0;
            g_talentAttackBonus = 0;
        }
    }
    if (g_reward2EffectTime > 0) {
        g_reward2EffectTime -= delta;
        if (g_reward2EffectTime <= 0) {
            g_reward2Active = false;
            g_currentPlayerSpeed = PLAYER_SPEED_X;
            g_arrowShotCount = 0;
        }
    }
    if (g_slowEffectTime > 0) {
        g_slowEffectTime -= delta;
        if (g_slowEffectTime <= 0) {
            g_isSlowed = false;
            g_currentPlayerSpeed = PLAYER_SPEED_X;
        }
    }

    // 奖励技能持续时间更新
    if (g_reward01Timer > 0) {
        g_reward01Timer -= delta;
        if (g_reward01Timer <= 0) g_spiritPets.clear();
    }
    if (g_reward02Timer > 0) {
        g_reward02Timer -= delta;
        if (g_reward02Timer <= 0) {
            g_reward2Active = false;
            g_currentPlayerSpeed = PLAYER_SPEED_X;
        }
    }
    if (g_reward03Timer > 0) {
        g_reward03Timer -= delta;
        if (g_reward03Timer <= 0) {
            g_swordRains.clear();
            g_bigSwordTimer = 0;
        }
    }
    if (g_reward04Timer > 0) {
        g_reward04Timer -= delta;
        if (g_reward04Timer <= 0) {
            g_bottomYellow = false;
            g_reward04Active = false;
        }
        else {
            g_reward04Active = true;
            g_reward04HealAcc += delta;
            if (g_reward04HealAcc >= 1.0f) {
                g_reward04HealAcc = 0;
                g_game_playerHP += 50;
                if (g_game_playerHP > 500 + 500) g_game_playerHP = 500 + 500;
            }
        }
    }
    if (g_reward05Timer > 0) {
        g_reward05Timer -= delta;
        if (g_reward05Timer <= 0) g_invincibleTime = 0;
        else g_invincibleTime = 20.0f;
    }

    // 冷却更新
    static float talentAcc = 0;
    talentAcc += delta;
    if (talentAcc >= 1.0f) {
        talentAcc -= 1.0f;
        if (g_talentCooldown > 0) g_talentCooldown--;
    }
    if (g_skillLeftCooldown > 0) g_skillLeftCooldown -= delta;
    if (g_skillLeftUpCooldown > 0) g_skillLeftUpCooldown -= delta;
    for (int i = 0; i < 3; i++) if (g_smallCooldown[i] > 0) g_smallCooldown[i] -= delta;
    if (g_skillLeftCooldown < 0) g_skillLeftCooldown = 0;
    if (g_skillLeftUpCooldown < 0) g_skillLeftUpCooldown = 0;
    for (int i = 0; i < 3; i++) if (g_smallCooldown[i] < 0) g_smallCooldown[i] = 0;
    if (g_summonCooldown > 0) g_summonCooldown -= delta;

    if (g_showRange && (float)GetTickCount() / 1000.0f >= g_rangeEndTime) g_showRange = false;

    if (g_hardState != HARD_PLAYING || g_hardPaused) return;

    float now = (float)GetTickCount() / 1000.0f;
    if (now - g_lastSecond >= 1.0f) {
        g_lastSecond = now; g_timeLeft--;
        if (g_timeLeft <= 0) { Hard_TimeOut(); return; }
    }

    if (!g_ultLockMove && !g_playerControlDisabled) {
        if (KEY_DOWN('A')) {
            g_game_playerX -= g_currentPlayerSpeed * delta;
            g_playerFacingLeft = true;
            if (g_game_playerX < 0) g_game_playerX = 0;
        }
        if (KEY_DOWN('D')) {
            g_game_playerX += g_currentPlayerSpeed * delta;
            g_playerFacingLeft = false;
            if (g_game_playerX > WIN_WIDTH - PLAYER_W) g_game_playerX = WIN_WIDTH - PLAYER_W;
        }
    }
    if (g_currentStage <= 3 && g_doorActive && g_chestClicked) {
        int doorX = WIN_WIDTH - 95;
        if (g_game_playerX + PLAYER_W > doorX) {
            g_doorActive = false;
            Hard_AdvanceToNextStage();
        }
    }

    static int lastW = 0, lastS = 0;
    if (!g_playerControlDisabled) {
        if (KEY_DOWN('W') && !lastW && g_game_curRoad > 0) { g_game_curRoad--; g_game_playerY = ROAD_Y[g_game_curRoad] - PLAYER_H; }
        if (KEY_DOWN('S') && !lastS && g_game_curRoad < 2) { g_game_curRoad++; g_game_playerY = ROAD_Y[g_game_curRoad] - PLAYER_H; }
    }
    lastW = KEY_DOWN('W'); lastS = KEY_DOWN('S');

    // 怪物射击（小怪、中怪）
    for (auto& mon : g_hardMonsters) {
        if (mon.hp <= 0) continue;
        if (g_talentSilenceTime > 0) continue;
        float interval = 0.0f; int damage = 0; bool isFire = false;
        if (mon.type == 0) { damage = 20; interval = 2.0f; isFire = false; }
        else if (mon.type == 3) { damage = 30; interval = 2.5f; isFire = true; }
        else continue;
        mon.shootTimer += delta;
        if (mon.shootTimer >= interval) {
            mon.shootTimer = 0;
            Ball b;
            b.size = BALL_SIZE;
            b.x = mon.x + ENEMY_W / 2 - BALL_SIZE / 2;
            b.y = mon.y + ENEMY_H / 2 - BALL_SIZE / 2;
            b.damage = damage;
            b.isFire = isFire;
            g_balls.push_back(b);
        }
    }

    // 子弹移动
    float ballStep = BALL_SPEED * delta;
    for (auto& ball : g_balls) ball.x -= ballStep;
    g_balls.erase(std::remove_if(g_balls.begin(), g_balls.end(),
        [](const Ball& b) { return b.x + BALL_SIZE < 0 || b.x > WIN_WIDTH; }), g_balls.end());

    // 近战士召唤（怪大怪01）
    if (g_meleeMinions.empty() && g_summonCooldown <= 0) {
        bool bigExists = false;
        for (auto& mon : g_hardMonsters) {
            if (mon.type == 1 && mon.imgIdx == 0 && mon.hp > 0) {
                bigExists = true;
                break;
            }
        }
        if (bigExists) {
            if (g_summonTimer > 0) {
                g_summonTimer -= delta;
                if (g_summonTimer <= 0) {
                    MeleeMinion m;
                    m.active = true;
                    m.road = g_game_curRoad;
                    m.x = g_game_playerX + PLAYER_W + 57;
                    float targetY = ROAD_Y[m.road] - 76;
                    m.y = targetY + 76;
                    m.riseOffset = 76;
                    m.hp = 1000;
                    m.moveSpeed = 19.0f;
                    m.attackTimer = 0;
                    m.imgIdx = rand() % 3;
                    g_meleeMinions.push_back(m);
                    g_summonCooldown = 5.0f;
                }
            }
            else {
                MeleeMinion m;
                m.active = true;
                m.road = g_game_curRoad;
                m.x = g_game_playerX + PLAYER_W + 57;
                float targetY = ROAD_Y[m.road] - 76;
                m.y = targetY + 76;
                m.riseOffset = 76;
                m.hp = 1000;
                m.moveSpeed = 19.0f;
                m.attackTimer = 0;
                m.imgIdx = rand() % 3;
                g_meleeMinions.push_back(m);
                g_summonCooldown = 5.0f;
            }
        }
    }

    // 近战士更新
    for (auto& m : g_meleeMinions) {
        if (!m.active) continue;
        if (m.riseOffset > 0) {
            m.riseOffset -= delta * 152;
            if (m.riseOffset < 0) m.riseOffset = 0;
            float targetY = ROAD_Y[m.road] - 76;
            m.y = targetY + m.riseOffset;
        }
        if (m.road != g_game_curRoad) {
            m.road = g_game_curRoad;
            float targetY = ROAD_Y[m.road] - 76;
            m.y = targetY + m.riseOffset;
        }
        m.x -= m.moveSpeed * delta;
        m.attackTimer += delta;
        if (m.attackTimer >= 1.5f) {
            m.attackTimer = 0;
            Slash s;
            s.active = true;
            s.x = m.x;
            s.y = m.y + 38;
            s.damage = 80;
            s.speedX = -1140.0f;
            s.traveledDist = 0;
            g_slashes.push_back(s);
        }
        if (m.x + 76 < 0 || m.hp <= 0) {
            m.active = false;
            g_summonCooldown = 5.0f;
        }
    }
    g_meleeMinions.erase(std::remove_if(g_meleeMinions.begin(), g_meleeMinions.end(),
        [](const MeleeMinion& m) { return !m.active; }), g_meleeMinions.end());

    // 斩击更新
    for (auto& s : g_slashes) {
        s.x += s.speedX * delta;
        s.traveledDist += fabs(s.speedX) * delta;
        if (s.x < 0 || s.traveledDist >= 380) s.active = false;
    }
    g_slashes.erase(std::remove_if(g_slashes.begin(), g_slashes.end(),
        [](const Slash& s) { return !s.active; }), g_slashes.end());
    for (auto& s : g_slashes) {
        if (s.x < g_game_playerX + PLAYER_W && s.x + 57 > g_game_playerX &&
            s.y < g_game_playerY + PLAYER_H && s.y + 57 > g_game_playerY) {
            if (g_invincibleTime <= 0) {
                g_game_playerHP -= s.damage;
                PlayMusic("hit.mp3", "hit", false);
                CheckPlayerDeath();
            }
            s.active = false;
        }
    }

    // 冰球发射（怪大怪01）
    static float iceBallTimer = 0;
    iceBallTimer += delta;
    if (iceBallTimer >= 5.0f) {
        iceBallTimer = 0;
        for (auto& mon : g_hardMonsters) {
            if (mon.type == 1 && mon.imgIdx == 0 && mon.hp > 0) {
                IceBall b;
                b.active = true;
                b.x = mon.x;
                b.y = mon.y + ENEMY_H / 2 - 19;
                b.damage = 30;
                b.speedX = -100.0f;
                g_iceBalls.push_back(b);
            }
        }
    }
    for (auto& b : g_iceBalls) {
        b.x += b.speedX * delta;
        if (b.x + 38 < 0 || b.x > WIN_WIDTH) b.active = false;
    }
    g_iceBalls.erase(std::remove_if(g_iceBalls.begin(), g_iceBalls.end(),
        [](const IceBall& b) { return !b.active; }), g_iceBalls.end());
    for (size_t i = 0; i < g_iceBalls.size(); ) {
        IceBall& b = g_iceBalls[i];
        if (b.x < g_game_playerX + PLAYER_W && b.x + 38 > g_game_playerX &&
            b.y < g_game_playerY + PLAYER_H && b.y + 38 > g_game_playerY) {
            if (g_invincibleTime <= 0) {
                g_game_playerHP -= b.damage;
                PlayMusic("hit.mp3", "hit", false);
                CheckPlayerDeath();
            }
            g_iceBalls.erase(g_iceBalls.begin() + i);
            continue;
        }
        ++i;
    }

    // 怪大怪02召唤冰怪
    if (g_iceMinionSummonTimer > 0) g_iceMinionSummonTimer -= delta;
    bool bigExists02 = false;
    for (auto& mon : g_hardMonsters) {
        if (mon.type == 1 && mon.imgIdx == 1 && mon.hp > 0) {
            bigExists02 = true;
            break;
        }
    }
    if (bigExists02 && g_iceMinionSummonTimer <= 0) {
        g_iceMinionSummonTimer = 5.0f;
        IceMinion ice;
        ice.active = true;
        ice.road = g_game_curRoad;
        ice.x = g_game_playerX + PLAYER_W + 50;
        ice.y = ROAD_Y[ice.road] - 76;
        ice.hp = 200;
        ice.moveSpeed = 50.0f;
        ice.attackTimer = 0;
        ice.imgIdx = rand() % 3;
        ice.facingRight = false;
        g_iceMinions.push_back(ice);
    }

    // 冰怪更新
    for (auto& ice : g_iceMinions) {
        if (!ice.active) continue;
        if (ice.x + 38 < g_game_playerX) ice.facingRight = true;
        else if (ice.x > g_game_playerX + PLAYER_W) ice.facingRight = false;
        float move = ice.moveSpeed * delta;
        if (ice.facingRight) ice.x += move;
        else ice.x -= move;
        if (ice.road != g_game_curRoad) {
            ice.road = g_game_curRoad;
            ice.y = ROAD_Y[ice.road] - 76;
        }
        ice.attackTimer += delta;
        if (ice.attackTimer >= ICE_ATTACK_INTERVAL) {
            ice.attackTimer = 0;
            IceClaw claw;
            claw.active = true;
            claw.x = ice.x + 38;
            claw.y = ice.y + 38;
            claw.damage = 50;
            claw.speedX = ice.facingRight ? 100.0f : -100.0f;
            claw.traveledDist = 0;
            g_iceClaws.push_back(claw);
        }
        if (ice.x + 76 < 0 || ice.x > WIN_WIDTH || ice.hp <= 0) ice.active = false;
    }
    g_iceMinions.erase(std::remove_if(g_iceMinions.begin(), g_iceMinions.end(),
        [](const IceMinion& i) { return !i.active; }), g_iceMinions.end());

    // 冰爪更新
    for (auto& claw : g_iceClaws) {
        claw.x += claw.speedX * delta;
        claw.traveledDist += fabs(claw.speedX) * delta;
        if (claw.traveledDist >= 50) claw.active = false;
    }
    g_iceClaws.erase(std::remove_if(g_iceClaws.begin(), g_iceClaws.end(),
        [](const IceClaw& c) { return !c.active; }), g_iceClaws.end());
    for (auto& claw : g_iceClaws) {
        if (claw.x < g_game_playerX + PLAYER_W && claw.x + 57 > g_game_playerX &&
            claw.y < g_game_playerY + PLAYER_H && claw.y + 57 > g_game_playerY) {
            if (g_invincibleTime <= 0) {
                g_game_playerHP -= claw.damage;
                PlayMusic("hit.mp3", "hit", false);
                g_slowEffectTime = 10.0f;
                g_isSlowed = true;
                g_currentPlayerSpeed = PLAYER_SPEED_X * 0.9f;
            }
            claw.active = false;
            break;
        }
    }

    // 怪大怪03 攻击逻辑
    bool big03Exists = false;
    for (auto& mon : g_hardMonsters) {
        if (mon.type == 1 && mon.imgIdx == 2 && mon.hp > 0) {
            big03Exists = true;
            break;
        }
    }
    if (big03Exists) {
        if (g_flameSwitchTimer <= 0) {
            g_flameSwitchTimer = 5.0f;
            g_flameAreas.clear();
            FlameArea flameUp, flameDown;
            flameUp.road = 0; flameUp.x = 1280 - 300; flameUp.y = ROAD_Y[0] - ENEMY_H; flameUp.width = 300; flameUp.height = ENEMY_H; flameUp.active = true;
            flameDown.road = 2; flameDown.x = 1280 - 300; flameDown.y = ROAD_Y[2] - ENEMY_H; flameDown.width = 300; flameDown.height = ENEMY_H; flameDown.active = true;
            g_flameAreas.push_back(flameUp);
            g_flameAreas.push_back(flameDown);
        }
        else g_flameSwitchTimer -= delta;

        bool inFlame = false;
        for (auto& f : g_flameAreas) {
            if (f.active && g_game_playerX + PLAYER_W > f.x && g_game_playerX < f.x + f.width &&
                g_game_playerY + PLAYER_H > f.y && g_game_playerY < f.y + f.height) inFlame = true;
        }
        if (inFlame) {
            if (!g_isInFlame) { g_isInFlame = true; g_flameDamageAcc = 0; g_flameDamageTotal = 0; }
            g_flameDamageAcc += delta;
            if (g_flameDamageAcc >= 0.5f) {
                g_flameDamageAcc = 0;
                int damageThisTick = 20;
                if (g_flameDamageTotal + damageThisTick > 200) damageThisTick = 200 - g_flameDamageTotal;
                g_game_playerHP -= damageThisTick;
                g_flameDamageTotal += damageThisTick;
                PlayMusic("hit.mp3", "hit", false);
                CheckPlayerDeath();
                if (g_flameDamageTotal >= 200) g_isInFlame = false;
            }
        }
        else g_isInFlame = false;

        if (!g_hand.active) {
            g_hand.active = true;
            g_hand.y = ROAD_Y[0] - 150;
            g_hand.x = WIN_WIDTH / 2;
            g_hand.moveDir = 1;
            g_hand.moveRange[0] = 200;
            g_hand.moveRange[1] = WIN_WIDTH - 200;
            g_fireBallIndex = 0;
            g_fireBallCooldown = 0;
        }
        if (g_hand.active) {
            g_hand.x += g_hand.moveDir * 150 * delta;
            if (g_hand.x < g_hand.moveRange[0]) { g_hand.x = g_hand.moveRange[0]; g_hand.moveDir = 1; }
            if (g_hand.x > g_hand.moveRange[1]) { g_hand.x = g_hand.moveRange[1]; g_hand.moveDir = -1; }
            if (g_fireBallCooldown <= 0) {
                g_fireBallCooldown = 1.0f;
                int targetRoad = 1;
                float targetY = ROAD_Y[targetRoad] - ENEMY_H / 2;
                FireBall fb;
                fb.type = g_fireBallIndex % 3;
                fb.active = true;
                fb.x = g_hand.x;
                fb.y = g_hand.y;
                float offsetX = (fb.type == 0) ? -200 : ((fb.type == 1) ? -120 : -40);
                fb.targetX = g_hand.x + offsetX;
                fb.targetY = targetY;
                fb.speedX = (fb.targetX - fb.x) / 0.5f;
                fb.speedY = (fb.targetY - fb.y) / 0.5f;
                fb.life = 0.5f;
                g_fireBalls.push_back(fb);
                g_fireBallIndex++;
                if (g_fireBallIndex >= 3) g_fireBallIndex = 0;
            }
            else g_fireBallCooldown -= delta;
        }
    }
    else {
        g_flameAreas.clear();
        g_hand.active = false;
        g_fireBalls.clear();
        g_explosions.clear();
        g_isInFlame = false;
        g_flameDamageTotal = 0;
    }

    // 火球移动爆炸
    for (auto& fb : g_fireBalls) {
        fb.life -= delta;
        if (fb.life <= 0) {
            fb.active = false;
            Explosion exp;
            exp.active = true;
            exp.x = fb.targetX;
            exp.y = fb.targetY;
            exp.type = fb.type;
            exp.timer = 0;
            g_explosions.push_back(exp);
        }
        else {
            fb.x += fb.speedX * delta;
            fb.y += fb.speedY * delta;
        }
    }
    g_fireBalls.erase(std::remove_if(g_fireBalls.begin(), g_fireBalls.end(), [](const FireBall& f) { return !f.active; }), g_fireBalls.end());
    for (auto& exp : g_explosions) { exp.timer += delta; if (exp.timer >= 3.0f) exp.active = false; }
    g_explosions.erase(std::remove_if(g_explosions.begin(), g_explosions.end(), [](const Explosion& e) { return !e.active; }), g_explosions.end());

    bool inExplosion = false;
    for (auto& exp : g_explosions) {
        int expW = (exp.type == 0) ? g_smallExplodeImg.getwidth() : ((exp.type == 1) ? g_midExplodeImg.getwidth() : g_bigExplodeImg.getwidth());
        int expH = (exp.type == 0) ? g_smallExplodeImg.getheight() : ((exp.type == 1) ? g_midExplodeImg.getheight() : g_bigExplodeImg.getheight());
        if (g_game_playerX + PLAYER_W > exp.x && g_game_playerX < exp.x + expW && g_game_playerY + PLAYER_H > exp.y && g_game_playerY < exp.y + expH) inExplosion = true;
    }
    if (inExplosion) {
        if (!g_isInFire) { g_isInFire = true; g_fireDamageAcc = 0; g_fireDamageTotal = 0; }
        g_fireDamageAcc += delta;
        if (g_fireDamageAcc >= 0.5f) {
            g_fireDamageAcc = 0;
            int damageThisTick = 20;
            if (g_fireDamageTotal + damageThisTick > 200) damageThisTick = 200 - g_fireDamageTotal;
            g_game_playerHP -= damageThisTick;
            g_fireDamageTotal += damageThisTick;
            PlayMusic("hit.mp3", "hit", false);
            CheckPlayerDeath();
            if (g_fireDamageTotal >= 200) g_isInFire = false;
        }
    }
    else g_isInFire = false;

    // ---------- Boss01 技能更新（含5秒随机语音） ----------
    bool bossAlive = false;
    for (auto& mon : g_hardMonsters) {
        if (mon.type == 2 && mon.hp > 0) {
            bossAlive = true;
            if (!g_bossMonster || g_bossMonster != &mon) g_bossMonster = &mon;
            break;
        }
    }
    if (!bossAlive) {
        g_bossMonster = nullptr;
        g_bossSkillState = BOSS_IDLE;
        g_bossReducedDamage = false;
        g_bossInvincible = false;
        g_thunderClouds.clear();
        g_marks.clear();
        g_spears.clear();
        g_spearLands.clear();
        g_bossClones.clear();
    }
    else {
        Boss_Update(delta);
        static bool skillStarted = false;
        if (!skillStarted && g_bossSkillState == BOSS_IDLE && g_hardState == HARD_PLAYING) {
            Boss_TryStartSkill();
            skillStarted = true;
        }
        if (g_bossMonster->hp <= 0) skillStarted = false;

        // boss 随机语音（每5秒）
        static float bossVoiceAcc = 0.0f;
        if (!g_hardPaused) {
            bossVoiceAcc += delta;
            if (bossVoiceAcc >= 5.0f) {
                bossVoiceAcc = 0.0f;
                int idx = rand() % 5 + 1;  // 1-5
                char voiceFile[32];
                sprintf_s(voiceFile, "boss%02d.mp3", idx);
                PlayMusic(voiceFile, "boss_voice", false);
            }
        }
    }

    // 灵宝相关（保持不变）
    if (!g_spiritPets.empty()) {
        float centerX = g_game_playerX + PLAYER_W / 2;
        for (auto& pet : g_spiritPets) {
            if (!pet.active) continue;
            switch (pet.type) {
            case 0: pet.x = centerX - 38; pet.y = ROAD_Y[0] - ENEMY_H; break;
            case 1: pet.x = centerX - 38; pet.y = ROAD_Y[2] - ENEMY_H; break;
            case 2: pet.x = centerX - 95; pet.y = g_game_playerY + PLAYER_H / 2 - 38; break;
            case 3: pet.x = centerX + PLAYER_W; pet.y = g_game_playerY + PLAYER_H / 2 - 38; break;
            }
            pet.shootTimer += delta;
            if (pet.shootTimer >= 2.0f) {
                pet.shootTimer = 0;
                SpiritStone stone;
                stone.active = true;
                stone.x = pet.x + 38;
                stone.y = pet.y + 38;
                stone.damage = 20;
                stone.speedX = 95.0f;
                stone.speedY = 0;
                g_spiritStones.push_back(stone);
            }
        }
        static int lastRoad = g_game_curRoad;
        if (lastRoad != g_game_curRoad) {
            lastRoad = g_game_curRoad;
            for (auto& pet : g_spiritPets) {
                if (pet.type == 0 || pet.type == 1) pet.active = (g_game_curRoad == 1);
            }
        }
    }
    for (auto& stone : g_spiritStones) {
        stone.x += stone.speedX * delta;
        if (stone.x > WIN_WIDTH || stone.x < 0) stone.active = false;
    }
    g_spiritStones.erase(std::remove_if(g_spiritStones.begin(), g_spiritStones.end(),
        [](const SpiritStone& s) { return !s.active; }), g_spiritStones.end());
    for (size_t i = 0; i < g_spiritStones.size(); ) {
        SpiritStone& s = g_spiritStones[i];
        bool hit = false;
        for (auto& mon : g_hardMonsters) {
            if (mon.hp <= 0) continue;
            float mx = mon.x, my = mon.y;
            int mw = (mon.type == 2) ? BOSS_W : ENEMY_W;
            int mh = (mon.type == 2) ? BOSS_H : ENEMY_H;
            if (s.x < mx + mw && s.x + 38 > mx && s.y < my + mh && s.y + 38 > my) {
                mon.hp -= s.damage;
                if (mon.hp <= 0) {
                    if (mon.type == 0) g_hardScore += 20;
                    else if (mon.type == 1 || mon.type == 3) g_hardScore += 100;
                    else if (mon.type == 2) g_hardScore += 500;
                    g_game_score = g_hardScore;
                }
                hit = true;
                break;
            }
        }
        if (hit) g_spiritStones.erase(g_spiritStones.begin() + i);
        else ++i;
    }

    // 子弹击中灵宝及玩家
    for (size_t i = 0; i < g_balls.size(); ) {
        Ball& b = g_balls[i];
        bool hitPet = false;
        for (auto& pet : g_spiritPets) {
            if (!pet.active) continue;
            if (b.x < pet.x + 76 && b.x + BALL_SIZE > pet.x && b.y < pet.y + 76 && b.y + BALL_SIZE > pet.y) {
                pet.hp -= b.damage;
                if (pet.hp <= 0) pet.active = false;
                hitPet = true;
                break;
            }
        }
        if (hitPet) { g_balls.erase(g_balls.begin() + i); continue; }
        if (b.x < g_game_playerX + PLAYER_W && b.x + BALL_SIZE > g_game_playerX &&
            b.y < g_game_playerY + PLAYER_H && b.y + BALL_SIZE > g_game_playerY) {
            if (g_invincibleTime <= 0) {
                int damage = b.damage;
                if (g_reward04Active) damage -= 20;
                if (damage < 1) damage = 1;
                g_game_playerHP -= damage;
                PlayMusic("hit.mp3", "hit", false);
                CheckPlayerDeath();
            }
            g_balls.erase(g_balls.begin() + i);
            continue;
        }
        ++i;
    }

    // 海浪
    float waveStep = ARROW_SPEED * delta;
    for (auto& w : g_waves) w.x += waveStep;
    g_waves.erase(std::remove_if(g_waves.begin(), g_waves.end(),
        [](const Wave& w) { return w.x + w.w < 0 || w.x > WIN_WIDTH; }), g_waves.end());
    for (size_t i = 0; i < g_waves.size(); ) {
        Wave& w = g_waves[i];
        bool hit = false;
        for (auto& mon : g_hardMonsters) {
            if (mon.hp <= 0) continue;
            float mx = mon.x, my = mon.y;
            int mw = (mon.type == 2) ? BOSS_W : ENEMY_W;
            int mh = (mon.type == 2) ? BOSS_H : ENEMY_H;
            if (w.x < mx + mw && w.x + w.w > mx && w.y < my + mh && w.y + w.h > my) {
                mon.hp -= w.damage;
                if (mon.hp <= 0) {
                    if (mon.type == 0) g_hardScore += 20;
                    else if (mon.type == 1 || mon.type == 3) g_hardScore += 100;
                    else if (mon.type == 2) g_hardScore += 500;
                    g_game_score = g_hardScore;
                }
                hit = true;
                break;
            }
        }
        if (hit) g_waves.erase(g_waves.begin() + i);
        else ++i;
    }

    // 箭矢移动
    float arrowStep = ARROW_SPEED * delta;
    for (auto& arr : g_arrows) arr.x += arr.dir * arrowStep;
    g_arrows.erase(std::remove_if(g_arrows.begin(), g_arrows.end(),
        [](const Arrow& a) { return a.x + a.w < 0 || a.x > WIN_WIDTH; }), g_arrows.end());

    // 奖励大剑和闪电的移动和伤害
    for (auto& bs : g_bigSwords) {
        bs.x += bs.speedX * delta;
        bs.traveledDist += fabs(bs.speedX) * delta;
        if (bs.x < 0 || bs.x > WIN_WIDTH || bs.traveledDist >= 500) bs.active = false;
    }
    for (size_t i = 0; i < g_bigSwords.size(); ) {
        BigSword& bs = g_bigSwords[i];
        bool hit = false;
        for (auto& mon : g_hardMonsters) {
            if (mon.hp <= 0) continue;
            float mx = mon.x, my = mon.y;
            int mw = (mon.type == 2) ? BOSS_W : ENEMY_W;
            int mh = (mon.type == 2) ? BOSS_H : ENEMY_H;
            if (bs.x < mx + mw && bs.x + 50 > mx && bs.y < my + mh && bs.y + 30 > my) {
                mon.hp -= bs.damage;
                if (mon.hp <= 0) {
                    if (mon.type == 0) g_hardScore += 20;
                    else if (mon.type == 1 || mon.type == 3) g_hardScore += 100;
                    else if (mon.type == 2) g_hardScore += 500;
                    g_game_score = g_hardScore;
                }
                hit = true;
                break;
            }
        }
        if (hit) bs.active = false;
        if (!bs.active) g_bigSwords.erase(g_bigSwords.begin() + i);
        else ++i;
    }

    for (auto& lt : g_lightnings) {
        lt.x += lt.speedX * delta;
        if (lt.x < 0 || lt.x > WIN_WIDTH) lt.active = false;
    }
    g_lightnings.erase(std::remove_if(g_lightnings.begin(), g_lightnings.end(),
        [](const Lightning& l) { return !l.active; }), g_lightnings.end());
    for (auto& lt : g_lightnings) {
        bool hit = false;
        for (auto& mon : g_hardMonsters) {
            if (mon.hp <= 0) continue;
            float mx = mon.x, my = mon.y;
            int mw = (mon.type == 2) ? BOSS_W : ENEMY_W;
            int mh = (mon.type == 2) ? BOSS_H : ENEMY_H;
            if (lt.x < mx + mw && lt.x + 30 > mx && lt.y < my + mh && lt.y + 30 > my) {
                mon.hp -= lt.damage;
                if (mon.hp <= 0) {
                    if (mon.type == 0) g_hardScore += 20;
                    else if (mon.type == 1 || mon.type == 3) g_hardScore += 100;
                    else if (mon.type == 2) g_hardScore += 500;
                    g_game_score = g_hardScore;
                }
                lt.active = false;
                break;
            }
        }
    }

    // 箭矢碰撞（包括boss减伤和无敌）
    for (size_t i = 0; i < g_arrows.size(); ) {
        Arrow& arr = g_arrows[i];
        bool hit = false;
        for (auto& m : g_meleeMinions) {
            if (!m.active) continue;
            if (arr.x < m.x + 76 && arr.x + ARROW_W > m.x && arr.y < m.y + 76 && arr.y + ARROW_H > m.y) {
                m.hp -= arr.damage;
                if (m.hp <= 0) { m.active = false; g_summonCooldown = 5.0f; }
                hit = true; break;
            }
        }
        if (!hit) {
            for (auto& ice : g_iceMinions) {
                if (!ice.active) continue;
                if (arr.x < ice.x + 76 && arr.x + ARROW_W > ice.x && arr.y < ice.y + 76 && arr.y + ARROW_H > ice.y) {
                    ice.hp -= arr.damage;
                    if (ice.hp <= 0) ice.active = false;
                    hit = true; break;
                }
            }
        }
        if (!hit) {
            for (auto& mon : g_hardMonsters) {
                if (mon.hp <= 0) continue;
                float mx = mon.x, my = mon.y;
                int mw = (mon.type == 2) ? BOSS_W : ENEMY_W;
                int mh = (mon.type == 2) ? BOSS_H : ENEMY_H;
                if (arr.x < mx + mw && arr.x + ARROW_W > mx && arr.y < my + mh && arr.y + ARROW_H > my) {
                    int damage = arr.damage;
                    if (mon.type == 2) {
                        if (g_bossReducedDamage) damage -= g_bossDamageReduction;
                        if (g_bossInvincible) damage = 0;
                        if (damage < 1) damage = 1;
                    }
                    if (g_reward04Active && mon.type != 2) {
                        damage -= 20;
                        if (damage < 1) damage = 1;
                    }
                    mon.hp -= damage;
                    if (mon.hp < 0) mon.hp = 0;
                    hit = true;
                    if (mon.hp <= 0) {
                        if (mon.type == 0) g_hardScore += 20;
                        else if (mon.type == 1 || mon.type == 3) g_hardScore += 100;
                        else if (mon.type == 2) g_hardScore += 500;
                        g_game_score = g_hardScore;
                    }
                    break;
                }
            }
        }
        if (hit) g_arrows.erase(g_arrows.begin() + i);
        else ++i;
    }

    Hard_CheckMapClear();
    if (g_chestAnimation) Hard_UpdateChestAnimation(delta);
    CheckPlayerDeath();
}

void Hard_AdvanceToNextStage() {
    Hard_ClearAllProjectiles();
    g_currentStage++;
    if (g_currentStage > 4) {
        g_hardState = HARD_VICTORY;
        return;
    }
    g_currentMap = 0;
    Hard_LoadMap(g_currentStage);
    g_game_playerX = 0; g_game_curRoad = 0; g_game_playerY = ROAD_Y[0] - PLAYER_H;
    g_mapCleared = false; g_chestClicked = false; g_bossDefeated = false; g_doorActive = false;
    g_reward2Active = false; g_reward2EffectTime = 0; g_arrowShotCount = 0;
    g_currentPlayerSpeed = PLAYER_SPEED_X;
    g_spiritPets.clear(); g_spiritStones.clear(); g_waves.clear();
    g_iceBalls.clear(); g_meleeMinions.clear(); g_slashes.clear();
    g_talentSilenceTime = 0; g_talentAttackBonus = 0;
    g_summonCooldown = 0;
    g_summonTimer = 10.0f;
    g_iceMinionSummonTimer = 0.0f;
    g_iceMinions.clear();
    g_iceClaws.clear();
    g_slowEffectTime = 0; g_isSlowed = false;

    g_reward01Timer = 0; g_reward02Timer = 0; g_reward03Timer = 0; g_reward04Timer = 0; g_reward05Timer = 0;
    g_bigSwordTimer = 0; g_lightningTimer = 0;
    g_swordRains.clear(); g_bigSwords.clear(); g_lightnings.clear();
    g_reward04Active = false; g_skill3Active = false;
    g_playerControlDisabled = false;
    g_boss02.active = false;
    g_lamps.clear();
}

void Hard_TimeOut() { g_hardState = HARD_DEFEAT; }
void Hard_PlayerDeath() { g_hardState = HARD_WAIT_RETRY; }

void Hard_ResetCurrentMap() {
    g_victoryMusicPlayed = false;
    g_defeatMusicPlayed = false;
    Hard_ClearAllProjectiles();
    g_currentStage = 1;
    Hard_LoadMap(1);
    g_hardScore = 1000; g_timeLeft = 15 * 60;
    g_lastSecond = (float)GetTickCount() / 1000.0f;
    g_game_playerHP = 500; g_game_playerX = 0; g_game_curRoad = 0; g_game_playerY = ROAD_Y[0] - PLAYER_H;
    g_mapCleared = false; g_chestClicked = false; g_bossDefeated = false;
    g_hardState = HARD_PLAYING;
    g_rewardButtons.clear(); g_rewardIndex = 0;
    for (int i = 0; i < 3; i++) g_smallCooldown[i] = 0;
    g_rageEffectTime = g_redeemEffectTime = g_invincibleTime = 0;
    g_reward2Active = false; g_reward2EffectTime = 0; g_arrowShotCount = 0;
    g_currentPlayerSpeed = PLAYER_SPEED_X;
    g_spiritPets.clear(); g_spiritStones.clear(); g_waves.clear();
    g_iceBalls.clear(); g_meleeMinions.clear(); g_slashes.clear();
    g_talentSilenceTime = 0; g_talentAttackBonus = 0;
    g_summonCooldown = 0;
    g_summonTimer = 10.0f;
    g_iceMinionSummonTimer = 0.0f;
    g_iceMinions.clear();
    g_iceClaws.clear();
    g_slowEffectTime = 0; g_isSlowed = false;
    InitRewardPool();

    g_reward01Timer = 0; g_reward02Timer = 0; g_reward03Timer = 0; g_reward04Timer = 0; g_reward05Timer = 0;
    g_bigSwordTimer = 0; g_lightningTimer = 0;
    g_swordRains.clear(); g_bigSwords.clear(); g_lightnings.clear();
    g_reward04Active = false; g_skill3Active = false;
    g_playerControlDisabled = false;
    g_boss02.active = false;
    g_lamps.clear();

    InitRewardPool();               // 重新随机抽取三个奖励
    g_obtainedRewardCount = 0;
    for (int i = 0; i < 3; i++) {
        g_rewardSkillAvailable[i] = false;
    }
    // 重置奖励技能相关变量
    g_reward01Timer = 0; g_reward02Timer = 0; g_reward03Timer = 0;
    g_reward04Timer = 0; g_reward05Timer = 0;
    g_reward04Active = false;
    g_reward06Used = false;
    g_bigSwordTimer = 0;
    g_lightningTimer = 0;
    g_swordRains.clear();
    g_bigSwords.clear();
    g_lightnings.clear();
    // 重置技能3相关
    g_skill3Active = false;
    g_playerControlDisabled = false;
    g_boss02.active = false;
    g_lamps.clear();
}
void Hard_StartChestAnimation() { g_chestAnimation = true; g_chestAnimTimer = 0.0f; }
void Hard_UpdateChestAnimation(float delta) {
    if (!g_chestAnimation) return;
    g_chestAnimTimer += delta;
    if (g_chestAnimTimer >= 1.0f) {
        g_chestAnimation = false;
        g_showReward = true;
    }
}
void putimage_alpha_scale(int x, int y, int dstW, int dstH, IMAGE* img) {
    int srcW = img->getwidth(), srcH = img->getheight();
    if (srcW == 0 || srcH == 0) return;
    AlphaBlend(GetImageHDC(NULL), x, y, dstW, dstH, GetImageHDC(img), 0, 0, srcW, srcH, { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA });
}
void Hard_DrawChestAnimation() {
    setfillcolor(RGB(0, 0, 0));
    solidrectangle(0, 0, WIN_WIDTH, WIN_HEIGHT);
    int cx = WIN_WIDTH / 2 - 95, cy = WIN_HEIGHT / 2 - 95;
    if (g_chestAnimTimer < 0.5f) putimage_alpha_scale(cx, cy, 190, 190, &g_chestImg);
    else putimage_alpha_scale(cx, cy, 190, 190, &g_chestOpenImg);
}
void Hard_AddRewardButton(int stage, int rewardIdx) {}

void Hard_HandleKeyboard() {
    if (g_hardState != HARD_PLAYING || g_hardPaused) return;
    bool qNow = KEY_DOWN('Q');
    if (qNow && !lastQ && g_skillLeftCooldown <= 0) {
        g_skillLeftEffectTime = 3.0f; g_rangeBonus = 100; g_skillLeftCooldown = 5.0f;
        PlayMusic("技能1.mp3", "skill", false);
    }
    lastQ = qNow;
    bool eNow = KEY_DOWN('E');
    if (eNow && !lastE && g_skillLeftUpCooldown <= 0) {
        g_skillLeftUpEffectTime = 3.0f; g_attackBonus = 10; g_skillLeftUpCooldown = 5.0f;
        PlayMusic("技能2.mp3", "skill", false);
    }
    lastE = eNow;
    bool rNow = KEY_DOWN('R');
    if (rNow && !lastR && g_skillUpCooldown <= 0) {
        g_skillUpEffectTime = 10.0f; g_ultLockMove = true; g_skillUpCooldown = 20.0f;
        PlayMusic("大招.mp3", "ultimate", false);
    }
    lastR = rNow;
    if (!g_playerControlDisabled) {
        for (int i = 0; i < 3; i++) {
            int vk = '1' + i;
            bool now = KEY_DOWN(vk);
            if (now && !lastDigit[i]) {
                int cost = (i == 0 || i == 1) ? 100 : 200;
                if (g_smallCooldown[i] <= 0 && g_hardScore >= cost) {
                    g_hardScore -= cost; g_game_score = g_hardScore;
                    if (i == 0) { g_rageEffectTime = 10.0f; g_smallCooldown[i] = 30.0f; }
                    else if (i == 1) { g_game_playerHP += 100; g_redeemEffectTime = 2.0f; g_smallCooldown[i] = 30.0f; }
                    else if (i == 2) { g_invincibleTime = 5.0f; g_smallCooldown[i] = 30.0f; }
                    PlayMusic("buy.mp3", "buy", false);
                }
            }
            lastDigit[i] = now;
        }
    }
}

void Hard_Draw() {
    if (g_showTalentIntro) {
        if (!g_talentMusicPlayed) {
            PlayMusic("本命天赋.mp3", "talent_music", false);
            g_talentMusicPlayed = true;
        }
        putimage(0, 0, &g_talentIntro);
        settextcolor(WHITE); settextstyle(38, 0, "微软雅黑");
        outtextxy(WIN_WIDTH / 2 - 190, WIN_HEIGHT - 95, "点击任意位置进入游戏");
        return;
    }
    if (g_showReward) {
        setfillcolor(RGB(0, 0, 0)); solidrectangle(0, 0, WIN_WIDTH, WIN_HEIGHT);
        IMAGE* bg = &g_rewardBgImgsFull[g_rewardIdx];
        if (bg->getwidth() > 0) putimage(0, 0, bg);
        else { settextcolor(WHITE); settextstyle(57, 0, "微软雅黑"); outtextxy(WIN_WIDTH / 2 - 190, WIN_HEIGHT / 2, "奖励图片缺失"); }
        settextcolor(WHITE); settextstyle(38, 0, "微软雅黑");
        outtextxy(WIN_WIDTH / 2 - 190, WIN_HEIGHT - 152, "点击任意位置继续");
        return;
    }

    IMAGE* bg = &g_bgMap[g_currentStage - 1];
    putimage(0, 0, bg);
    if (g_currentStage <= 3 && g_chestClicked && g_doorActive) {
        int doorX = WIN_WIDTH - 95, doorY = ROAD_Y[1] - ENEMY_H;
        putimage_alpha(doorX, doorY, &g_doorImg);
    }
    if (g_currentStage <= 3 && !g_chestClicked) {
        int chestY = ROAD_Y[1] - ENEMY_H;
        putimage_alpha(WIN_WIDTH - 95, chestY, &g_chestImg);
    }
    // 火焰
    for (auto& f : g_flameAreas) if (f.active) putimage_alpha((int)f.x, (int)f.y, &g_fireImg);
    if (g_hand.active) putimage_alpha((int)g_hand.x - 25, (int)g_hand.y - 25, &g_handImg);
    for (auto& fb : g_fireBalls) {
        IMAGE* fireImg = (fb.type == 0) ? &g_smallFireBallImg : ((fb.type == 1) ? &g_midFireBallImg : &g_bigFireBallImg);
        putimage_alpha((int)fb.x - fireImg->getwidth() / 2, (int)fb.y - fireImg->getheight() / 2, fireImg);
    }
    for (auto& exp : g_explosions) {
        IMAGE* expImg = (exp.type == 0) ? &g_smallExplodeImg : ((exp.type == 1) ? &g_midExplodeImg : &g_bigExplodeImg);
        putimage_alpha((int)exp.x - expImg->getwidth() / 2, (int)exp.y - expImg->getheight() / 2, expImg);
    }
    // Boss技能绘制
    for (auto& cloud : g_thunderClouds) putimage_alpha((int)cloud.x - 50, (int)cloud.y - 30, &g_thunderCloudImg);
    for (auto& mark : g_marks) putimage_alpha((int)mark.x, (int)mark.y, &g_markImg);
    for (auto& spear : g_spears) if (spear.isFalling) putimage_alpha((int)spear.x, (int)spear.y, &g_spearFallingImg);
    for (auto& land : g_spearLands) putimage_alpha((int)land.x, (int)land.y, &g_spearDownImg);
    for (auto& clone : g_bossClones) {
        putimage_alpha((int)clone.x, (int)clone.y, &g_bossSmallImg);
        if (clone.hasLeftSpear) putimage_alpha((int)clone.x - 80, (int)clone.y + BOSS_SMALL_H / 2 - 5, &g_spearLeftImg);
        if (clone.hasRightSpear) putimage_alpha((int)clone.x + BOSS_SMALL_W, (int)clone.y + BOSS_SMALL_H / 2 - 5, &g_spearRightImg);
    }

    // 奖励技能绘制
    for (auto& rain : g_swordRains) putimage_alpha((int)rain.x, (int)rain.y, &g_faZhenImg);
    for (auto& bs : g_bigSwords) {
        IMAGE* img = (bs.speedX > 0) ? &g_bigSwordRightImg : &g_bigSwordLeftImg;
        putimage_alpha((int)bs.x, (int)bs.y, img);
    }
    for (auto& lt : g_lightnings) putimage_alpha((int)lt.x - 40, (int)lt.y - 40, &g_lightningImg);

    // 怪物绘制
    for (auto& mon : g_hardMonsters) {
        if (!mon.active || mon.hp <= 0) continue;
        IMAGE* img = nullptr;
        if (mon.type == 2) img = &g_bossImgs[mon.imgIdx];
        else if (mon.type == 1) img = &g_bigMonsterImgs[mon.imgIdx];
        else if (mon.type == 3) img = &g_mediumImgs[mon.imgIdx];
        else img = &g_smallImgs[mon.imgIdx];
        putimage_alpha((int)mon.x, (int)mon.y, img);
        int barWidth = (mon.type == 2) ? BOSS_W : ENEMY_W;
        int hpPercent = (mon.hp * 100) / mon.maxHp;
        if (mon.type == 2 && g_bossReducedDamage) setfillcolor(RGB(0, 0, 255));
        else if (g_reward04Active && mon.type != 2) setfillcolor(RGB(0, 0, 255));
        else setfillcolor(RGB(255, 0, 0));
        solidrectangle(mon.x, mon.y - 15, mon.x + barWidth, mon.y - 10);
        setfillcolor(RGB(0, 255, 0));
        solidrectangle(mon.x, mon.y - 15, mon.x + barWidth * hpPercent / 100, mon.y - 10);
    }
    // 冰怪
    for (auto& ice : g_iceMinions) {
        IMAGE* img = ice.facingRight ? &g_iceMinionRight[ice.imgIdx] : &g_iceMinionLeft[ice.imgIdx];
        putimage_alpha((int)ice.x, (int)ice.y, img);
        setfillcolor(RGB(255, 0, 0)); solidrectangle(ice.x, ice.y - 8, ice.x + 76, ice.y - 5);
        setfillcolor(RGB(0, 255, 0)); solidrectangle(ice.x, ice.y - 8, ice.x + 76 * ice.hp / 200, ice.y - 5);
    }
    for (auto& claw : g_iceClaws) {
        IMAGE* clawImg = (claw.speedX > 0) ? &g_iceClawRight : &g_iceClawLeft;
        putimage_alpha((int)claw.x, (int)claw.y, clawImg);
    }
    // 灵宝
    for (auto& pet : g_spiritPets) {
        if (!pet.active) continue;
        putimage_alpha((int)pet.x, (int)pet.y, &g_spiritPetImg);
        setfillcolor(RGB(255, 0, 0)); solidrectangle(pet.x, pet.y - 15, pet.x + 76, pet.y - 10);
        setfillcolor(RGB(0, 255, 0)); solidrectangle(pet.x, pet.y - 15, pet.x + 76 * pet.hp / 200, pet.y - 10);
    }
    // 子弹
    for (auto& ball : g_balls) {
        if (ball.isFire) {
            putimage_alpha((int)ball.x, (int)ball.y, &g_fireBallImg);
        }
        else {
            putimage_alpha((int)ball.x, (int)ball.y, &g_planetImg);
        }
    }
    for (auto& stone : g_spiritStones) putimage_alpha((int)stone.x, (int)stone.y, &g_spiritStoneImg);
    for (auto& w : g_waves) putimage_alpha((int)w.x, (int)w.y, &g_waveImg);
    for (auto& b : g_iceBalls) putimage_alpha((int)b.x, (int)b.y, &g_iceBallImg);
    for (auto& m : g_meleeMinions) {
        if (!m.active) continue;
        putimage_alpha((int)m.x, (int)m.y, &g_meleeImgs[m.imgIdx]);
        setfillcolor(RGB(255, 0, 0)); solidrectangle(m.x, m.y - 15, m.x + 76, m.y - 10);
        setfillcolor(RGB(0, 255, 0)); solidrectangle(m.x, m.y - 15, m.x + 76 * m.hp / 1000, m.y - 10);
    }
    for (auto& s : g_slashes) putimage_alpha((int)s.x, (int)s.y, &g_slashImg);
    if (g_reward04Active) putimage_alpha(g_game_playerX, g_game_playerY, &g_redeemEffect);
    // 绘制 boss02
    if (g_boss02.active) {
        putimage_alpha((int)g_boss02.x, (int)g_boss02.y, &g_boss02Img);
    }
    // 绘制灯
    for (auto& lamp : g_lamps) {
        putimage_alpha((int)lamp.x, (int)lamp.y, &g_lampImg);
    }
    IMAGE* playerImg = &g_playerImg;
    if (g_playerFacingLeft) playerImg = &g_playerImgLeft;
    if (g_isSlowed && g_slowEffectTime > 0) playerImg = &g_frozenPlayerImg;
    else if (g_reward2Active) playerImg = &g_reward2PlayerImg;
    else if (g_playerControlDisabled) playerImg = &g_suppressImg;
    if (g_invincibleTime > 0 && ((int)(GetTickCount() / 100) % 2 == 0)) {}
    else putimage_alpha((int)g_game_playerX, g_game_playerY, playerImg);
    setlinecolor(g_bottomYellow ? RGB(255, 255, 0) : RGB(255, 0, 0));
    setlinestyle(PS_SOLID, 2);
    line((int)g_game_playerX, g_game_playerY + PLAYER_H, (int)g_game_playerX + PLAYER_W, g_game_playerY + PLAYER_H);

    for (auto& arr : g_arrows) {
        bool hasBonus = (g_attackBonus > 0 || g_rageEffectTime > 0);
        IMAGE* curArrow;
        if (g_playerFacingLeft) curArrow = hasBonus ? &g_arrowImg2Left : &g_arrowImgLeft;
        else curArrow = hasBonus ? &g_arrowImg2 : &g_arrowImg;
        putimage_alpha((int)arr.x, (int)arr.y, curArrow);
    }
    if (g_showRange) {
        int bx = (int)g_game_playerX + PLAYER_W + (int)(ATTACK_RANGE + g_rangeBonus);
        int cy = g_game_playerY + PLAYER_H / 2;
        setlinecolor(RGB(0, 100, 255)); setlinestyle(PS_DASH, 1);
        line(bx, cy - 10, bx, cy + 10);
        setlinestyle(PS_SOLID, 1);
    }

    // 攻击按钮
    if (g_skillBtnImgs[3].getwidth() > 0) {
        int r = ATK_BTN_RADIUS;
        putimage_alpha(ATK_BTN_X - r, ATK_BTN_Y - r, &g_skillBtnImgs[3]);
    }
    else {
        setfillcolor(RGB(200, 80, 80));
        setlinecolor(RGB(255, 255, 255));
        fillcircle(ATK_BTN_X, ATK_BTN_Y, ATK_BTN_RADIUS);
    }
    setbkmode(TRANSPARENT);
    settextcolor(WHITE);
    settextstyle(38, 0, "微软雅黑");
    outtextxy(ATK_BTN_X - 38, ATK_BTN_Y - 19, "攻击");

    DrawSkillButton(g_skillLeftX, g_skillLeftY, g_skillLeftCooldown, "范围", 0);
    DrawSkillButton(g_skillLeftUpX, g_skillLeftUpY, g_skillLeftUpCooldown, "攻击", 1);
    for (int i = 0; i < 3; i++) DrawSmallButton(i, g_smallBtnX[i], g_smallBtnY[i], g_smallCooldown[i]);
    for (auto& btn : g_rewardButtons) putimage_alpha(btn.rect.left, btn.rect.top, btn.img);

    // 本命天赋按钮
    if (g_hardState == HARD_PLAYING || g_hardState == HARD_IDLE) {
        if (g_talentCooldown > 0) {
            setfillcolor(RGB(100, 100, 100));
            setlinecolor(RGB(100, 100, 100));
            solidrectangle(g_talentBtn.left, g_talentBtn.top, g_talentBtn.right, g_talentBtn.bottom);
            settextcolor(RGB(255, 255, 255)); settextstyle(46, 0, "微软雅黑");
            char cd[16]; sprintf_s(cd, "%d", (int)ceil(g_talentCooldown));
            int tw = textwidth(cd), th = textheight(cd);
            int cx = g_talentBtn.left + (g_talentBtn.right - g_talentBtn.left - tw) / 2;
            int cy = g_talentBtn.top + (g_talentBtn.bottom - g_talentBtn.top - th) / 2;
            outtextxy(cx, cy, cd);
        }
        else {
            putimage_alpha(g_talentBtn.left, g_talentBtn.top, &g_talentSmall);
        }
    }
    for (int i = 0; i < 3; i++) {
        if (g_rewardSkillAvailable[i]) {
            int rewardIdx = g_obtainedRewards[i];
            putimage_alpha(g_rewardSkillBtns[i].left, g_rewardSkillBtns[i].top, &g_rewardSmallImgs[rewardIdx]);
        }
    }

    setfillcolor(RGB(200, 200, 0)); setlinecolor(BLACK);
    fillrectangle(g_pauseHardBtn.left, g_pauseHardBtn.top, g_pauseHardBtn.right, g_pauseHardBtn.bottom);
    setbkmode(TRANSPARENT); settextcolor(BLACK); settextstyle(34, 0, "微软雅黑");
    outtextxy(g_pauseHardBtn.left + 38, g_pauseHardBtn.top + 19, g_hardPaused ? "继续" : "暂停");

    setfillcolor(RGB(200, 50, 50)); setlinecolor(BLACK);
    fillrectangle(g_exitHardBtn.left, g_exitHardBtn.top, g_exitHardBtn.right, g_exitHardBtn.bottom);
    setbkmode(TRANSPARENT); settextcolor(WHITE); settextstyle(34, 0, "微软雅黑");
    outtextxy(g_exitHardBtn.left + 38, g_exitHardBtn.top + 19, "结束游戏");

    if (g_hardState == HARD_IDLE) {
        setfillcolor(RGB(100, 200, 100)); setlinecolor(BLACK);
        fillrectangle(g_startHardBtn.left, g_startHardBtn.top, g_startHardBtn.right, g_startHardBtn.bottom);
        setbkmode(TRANSPARENT); settextcolor(BLACK); settextstyle(47, 0, "微软雅黑");
        outtextxy(g_startHardBtn.left + 76, g_startHardBtn.top + 19, "开始游戏");
    }

    settextcolor(WHITE); settextstyle(46, 0, "微软雅黑");
    char buf[64];
    sprintf_s(buf, "积分: %d", g_hardScore); outtextxy(10, 10, buf);
    sprintf_s(buf, "血量: %d", g_game_playerHP); outtextxy(WIN_WIDTH - 380, 10, buf);
    int minutes = g_timeLeft / 60, seconds = g_timeLeft % 60;
    sprintf_s(buf, "剩余时间: %02d:%02d", minutes, seconds);
    int tw = textwidth(buf); outtextxy((WIN_WIDTH - tw) / 2, 10, buf);
    if (g_reward04Active) {
        settextcolor(RGB(0, 255, 255));
        outtextxy(10, 100, "减伤");
    }
    if (g_reward05Timer > 0) {
        settextcolor(RGB(255, 255, 0));
        outtextxy(10, 130, "霸体");
    }

    int startX = WIN_WIDTH / 2 - 190, y = 95;
    for (int i = 1; i <= TOTAL_STAGES; i++) {
        COLORREF fill = (i < g_currentStage) ? RGB(150, 150, 150) : (i == g_currentStage ? RGB(0, 200, 0) : RGB(100, 100, 100));
        setfillcolor(fill); fillcircle(startX + (i - 1) * 114, y, 28);
        char num[4]; sprintf_s(num, "%d", i); outtextxy(startX + (i - 1) * 114 - 10, y - 15, num);
        if (i < TOTAL_STAGES) outtextxy(startX + (i - 1) * 114 + 38, y - 15, "→");
    }

    if (g_chestAnimation) Hard_DrawChestAnimation();

    if (g_hardState == HARD_WAIT_NEXT) {
        setfillcolor(RGB(0, 0, 0)); fillrectangle(0, 0, WIN_WIDTH, WIN_HEIGHT);
        settextcolor(RGB(255, 215, 0)); settextstyle(76, 0, "微软雅黑");
        outtextxy(WIN_WIDTH / 2 - 152, WIN_HEIGHT / 2 - 76, "VICTORY");
        settextstyle(38, 0, "微软雅黑"); outtextxy(WIN_WIDTH / 2 - 190, WIN_HEIGHT / 2 + 38, "点击任意位置继续");
    }
    else if (g_hardState == HARD_WAIT_RETRY) {
        setfillcolor(RGB(0, 0, 0)); fillrectangle(0, 0, WIN_WIDTH, WIN_HEIGHT);
        settextcolor(RGB(255, 0, 0)); settextstyle(76, 0, "微软雅黑");
        outtextxy(WIN_WIDTH / 2 - 152, WIN_HEIGHT / 2 - 152, "DEFEAT");
        setfillcolor(RGB(100, 200, 100)); fillrectangle(g_retryBtn.left, g_retryBtn.top, g_retryBtn.right, g_retryBtn.bottom);
        settextcolor(BLACK); outtextxy(g_retryBtn.left + 19, g_retryBtn.top + 19, "重新挑战");
        setfillcolor(RGB(200, 100, 100)); fillrectangle(g_giveUpBtn.left, g_giveUpBtn.top, g_giveUpBtn.right, g_giveUpBtn.bottom);
        outtextxy(g_giveUpBtn.left + 38, g_giveUpBtn.top + 19, "认输");
    }
    else if (g_hardState == HARD_VICTORY) {
        if (!g_victoryMusicPlayed) {
            StopMusic("game_music");        // 停止背景音乐
            PlayMusic("胜利.mp3", "result_music", false);
            g_victoryMusicPlayed = true;
        }

        putimage(0, 0, &g_victoryBg);
        // 绘制重新挑战和结束游戏按钮
        setfillcolor(RGB(100, 200, 100)); setlinecolor(BLACK);
        fillrectangle(g_hardRestartBtn.left, g_hardRestartBtn.top, g_hardRestartBtn.right, g_hardRestartBtn.bottom);
        setbkmode(TRANSPARENT); settextcolor(BLACK); settextstyle(34, 0, "微软雅黑");
        outtextxy(g_hardRestartBtn.left + 38, g_hardRestartBtn.top + 19, "重新挑战");
        setfillcolor(RGB(200, 50, 50)); setlinecolor(BLACK);
        fillrectangle(g_hardExitBtn.left, g_hardExitBtn.top, g_hardExitBtn.right, g_hardExitBtn.bottom);
        setbkmode(TRANSPARENT); settextcolor(WHITE); settextstyle(34, 0, "微软雅黑");
        outtextxy(g_hardExitBtn.left + 38, g_hardExitBtn.top + 19, "结束游戏");
    }
    else if (g_hardState == HARD_DEFEAT) {
        if (!g_defeatMusicPlayed) {
            StopMusic("game_music");        // 停止背景音乐
            PlayMusic("失败.mp3", "result_music", false);
            g_defeatMusicPlayed = true;
        }

        putimage(0, 0, &g_defeatBg);
        setfillcolor(RGB(100, 200, 100)); setlinecolor(BLACK);
        fillrectangle(g_hardRestartBtn.left, g_hardRestartBtn.top, g_hardRestartBtn.right, g_hardRestartBtn.bottom);
        setbkmode(TRANSPARENT); settextcolor(BLACK); settextstyle(34, 0, "微软雅黑");
        outtextxy(g_hardRestartBtn.left + 38, g_hardRestartBtn.top + 19, "重新挑战");
        setfillcolor(RGB(200, 50, 50)); setlinecolor(BLACK);
        fillrectangle(g_hardExitBtn.left, g_hardExitBtn.top, g_hardExitBtn.right, g_hardExitBtn.bottom);
        setbkmode(TRANSPARENT); settextcolor(WHITE); settextstyle(34, 0, "微软雅黑");
        outtextxy(g_hardExitBtn.left + 38, g_hardExitBtn.top + 19, "结束游戏");
    }
}

void Hard_HandleMouse(float currentTime) {
    if (g_showTalentIntro) {
        if (MouseHit()) {
            MOUSEMSG msg = GetMouseMsg();
            if (msg.uMsg == WM_LBUTTONDOWN) {
                g_showTalentIntro = false;
                g_talentMusicPlayed = false;
                Hard_LoadMap(1);
                g_hardState = HARD_IDLE;
                g_game_playerX = 0; g_game_curRoad = 0; g_game_playerY = ROAD_Y[0] - PLAYER_H;
                g_game_playerHP = 500; g_hardScore = 1000; g_timeLeft = 15 * 60;
                g_talentCooldown = 0;
                g_reward2Active = false; g_reward2EffectTime = 0; g_arrowShotCount = 0;
                g_currentPlayerSpeed = PLAYER_SPEED_X;
                g_spiritPets.clear(); g_spiritStones.clear(); g_waves.clear();
                g_iceBalls.clear(); g_meleeMinions.clear(); g_slashes.clear();
                g_talentSilenceTime = 0; g_talentAttackBonus = 0;
                g_summonCooldown = 0;
                g_iceMinions.clear(); g_iceClaws.clear();
                g_slowEffectTime = 0; g_isSlowed = false;
                InitRewardPool();
                g_obtainedRewardCount = 0;
                for (int i = 0; i < 3; i++) g_rewardSkillAvailable[i] = false;
                // 播放背景音乐：简单模式.mp3
                StopMusic("game_music");
                PlayMusic("简单模式.mp3", "game_music", true);
            }
        }
        return;
    }
    if (g_showReward) {
        if (MouseHit()) {
            MOUSEMSG msg = GetMouseMsg();
            if (msg.uMsg == WM_LBUTTONDOWN) {
                g_showReward = false;
                g_hardScore += 200;
                g_game_score = g_hardScore;
                int rewardIdx = g_obtainedRewards[g_obtainedRewardCount];
                // 播放奖励音效
                char rewardSound[32];
                sprintf_s(rewardSound, "奖励%02d.mp3", rewardIdx + 1);
                PlayMusic(rewardSound, "reward_sound", false);
                g_rewardSkillAvailable[g_obtainedRewardCount] = true;
                g_obtainedRewardCount++;
                g_doorActive = true;
            }
        }
        return;
    }

    static bool lastLeft = false;
    if (MouseHit()) {
        MOUSEMSG msg = GetMouseMsg();
        if (msg.uMsg == WM_LBUTTONDOWN && !lastLeft) {
            if (g_hardState == HARD_IDLE && msg.x >= g_startHardBtn.left && msg.x <= g_startHardBtn.right &&
                msg.y >= g_startHardBtn.top && msg.y <= g_startHardBtn.bottom) {
                Hard_StartGame(); return;
            }
            if (g_hardState == HARD_PLAYING) {
                if (msg.x >= g_pauseHardBtn.left && msg.x <= g_pauseHardBtn.right &&
                    msg.y >= g_pauseHardBtn.top && msg.y <= g_pauseHardBtn.bottom) {
                    g_hardPaused = !g_hardPaused;
                    if (g_hardPaused) PauseMusic("game_music");
                    else ResumeMusic("game_music");
                    return;
                }
                if (msg.x >= g_exitHardBtn.left && msg.x <= g_exitHardBtn.right &&
                    msg.y >= g_exitHardBtn.top && msg.y <= g_exitHardBtn.bottom) {
                    StopMusic("game_music"); StopMusic("result_music");
                    g_appState = STATE_DIFF_SELECT;
                    PlayMusic("05.mp3", "intro_music", true);
                    return;
                }
                if (msg.x >= g_talentBtn.left && msg.x <= g_talentBtn.right &&
                    msg.y >= g_talentBtn.top && msg.y <= g_talentBtn.bottom) {
                    if (g_talentCooldown == 0) {
                        g_talentCooldown = 30;
                        g_talentSilenceTime = 10.0f;
                        g_talentAttackBonus = 100;
                        PlayMusic("本命天赋.mp3", "talent", false);
                    }
                    return;
                }
                for (int i = 0; i < 3; i++) {
                    if (g_rewardSkillAvailable[i] && msg.x >= g_rewardSkillBtns[i].left && msg.x <= g_rewardSkillBtns[i].right &&
                        msg.y >= g_rewardSkillBtns[i].top && msg.y <= g_rewardSkillBtns[i].bottom) {
                        int rewardIdx = g_obtainedRewards[i];
                        g_rewardSkillAvailable[i] = false;
                        switch (rewardIdx) {
                        case 0:
                            g_reward01Timer = 20.0f;
                            g_spiritPets.clear();
                            for (int j = 0; j < 4; j++) {
                                SpiritPet pet;
                                pet.type = j; pet.hp = 200; pet.active = true; pet.shootTimer = 0;
                                g_spiritPets.push_back(pet);
                            }
                            break;
                        case 1:
                            g_reward02Timer = 20.0f;
                            g_reward2Active = true;
                            g_currentPlayerSpeed = PLAYER_SPEED_X * 1.5f;
                            g_arrowShotCount = 0;
                            break;
                        case 2:
                            g_reward03Timer = 8.0f;
                            g_bigSwordTimer = 8.0f;
                            g_bigSwordCount = 0;
                            g_swordRains.clear();
                            for (auto& mon : g_hardMonsters) {
                                if ((mon.type == 1 && mon.imgIdx <= 2) || mon.type == 2) {
                                    SwordRain rain;
                                    rain.active = true;
                                    rain.x = mon.x + ENEMY_W / 2 - 25;
                                    rain.y = mon.y - 200;
                                    rain.timer = 2.5f;
                                    rain.swordsLeft = 100;
                                    g_swordRains.push_back(rain);
                                }
                            }
                            break;
                        case 3:
                            g_reward04Timer = 10.0f;
                            g_reward04Active = true;
                            break;
                        case 4:
                            g_reward05Timer = 20.0f;
                            g_invincibleTime = 20.0f;
                            g_lightningCount = 0;
                            break;
                        case 5:
                            if (!g_reward06Used) {
                                g_reward06Used = true;
                                g_reviveAvailable = true;
                                g_bottomYellow = true;
                            }
                            break;
                        }
                        char rewardSound[32];
                        sprintf_s(rewardSound, "奖励%02d.mp3", rewardIdx + 1);
                        PlayMusic(rewardSound, "reward_skill", false);
                        return;
                    }
                }
            }

            int dx = msg.x - ATK_BTN_X, dy = msg.y - ATK_BTN_Y;
            if (dx * dx + dy * dy <= ATK_BTN_RADIUS * ATK_BTN_RADIUS && g_hardState == HARD_PLAYING && !g_hardPaused && !g_playerControlDisabled) {
                g_showRange = true; g_rangeEndTime = currentTime + 0.2f;
                PlayMusic("普攻.mp3", "attack", false);
                float px = g_game_playerX + PLAYER_W / 2.0f, py = g_game_playerY + PLAYER_H / 2.0f;
                float minDist = ATTACK_RANGE + g_rangeBonus + 1;
                for (auto& mon : g_hardMonsters) {
                    float ex = (mon.type == 2) ? mon.x + BOSS_W / 2 : mon.x + ENEMY_W / 2.0f;
                    float ey = (mon.type == 2) ? mon.y + BOSS_H / 2 : mon.y + ENEMY_H / 2.0f;
                    float d = sqrtf((px - ex) * (px - ex) + (py - ey) * (py - ey));
                    if (d < minDist) minDist = d;
                }
                for (auto& m : g_meleeMinions) {
                    if (!m.active) continue;
                    float ex = m.x + 38, ey = m.y + 38;
                    float d = sqrtf((px - ex) * (px - ex) + (py - ey) * (py - ey));
                    if (d < minDist) minDist = d;
                }
                for (auto& ice : g_iceMinions) {
                    if (!ice.active) continue;
                    float ex = ice.x + 38, ey = ice.y + 38;
                    float d = sqrtf((px - ex) * (px - ex) + (py - ey) * (py - ey));
                    if (d < minDist) minDist = d;
                }
                if (minDist <= ATTACK_RANGE + g_rangeBonus) {
                    int extra = g_attackBonus + (g_rageEffectTime > 0 ? 20 : 0) + g_talentAttackBonus;
                    if (g_reward2Active) {
                        g_arrowShotCount++;
                        if (g_arrowShotCount >= 3) {
                            g_arrowShotCount = 0;
                            Wave w;
                            w.x = g_game_playerX + PLAYER_W;
                            w.y = g_game_playerY + PLAYER_H / 2 - 38;
                            w.w = 152; w.h = 76;
                            w.damage = 100;
                            w.speedX = ARROW_SPEED;
                            g_waves.push_back(w);
                            PlayMusic("wave.mp3", "wave", false);
                        }
                        else {
                            Arrow arr; arr.w = ARROW_W; arr.h = ARROW_H;
                            if (g_playerFacingLeft) {
                                arr.x = g_game_playerX - ARROW_W;
                                arr.dir = -1;
                            }
                            else {
                                arr.x = g_game_playerX + PLAYER_W;
                                arr.dir = 1;
                            }
                            arr.y = g_game_playerY + PLAYER_H / 2 - ARROW_H / 2;
                            arr.damage = 20 + extra;
                            g_arrows.push_back(arr);
                        }
                    }
                    else {
                        Arrow arr; arr.w = ARROW_W; arr.h = ARROW_H;
                        if (g_playerFacingLeft) {
                            arr.x = g_game_playerX - ARROW_W;
                            arr.dir = -1;
                        }
                        else {
                            arr.x = g_game_playerX + PLAYER_W;
                            arr.dir = 1;
                        }
                        arr.y = g_game_playerY + PLAYER_H / 2 - ARROW_H / 2;
                        arr.damage = 20 + extra;
                        g_arrows.push_back(arr);
                    }
                    if (g_bigSwordTimer > 0) {
                        static int bigCount = 0;
                        bigCount++;
                        if (bigCount >= 5) {
                            bigCount = 0;
                            BigSword bs;
                            bs.active = true;
                            bs.x = g_game_playerX + (g_playerFacingLeft ? -50 : PLAYER_W);
                            bs.y = g_game_playerY + PLAYER_H / 2 - 20;
                            bs.damage = 30;
                            bs.speedX = g_playerFacingLeft ? -600 : 600;
                            bs.traveledDist = 0;
                            g_bigSwords.push_back(bs);
                        }
                    }
                    if (g_reward05Timer > 0) {
                        static int lightningCount = 0;
                        lightningCount++;
                        if (lightningCount >= 3) {
                            lightningCount = 0;
                            Lightning lt;
                            lt.active = true;
                            lt.x = g_game_playerX + (g_playerFacingLeft ? 0 : PLAYER_W);
                            lt.y = g_game_playerY + PLAYER_H / 2 - 20;
                            lt.speedX = g_playerFacingLeft ? -500 : 500;
                            lt.damage = 50;
                            g_lightnings.push_back(lt);
                        }
                    }
                }
            }
            if (g_hardState == HARD_PLAYING && !g_hardPaused && g_currentStage <= 3 && g_mapCleared && !g_chestClicked) {
                int chestX = WIN_WIDTH - 95, chestY = ROAD_Y[1] - ENEMY_H;
                if (msg.x >= chestX && msg.x <= chestX + 95 && msg.y >= chestY && msg.y <= chestY + 95) {
                    g_chestClicked = true;
                    g_rewardStage = g_currentStage;
                    g_rewardIdx = g_obtainedRewards[g_obtainedRewardCount];
                    Hard_StartChestAnimation(); PlayMusic("宝箱.mp3", "chest", false);
                }
            }
            if (g_hardState == HARD_WAIT_RETRY) {
                if (msg.x >= g_retryBtn.left && msg.x <= g_retryBtn.right && msg.y >= g_retryBtn.top && msg.y <= g_retryBtn.bottom) {
                    Hard_ResetCurrentMap();
                }
                else if (msg.x >= g_giveUpBtn.left && msg.x <= g_giveUpBtn.right && msg.y >= g_giveUpBtn.top && msg.y <= g_giveUpBtn.bottom) {
                    g_hardState = HARD_DEFEAT;
                }
                return;
            }
            if (g_hardState == HARD_WAIT_NEXT) {
                g_hardState = HARD_PLAYING;
                Hard_AdvanceToNextStage();
                return;
            }
            if (g_hardState == HARD_VICTORY) {
                if (msg.x >= g_hardRestartBtn.left && msg.x <= g_hardRestartBtn.right &&
                    msg.y >= g_hardRestartBtn.top && msg.y <= g_hardRestartBtn.bottom) {
                    StopMusic("result_music");   // 停止胜利音乐
                    g_victoryMusicPlayed = false; // 重置标志
                    Hard_ResetCurrentMap();
                }
                else if (msg.x >= g_hardExitBtn.left && msg.x <= g_hardExitBtn.right &&
                    msg.y >= g_hardExitBtn.top && msg.y <= g_hardExitBtn.bottom) {
                    StopMusic("result_music");
                    g_victoryMusicPlayed = false;
                    g_hardState = HARD_IDLE;
                    g_appState = STATE_DIFF_SELECT;
                    PlayMusic("05.mp3", "intro_music", true);

                    return;
                }
            }
            if (g_hardState == HARD_DEFEAT) {
                if (msg.x >= g_hardRestartBtn.left && msg.x <= g_hardRestartBtn.right &&
                    msg.y >= g_hardRestartBtn.top && msg.y <= g_hardRestartBtn.bottom) {
                    StopMusic("result_music");
                    g_defeatMusicPlayed = false;
                    Hard_ResetCurrentMap();
                }
                else if (msg.x >= g_hardExitBtn.left && msg.x <= g_hardExitBtn.right &&
                    msg.y >= g_hardExitBtn.top && msg.y <= g_hardExitBtn.bottom) {
                    StopMusic("result_music");
                    g_defeatMusicPlayed = false;
                    g_hardState = HARD_IDLE;
                    g_appState = STATE_DIFF_SELECT;
                    PlayMusic("05.mp3", "intro_music", true);

                    return;
                }
            }
        }
        lastLeft = (msg.uMsg == WM_LBUTTONDOWN);
    }
    else { lastLeft = false; }
}

// ==================== UI 和主循环 ====================
void InitIntroImages() {
    loadimage(&g_intro1, "01.jpg", WIN_WIDTH, WIN_HEIGHT);
    loadimage(&g_intro2, "02.jpg", WIN_WIDTH, WIN_HEIGHT);
    loadimage(&g_intro3, "03.jpg", WIN_WIDTH, WIN_HEIGHT);
    loadimage(&g_intro4, "玩法介绍.jpg", WIN_WIDTH, WIN_HEIGHT);
    loadimage(&g_bgRole, "04.jpg", WIN_WIDTH, WIN_HEIGHT);
    loadimage(&g_gameOver1, "06.jpg", WIN_WIDTH, WIN_HEIGHT);
    loadimage(&g_gameOver2, "07.jpg", WIN_WIDTH, WIN_HEIGHT);
    loadimage(&g_characterImg, "伽罗.jpg", WIN_WIDTH, WIN_HEIGHT);
}
void InitButtons() {
    int btnW = 300, btnH = 70, gap = 20, startY = 200;
    for (int i = 0; i < 5; i++) {
        Button btn;
        btn.rect.left = WIN_WIDTH / 2 - btnW / 2; btn.rect.right = WIN_WIDTH / 2 + btnW / 2;
        btn.rect.top = startY + i * (btnH + gap); btn.rect.bottom = btn.rect.top + btnH;
        const char* names[] = { "对抗路", "打野", "中路", "发育路", "辅助" };
        btn.text = names[i];
        g_roleButtons.push_back(btn);
    }
    int diffStartY = 380;
    for (int i = 0; i < 3; i++) {
        Button btn;
        btn.rect.left = WIN_WIDTH / 2 - 152; btn.rect.right = WIN_WIDTH / 2 + 152;
        btn.rect.top = diffStartY + i * 133; btn.rect.bottom = btn.rect.top + 95;
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
    int baseX = SMALL_BTN_START_X, baseY = SMALL_BTN_START_Y;
    g_smallBtnX[0] = baseX; g_smallBtnY[0] = baseY;
    g_smallBtnX[1] = baseX + SMALL_BTN_GAP_X; g_smallBtnY[1] = baseY;
    g_smallBtnX[2] = baseX; g_smallBtnY[2] = baseY + SMALL_BTN_GAP_Y;
    g_smallBtnX[3] = baseX + SMALL_BTN_GAP_X; g_smallBtnY[3] = baseY + SMALL_BTN_GAP_Y;
}
void DrawCurrentState() {
    switch (g_appState) {
    case STATE_INTRO1: putimage(0, 0, &g_intro1); break;
    case STATE_INTRO2: putimage(0, 0, &g_intro2); break;
    case STATE_INTRO3:
        if (!g_welcomePlayed) {
            PlayMusic("欢迎.mp3", "welcome", false);
            g_welcomePlayed = true;
        }
        putimage(0, 0, &g_intro3);
        break;
    case STATE_INTRO4: putimage(0, 0, &g_intro4); break;
    case STATE_ROLE_SELECT: case STATE_DIFF_SELECT: putimage(0, 0, &g_bgRole); break;
    case STATE_CHARACTER_SHOW: putimage(0, 0, &g_characterImg); break;
    case STATE_GAME: Game_Draw(); return;
    case STATE_GAME_OVER1: putimage(0, 0, &g_gameOver1); break;
    case STATE_SCORE: {
        if (!g_scoreMusicPlayed) {
            PlayMusic("胜利结算.mp3", "score_music", true);
            g_scoreMusicPlayed = true;
        }
        putimage(0, 0, &g_gameOver2);
        setbkmode(TRANSPARENT); settextcolor(RGB(255, 215, 0)); settextstyle(53, 0, "微软雅黑");
        char scoreText[128]; sprintf_s(scoreText, "玩家本局游戏得分为：%d", g_finalScore); outtextxy(285, 475, scoreText);
        int minutes = (int)(g_finalGameTime / 60), seconds = (int)g_finalGameTime % 60;
        char timeText[128]; sprintf_s(timeText, "本局对局时长为：%d分%02d秒", minutes, seconds); outtextxy(285, 532, timeText);
        const char* title;
        if (g_finalScore <= 100) title = "菜鸟"; else if (g_finalScore <= 200) title = "NPC"; else if (g_finalScore <= 400) title = "铜牌发育路";
        else if (g_finalScore <= 600) title = "银牌发育路"; else if (g_finalScore <= 800) title = "金牌发育路"; else title = "顶级发育路";
        char titleText[128]; sprintf_s(titleText, "恭喜获得称号：%s", title); outtextxy(285, 608, titleText);
        setfillcolor(RGB(100, 150, 200)); setlinecolor(BLACK);
        fillrectangle(g_backRoleBtn.left, g_backRoleBtn.top, g_backRoleBtn.right, g_backRoleBtn.bottom);
        setbkmode(TRANSPARENT); settextcolor(WHITE); settextstyle(38, 0, "微软雅黑");
        outtextxy(g_backRoleBtn.left + 38, g_backRoleBtn.top + 19, "返回分路");
        setfillcolor(RGB(100, 200, 100));
        fillrectangle(g_backDiffBtn.left, g_backDiffBtn.top, g_backDiffBtn.right, g_backDiffBtn.bottom);
        outtextxy(g_backDiffBtn.left + 38, g_backDiffBtn.top + 19, "返回难度");
        break;
    }
    case STATE_HARD_GAME: Hard_Draw(); return;
    }
    if (g_appState == STATE_DIFF_SELECT) {
        setfillcolor(RGB(200, 200, 200)); setlinecolor(BLACK); setlinestyle(PS_SOLID, 2);
        fillcircle(85, 85, 47);
        setbkmode(TRANSPARENT); settextcolor(BLACK); settextstyle(38, 0, "微软雅黑");
        outtextxy(67, 67, "←");
    }
    if (g_appState == STATE_ROLE_SELECT) {
        for (auto& btn : g_roleButtons) {
            setfillcolor(RGB(0, 160, 200)); setlinecolor(BLACK); fillrectangle(btn.rect.left, btn.rect.top, btn.rect.right, btn.rect.bottom);
            setbkmode(TRANSPARENT); settextcolor(WHITE); settextstyle(47, 0, "微软雅黑");
            int tw = textwidth(btn.text), th = textheight(btn.text);
            outtextxy(btn.rect.left + (btn.rect.right - btn.rect.left - tw) / 2, btn.rect.top + (btn.rect.bottom - btn.rect.top - th) / 2, btn.text);
        }
    }
    else if (g_appState == STATE_DIFF_SELECT) {
        for (auto& btn : g_diffButtons) {
            setfillcolor(RGB(200, 150, 0)); setlinecolor(BLACK); fillrectangle(btn.rect.left, btn.rect.top, btn.rect.right, btn.rect.bottom);
            setbkmode(TRANSPARENT); settextcolor(WHITE); settextstyle(47, 0, "微软雅黑");
            int tw = textwidth(btn.text), th = textheight(btn.text);
            outtextxy(btn.rect.left + (btn.rect.right - btn.rect.left - tw) / 2, btn.rect.top + (btn.rect.bottom - btn.rect.top - th) / 2, btn.text);
        }
    }
}
void HandleIntroMouseClicks() {
    static bool lastLeft = false;
    if (MouseHit()) {
        MOUSEMSG msg = GetMouseMsg();
        if (msg.uMsg == WM_LBUTTONDOWN && !lastLeft) {
            if (g_appState == STATE_DIFF_SELECT) {
                int dx = msg.x - 85, dy = msg.y - 85;
                if (dx * dx + dy * dy <= 47 * 47) { g_appState = STATE_ROLE_SELECT; return; }
            }
            if (g_appState == STATE_SCORE) {
                if (msg.x >= g_backRoleBtn.left && msg.x <= g_backRoleBtn.right &&
                    msg.y >= g_backRoleBtn.top && msg.y <= g_backRoleBtn.bottom) {
                    StopMusic("score_music");
                    g_scoreMusicPlayed = false;
                    g_appState = STATE_ROLE_SELECT;
                    PlayMusic("05.mp3", "intro_music", true);   // 重新播放背景音乐
                    return;
                }
                if (msg.x >= g_backDiffBtn.left && msg.x <= g_backDiffBtn.right &&
                    msg.y >= g_backDiffBtn.top && msg.y <= g_backDiffBtn.bottom) {
                    StopMusic("score_music");
                    g_scoreMusicPlayed = false;
                    g_appState = STATE_DIFF_SELECT;
                    PlayMusic("05.mp3", "intro_music", true);   // 重新播放背景音乐
                    return;
                }
                return;
            }
            if (g_appState == STATE_INTRO3) {
                g_appState = STATE_INTRO4; g_stateStartTime = GetTickCount();
            }
            else if (g_appState == STATE_INTRO4) {
                g_appState = STATE_ROLE_SELECT; g_stateStartTime = GetTickCount();
            }
            else if (g_appState == STATE_ROLE_SELECT) {
                for (size_t i = 0; i < g_roleButtons.size(); i++) {
                    if (msg.x >= g_roleButtons[i].rect.left && msg.x <= g_roleButtons[i].rect.right &&
                        msg.y >= g_roleButtons[i].rect.top && msg.y <= g_roleButtons[i].rect.bottom) {
                        if (i == 3) {
                            g_appState = STATE_CHARACTER_SHOW;
                            g_stateStartTime = GetTickCount();
                            PlayMusic("06.mp3", "character_music", false);
                        }
                        break;
                    }
                }
            }
            else if (g_appState == STATE_DIFF_SELECT) {
                for (size_t i = 0; i < g_diffButtons.size(); i++) {
                    if (msg.x >= g_diffButtons[i].rect.left && msg.x <= g_diffButtons[i].rect.right &&
                        msg.y >= g_diffButtons[i].rect.top && msg.y <= g_diffButtons[i].rect.bottom) {
                        StopMusic("intro_music");
                        if (i == 0) {
                            g_difficulty = 0;
                            g_appState = STATE_GAME;
                            Game_Init();
                            PlayMusic("简单模式.mp3", "game_music", true);
                        }
                        else if (i == 1) {
                            g_difficulty = 1;
                            g_appState = STATE_GAME;
                            Game_Init();
                            PlayMusic("中等模式.mp3", "game_music", true);
                        }
                        else if (i == 2) {
                            g_difficulty = 2;
                            g_appState = STATE_HARD_GAME;
                            Hard_Init();
                            // 困难模式背景音乐在天赋页面点击后播放，此处不播放
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
    else { lastLeft = false; }
}

void LoadSmallButtonImages() {
    for (int i = 0; i < 4; i++) {
        char name[64];
        sprintf_s(name, "%d号.png", i + 1);
        int w = 2 * SMALL_BTN_RADIUS;
        int h = 2 * SMALL_BTN_RADIUS;
        loadimage(&g_smallBtnImgs[i], name, w, h);
    }
}
void LoadSkillButtonImages() {
    int wSkill = 2 * SKILL_RADIUS;
    int hSkill = 2 * SKILL_RADIUS;
    loadimage(&g_skillBtnImgs[0], "1技能.png", wSkill, hSkill);
    loadimage(&g_skillBtnImgs[1], "2技能.png", wSkill, hSkill);
    loadimage(&g_skillBtnImgs[2], "大招.png", wSkill, hSkill);
    int wAtk = 2 * ATK_BTN_RADIUS;
    int hAtk = 2 * ATK_BTN_RADIUS;
    loadimage(&g_skillBtnImgs[3], "普攻键.png", wAtk, hAtk);
}

// ------------------- 主函数 -------------------
int main() {
    srand((unsigned)time(NULL));
    initgraph(WIN_WIDTH, WIN_HEIGHT);
    InitIntroImages();
    InitButtons();
    InitSkillButtons();
    InitSmallButtons();
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
    PlayMusic("03.mp3", "intro_music", false);
    g_appState = STATE_INTRO1;
    g_stateStartTime = GetTickCount();
    LARGE_INTEGER perfFreq, perfLast, perfNow;
    QueryPerformanceFrequency(&perfFreq);
    QueryPerformanceCounter(&perfLast);
    float lastFrameTime = (float)GetTickCount() / 1000.0f;
    while (true) {
        QueryPerformanceCounter(&perfNow);
        float delta = (float)(perfNow.QuadPart - perfLast.QuadPart) / perfFreq.QuadPart;
        perfLast = perfNow;
        if (delta > 0.05f) delta = 0.05f;
        float now = (float)GetTickCount() / 1000.0f;

        if (g_appState == STATE_INTRO1 && GetTickCount() - g_stateStartTime >= 4000) {
            g_appState = STATE_INTRO2; g_stateStartTime = GetTickCount();
            StopMusic("intro_music");
            PlayMusic("04.mp3", "intro_music", false);
        }
        else if (g_appState == STATE_INTRO2 && GetTickCount() - g_stateStartTime >= 2000) {
            g_appState = STATE_INTRO3; g_stateStartTime = GetTickCount();
            StopMusic("intro_music");
            PlayMusic("05.mp3", "intro_music", true); g_welcomePlayed = false;
        }
        else if (g_appState == STATE_CHARACTER_SHOW && GetTickCount() - g_stateStartTime >= 5000) {
            g_appState = STATE_DIFF_SELECT; g_stateStartTime = GetTickCount();
            // 05.mp3 继续播放，不停止
        }

        if (g_appState == STATE_GAME) {
            Game_Update(delta);
            Game_HandleKeyboard();
            Game_HandleMouse(now);
            if (g_showRange && now >= g_rangeEndTime) g_showRange = false;
            if (g_gameState == GAME_OVER) {
                g_finalScore = g_game_score; g_finalGameTime = g_gameTimer;
                StopMusic("game_music");
                g_appState = STATE_GAME_OVER1;
                g_monsters.clear(); g_arrows.clear(); g_balls.clear();
            }
            BeginBatchDraw(); Game_Draw(); EndBatchDraw();
        }
        else if (g_appState == STATE_HARD_GAME) {
            Hard_Update(delta);
            Hard_HandleKeyboard();
            Hard_HandleMouse(now);
            BeginBatchDraw(); Hard_Draw(); EndBatchDraw();
        }
        else {
            HandleIntroMouseClicks();
            BeginBatchDraw(); DrawCurrentState(); EndBatchDraw();
        }

        if (KEY_DOWN(VK_ESCAPE)) break;
        Sleep(0);
    }

    StopMusic("intro_music"); StopMusic("game_music"); StopMusic("attack");
    StopMusic("hit"); StopMusic("skill"); StopMusic("ultimate");
    StopMusic("buy"); StopMusic("revive");
    closegraph();
    return 0;
}
#ifndef GAME_WINDOW_H
#define GAME_WINDOW_H

#include <GLFW/glfw3.h>
#include "../core/GameEngine.h"
#include "Renderer.h"
#include <string>
#include <vector>
#include <functional>

using namespace std;

struct CardAnim {
    Card card;
    float x = 0.0f;
    float y = 780.0f;
    float elevation = 0.0f;
    bool initialized = false;
    float delayTimer = 0.0f;

    CardAnim() : card(Suit::SPADES, Rank::TWO) {}
    CardAnim(const Card& c) : card(c) {}
};

enum class ScoringStep {
    IDLE,
    SLIDE_IN,          // Kartu meluncur ke tengah meja
    CARD_TRIGGERS,     // Kartu terpicu bergantian satu per satu (+Chips pop)
    MULT_TRIGGER,      // Poker hand base mult terpicu (+Mult pop)
    JOKER_TRIGGERS,    // Joker terpicu berurutan: menyala + floating bonus (+Mult / +Chips)
    FINAL_SLAM,        // Chips x Mult bertubrukan (Screen shake!)
    SCORE_TICK,        // Skor ronde bertambah bergulir & progress bar terisi
    SLIDE_OUT,         // Kartu meluncur ke kanan keluar layar
    FINISHED
};

struct JokerAnimState {
    float glow = 0.0f;
    float scale = 1.0f;
};

struct FloatingText {
    string text;
    float x, y;
    float vy;
    Color color;
    float scale;
    float alpha;
    float lifetime;
};

struct AnimatedCard {
    Card card;
    float startX, startY;
    float currentX, currentY;
    float targetX, targetY;
    float scale;
    float glow;
    bool isScoring;
    bool triggered;

    AnimatedCard(const Card& c)
        : card(c), startX(0.0f), startY(0.0f), currentX(0.0f), currentY(0.0f),
          targetX(0.0f), targetY(0.0f), scale(1.0f), glow(0.0f),
          isScoring(false), triggered(false) {}
};

// Partikel mengambang ke luar di sekitar frame blind terpilih
struct FrameParticle {
    float x, y;
    float vx, vy;
    float size;
    float life;
    float maxLife;
    Color color;
};

class GameWindow {
private:
    GLFWwindow* window;
    int windowWidth;
    int windowHeight;
    string windowTitle;
    GameEngine engine;

    // State Animasi Kartu yang Mulus (Lerp & Hover Physics)
    vector<CardAnim> cardAnims;

    // State Animasi Scoring Berurutan (The Dopamine Loop)
    ScoringStep scoringStep;
    float scoringTimer;
    float animSpeedMultiplier;
    int currentTriggerIndex;
    int currentJokerTriggerIndex;
    vector<JokerAnimState> jokerAnimStates;

    ScoreResult activeScoreResult;
    int animChips;
    int animMult;
    long long animFinalScore;
    float chipsBoxScale;
    float multBoxScale;

    int displayedRoundScore;
    int targetRoundScore;
    float cameraShake;

    vector<AnimatedCard> scoringAnimatedCards;
    vector<FloatingText> floatingTexts;

    // State Animasi Discard (Kartu bergeser ke kanan ke luar layar)
    bool isDiscarding;
    float discardTimer;
    vector<AnimatedCard> discardAnimatedCards;

    // State Tooltip Info Kartu saat Hover (Balatro Style)
    int hoveredCardIndex;
    float hoverTooltipAlpha;

    // State Toast Notification (Error / Warning feedback)
    string toastMessage;
    float toastTimer;

    // Tracking Mouse
    double mouseX;
    double mouseY;

    // State Efek Partikel Frame Blind
    vector<FrameParticle> frameParticles;
    float frameParticleTimer;

    // State Efek Transisi Layar CRT TV Jadul (Turn Off -> White Flash Lines Meet in Center -> Turn On)
    bool isTransitioning;
    float transitionTimer;
    float transitionDuration;
    bool transitionSwapped;
    std::function<void()> pendingTransitionAction;
    string renderedState; // State yang sedang digambar pada layar

    void updateFrameParticles(float dt, float fx, float fy, float fw, float fh, Color col);
    void renderFrameParticles();
    void startTransition(std::function<void()> action);
    void updateTransition(float dt);
    void renderCrtTransition();

    void processInput();
    void updateCardAnimations(float dt);
    void triggerDealAnimation();
    void updateScoringSequence(float dt);
    void updateDiscardSequence(float dt);
    void updateFloatingTexts(float dt);
    void startScoringSequence();
    void startDiscardSequence();
    void spawnFloatingText(const string& text, float x, float y, Color col, float scale = 1.6f);
    void renderFloatingTexts();
    void renderCardTooltip(float dt);
    void renderJokerTooltip(const Joker& joker, float x, float y);

    // State Modal Run Info (Poker Hands / Level Overview)
    bool showRunInfoModal;
    int hoveredRunInfoRow;
    int runInfoActiveTab; // 0 = Poker Hands, 1 = Blinds, 2 = Vouchers

    // State Modal Settings & Gear Icon
    bool showSettingsModal;
    float gearRotationAngle;
    bool isDraggingBgmSlider;
    bool isDraggingSfxSlider;

    void renderSettingsModal();
    void drawGearIcon(float cx, float cy, float radius, float angle, Color col);

    void renderUI(float dt);

    // Layar Menu Utama (Title Screen)
    void renderMainMenu(float dt);

    // Layar Pemilihan Blind (Small, Big, Boss Blind)
    void renderBlindSelect();

    // Layar Toko (The Shop)
    void renderShop();

    // Layar Gameplay Ronde
    void renderRoundGameplay();
    void renderJokerSlots();
    void renderHUD();
    void renderCards();
    void renderControls();
    void renderScoringPreview();
    void renderActiveScoringDisplay();
    void renderRunInfoModal();
    void renderRunInfoBlindsTab(float modalX, float modalY, float modalW, float modalH, float mx, float my);
    void renderRunInfoVouchersTab(float modalX, float modalY, float modalW, float modalH, float mx, float my);
    void renderPokerHandDetailTooltip(HandType type, float x, float y);
    void renderVoucherTooltip(const Voucher& voucher, float x, float y);
    void getCardLayout(int handLimit, float& startX, float& cardW, float& cardH, float& spacing, float& baseY) const;

    bool isPointInRect(float px, float py, float rx, float ry, float rw, float rh) const;

public:
    GameWindow(int width = 1024, int height = 720, const string& title = "Balatro Mini");
    ~GameWindow();

    bool init();
    void run();
    void showToast(const string& message);
    void clearNotifications();

    // Callback mouse click & keyboard
    void handleMouseClick(int button, int action, int mods);
    void handleKey(int key, int scancode, int action, int mods);
};

#endif // GAME_WINDOW_H

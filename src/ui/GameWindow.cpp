#include "../../include/ui/GameWindow.h"
#include "../../include/ui/PixelFont.h"
#include "../../include/core/Exceptions.h"
#include <iostream>
#include <algorithm>

using namespace std;

static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    auto* app = static_cast<GameWindow*>(glfwGetWindowUserPointer(window));
    if (app) {
        app->handleMouseClick(button, action, mods);
    }
}

static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    auto* app = static_cast<GameWindow*>(glfwGetWindowUserPointer(window));
    if (app) {
        app->handleKey(key, scancode, action, mods);
    }
}

static void framebufferSizeCallback(GLFWwindow* /*window*/, int width, int height) {
    Renderer::updateViewport(width, height);
}

GameWindow::GameWindow(int width, int height, const string& title)
    : window(nullptr), windowWidth(width), windowHeight(height), windowTitle(title), toastTimer(0.0f), mouseX(0), mouseY(0) {}

GameWindow::~GameWindow() {
    if (window) {
        glfwDestroyWindow(window);
    }
    glfwTerminate();
}

bool GameWindow::init() {
    if (!glfwInit()) {
        cerr << "[ERROR] Gagal inisialisasi GLFW!\n";
        return false;
    }

    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    window = glfwCreateWindow(windowWidth, windowHeight, "Balatro Mini", nullptr, nullptr);
    if (!window) {
        cerr << "[ERROR] Gagal membuat Window GLFW!\n";
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    glfwSetWindowUserPointer(window, this);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

    Renderer::initGL(windowWidth, windowHeight);
    return true;
}

void GameWindow::showToast(const string& message) {
    toastMessage = message;
    toastTimer = 2.5f;
}

bool GameWindow::isPointInRect(float px, float py, float rx, float ry, float rw, float rh) const {
    return (px >= rx && px <= rx + rw && py >= ry && py <= ry + rh);
}

void GameWindow::handleMouseClick(int button, int action, int /*mods*/) {
    if (button != GLFW_MOUSE_BUTTON_LEFT || action != GLFW_PRESS) {
        return;
    }

    float mx = static_cast<float>(mouseX);
    float my = static_cast<float>(mouseY);
    string state = engine.getCurrentStateName();

    // 1. KLIK PADA MENU UTAMA
    if (state == "MAIN_MENU") {
        // Tombol START NEW RUN (x: 382, y: 310, w: 260, h: 50)
        if (isPointInRect(mx, my, 382.0f, 310.0f, 260.0f, 50.0f)) {
            engine.startNewRun();
            return;
        }
        // Tombol QUIT (x: 432, y: 380, w: 160, h: 42)
        if (isPointInRect(mx, my, 432.0f, 380.0f, 160.0f, 42.0f)) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
            return;
        }
        return;
    }

    // 2. KLIK PADA LAYAR PEMILIHAN BLIND
    if (state == "BLIND_SELECT") {
        float btnXs[3] = { 102.0f, 412.0f, 722.0f };
        for (int i = 0; i < 3; ++i) {
            if (isPointInRect(mx, my, btnXs[i], 495.0f, 200.0f, 45.0f)) {
                if (engine.isBlindCurrent(i)) {
                    engine.selectBlind();
                    return;
                } else if (engine.isBlindDefeated(i)) {
                    showToast("Blind already defeated!");
                } else {
                    showToast("Blind is locked! Defeat previous blind first.");
                }
                return;
            }
        }

        // Tombol MAIN MENU (x: 432, y: 600, w: 160, h: 40)
        if (isPointInRect(mx, my, 432.0f, 600.0f, 160.0f, 40.0f)) {
            engine.toMainMenu();
            return;
        }
        return;
    }

    // 3. KLIK PADA LAYAR GAMEPLAY (SELECTING_CARDS)
    if (state == "SELECTING_CARDS") {
        const auto& hand = engine.getHand();
        float cardW = 88.0f;
        float cardH = 130.0f;
        float startX = 55.0f;
        float spacing = 28.0f;
        float baseY = 530.0f;

        for (size_t i = 0; i < hand.size(); ++i) {
            float cx = (i < cardAnims.size() && cardAnims[i].initialized) ? cardAnims[i].x : (startX + i * (cardW + spacing));
            float cy = (i < cardAnims.size() && cardAnims[i].initialized) ? cardAnims[i].y : baseY;

            if (isPointInRect(mx, my, cx, cy, cardW, cardH)) {
                try {
                    engine.toggleSelectCard(static_cast<int>(i));
                } catch (const GameException& e) {
                    showToast(e.what());
                }
                return;
            }
        }

        // Tombol PLAY HAND (x: 410, y: 440, w: 140, h: 48)
        if (isPointInRect(mx, my, 410.0f, 440.0f, 140.0f, 48.0f)) {
            try {
                engine.playHand();
            } catch (const GameException& e) {
                showToast(e.what());
            }
            return;
        }

        // Tombol DISCARD (x: 580, y: 440, w: 140, h: 48)
        if (isPointInRect(mx, my, 580.0f, 440.0f, 140.0f, 48.0f)) {
            try {
                engine.discard();
            } catch (const GameException& e) {
                showToast(e.what());
            }
            return;
        }
        return;
    }

    // 4. KLIK PADA LAYAR ROUND_WON (CASH OUT)
    if (state == "ROUND_WON") {
        if (isPointInRect(mx, my, 460.0f, 440.0f, 180.0f, 48.0f)) {
            engine.cashOut();
            return;
        }
        return;
    }

    // 5. KLIK PADA LAYAR GAME_OVER
    if (state == "GAME_OVER") {
        if (isPointInRect(mx, my, 390.0f, 440.0f, 160.0f, 48.0f)) {
            engine.startNewRun();
            showToast("New run started!");
            return;
        }
        if (isPointInRect(mx, my, 570.0f, 440.0f, 160.0f, 48.0f)) {
            engine.toMainMenu();
            return;
        }
        return;
    }
}

void GameWindow::handleKey(int key, int /*scancode*/, int action, int /*mods*/) {
    if (action != GLFW_PRESS) return;

    string state = engine.getCurrentStateName();

    if (key == GLFW_KEY_ESCAPE) {
        if (state == "SELECTING_CARDS" || state == "BLIND_SELECT") {
            engine.toMainMenu();
        } else {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
    } else if (key == GLFW_KEY_SPACE || key == GLFW_KEY_ENTER) {
        if (state == "MAIN_MENU") {
            engine.startNewRun();
        } else if (state == "BLIND_SELECT") {
            engine.selectBlind();
        } else if (state == "SELECTING_CARDS") {
            try { engine.playHand(); } catch (const GameException& e) { showToast(e.what()); }
        } else if (state == "ROUND_WON") {
            engine.cashOut();
        } else if (state == "GAME_OVER") {
            engine.startNewRun();
        }
    } else if (key == GLFW_KEY_D) {
        if (state == "SELECTING_CARDS") {
            try { engine.discard(); } catch (const GameException& e) { showToast(e.what()); }
        }
    } else if (key == GLFW_KEY_R) {
        engine.startNewRun();
        showToast("Run restarted!");
    } else if (key >= GLFW_KEY_1 && key <= GLFW_KEY_8) {
        if (state == "SELECTING_CARDS") {
            int idx = key - GLFW_KEY_1;
            if (idx < static_cast<int>(engine.getHand().size())) {
                try { engine.toggleSelectCard(idx); } catch (const GameException& e) { showToast(e.what()); }
            }
        }
    }
}

// ================= RENDER MENU UTAMA =================
void GameWindow::renderMainMenu() {
    float mx = static_cast<float>(mouseX);
    float my = static_cast<float>(mouseY);

    // Judul Besar Balatro (Presisi di Tengah Layar)
    Renderer::drawCenteredText("BALATRO", 512.0f, 130.0f, 4.5f, Theme::GOLD);
    Renderer::drawCenteredText("M  I  N  I", 512.0f, 205.0f, 2.5f, Theme::MULT_RED);
    Renderer::drawCenteredText("A Poker Roguelike", 512.0f, 250.0f, 1.4f, Theme::TEXT_WHITE);

    // Garis Aksen Tengah
    Renderer::drawRect(362.0f, 280.0f, 300.0f, 2.0f, Color(0.3f, 0.5f, 0.4f, 0.8f));

    // Tombol START NEW RUN
    bool hoverRun = isPointInRect(mx, my, 382.0f, 310.0f, 260.0f, 50.0f);
    Renderer::drawButton("START NEW RUN", 382.0f, 310.0f, 260.0f, 50.0f, Theme::CHIPS_BLUE, Theme::TEXT_WHITE, hoverRun, true);

    // Tombol QUIT
    bool hoverQuit = isPointInRect(mx, my, 432.0f, 380.0f, 160.0f, 42.0f);
    Renderer::drawButton("QUIT", 432.0f, 380.0f, 160.0f, 42.0f, Color(0.3f, 0.1f, 0.1f, 0.9f), Theme::TEXT_WHITE, hoverQuit, true);

    // Panel Informasi & Cara Bermain (Lebar 740px, teks rapi dengan margin lega)
    float panelW = 740.0f;
    float panelX = (static_cast<float>(windowWidth) - panelW) / 2.0f;
    Renderer::drawRect(panelX, 470.0f, panelW, 190.0f, Theme::PANEL_BG);
    Renderer::drawRectOutline(panelX, 470.0f, panelW, 190.0f, 1.5f, Color(0.2f, 0.4f, 0.35f, 0.8f));

    Renderer::drawCenteredText("HOW TO PLAY / OBJECTIVE", 512.0f, 490.0f, 1.6f, Theme::GOLD);

    float textLeft = panelX + 35.0f;
    Renderer::drawText("- Play poker combinations (1-5 cards) to score Chips x Mult", textLeft, 525.0f, 1.2f, Theme::TEXT_WHITE);
    Renderer::drawText("- Reach the target score before running out of Hands", textLeft, 552.0f, 1.2f, Theme::TEXT_WHITE);
    Renderer::drawText("- Defeat Small, Big, and Boss Blinds to advance each Ante!", textLeft, 579.0f, 1.2f, Theme::TEXT_WHITE);
    
    Renderer::drawRect(panelX + 35.0f, 608.0f, panelW - 70.0f, 1.0f, Color(0.25f, 0.45f, 0.4f, 0.6f));
    Renderer::drawText("Controls: Mouse Click | Keys [1-8] | Space/Enter to Play | D to Discard", textLeft, 622.0f, 1.15f, Theme::CHIPS_BLUE);
}

// ================= RENDER PEMILIHAN BLIND =================
void GameWindow::renderBlindSelect() {
    float mx = static_cast<float>(mouseX);
    float my = static_cast<float>(mouseY);

    // Header Atas (Lebar 944, Rapi & Tidak Keluar Kotak)
    Renderer::drawRect(40.0f, 25.0f, 944.0f, 80.0f, Theme::PANEL_BG);
    Renderer::drawRectOutline(40.0f, 25.0f, 944.0f, 80.0f, 2.0f, Color(0.2f, 0.4f, 0.35f, 0.8f));

    // Kiri: ANTE
    string anteStr = "ANTE " + to_string(engine.getCurrentAnte()) + " / 8";
    Renderer::drawText(anteStr, 65.0f, 52.0f, 2.2f, Theme::GOLD);

    // Tengah: CHOOSE YOUR BLIND (Tepat di Sumbu Tengah)
    Renderer::drawCenteredText("CHOOSE YOUR BLIND", 512.0f, 52.0f, 2.2f, Theme::TEXT_WHITE);

    // Kanan: Cash & Deck (Dua baris vertikal rapi, berjarak dari border kanan)
    string cashStr = "Cash: $" + to_string(engine.getMoney());
    Renderer::drawText(cashStr, 795.0f, 43.0f, 1.4f, Theme::GOLD);

    string deckStr = "Deck: " + to_string(engine.getDeckRemainingCount()) + " cards";
    Renderer::drawText(deckStr, 795.0f, 67.0f, 1.2f, Theme::TEXT_WHITE);

    // 3 Kartu Blind (Small Blind, Big Blind, Boss Blind)
    float cardXs[3] = { 67.0f, 377.0f, 687.0f };
    float cardW = 270.0f;
    float cardH = 430.0f;
    float cardY = 135.0f;

    for (int i = 0; i < 3; ++i) {
        Blind blind = engine.getBlindByIndex(i);
        bool isCurrent = engine.isBlindCurrent(i);
        bool isDefeated = engine.isBlindDefeated(i);
        bool isLocked = engine.isBlindLocked(i);

        float cx = cardXs[i];
        float cardCenterX = cx + (cardW / 2.0f);

        // Background Kartu
        Color bg = Theme::PANEL_BG;
        if (isCurrent) bg = Color(0.08f, 0.18f, 0.15f, 0.95f);
        else if (isDefeated) bg = Color(0.05f, 0.1f, 0.08f, 0.6f);

        Renderer::drawRect(cx, cardY, cardW, cardH, bg);

        // Border
        Color borderCol = Color(0.2f, 0.35f, 0.3f);
        if (isCurrent) {
            borderCol = (i == 2) ? Theme::MULT_RED : (i == 1 ? Theme::GOLD : Theme::CHIPS_BLUE);
        }
        Renderer::drawRectOutline(cx, cardY, cardW, cardH, isCurrent ? 3.0f : 1.5f, borderCol);

        // Judul Blind (Rata Tengah pada Kartu)
        Color titleColor = (i == 2) ? Theme::MULT_RED : (i == 1 ? Theme::GOLD : Theme::CHIPS_BLUE);
        if (isDefeated) titleColor = Theme::DEBUFF_GRAY;

        Renderer::drawCenteredText(blind.getName(), cardCenterX, cardY + 30.0f, 2.0f, titleColor);

        // Garis Pemisah
        Renderer::drawRect(cx + 25.0f, cardY + 68.0f, cardW - 50.0f, 1.5f, Color(0.3f, 0.5f, 0.4f, 0.5f));

        // Target Skor
        Renderer::drawCenteredText("Score at least:", cardCenterX, cardY + 95.0f, 1.4f, Theme::TEXT_WHITE);
        string targetVal = to_string(blind.getTargetScore());
        Renderer::drawCenteredText(targetVal, cardCenterX, cardY + 125.0f, 3.0f, titleColor);

        // Reward
        string rewardStr = "Reward: $" + to_string(blind.getRewardMoney());
        Renderer::drawCenteredText(rewardStr, cardCenterX, cardY + 185.0f, 1.6f, Theme::GOLD);

        // Info Debuff jika Boss Blind
        if (blind.getDebuff() != BossDebuff::NONE) {
            float debuffBoxW = 230.0f;
            float debuffBoxX = cx + (cardW - debuffBoxW) / 2.0f;
            Renderer::drawRect(debuffBoxX, cardY + 230.0f, debuffBoxW, 60.0f, Color(0.25f, 0.05f, 0.05f, 0.9f));
            Renderer::drawRectOutline(debuffBoxX, cardY + 230.0f, debuffBoxW, 60.0f, 1.5f, Theme::MULT_RED);
            Renderer::drawCenteredText("BOSS DEBUFF:", cardCenterX, cardY + 242.0f, 1.1f, Theme::MULT_RED);
            Renderer::drawCenteredText(blind.getDebuffDescription(), cardCenterX, cardY + 266.0f, 1.0f, Theme::TEXT_WHITE);
        }

        // Tombol Aksi di Bawah Kartu
        float btnW = 200.0f;
        float btnH = 45.0f;
        float btnX = cx + (cardW - btnW) / 2.0f;
        float btnY = cardY + cardH - 65.0f;

        if (isDefeated) {
            Renderer::drawButton("DEFEATED", btnX, btnY, btnW, btnH, Color(0.2f, 0.35f, 0.25f), Color(0.6f, 0.8f, 0.6f), false, false);
        } else if (isCurrent) {
            bool hoverBtn = isPointInRect(mx, my, btnX, btnY, btnW, btnH);
            Renderer::drawButton("SELECT BLIND", btnX, btnY, btnW, btnH, titleColor, Theme::TEXT_WHITE, hoverBtn, true);
        } else if (isLocked) {
            Renderer::drawButton("LOCKED", btnX, btnY, btnW, btnH, Color(0.15f, 0.2f, 0.18f), Theme::DEBUFF_GRAY, false, false);
        }
    }

    // Tombol Kembali ke Menu
    bool hoverBack = isPointInRect(mx, my, 432.0f, 600.0f, 160.0f, 40.0f);
    Renderer::drawButton("MAIN MENU", 432.0f, 600.0f, 160.0f, 40.0f, Color(0.2f, 0.3f, 0.25f), Theme::TEXT_WHITE, hoverBack, true);
}

// ================= RENDER GAMEPLAY RONDE =================
void GameWindow::renderHUD() {
    Blind blind = engine.getCurrentBlind();

    Renderer::drawRect(25.0f, 25.0f, 260.0f, 465.0f, Theme::PANEL_BG);
    Renderer::drawRectOutline(25.0f, 25.0f, 260.0f, 465.0f, 2.0f, Color(0.2f, 0.4f, 0.35f, 0.8f));

    // Judul Game (Hanya "BALATRO MINI", tanpa teks subtitle)
    Renderer::drawText("BALATRO MINI", 45.0f, 45.0f, 2.2f, Theme::GOLD);

    // Garis Pemisah
    Renderer::drawRect(45.0f, 75.0f, 220.0f, 2.0f, Color(0.3f, 0.5f, 0.4f, 0.6f));

    // Ante & Blind
    string anteStr = "ANTE: " + to_string(engine.getCurrentAnte()) + " / 8";
    Renderer::drawText(anteStr, 45.0f, 95.0f, 1.8f, Theme::TEXT_WHITE);

    Color blindColor = Theme::TEXT_WHITE;
    if (blind.getType() == BlindType::BOSS_BLIND) blindColor = Theme::MULT_RED;
    else if (blind.getType() == BlindType::BIG_BLIND) blindColor = Theme::GOLD;

    Renderer::drawText(blind.getName(), 45.0f, 122.0f, 2.0f, blindColor);

    string targetStr = "Target: " + to_string(blind.getTargetScore());
    Renderer::drawText(targetStr, 45.0f, 152.0f, 1.6f, Theme::CHIPS_BLUE);

    string rewardStr = "Reward: $" + to_string(blind.getRewardMoney());
    Renderer::drawText(rewardStr, 45.0f, 177.0f, 1.5f, Theme::GOLD);

    if (blind.getDebuff() != BossDebuff::NONE) {
        Renderer::drawRect(40.0f, 205.0f, 230.0f, 38.0f, Color(0.3f, 0.05f, 0.05f, 0.8f));
        Renderer::drawRectOutline(40.0f, 205.0f, 230.0f, 38.0f, 1.5f, Theme::MULT_RED);
        Renderer::drawText("DEBUFF ACTIVE:", 46.0f, 212.0f, 1.1f, Theme::MULT_RED);
        Renderer::drawText(blind.getDebuffDescription(), 46.0f, 228.0f, 1.0f, Theme::TEXT_WHITE);
    }

    Renderer::drawRect(45.0f, 260.0f, 220.0f, 2.0f, Color(0.3f, 0.5f, 0.4f, 0.6f));

    Renderer::drawText("ROUND SCORE:", 45.0f, 275.0f, 1.4f, Theme::TEXT_WHITE);
    string scoreStr = to_string(engine.getCurrentScore()) + " / " + to_string(blind.getTargetScore());
    Renderer::drawText(scoreStr, 45.0f, 300.0f, 2.2f, Theme::GOLD);

    string moneyStr = "Money: $" + to_string(engine.getMoney());
    Renderer::drawText(moneyStr, 45.0f, 345.0f, 1.8f, Theme::GOLD);

    string handsStr = "Hands: " + to_string(engine.getHandsRemaining());
    Renderer::drawText(handsStr, 45.0f, 380.0f, 1.8f, Theme::CHIPS_BLUE);

    string discardsStr = "Discards: " + to_string(engine.getDiscardsRemaining());
    Renderer::drawText(discardsStr, 45.0f, 410.0f, 1.8f, Theme::MULT_RED);

    string deckStr = "Deck: " + to_string(engine.getDeckRemainingCount()) + " cards";
    Renderer::drawText(deckStr, 45.0f, 440.0f, 1.3f, Theme::TEXT_WHITE);
}

void GameWindow::renderScoringPreview() {
    Renderer::drawRect(310.0f, 25.0f, 680.0f, 380.0f, Theme::PANEL_BG);
    Renderer::drawRectOutline(310.0f, 25.0f, 680.0f, 380.0f, 2.0f, Color(0.2f, 0.4f, 0.35f, 0.8f));

    string state = engine.getCurrentStateName();

    if (state == "ROUND_WON") {
        Renderer::drawText("ROUND CLEARED!", 460.0f, 120.0f, 3.2f, Theme::GOLD);
        string winSub = "Target reached! Blind reward $" + to_string(engine.getCurrentBlind().getRewardMoney()) + " earned.";
        Renderer::drawText(winSub, 400.0f, 175.0f, 1.5f, Theme::TEXT_WHITE);
        Renderer::drawText("Click 'CASH OUT' to collect and return to Blind Select", 350.0f, 210.0f, 1.4f, Theme::CHIPS_BLUE);
        return;
    }

    if (state == "GAME_OVER") {
        Renderer::drawText("GAME OVER!", 480.0f, 120.0f, 3.5f, Theme::MULT_RED);
        Renderer::drawText("You ran out of hands before beating the blind.", 390.0f, 175.0f, 1.5f, Theme::TEXT_WHITE);
        Renderer::drawText("Click 'TRY AGAIN' or return to 'MAIN MENU'", 410.0f, 210.0f, 1.5f, Theme::GOLD);
        return;
    }

    vector<Card> selected = engine.getSelectedCards();
    if (!selected.empty() && selected.size() <= 5) {
        StandardPokerEvaluator eval;
        auto handRes = eval.evaluate(selected);
        HandInfo base = getHandBaseInfo(handRes.handType);

        int cardChips = 0;
        for (const auto& c : handRes.scoringCards) {
            cardChips += c.getBaseChips();
        }
        int totalC = base.baseChips + cardChips;
        int totalM = base.baseMult;
        long long estScore = static_cast<long long>(totalC) * totalM;

        Renderer::drawText(handRes.handName, 530.0f, 60.0f, 2.6f, Theme::GOLD);

        Renderer::drawRect(420.0f, 120.0f, 180.0f, 85.0f, Theme::CHIPS_BLUE);
        Renderer::drawRectOutline(420.0f, 120.0f, 180.0f, 85.0f, 3.0f, Color(1,1,1,0.8f));
        Renderer::drawText("CHIPS", 475.0f, 130.0f, 1.4f, Theme::TEXT_WHITE);
        string cStr = to_string(totalC);
        Renderer::drawText(cStr, 480.0f, 155.0f, 2.8f, Theme::TEXT_WHITE);

        Renderer::drawText("X", 630.0f, 145.0f, 3.5f, Theme::TEXT_WHITE);

        Renderer::drawRect(680.0f, 120.0f, 180.0f, 85.0f, Theme::MULT_RED);
        Renderer::drawRectOutline(680.0f, 120.0f, 180.0f, 85.0f, 3.0f, Color(1,1,1,0.8f));
        Renderer::drawText("MULT", 740.0f, 130.0f, 1.4f, Theme::TEXT_WHITE);
        string mStr = to_string(totalM);
        Renderer::drawText(mStr, 755.0f, 155.0f, 2.8f, Theme::TEXT_WHITE);

        string estStr = "Estimated Score: " + to_string(estScore);
        Renderer::drawText(estStr, 510.0f, 235.0f, 1.8f, Theme::TEXT_WHITE);

        string hintStr = to_string(selected.size()) + " card(s) selected (" + 
                         to_string(handRes.scoringCards.size()) + " scoring cards)";
        Renderer::drawText(hintStr, 480.0f, 275.0f, 1.4f, Color(0.8f, 0.8f, 0.8f, 0.9f));
    } else {
        Renderer::drawText("SELECT 1 - 5 CARDS", 480.0f, 130.0f, 2.4f, Theme::TEXT_WHITE);
        Renderer::drawText("Click cards below or press [1 - 8] on keyboard", 400.0f, 175.0f, 1.5f, Color(0.7f, 0.8f, 0.75f, 0.9f));
        Renderer::drawText("Then press PLAY HAND to score or DISCARD to draw new cards", 350.0f, 210.0f, 1.3f, Color(0.6f, 0.7f, 0.65f, 0.8f));

        const auto& last = engine.getLastScoreResult();
        if (last.finalScore > 0) {
            string lastStr = "Last Hand: " + last.handName + " scored " + to_string(last.finalScore) + " pts!";
            Renderer::drawText(lastStr, 450.0f, 290.0f, 1.4f, Theme::GOLD);
        }
    }
}

void GameWindow::renderControls() {
    float mx = static_cast<float>(mouseX);
    float my = static_cast<float>(mouseY);
    string state = engine.getCurrentStateName();

    if (state == "ROUND_WON") {
        bool hoverCash = isPointInRect(mx, my, 460.0f, 440.0f, 180.0f, 48.0f);
        Renderer::drawButton("CASH OUT", 460.0f, 440.0f, 180.0f, 48.0f, Theme::GOLD, Theme::TEXT_DARK, hoverCash, true);
        return;
    }

    if (state == "GAME_OVER") {
        bool hoverTry = isPointInRect(mx, my, 390.0f, 440.0f, 160.0f, 48.0f);
        bool hoverMenu = isPointInRect(mx, my, 570.0f, 440.0f, 160.0f, 48.0f);
        Renderer::drawButton("TRY AGAIN", 390.0f, 440.0f, 160.0f, 48.0f, Theme::CHIPS_BLUE, Theme::TEXT_WHITE, hoverTry, true);
        Renderer::drawButton("MAIN MENU", 570.0f, 440.0f, 160.0f, 48.0f, Theme::MULT_RED, Theme::TEXT_WHITE, hoverMenu, true);
        return;
    }

    int selCount = engine.getSelectedCount();
    bool canPlay = (selCount >= 1 && selCount <= 5 && engine.getHandsRemaining() > 0);
    bool canDiscard = (selCount >= 1 && selCount <= 5 && engine.getDiscardsRemaining() > 0);

    bool hoverPlay = isPointInRect(mx, my, 410.0f, 440.0f, 140.0f, 48.0f);
    bool hoverDiscard = isPointInRect(mx, my, 580.0f, 440.0f, 140.0f, 48.0f);

    Renderer::drawButton("PLAY HAND", 410.0f, 440.0f, 140.0f, 48.0f, Theme::CHIPS_BLUE, Theme::TEXT_WHITE, hoverPlay, canPlay);
    Renderer::drawButton("DISCARD", 580.0f, 440.0f, 140.0f, 48.0f, Theme::MULT_RED, Theme::TEXT_WHITE, hoverDiscard, canDiscard);
}

void GameWindow::updateCardAnimations(float dt) {
    const auto& hand = engine.getHand();
    if (cardAnims.size() != hand.size()) {
        cardAnims.resize(hand.size());
    }

    float cardW = 88.0f;
    float cardH = 130.0f;
    float startX = 55.0f;
    float spacing = 28.0f;
    float baseY = 530.0f;

    float mx = static_cast<float>(mouseX);
    float my = static_cast<float>(mouseY);

    for (size_t i = 0; i < hand.size(); ++i) {
        float targetX = startX + i * (cardW + spacing);

        float curX = cardAnims[i].initialized ? cardAnims[i].x : targetX;
        float curY = cardAnims[i].initialized ? cardAnims[i].y : baseY;
        bool isHovered = isPointInRect(mx, my, curX, curY, cardW, cardH);

        float targetElev = hand[i].isSelected() ? 1.0f : (isHovered ? 0.35f : 0.0f);
        float targetY = baseY - targetElev * 32.0f;

        if (!cardAnims[i].initialized) {
            cardAnims[i].x = targetX;
            cardAnims[i].y = targetY;
            cardAnims[i].elevation = targetElev;
            cardAnims[i].initialized = true;
        } else {
            float speed = min(1.0f, 18.0f * dt);
            cardAnims[i].x += (targetX - cardAnims[i].x) * speed;
            cardAnims[i].y += (targetY - cardAnims[i].y) * speed;
            cardAnims[i].elevation += (targetElev - cardAnims[i].elevation) * speed;
        }
    }
}

void GameWindow::renderCards() {
    const auto& hand = engine.getHand();
    float cardW = 88.0f;
    float cardH = 130.0f;
    float startX = 55.0f;
    float spacing = 28.0f;
    float baseY = 530.0f;

    float mx = static_cast<float>(mouseX);
    float my = static_cast<float>(mouseY);

    for (size_t i = 0; i < hand.size(); ++i) {
        float cx = (i < cardAnims.size() && cardAnims[i].initialized) ? cardAnims[i].x : (startX + i * (cardW + spacing));
        float cy = (i < cardAnims.size() && cardAnims[i].initialized) ? cardAnims[i].y : baseY;
        float elev = (i < cardAnims.size() && cardAnims[i].initialized) ? cardAnims[i].elevation : (hand[i].isSelected() ? 1.0f : 0.0f);

        bool isHovered = isPointInRect(mx, my, cx, cy, cardW, cardH);

        Renderer::drawCard(hand[i], cx, cy, cardW, cardH, isHovered, elev);

        string keyNum = "[" + to_string(i + 1) + "]";
        Renderer::drawText(keyNum, cx + (cardW / 2.0f) - 10.0f, baseY + cardH + 12.0f, 1.1f, Color(0.6f, 0.7f, 0.65f, 0.8f));
    }
}

void GameWindow::renderRoundGameplay() {
    renderHUD();
    renderScoringPreview();
    renderControls();
    renderCards();
}

void GameWindow::renderUI(float dt) {
    glClearColor(0.06f, 0.14f, 0.11f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Dynamic Hypnotic Felt Swirl Background khas Balatro
    float time = static_cast<float>(glfwGetTime());
    Renderer::drawAnimatedBackground(static_cast<float>(windowWidth), static_cast<float>(windowHeight), time);

    string state = engine.getCurrentStateName();

    if (state == "MAIN_MENU") {
        renderMainMenu();
    } else if (state == "BLIND_SELECT") {
        renderBlindSelect();
    } else {
        updateCardAnimations(dt);
        renderRoundGameplay();
    }

    if (toastTimer > 0.0f) {
        float alpha = min(1.0f, toastTimer);
        Renderer::drawToast(toastMessage, static_cast<float>(windowWidth), static_cast<float>(windowHeight), alpha);
    }
}

void GameWindow::run() {
    double lastTime = glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        double currentTime = glfwGetTime();
        float dt = static_cast<float>(currentTime - lastTime);
        lastTime = currentTime;
        if (dt > 0.1f) dt = 0.1f;

        if (toastTimer > 0.0f) {
            toastTimer -= dt;
            if (toastTimer < 0.0f) toastTimer = 0.0f;
        }

        glfwGetCursorPos(window, &mouseX, &mouseY);

        renderUI(dt);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}

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
    : window(nullptr), windowWidth(width), windowHeight(height), windowTitle(title),
      scoringStep(ScoringStep::IDLE), scoringTimer(0.0f), animSpeedMultiplier(1.0f),
      currentTriggerIndex(0), animChips(0), animMult(0), animFinalScore(0),
      chipsBoxScale(1.0f), multBoxScale(1.0f), displayedRoundScore(0),
      targetRoundScore(0), cameraShake(0.0f),
      isDiscarding(false), discardTimer(0.0f),
      hoveredCardIndex(-1), hoverTooltipAlpha(0.0f),
      toastTimer(0.0f), mouseX(0), mouseY(0) {}

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

    // Fast-forward animasi scoring jika sedang berjalan
    if (scoringStep != ScoringStep::IDLE) {
        animSpeedMultiplier = 3.5f;
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
            displayedRoundScore = 0;
            targetRoundScore = 0;
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
                    displayedRoundScore = engine.getCurrentScore();
                    targetRoundScore = engine.getCurrentScore();
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

        // Tombol PLAY HAND (Memicu Sekuens Animasi Scoring)
        if (isPointInRect(mx, my, 410.0f, 440.0f, 140.0f, 48.0f)) {
            startScoringSequence();
            return;
        }

        // Tombol DISCARD (x: 580, y: 440, w: 140, h: 48)
        if (isPointInRect(mx, my, 580.0f, 440.0f, 140.0f, 48.0f)) {
            startDiscardSequence();
            return;
        }
        return;
    }

    // 4. KLIK PADA LAYAR ROUND_WON (CASH OUT)
    if (state == "ROUND_WON") {
        if (isPointInRect(mx, my, 460.0f, 440.0f, 180.0f, 48.0f)) {
            engine.cashOut();
            displayedRoundScore = 0;
            targetRoundScore = 0;
            return;
        }
        return;
    }

    // 5. KLIK PADA LAYAR SHOP (THE SHOP)
    if (state == "SHOP") {
        const auto& inventory = engine.getShopInventory();
        float itemW = 92.0f;
        float itemH = 135.0f;
        float spacing = 150.0f;
        float startX = 512.0f - ((inventory.size() * spacing) * 0.5f) + 30.0f;

        // Cek klik tombol BUY pada etalase toko
        for (size_t i = 0; i < inventory.size(); ++i) {
            float jx = startX + i * spacing;
            float jy = 300.0f;
            float btnW = 90.0f;
            float btnH = 34.0f;
            float btnX = jx + (itemW - btnW) * 0.5f;
            float btnY = jy + itemH + 15.0f;

            if (isPointInRect(mx, my, btnX, btnY, btnW, btnH)) {
                try {
                    engine.buyJoker(static_cast<int>(i));
                    showToast("Purchased " + inventory[i].getName() + "!");
                } catch (const GameException& e) {
                    showToast(e.what());
                }
                return;
            }
        }

        // Cek klik tombol SELL pada slot Joker pemain
        const auto& jokers = engine.getJokers();
        int maxJokers = engine.getMaxJokers();
        float slotW = 76.0f;
        float slotH = 110.0f;
        float slotSpacing = 16.0f;
        float jokersStartX = 512.0f - (maxJokers * slotW + (maxJokers - 1) * slotSpacing) * 0.5f;
        float slotY = 125.0f;

        for (size_t i = 0; i < jokers.size(); ++i) {
            float sx = jokersStartX + i * (slotW + slotSpacing);
            float sellW = 70.0f;
            float sellH = 24.0f;
            float sellX = sx + (slotW - sellW) * 0.5f;
            float sellY = slotY + slotH + 6.0f;

            if (isPointInRect(mx, my, sellX, sellY, sellW, sellH)) {
                try {
                    int sellVal = jokers[i].getSellValue();
                    string jName = jokers[i].getName();
                    engine.sellJoker(static_cast<int>(i));
                    showToast("Sold " + jName + " for $" + to_string(sellVal) + "!");
                } catch (const GameException& e) {
                    showToast(e.what());
                }
                return;
            }
        }

        // Tombol REROLL ($5) (x: 310, y: 605, w: 180, h: 48)
        if (isPointInRect(mx, my, 310.0f, 605.0f, 180.0f, 48.0f)) {
            try {
                engine.rerollShop();
                showToast("Shop inventory rerolled!");
            } catch (const GameException& e) {
                showToast(e.what());
            }
            return;
        }

        // Tombol NEXT ROUND (x: 530, y: 605, w: 180, h: 48)
        if (isPointInRect(mx, my, 530.0f, 605.0f, 180.0f, 48.0f)) {
            engine.leaveShop();
            return;
        }
        return;
    }

    // 6. KLIK PADA LAYAR GAME_OVER
    if (state == "GAME_OVER") {
        if (isPointInRect(mx, my, 390.0f, 440.0f, 160.0f, 48.0f)) {
            engine.startNewRun();
            displayedRoundScore = 0;
            targetRoundScore = 0;
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

    // Fast-forward animasi scoring jika tombol Space/Enter ditekan
    if (scoringStep != ScoringStep::IDLE) {
        if (key == GLFW_KEY_SPACE || key == GLFW_KEY_ENTER) {
            animSpeedMultiplier = 3.5f;
        }
        return;
    }

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
            displayedRoundScore = 0;
            targetRoundScore = 0;
        } else if (state == "BLIND_SELECT") {
            engine.selectBlind();
            displayedRoundScore = engine.getCurrentScore();
            targetRoundScore = engine.getCurrentScore();
        } else if (state == "SELECTING_CARDS") {
            startScoringSequence();
        } else if (state == "ROUND_WON") {
            engine.cashOut();
            displayedRoundScore = 0;
            targetRoundScore = 0;
        } else if (state == "SHOP") {
            engine.leaveShop();
        } else if (state == "GAME_OVER") {
            engine.startNewRun();
            displayedRoundScore = 0;
            targetRoundScore = 0;
        }
    } else if (key == GLFW_KEY_D) {
        if (state == "SELECTING_CARDS") {
            startDiscardSequence();
        }
    } else if (key == GLFW_KEY_R) {
        engine.startNewRun();
        displayedRoundScore = 0;
        targetRoundScore = 0;
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

// ================= RENDER TOKO (THE SHOP) =================
void GameWindow::renderShop() {
    float mx = static_cast<float>(mouseX);
    float my = static_cast<float>(mouseY);

    // 1. Header Toko
    Renderer::drawCenteredText("THE SHOP", 512.0f, 40.0f, 3.8f, Theme::GOLD);
    string moneyStr = "Your Money: $" + to_string(engine.getMoney());
    Renderer::drawCenteredText(moneyStr, 512.0f, 90.0f, 2.0f, Theme::GOLD);

    // 2. Baris Slot Joker Pemain Saat Ini (di atas)
    renderJokerSlots();

    // 3. Panel Etalase Kartu Toko
    Renderer::drawRect(120.0f, 240.0f, 784.0f, 340.0f, Theme::PANEL_BG);
    Renderer::drawRectOutline(120.0f, 240.0f, 784.0f, 340.0f, 2.0f, Color(0.2f, 0.4f, 0.35f, 0.8f));
    Renderer::drawCenteredText("JOKERS FOR SALE", 512.0f, 255.0f, 1.6f, Theme::TEXT_WHITE);

    const auto& inventory = engine.getShopInventory();
    float itemW = 92.0f;
    float itemH = 135.0f;
    float spacing = 150.0f;
    float startX = 512.0f - ((inventory.size() * spacing) * 0.5f) + 30.0f;

    for (size_t i = 0; i < inventory.size(); ++i) {
        float jx = startX + i * spacing;
        float jy = 300.0f;
        bool isHovered = isPointInRect(mx, my, jx, jy, itemW, itemH);

        Renderer::drawJokerCard(inventory[i].getName(), inventory[i].getDescription(), inventory[i].getCost(),
                                jx, jy, itemW, itemH, isHovered, isHovered ? 1.05f : 1.0f);

        // Tombol BUY
        float btnW = 90.0f;
        float btnH = 34.0f;
        float btnX = jx + (itemW - btnW) * 0.5f;
        float btnY = jy + itemH + 15.0f;
        bool canBuy = (engine.getMoney() >= inventory[i].getCost() && static_cast<int>(engine.getJokers().size()) < engine.getMaxJokers());
        bool hoverBuy = isPointInRect(mx, my, btnX, btnY, btnW, btnH);

        Renderer::drawButton("BUY", btnX, btnY, btnW, btnH, Theme::CHIPS_BLUE, Theme::TEXT_WHITE, hoverBuy, canBuy);

        if (isHovered) {
            renderJokerTooltip(inventory[i], jx, jy);
        }
    }

    if (inventory.empty()) {
        Renderer::drawCenteredText("SOLD OUT! REROLL FOR MORE JOKERS.", 512.0f, 380.0f, 1.8f, Theme::DEBUFF_GRAY);
    }

    // 4. Tombol Kontrol Toko: REROLL ($5) & NEXT ROUND
    float btnY = 605.0f;
    bool canReroll = (engine.getMoney() >= engine.getRerollCost());
    bool hoverReroll = isPointInRect(mx, my, 310.0f, btnY, 180.0f, 48.0f);
    string rerollText = "REROLL ($" + to_string(engine.getRerollCost()) + ")";
    Renderer::drawButton(rerollText, 310.0f, btnY, 180.0f, 48.0f, Theme::MULT_RED, Theme::TEXT_WHITE, hoverReroll, canReroll);

    bool hoverNext = isPointInRect(mx, my, 530.0f, btnY, 180.0f, 48.0f);
    Renderer::drawButton("NEXT ROUND", 530.0f, btnY, 180.0f, 48.0f, Theme::GOLD, Theme::TEXT_DARK, hoverNext, true);
}

// ================= RENDER 5 SLOT JOKER DI ATAS MEJA =================
void GameWindow::renderJokerSlots() {
    float mx = static_cast<float>(mouseX);
    float my = static_cast<float>(mouseY);

    const auto& jokers = engine.getJokers();
    int maxJokers = engine.getMaxJokers();

    float slotW = 76.0f;
    float slotH = 110.0f;
    float spacing = 16.0f;
    float startX = 310.0f + (680.0f - (maxJokers * slotW + (maxJokers - 1) * spacing)) * 0.5f;
    float slotY = 30.0f;

    string state = engine.getCurrentStateName();
    if (state == "SHOP") {
        startX = 512.0f - (maxJokers * slotW + (maxJokers - 1) * spacing) * 0.5f;
        slotY = 125.0f;
    }

    for (int i = 0; i < maxJokers; ++i) {
        float sx = startX + i * (slotW + spacing);

        if (i < static_cast<int>(jokers.size())) {
            bool isHovered = isPointInRect(mx, my, sx, slotY, slotW, slotH);
            Renderer::drawJokerCard(jokers[i].getName(), jokers[i].getDescription(), 0,
                                    sx, slotY, slotW, slotH, isHovered, isHovered ? 1.06f : 1.0f);

            // Jika di Shop, tampilkan tombol SELL di bawah slot Joker
            if (state == "SHOP") {
                float sellW = 70.0f;
                float sellH = 24.0f;
                float sellX = sx + (slotW - sellW) * 0.5f;
                float sellY = slotY + slotH + 6.0f;
                bool hoverSell = isPointInRect(mx, my, sellX, sellY, sellW, sellH);
                string sellText = "SELL $" + to_string(jokers[i].getSellValue());
                Renderer::drawButton(sellText, sellX, sellY, sellW, sellH, Color(0.35f, 0.20f, 0.20f), Theme::GOLD, hoverSell, true);
            }

            if (isHovered) {
                renderJokerTooltip(jokers[i], sx, slotY);
            }
        } else {
            // Slot kosong
            Renderer::drawRect(sx, slotY, slotW, slotH, Color(0.06f, 0.12f, 0.10f, 0.7f));
            Renderer::drawRectOutline(sx, slotY, slotW, slotH, 1.5f, Color(0.25f, 0.40f, 0.35f, 0.6f));
            Renderer::drawCenteredText("EMPTY", sx + slotW * 0.5f, slotY + slotH * 0.5f - 8.0f, 1.0f, Color(0.4f, 0.5f, 0.45f, 0.8f));
        }
    }
}

void GameWindow::renderJokerTooltip(const Joker& joker, float x, float y) {
    float tipW = 220.0f;
    float tipH = 95.0f;
    float tipX = x + 38.0f - tipW * 0.5f;
    tipX = max(20.0f, min(static_cast<float>(windowWidth) - tipW - 20.0f, tipX));
    float tipY = y - tipH - 12.0f;
    if (tipY < 15.0f) {
        tipY = y + 120.0f;
    }

    Renderer::drawRect(tipX + 4.0f, tipY + 4.0f, tipW, tipH, Color(0.0f, 0.0f, 0.0f, 0.45f));
    Renderer::drawRect(tipX, tipY, tipW, tipH, Color(0.97f, 0.96f, 0.93f));
    Renderer::drawRectOutline(tipX, tipY, tipW, tipH, 2.5f, Theme::GOLD);

    // Title Badge
    Renderer::drawRect(tipX + 6.0f, tipY + 6.0f, tipW - 12.0f, 22.0f, Theme::MULT_RED);
    Renderer::drawCenteredText(joker.getName(), tipX + tipW * 0.5f, tipY + 10.0f, 1.25f, Theme::TEXT_WHITE);

    // Divider
    Renderer::drawRect(tipX + 10.0f, tipY + 34.0f, tipW - 20.0f, 1.0f, Color(0.75f, 0.75f, 0.70f));

    // Description
    Renderer::drawCenteredText(joker.getDescription(), tipX + tipW * 0.5f, tipY + 45.0f, 1.05f, Theme::TEXT_DARK);

    // Rarity Badge
    string rarityStr = (joker.getRarity() == JokerRarity::RARE) ? "RARE" : 
                       ((joker.getRarity() == JokerRarity::UNCOMMON) ? "UNCOMMON" : "COMMON");
    Color rarityCol = (joker.getRarity() == JokerRarity::RARE) ? Theme::MULT_RED : 
                      ((joker.getRarity() == JokerRarity::UNCOMMON) ? Theme::CHIPS_BLUE : Color(0.3f, 0.6f, 0.4f));
    Renderer::drawCenteredText(rarityStr, tipX + tipW * 0.5f, tipY + 74.0f, 1.0f, rarityCol);
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

    // Round Score dengan Nilai Bergulir Halus
    Renderer::drawText("ROUND SCORE:", 45.0f, 275.0f, 1.4f, Theme::TEXT_WHITE);
    string scoreStr = to_string(displayedRoundScore) + " / " + to_string(blind.getTargetScore());
    Renderer::drawText(scoreStr, 45.0f, 298.0f, 2.0f, Theme::GOLD);

    // Dynamic Target Progress Bar (Bilah Kemajuan Skor)
    float barX = 45.0f;
    float barY = 325.0f;
    float barW = 220.0f;
    float barH = 10.0f;
    Renderer::drawRect(barX, barY, barW, barH, Color(0.10f, 0.16f, 0.14f, 0.95f));
    Renderer::drawRectOutline(barX, barY, barW, barH, 1.5f, Color(0.25f, 0.40f, 0.35f, 0.8f));

    float progress = min(1.0f, static_cast<float>(displayedRoundScore) / static_cast<float>(max(1, blind.getTargetScore())));
    if (progress > 0.001f) {
        float fillW = barW * progress;
        Color barColor = (displayedRoundScore >= blind.getTargetScore()) ? Color(0.25f, 0.95f, 0.45f) : Theme::GOLD;
        Renderer::drawRect(barX, barY, fillW, barH, barColor);
        if (fillW > 3.0f) {
            Renderer::drawRect(barX + fillW - 2.0f, barY, 2.0f, barH, Color(1.0f, 1.0f, 1.0f, 0.85f));
        }
    }

    string moneyStr = "Money: $" + to_string(engine.getMoney());
    Renderer::drawText(moneyStr, 45.0f, 346.0f, 1.8f, Theme::GOLD);

    string handsStr = "Hands: " + to_string(engine.getHandsRemaining());
    Renderer::drawText(handsStr, 45.0f, 380.0f, 1.8f, Theme::CHIPS_BLUE);

    string discardsStr = "Discards: " + to_string(engine.getDiscardsRemaining());
    Renderer::drawText(discardsStr, 45.0f, 410.0f, 1.8f, Theme::MULT_RED);

    string deckStr = "Deck: " + to_string(engine.getDeckRemainingCount()) + " cards";
    Renderer::drawText(deckStr, 45.0f, 440.0f, 1.3f, Theme::TEXT_WHITE);
}

void GameWindow::spawnFloatingText(const string& text, float x, float y, Color col, float scale) {
    FloatingText ft;
    ft.text = text;
    ft.x = x;
    ft.y = y;
    ft.vy = -45.0f;
    ft.color = col;
    ft.scale = scale;
    ft.alpha = 1.0f;
    ft.lifetime = 0.0f;
    floatingTexts.push_back(ft);
}

void GameWindow::updateFloatingTexts(float dt) {
    float effectiveDt = dt * animSpeedMultiplier;
    for (auto it = floatingTexts.begin(); it != floatingTexts.end(); ) {
        it->lifetime += effectiveDt;
        it->y += it->vy * effectiveDt;
        it->vy += 15.0f * effectiveDt; // pelan-pelan melambat
        if (it->lifetime > 0.35f) {
            it->alpha -= 2.2f * effectiveDt;
        }

        if (it->alpha <= 0.01f || it->lifetime >= 0.85f) {
            it = floatingTexts.erase(it);
        } else {
            ++it;
        }
    }
}

void GameWindow::renderFloatingTexts() {
    for (const auto& ft : floatingTexts) {
        Color c = ft.color;
        c.a = max(0.0f, min(1.0f, ft.alpha));
        Renderer::drawCenteredText(ft.text, ft.x, ft.y, ft.scale, c);
    }
}

void GameWindow::startScoringSequence() {
    vector<Card> selected = engine.getSelectedCards();
    if (selected.empty()) {
        showToast("Select 1 to 5 cards first!");
        return;
    }
    if (selected.size() > 5) {
        showToast("Maximum 5 cards can be played!");
        return;
    }
    if (engine.getHandsRemaining() <= 0) {
        showToast("No hands remaining!");
        return;
    }

    ScoringEngine se;
    ScoreResult baseResult = se.calculateScore(selected);

    // Hitung efek Joker untuk sekuens animasi
    int bonusChips = 0;
    int bonusMult = 0;
    ScoringContext ctx{
        selected,
        baseResult.scoringCards,
        engine.getDeckRemainingCount(),
        engine.getDiscardsRemaining()
    };
    for (const auto& joker : engine.getJokers()) {
        joker.trigger(ctx, bonusChips, bonusMult);
    }

    activeScoreResult = se.calculateScore(selected, bonusChips, bonusMult);

    scoringAnimatedCards.clear();
    floatingTexts.clear();

    float cardW = 88.0f;
    float spacing = 20.0f;
    size_t count = selected.size();
    float totalW = count * cardW + (count - 1) * spacing;
    float startCenterX = 310.0f + (680.0f - totalW) * 0.5f;
    float centerTargetY = 245.0f;

    const auto& hand = engine.getHand();
    for (size_t i = 0; i < count; ++i) {
        AnimatedCard ac(selected[i]);

        ac.isScoring = false;
        for (const auto& sc : activeScoreResult.scoringCards) {
            if (sc == ac.card) {
                ac.isScoring = true;
                break;
            }
        }

        float originX = 55.0f + i * 116.0f;
        float originY = 530.0f;
        for (size_t hIdx = 0; hIdx < hand.size(); ++hIdx) {
            if (hand[hIdx] == ac.card && hIdx < cardAnims.size() && cardAnims[hIdx].initialized) {
                originX = cardAnims[hIdx].x;
                originY = cardAnims[hIdx].y;
                break;
            }
        }

        ac.startX = originX;
        ac.startY = originY;
        ac.currentX = originX;
        ac.currentY = originY;
        ac.targetX = startCenterX + i * (cardW + spacing);
        ac.targetY = centerTargetY;
        ac.scale = 1.0f;
        ac.glow = 0.0f;
        ac.triggered = false;

        scoringAnimatedCards.push_back(ac);
    }

    scoringStep = ScoringStep::SLIDE_IN;
    scoringTimer = 0.0f;
    animSpeedMultiplier = 1.0f;
    currentTriggerIndex = 0;

    animChips = activeScoreResult.baseChips;
    animMult = 1;
    animFinalScore = 0;
    chipsBoxScale = 1.0f;
    multBoxScale = 1.0f;
    cameraShake = 0.0f;

    displayedRoundScore = engine.getCurrentScore();
    targetRoundScore = engine.getCurrentScore();
}

void GameWindow::updateScoringSequence(float dt) {
    if (scoringStep == ScoringStep::IDLE) return;

    float effectiveDt = dt * animSpeedMultiplier;
    scoringTimer += effectiveDt;

    // Lerp chip/mult box scaling back to 1.0f
    chipsBoxScale += (1.0f - chipsBoxScale) * min(1.0f, 12.0f * effectiveDt);
    multBoxScale += (1.0f - multBoxScale) * min(1.0f, 12.0f * effectiveDt);

    // Lerp card scale and glow back to normal
    for (auto& ac : scoringAnimatedCards) {
        ac.scale += (1.0f - ac.scale) * min(1.0f, 10.0f * effectiveDt);
        ac.glow += (0.0f - ac.glow) * min(1.0f, 8.0f * effectiveDt);
    }

    if (scoringStep == ScoringStep::SLIDE_IN) {
        for (auto& ac : scoringAnimatedCards) {
            ac.currentX += (ac.targetX - ac.currentX) * min(1.0f, 14.0f * effectiveDt);
            ac.currentY += (ac.targetY - ac.currentY) * min(1.0f, 14.0f * effectiveDt);
        }

        if (scoringTimer >= 0.35f) {
            scoringStep = ScoringStep::CARD_TRIGGERS;
            scoringTimer = 0.0f;
            currentTriggerIndex = 0;
        }
    } 
    else if (scoringStep == ScoringStep::CARD_TRIGGERS) {
        float triggerInterval = 0.32f;
        if (currentTriggerIndex < static_cast<int>(scoringAnimatedCards.size())) {
            if (scoringTimer >= triggerInterval) {
                scoringTimer -= triggerInterval;
                auto& ac = scoringAnimatedCards[currentTriggerIndex];
                ac.triggered = true;
                ac.scale = 1.25f;
                ac.glow = 1.0f;

                if (ac.isScoring) {
                    int chipGained = ac.card.getBaseChips();
                    animChips += chipGained;
                    chipsBoxScale = 1.30f;

                    string chipPop = "+" + to_string(chipGained);
                    spawnFloatingText(chipPop, ac.targetX + 44.0f, ac.targetY - 20.0f, Theme::CHIPS_BLUE, 1.8f);
                } else {
                    spawnFloatingText("...", ac.targetX + 44.0f, ac.targetY - 14.0f, Theme::DEBUFF_GRAY, 1.2f);
                }

                currentTriggerIndex++;
            }
        } else {
            if (scoringTimer >= 0.28f) {
                scoringStep = ScoringStep::MULT_TRIGGER;
                scoringTimer = 0.0f;
            }
        }
    } 
    else if (scoringStep == ScoringStep::MULT_TRIGGER) {
        if (scoringTimer >= 0.10f && animMult == 1) {
            animMult = activeScoreResult.totalMult;
            multBoxScale = 1.35f;
            string multPop = "+" + to_string(activeScoreResult.totalMult) + " MULT";
            spawnFloatingText(multPop, 790.0f, 80.0f, Theme::MULT_RED, 2.0f);
        }

        if (scoringTimer >= 0.45f) {
            scoringStep = ScoringStep::FINAL_SLAM;
            scoringTimer = 0.0f;
        }
    } 
    else if (scoringStep == ScoringStep::FINAL_SLAM) {
        if (animFinalScore == 0) {
            animFinalScore = activeScoreResult.finalScore;
            chipsBoxScale = 1.35f;
            multBoxScale = 1.35f;
            cameraShake = 8.0f;

            string finalPop = "+" + to_string(animFinalScore) + "!";
            spawnFloatingText(finalPop, 650.0f, 195.0f, Theme::GOLD, 2.6f);
        }

        if (scoringTimer >= 0.50f) {
            scoringStep = ScoringStep::SCORE_TICK;
            scoringTimer = 0.0f;
            targetRoundScore = engine.getCurrentScore() + activeScoreResult.finalScore;
        }
    } 
    else if (scoringStep == ScoringStep::SCORE_TICK) {
        if (displayedRoundScore < targetRoundScore) {
            int step = max(1, static_cast<int>((targetRoundScore - displayedRoundScore) * 12.0f * effectiveDt));
            displayedRoundScore = min(targetRoundScore, displayedRoundScore + step);
        }

        if (scoringTimer >= 0.55f && displayedRoundScore >= targetRoundScore) {
            scoringStep = ScoringStep::SLIDE_OUT;
            scoringTimer = 0.0f;
            for (auto& ac : scoringAnimatedCards) {
                ac.targetX = static_cast<float>(windowWidth) + 180.0f; // Meluncur ke kanan ke luar layar!
            }
        }
    } 
    else if (scoringStep == ScoringStep::SLIDE_OUT) {
        for (auto& ac : scoringAnimatedCards) {
            ac.currentX += (ac.targetX - ac.currentX) * min(1.0f, 15.0f * effectiveDt);
        }

        if (scoringTimer >= 0.35f) {
            scoringStep = ScoringStep::FINISHED;
        }
    }
    else if (scoringStep == ScoringStep::FINISHED) {
        try {
            engine.playHand();
        } catch (const GameException& e) {
            showToast(e.what());
        }

        displayedRoundScore = engine.getCurrentScore();
        targetRoundScore = engine.getCurrentScore();
        scoringAnimatedCards.clear();
        scoringStep = ScoringStep::IDLE;
    }
}

void GameWindow::renderActiveScoringDisplay() {
    // 1. Nama Hand Poker Besar di Atas
    Renderer::drawCenteredText(activeScoreResult.handName, 650.0f, 55.0f, 2.6f, Theme::GOLD);

    // 2. Kotak CHIPS Berdenyut (Biru)
    float bw = 180.0f * chipsBoxScale;
    float bh = 85.0f * chipsBoxScale;
    float bx = 510.0f - bw * 0.5f;
    float by = 145.0f - bh * 0.5f;

    Renderer::drawRect(bx, by, bw, bh, Theme::CHIPS_BLUE);
    Renderer::drawRectOutline(bx, by, bw, bh, 3.0f, (chipsBoxScale > 1.05f) ? Color(1.0f, 1.0f, 1.0f, 1.0f) : Color(1, 1, 1, 0.8f));
    Renderer::drawCenteredText("CHIPS", bx + bw * 0.5f, by + 10.0f, 1.4f, Theme::TEXT_WHITE);
    string cStr = to_string(animChips);
    Renderer::drawCenteredText(cStr, bx + bw * 0.5f, by + 35.0f, 2.8f, Theme::TEXT_WHITE);

    // 3. Simbol Perkalian "X"
    Renderer::drawCenteredText("X", 650.0f, 140.0f, 3.5f, Theme::TEXT_WHITE);

    // 4. Kotak MULT Berdenyut (Merah)
    float mw = 180.0f * multBoxScale;
    float mh = 85.0f * multBoxScale;
    float mx = 790.0f - mw * 0.5f;
    float my = 145.0f - mh * 0.5f;

    Renderer::drawRect(mx, my, mw, mh, Theme::MULT_RED);
    Renderer::drawRectOutline(mx, my, mw, mh, 3.0f, (multBoxScale > 1.05f) ? Color(1.0f, 1.0f, 1.0f, 1.0f) : Color(1, 1, 1, 0.8f));
    Renderer::drawCenteredText("MULT", mx + mw * 0.5f, my + 10.0f, 1.4f, Theme::TEXT_WHITE);
    string mStr = to_string(animMult);
    Renderer::drawCenteredText(mStr, mx + mw * 0.5f, my + 35.0f, 2.8f, Theme::TEXT_WHITE);

    // 5. Total Skor yang Terhitung saat Slam
    if (scoringStep == ScoringStep::FINAL_SLAM || scoringStep == ScoringStep::SCORE_TICK) {
        string totalStr = "SCORED +" + to_string(animFinalScore) + " PTS!";
        Renderer::drawCenteredText(totalStr, 650.0f, 215.0f, 2.2f, Theme::GOLD);
    }

    // 6. Kartu-kartu yang Dimainkan di Meja Tengah
    for (const auto& ac : scoringAnimatedCards) {
        float cardW = 88.0f * ac.scale;
        float cardH = 130.0f * ac.scale;
        float renderX = ac.currentX - (cardW - 88.0f) * 0.5f;
        float renderY = ac.currentY - (cardH - 130.0f) * 0.5f;

        Renderer::drawCard(ac.card, renderX, renderY, cardW, cardH, false, ac.isScoring ? 0.6f : 0.0f);

        if (ac.glow > 0.05f) {
            Renderer::drawRectOutline(renderX - 3.0f, renderY - 3.0f, cardW + 6.0f, cardH + 6.0f, 3.0f, Color(1.0f, 0.90f, 0.25f, ac.glow));
        }
    }
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

    if (scoringStep != ScoringStep::IDLE) {
        renderActiveScoringDisplay();
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

        // Hitung efek sinergi Joker pada perkiraan skor
        int bonusChips = 0;
        int bonusMult = 0;
        ScoringContext ctx{
            selected,
            handRes.scoringCards,
            engine.getDeckRemainingCount(),
            engine.getDiscardsRemaining()
        };
        for (const auto& joker : engine.getJokers()) {
            joker.trigger(ctx, bonusChips, bonusMult);
        }

        int totalC = base.baseChips + cardChips + bonusChips;
        int totalM = max(1, base.baseMult + bonusMult);
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

    if (scoringStep != ScoringStep::IDLE) {
        Renderer::drawButton("SCORING...", 410.0f, 440.0f, 140.0f, 48.0f, Theme::GOLD, Theme::TEXT_DARK, false, false);
        bool hoverFast = isPointInRect(mx, my, 580.0f, 440.0f, 140.0f, 48.0f);
        Renderer::drawButton("FAST [SPC]", 580.0f, 440.0f, 140.0f, 48.0f, Color(0.20f, 0.45f, 0.35f), Theme::TEXT_WHITE, hoverFast, true);
        return;
    }

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

void GameWindow::startDiscardSequence() {
    if (scoringStep != ScoringStep::IDLE || isDiscarding) return;

    vector<Card> selected = engine.getSelectedCards();
    if (selected.empty()) {
        showToast("Select 1 to 5 cards to discard!");
        return;
    }
    if (selected.size() > 5) {
        showToast("Maximum 5 cards can be discarded!");
        return;
    }
    if (engine.getDiscardsRemaining() <= 0) {
        showToast("No discards remaining!");
        return;
    }

    discardAnimatedCards.clear();
    const auto& hand = engine.getHand();
    for (size_t i = 0; i < hand.size(); ++i) {
        if (hand[i].isSelected()) {
            AnimatedCard ac(hand[i]);
            ac.currentX = (i < cardAnims.size() && cardAnims[i].initialized) ? cardAnims[i].x : (55.0f + i * 116.0f);
            ac.currentY = (i < cardAnims.size() && cardAnims[i].initialized) ? cardAnims[i].y : 530.0f;
            ac.targetX = static_cast<float>(windowWidth) + 180.0f; // Geser ke kanan ke luar layar!
            ac.targetY = ac.currentY;
            discardAnimatedCards.push_back(ac);
        }
    }

    isDiscarding = true;
    discardTimer = 0.0f;
}

void GameWindow::updateDiscardSequence(float dt) {
    if (!isDiscarding) return;

    discardTimer += dt;
    for (auto& ac : discardAnimatedCards) {
        ac.currentX += (ac.targetX - ac.currentX) * min(1.0f, 15.0f * dt);
    }

    if (discardTimer >= 0.28f) {
        try {
            engine.discard();
        } catch (const GameException& e) {
            showToast(e.what());
        }
        isDiscarding = false;
        discardAnimatedCards.clear();
    }
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

    hoveredCardIndex = -1;

    for (size_t i = 0; i < hand.size(); ++i) {
        float targetX = startX + i * (cardW + spacing);

        float curX = cardAnims[i].initialized ? cardAnims[i].x : targetX;
        float curY = cardAnims[i].initialized ? cardAnims[i].y : baseY;
        bool isHovered = (scoringStep == ScoringStep::IDLE && !isDiscarding) && isPointInRect(mx, my, curX, curY, cardW, cardH);
        if (isHovered) {
            hoveredCardIndex = static_cast<int>(i);
        }

        float targetElev = hand[i].isSelected() ? 1.0f : (isHovered ? 0.35f : 0.0f);
        float targetY = baseY - targetElev * 32.0f;

        if (!cardAnims[i].initialized) {
            // Kartu baru: Meluncur anggun dari bawah luar layar!
            cardAnims[i].x = targetX;
            cardAnims[i].y = static_cast<float>(windowHeight) + 160.0f;
            cardAnims[i].elevation = targetElev;
            cardAnims[i].delayTimer = i * 0.045f; // Dealing bertahap
            cardAnims[i].initialized = true;
        } else {
            if (cardAnims[i].delayTimer > 0.0f) {
                cardAnims[i].delayTimer -= dt;
            } else {
                float speed = min(1.0f, 16.0f * dt);
                cardAnims[i].x += (targetX - cardAnims[i].x) * speed;
                cardAnims[i].y += (targetY - cardAnims[i].y) * speed;
                cardAnims[i].elevation += (targetElev - cardAnims[i].elevation) * speed;
            }
        }
    }
}

void GameWindow::renderCardTooltip(float dt) {
    const auto& hand = engine.getHand();

    if (hoveredCardIndex >= 0 && hoveredCardIndex < static_cast<int>(hand.size()) && scoringStep == ScoringStep::IDLE && !isDiscarding) {
        hoverTooltipAlpha += (1.0f - hoverTooltipAlpha) * min(1.0f, 22.0f * dt);
    } else {
        hoverTooltipAlpha += (0.0f - hoverTooltipAlpha) * min(1.0f, 22.0f * dt);
    }

    if (hoverTooltipAlpha <= 0.01f || hoveredCardIndex < 0 || hoveredCardIndex >= static_cast<int>(hand.size())) {
        return;
    }

    const Card& card = hand[hoveredCardIndex];
    float cardX = (hoveredCardIndex < static_cast<int>(cardAnims.size()) && cardAnims[hoveredCardIndex].initialized) ? 
                   cardAnims[hoveredCardIndex].x : (55.0f + hoveredCardIndex * 116.0f);
    float cardY = (hoveredCardIndex < static_cast<int>(cardAnims.size()) && cardAnims[hoveredCardIndex].initialized) ? 
                   cardAnims[hoveredCardIndex].y : 530.0f;
    float cardW = 88.0f;

    float tipW = 160.0f;
    float tipH = 70.0f;
    float tipX = cardX + (cardW - tipW) * 0.5f;
    tipX = max(15.0f, min(static_cast<float>(windowWidth) - tipW - 15.0f, tipX));
    float tipY = cardY - tipH - 14.0f;

    // 1. Bayangan Tooltip
    Renderer::drawRect(tipX + 4.0f, tipY + 4.0f, tipW, tipH, Color(0.0f, 0.0f, 0.0f, 0.38f * hoverTooltipAlpha));

    // 2. Bodi Kotak Putih Bersih
    Renderer::drawRect(tipX, tipY, tipW, tipH, Color(0.97f, 0.97f, 0.95f, hoverTooltipAlpha));
    Renderer::drawRectOutline(tipX, tipY, tipW, tipH, 2.0f, Color(0.25f, 0.32f, 0.30f, hoverTooltipAlpha));

    // 3. Kotak Badge Header Kartu ("9 of Hearts")
    float badgeX = tipX + 6.0f;
    float badgeY = tipY + 6.0f;
    float badgeW = tipW - 12.0f;
    float badgeH = 26.0f;
    Renderer::drawRectOutline(badgeX, badgeY, badgeW, badgeH, 1.5f, Color(0.65f, 0.70f, 0.68f, hoverTooltipAlpha));

    // Format Nama Lengkap Kartu: Rank + " of " + Suit
    string rankName = card.getRankString();
    Rank r = card.getRank();
    if (r == Rank::JACK) rankName = "Jack";
    else if (r == Rank::QUEEN) rankName = "Queen";
    else if (r == Rank::KING) rankName = "King";
    else if (r == Rank::ACE) rankName = "Ace";

    string suitName = "Spades";
    Color suitCol = Theme::TEXT_DARK;
    if (card.getSuit() == Suit::HEARTS) { suitName = "Hearts"; suitCol = Theme::MULT_RED; }
    else if (card.getSuit() == Suit::DIAMONDS) { suitName = "Diamonds"; suitCol = Theme::MULT_RED; }
    else if (card.getSuit() == Suit::CLUBS) { suitName = "Clubs"; suitCol = Color(0.18f, 0.40f, 0.28f); }
    else if (card.getSuit() == Suit::SPADES) { suitName = "Spades"; suitCol = Theme::TEXT_DARK; }

    if (card.isDebuffed()) suitCol = Theme::DEBUFF_GRAY;

    string fullTitle = rankName + " of " + suitName;
    float titleScale = (fullTitle.length() > 14) ? 1.05f : 1.2f;
    Renderer::drawCenteredText(fullTitle, badgeX + badgeW * 0.5f, badgeY + 6.0f, titleScale, 
                               Color(suitCol.r, suitCol.g, suitCol.b, hoverTooltipAlpha));

    // 4. Garis Pembatas
    Renderer::drawRect(tipX + 10.0f, tipY + 36.0f, tipW - 20.0f, 1.0f, Color(0.78f, 0.80f, 0.78f, hoverTooltipAlpha));

    // 5. Bagian Bawah: "+X chips" persis seperti di Balatro
    if (card.isDebuffed()) {
        Renderer::drawCenteredText("DEBUFFED (0 chips)", tipX + tipW * 0.5f, tipY + 45.0f, 1.15f, 
                                   Color(Theme::MULT_RED.r, Theme::MULT_RED.g, Theme::MULT_RED.b, hoverTooltipAlpha));
    } else {
        string chipStr = "+" + to_string(card.getBaseChips()) + " chips";
        Renderer::drawCenteredText(chipStr, tipX + tipW * 0.5f, tipY + 45.0f, 1.4f, 
                                   Color(Theme::CHIPS_BLUE.r, Theme::CHIPS_BLUE.g, Theme::CHIPS_BLUE.b, hoverTooltipAlpha));
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
        // Jika animasi scoring atau discard sedang berjalan dan kartu ini sedang dianimasikan, lewati rendering kartu tangan
        if (scoringStep != ScoringStep::IDLE && hand[i].isSelected()) {
            continue;
        }
        if (isDiscarding && hand[i].isSelected()) {
            continue;
        }

        float cx = (i < cardAnims.size() && cardAnims[i].initialized) ? cardAnims[i].x : (startX + i * (cardW + spacing));
        float cy = (i < cardAnims.size() && cardAnims[i].initialized) ? cardAnims[i].y : baseY;
        float elev = (i < cardAnims.size() && cardAnims[i].initialized) ? cardAnims[i].elevation : (hand[i].isSelected() ? 1.0f : 0.0f);

        bool isHovered = (scoringStep == ScoringStep::IDLE && !isDiscarding) && isPointInRect(mx, my, cx, cy, cardW, cardH);

        Renderer::drawCard(hand[i], cx, cy, cardW, cardH, isHovered, elev);

        string keyNum = "[" + to_string(i + 1) + "]";
        Renderer::drawText(keyNum, cx + (cardW / 2.0f) - 10.0f, baseY + cardH + 12.0f, 1.1f, Color(0.6f, 0.7f, 0.65f, 0.8f));
    }

    // Gambar kartu yang sedang meluncur ke kanan karena di-discard
    if (isDiscarding) {
        for (const auto& dac : discardAnimatedCards) {
            Renderer::drawCard(dac.card, dac.currentX, dac.currentY, cardW, cardH, false, 0.5f);
        }
    }
}

void GameWindow::renderRoundGameplay() {
    renderHUD();
    renderScoringPreview();
    renderJokerSlots();
    renderControls();
    renderCards();
    renderCardTooltip(0.016f); // Update dan render tooltip
    renderFloatingTexts();
}

void GameWindow::renderUI(float dt) {
    glClearColor(0.06f, 0.14f, 0.11f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Dynamic Hypnotic Felt Swirl Background khas Balatro
    float time = static_cast<float>(glfwGetTime());
    Renderer::drawAnimatedBackground(static_cast<float>(windowWidth), static_cast<float>(windowHeight), time);

    // Screen Shake effect saat Mult Slam
    float shakeX = 0.0f;
    float shakeY = 0.0f;
    if (cameraShake > 0.01f) {
        shakeX = sin(time * 50.0f) * cameraShake;
        shakeY = cos(time * 60.0f) * cameraShake;
        cameraShake -= 16.0f * dt;
        if (cameraShake < 0.0f) cameraShake = 0.0f;
    }

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(shakeX, shakeY, 0.0f);

    string state = engine.getCurrentStateName();

    if (state == "MAIN_MENU") {
        renderMainMenu();
    } else if (state == "BLIND_SELECT") {
        renderBlindSelect();
    } else if (state == "SHOP") {
        renderShop();
    } else {
        updateCardAnimations(dt);
        updateScoringSequence(dt);
        updateDiscardSequence(dt);
        updateFloatingTexts(dt);
        renderRoundGameplay();
    }

    // Reset Modelview matrix untuk Toast notification
    glLoadIdentity();

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

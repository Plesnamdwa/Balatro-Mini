#include "../../include/ui/GameWindow.h"
#include "../../include/ui/PixelFont.h"
#include "../../include/core/Exceptions.h"
#include "../../include/audio/AudioManager.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cmath>

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
      toastTimer(0.0f), mouseX(0), mouseY(0),
      frameParticleTimer(0.0f),
      isTransitioning(false),
      transitionTimer(0.0f),
      transitionDuration(0.36f),
      transitionSwapped(false),
      renderedState("MAIN_MENU"),
      showRunInfoModal(false), hoveredRunInfoRow(-1), runInfoActiveTab(0),
      showSettingsModal(false), gearRotationAngle(0.0f),
      isDraggingBgmSlider(false), isDraggingSfxSlider(false) {}

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

    // Inisialisasi Audio Engine & Putar Musik Menu Utama
    AudioManager::getInstance().init();
    AudioManager::getInstance().playMusic(AudioAssets::BGM_MENU, true, 0.70f);

    return true;
}

void GameWindow::showToast(const string& message) {
    toastMessage = message;
    toastTimer = 2.5f;
}

void GameWindow::clearNotifications() {
    floatingTexts.clear();
    toastTimer = 0.0f;
    toastMessage.clear();
}

bool GameWindow::isPointInRect(float px, float py, float rx, float ry, float rw, float rh) const {
    return (px >= rx && px <= rx + rw && py >= ry && py <= ry + rh);
}

void GameWindow::handleMouseClick(int button, int action, int /*mods*/) {
    if (button != GLFW_MOUSE_BUTTON_LEFT) {
        return;
    }

    if (action == GLFW_RELEASE) {
        isDraggingBgmSlider = false;
        isDraggingSfxSlider = false;
        return;
    }

    if (action != GLFW_PRESS) {
        return;
    }

    float mx = static_cast<float>(mouseX);
    float my = static_cast<float>(mouseY);

    // 0A. KLIK PADA MODAL SETTINGS (Jika Sedang Terbuka)
    if (showSettingsModal) {
        float modalW = 440.0f;
        float modalH = 310.0f;
        float modalX = (static_cast<float>(windowWidth) - modalW) * 0.5f;
        float modalY = (static_cast<float>(windowHeight) - modalH) * 0.5f;

        // Tombol Close (w: 160, h: 40)
        float closeW = 160.0f;
        float closeH = 40.0f;
        float closeX = modalX + (modalW - closeW) * 0.5f;
        float closeY = modalY + modalH - 58.0f;
        if (isPointInRect(mx, my, closeX, closeY, closeW, closeH)) {
            AudioManager::getInstance().playSound(AudioAssets::SFX_BUTTON);
            showSettingsModal = false;
            isDraggingBgmSlider = false;
            isDraggingSfxSlider = false;
            return;
        }

        // BGM Slider Track Click
        float trackX = modalX + 35.0f;
        float trackW = modalW - 70.0f;
        if (isPointInRect(mx, my, trackX, modalY + 95.0f, trackW, 30.0f)) {
            isDraggingBgmSlider = true;
            float newVol = (mx - trackX) / trackW;
            if (newVol < 0.0f) newVol = 0.0f;
            if (newVol > 1.0f) newVol = 1.0f;
            AudioManager::getInstance().setBGMVolume(newVol);
            return;
        }

        // SFX Slider Track Click
        if (isPointInRect(mx, my, trackX, modalY + 165.0f, trackW, 30.0f)) {
            isDraggingSfxSlider = true;
            float newVol = (mx - trackX) / trackW;
            if (newVol < 0.0f) newVol = 0.0f;
            if (newVol > 1.0f) newVol = 1.0f;
            AudioManager::getInstance().setSFXVolume(newVol);
            AudioManager::getInstance().playSound(AudioAssets::SFX_BUTTON);
            return;
        }

        // Klik di luar area modal menutup modal
        if (!isPointInRect(mx, my, modalX, modalY, modalW, modalH)) {
            AudioManager::getInstance().playSound(AudioAssets::SFX_BUTTON);
            showSettingsModal = false;
            isDraggingBgmSlider = false;
            isDraggingSfxSlider = false;
            return;
        }
        return; // Blokir klik ke elemen game di belakang modal
    }

    // 0B. KLIK PADA MODAL RUN INFO (Jika Sedang Terbuka)
    if (showRunInfoModal) {
        float modalW = 540.0f;
        float modalH = 580.0f;
        float modalX = (static_cast<float>(windowWidth) - modalW) * 0.5f;
        float modalY = (static_cast<float>(windowHeight) - modalH) * 0.5f;
        float backX = modalX + 25.0f;
        float backY = modalY + modalH - 50.0f;
        float backW = 490.0f;
        float backH = 38.0f;

        // Tombol Back menutup modal
        if (isPointInRect(mx, my, backX, backY, backW, backH)) {
            AudioManager::getInstance().playSound(AudioAssets::SFX_BUTTON);
            showRunInfoModal = false;
            return;
        }

        // Tab Bar di Bagian Atas Modal
        float tab1X = modalX + 45.0f;
        float tabY = modalY + 28.0f;
        float tab1W = 145.0f;
        float tabH = 34.0f;
        float tab2X = tab1X + tab1W + 15.0f;
        float tab2W = 130.0f;
        float tab3X = tab2X + tab2W + 15.0f;
        float tab3W = 145.0f;

        // Klik Tab 1: Poker Hands
        if (isPointInRect(mx, my, tab1X, tabY, tab1W, tabH)) {
            AudioManager::getInstance().playSound(AudioAssets::SFX_BUTTON);
            runInfoActiveTab = 0;
            return;
        }
        // Klik Tab 2: Blinds
        if (isPointInRect(mx, my, tab2X, tabY, tab2W, tabH)) {
            AudioManager::getInstance().playSound(AudioAssets::SFX_BUTTON);
            runInfoActiveTab = 1;
            return;
        }
        // Klik Tab 3: Vouchers
        if (isPointInRect(mx, my, tab3X, tabY, tab3W, tabH)) {
            AudioManager::getInstance().playSound(AudioAssets::SFX_BUTTON);
            runInfoActiveTab = 2;
            return;
        }

        // Tombol Back di bawah modal
        float backBtnW = 490.0f;
        float backBtnH = 38.0f;
        float backBtnX = modalX + (modalW - backBtnW) * 0.5f;
        float backBtnY = modalY + modalH - 52.0f;
        if (isPointInRect(mx, my, backBtnX, backBtnY, backBtnW, backBtnH)) {
            AudioManager::getInstance().playSound(AudioAssets::SFX_BUTTON);
            showRunInfoModal = false;
            return;
        }

        // Klik di luar area modal juga menutup modal
        if (!isPointInRect(mx, my, modalX, modalY, modalW, modalH)) {
            showRunInfoModal = false;
            return;
        }
        return; // Blokir klik ke elemen game di belakang modal
    }

    // Fast-forward animasi scoring jika sedang berjalan
    if (scoringStep != ScoringStep::IDLE) {
        animSpeedMultiplier = 3.5f;
        return;
    }

    string state = engine.getCurrentStateName();

    // 1. KLIK PADA MENU UTAMA
    if (state == "MAIN_MENU") {
        // Tombol START NEW RUN (x: 362, y: 258, w: 300, h: 52)
        if (isPointInRect(mx, my, 362.0f, 258.0f, 300.0f, 52.0f)) {
            AudioManager::getInstance().playSound(AudioAssets::SFX_BUTTON);
            startTransition([this]() {
                engine.startNewRun();
                displayedRoundScore = 0;
                targetRoundScore = 0;
                clearNotifications();
            });
            return;
        }
        // Tombol QUIT (x: 422, y: 328, w: 180, h: 42)
        if (isPointInRect(mx, my, 422.0f, 328.0f, 180.0f, 42.0f)) {
            AudioManager::getInstance().playSound(AudioAssets::SFX_BUTTON);
            glfwSetWindowShouldClose(window, GLFW_TRUE);
            return;
        }
        // Ikon Gerigi Settings (x: 960, y: 665, r: 15) di Kanan Bawah
        float gearCx = 960.0f;
        float gearCy = 665.0f;
        if (isPointInRect(mx, my, gearCx - 20.0f, gearCy - 20.0f, 40.0f, 40.0f)) {
            AudioManager::getInstance().playSound(AudioAssets::SFX_BUTTON);
            showSettingsModal = true;
            return;
        }
        return;
    }

    // 2. KLIK PADA LAYAR PEMILIHAN BLIND
    if (state == "BLIND_SELECT") {
        float cardXs[3] = { 67.0f, 377.0f, 687.0f };
        float cardW = 270.0f;
        float cardH = 430.0f;
        float cardY = 135.0f;
        float btnW = 210.0f;
        float btnH = 42.0f;

        for (int i = 0; i < 3; ++i) {
            float btnX = cardXs[i] + (cardW - btnW) / 2.0f;
            float selectY = cardY + cardH - 105.0f;
            float skipY = selectY + 48.0f;

            // Klik Tombol SELECT BLIND
            if (isPointInRect(mx, my, btnX, selectY, btnW, btnH)) {
                if (engine.isBlindCurrent(i)) {
                    AudioManager::getInstance().playSound(AudioAssets::SFX_BLIND_SELECT);
                    startTransition([this]() {
                        engine.selectBlind();
                        triggerDealAnimation();
                        displayedRoundScore = engine.getCurrentScore();
                        targetRoundScore = engine.getCurrentScore();
                        AudioManager::getInstance().playMusic(AudioAssets::BGM_GAMEPLAY, true, 0.65f);
                    });
                    return;
                } else if (engine.isBlindDefeated(i)) {
                    showToast("Blind already defeated!");
                } else {
                    showToast("Blind is locked! Defeat previous blind first.");
                }
                return;
            }

            // Klik Tombol SKIP BLIND (hanya untuk Small Blind index 0 atau Big Blind index 1)
            // Bisa diklik pada tombol biru Skip Reward atau seluruh kapsul skip
            if (i < 2 && engine.isBlindCurrent(i)) {
                float iconBoxW = 46.0f;
                float skipBtnX = btnX + iconBoxW + 3.0f;
                float skipBtnW = btnW - iconBoxW - 3.0f;
                if (isPointInRect(mx, my, skipBtnX, skipY, skipBtnW, btnH)) {
                    Blind b = engine.getBlindByIndex(i);
                    TagReward tr = b.getTagReward();
                    AudioManager::getInstance().playSound(AudioAssets::SFX_CARD_DEAL);
                    
                    try {
                        engine.skipBlind();
                        string msg = "Skipped! Received " + tr.name;
                        showToast(msg);
                        spawnFloatingText("SKIPPED: " + tr.name, btnX + btnW * 0.5f, skipY - 20.0f, Theme::GOLD, 1.8f);
                    } catch (const exception& e) {
                        showToast(e.what());
                    }
                    return;
                }
            }

            // Klik Tombol REROLL BOSS BLIND (hanya untuk Boss Blind index 2 jika pemain punya Director's Cut / Retcon)
            if (i == 2 && engine.isBlindCurrent(i) && (engine.hasVoucher(VoucherType::DIRECTORS_CUT) || engine.hasVoucher(VoucherType::RETCON))) {
                if (isPointInRect(mx, my, btnX, skipY, btnW, btnH)) {
                    try {
                        engine.rerollBoss();
                        AudioManager::getInstance().playSound(AudioAssets::SFX_CARD_DEAL);
                        showToast("Boss Blind rerolled: " + engine.getCurrentBlind().getName());
                    } catch (const exception& e) {
                        showToast(e.what());
                    }
                    return;
                }
            }
        }

        // Tombol MAIN MENU (x: 432, y: 600, w: 160, h: 40)
        if (isPointInRect(mx, my, 432.0f, 600.0f, 160.0f, 40.0f)) {
            AudioManager::getInstance().playSound(AudioAssets::SFX_BUTTON);
            startTransition([this]() {
                engine.toMainMenu();
                clearNotifications();
                AudioManager::getInstance().playMusic(AudioAssets::BGM_MENU, true, 0.70f);
            });
            return;
        }
        return;
    }

    // 3. KLIK PADA LAYAR GAMEPLAY (SELECTING_CARDS)
    if (state == "SELECTING_CARDS") {
        const auto& hand = engine.getHand();
        int handLimit = max(8, max(static_cast<int>(hand.size()), engine.getMaxHandSize()));
        float startX, cardW, cardH, spacing, baseY;
        getCardLayout(handLimit, startX, cardW, cardH, spacing, baseY);

        for (size_t i = 0; i < hand.size(); ++i) {
            float cx = (i < cardAnims.size() && cardAnims[i].initialized) ? cardAnims[i].x : (startX + i * (cardW + spacing));
            float cy = (i < cardAnims.size() && cardAnims[i].initialized) ? cardAnims[i].y : baseY;

            if (isPointInRect(mx, my, cx, cy, cardW, cardH)) {
                try {
                    engine.toggleSelectCard(static_cast<int>(i));
                    AudioManager::getInstance().playSound(AudioAssets::SFX_CARD_SELECT);
                } catch (const GameException& e) {
                    showToast(e.what());
                }
                return;
            }
        }

        // Tombol PLAY HAND (Memicu Sekuens Animasi Scoring)
        if (isPointInRect(mx, my, 410.0f, 440.0f, 140.0f, 48.0f)) {
            AudioManager::getInstance().playSound(AudioAssets::SFX_BUTTON);
            startScoringSequence();
            return;
        }

        // Tombol DISCARD (x: 580, y: 440, w: 140, h: 48)
        if (isPointInRect(mx, my, 580.0f, 440.0f, 140.0f, 48.0f)) {
            AudioManager::getInstance().playSound(AudioAssets::SFX_BUTTON);
            startDiscardSequence();
            return;
        }

        // Tombol RUN INFO (x: 750, y: 440, w: 140, h: 48)
        if (isPointInRect(mx, my, 750.0f, 440.0f, 140.0f, 48.0f)) {
            AudioManager::getInstance().playSound(AudioAssets::SFX_BUTTON);
            showRunInfoModal = true;
            hoveredRunInfoRow = -1;
            return;
        }
        return;
    }

    // 4. KLIK PADA LAYAR ROUND_WON (CASH OUT)
    if (state == "ROUND_WON") {
        if (isPointInRect(mx, my, 440.0f, 440.0f, 160.0f, 48.0f)) {
            AudioManager::getInstance().playSound(AudioAssets::SFX_WIN_CASHOUT);
            startTransition([this]() {
                engine.cashOut();
                displayedRoundScore = 0;
                targetRoundScore = 0;
                AudioManager::getInstance().playMusic(AudioAssets::BGM_SHOP, true, 0.70f);
            });
            return;
        }
        if (isPointInRect(mx, my, 620.0f, 440.0f, 140.0f, 48.0f)) {
            AudioManager::getInstance().playSound(AudioAssets::SFX_BUTTON);
            showRunInfoModal = true;
            hoveredRunInfoRow = -1;
            return;
        }
        return;
    }

    // 5. KLIK PADA LAYAR SHOP (THE SHOP)
    if (state == "SHOP") {
        float panelX = 80.0f;
        float panelY = 240.0f;
        float panelW = 864.0f;

        // Cek klik tombol BUY VOUCHER
        if (!engine.isVoucherSoldOut() && engine.getCurrentShopVoucher().type != VoucherType::NONE) {
            float vx = panelX + 47.0f;
            float vy = panelY + 44.0f;
            float vh = 138.0f;
            float vBtnW = 90.0f;
            float vBtnH = 34.0f;
            float vBtnX = vx + (96.0f - vBtnW) * 0.5f;
            float vBtnY = vy + vh + 34.0f;

            if (isPointInRect(mx, my, vBtnX, vBtnY, vBtnW, vBtnH)) {
                try {
                    Voucher v = engine.getCurrentShopVoucher();
                    engine.buyVoucher();
                    AudioManager::getInstance().playSound(AudioAssets::SFX_WIN_CASHOUT);
                    showToast("Redeemed " + v.name + "!");
                } catch (const GameException& e) {
                    showToast(e.what());
                }
                return;
            }
        }

        // Cek klik tombol BUY pada etalase Joker toko
        const auto& inventory = engine.getShopInventory();
        float itemW = 92.0f;
        float itemH = 135.0f;
        float jokersAreaCenterX = panelX + 195.0f + (panelW - 195.0f) * 0.5f;
        float spacing = (inventory.size() > 3) ? 125.0f : ((inventory.size() == 3) ? 135.0f : 150.0f);
        float startX = jokersAreaCenterX - ((inventory.size() - 1) * spacing) * 0.5f - (itemW * 0.5f);

        for (size_t i = 0; i < inventory.size(); ++i) {
            float jx = startX + i * spacing;
            float jy = panelY + 45.0f;
            float btnW = 90.0f;
            float btnH = 34.0f;
            float btnX = jx + (itemW - btnW) * 0.5f;
            float btnY = jy + itemH + 34.0f;

            if (isPointInRect(mx, my, btnX, btnY, btnW, btnH)) {
                try {
                    engine.buyJoker(static_cast<int>(i));
                    AudioManager::getInstance().playSound(AudioAssets::SFX_CARD_DEAL);
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
        float slotW = 74.0f;
        float slotH = 98.0f;
        float slotSpacing = 18.0f;
        float jokersStartX = 512.0f - (maxJokers * slotW + (maxJokers - 1) * slotSpacing) * 0.5f;
        float slotY = 98.0f;

        for (size_t i = 0; i < jokers.size(); ++i) {
            float sx = jokersStartX + i * (slotW + slotSpacing);
            float sellW = 72.0f;
            float sellH = 26.0f;
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
            AudioManager::getInstance().playSound(AudioAssets::SFX_BUTTON);
            startTransition([this]() {
                engine.leaveShop();
                AudioManager::getInstance().playMusic(AudioAssets::BGM_GAMEPLAY, true, 0.65f);
            });
            return;
        }
        return;
    }

    // 6. KLIK PADA LAYAR GAME_OVER
    if (state == "GAME_OVER") {
        if (isPointInRect(mx, my, 390.0f, 440.0f, 160.0f, 48.0f)) {
            AudioManager::getInstance().playSound(AudioAssets::SFX_BUTTON);
            startTransition([this]() {
                engine.startNewRun();
                displayedRoundScore = 0;
                targetRoundScore = 0;
                clearNotifications();
                AudioManager::getInstance().playMusic(AudioAssets::BGM_MENU, true, 0.70f);
            });
            showToast("New run started!");
            return;
        }
        if (isPointInRect(mx, my, 570.0f, 440.0f, 160.0f, 48.0f)) {
            AudioManager::getInstance().playSound(AudioAssets::SFX_BUTTON);
            startTransition([this]() {
                engine.toMainMenu();
                clearNotifications();
                AudioManager::getInstance().playMusic(AudioAssets::BGM_MENU, true, 0.70f);
            });
            return;
        }
        return;
    }
}

void GameWindow::handleKey(int key, int /*scancode*/, int action, int /*mods*/) {
    if (action != GLFW_PRESS) return;

    // Jika Modal Settings sedang terbuka
    if (showSettingsModal) {
        if (key == GLFW_KEY_ESCAPE) {
            showSettingsModal = false;
            isDraggingBgmSlider = false;
            isDraggingSfxSlider = false;
        }
        return;
    }

    // Jika Modal Run Info sedang terbuka
    if (showRunInfoModal) {
        if (key == GLFW_KEY_ESCAPE || key == GLFW_KEY_I) {
            showRunInfoModal = false;
        }
        return;
    }

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
            AudioManager::getInstance().playSound(AudioAssets::SFX_BUTTON);
            startTransition([this]() {
                engine.toMainMenu();
                clearNotifications();
                AudioManager::getInstance().playMusic(AudioAssets::BGM_MENU, true, 0.70f);
            });
        } else {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
    } else if (key == GLFW_KEY_I && (state == "SELECTING_CARDS" || state == "ROUND_WON")) {
        AudioManager::getInstance().playSound(AudioAssets::SFX_BUTTON);
        showRunInfoModal = true;
        hoveredRunInfoRow = -1;
        return;
    } else if (key == GLFW_KEY_SPACE || key == GLFW_KEY_ENTER) {
        if (state == "MAIN_MENU") {
            AudioManager::getInstance().playSound(AudioAssets::SFX_BUTTON);
            startTransition([this]() {
                engine.startNewRun();
                displayedRoundScore = 0;
                targetRoundScore = 0;
                clearNotifications();
            });
        } else if (state == "BLIND_SELECT") {
            AudioManager::getInstance().playSound(AudioAssets::SFX_BLIND_SELECT);
            startTransition([this]() {
                engine.selectBlind();
                triggerDealAnimation();
                displayedRoundScore = engine.getCurrentScore();
                targetRoundScore = engine.getCurrentScore();
                AudioManager::getInstance().playMusic(AudioAssets::BGM_GAMEPLAY, true, 0.65f);
            });
        } else if (state == "SELECTING_CARDS") {
            AudioManager::getInstance().playSound(AudioAssets::SFX_BUTTON);
            startScoringSequence();
        } else if (state == "ROUND_WON") {
            AudioManager::getInstance().playSound(AudioAssets::SFX_WIN_CASHOUT);
            startTransition([this]() {
                engine.cashOut();
                displayedRoundScore = 0;
                targetRoundScore = 0;
                AudioManager::getInstance().playMusic(AudioAssets::BGM_SHOP, true, 0.70f);
            });
        } else if (state == "SHOP") {
            AudioManager::getInstance().playSound(AudioAssets::SFX_BUTTON);
            startTransition([this]() {
                engine.leaveShop();
                AudioManager::getInstance().playMusic(AudioAssets::BGM_GAMEPLAY, true, 0.65f);
            });
        } else if (state == "GAME_OVER") {
            AudioManager::getInstance().playSound(AudioAssets::SFX_BUTTON);
            startTransition([this]() {
                engine.startNewRun();
                displayedRoundScore = 0;
                targetRoundScore = 0;
                clearNotifications();
                AudioManager::getInstance().playMusic(AudioAssets::BGM_MENU, true, 0.70f);
            });
        }
    } else if (key == GLFW_KEY_D) {
        if (state == "SELECTING_CARDS") {
            AudioManager::getInstance().playSound(AudioAssets::SFX_BUTTON);
            startDiscardSequence();
        }
    } else if (key == GLFW_KEY_R) {
        AudioManager::getInstance().playSound(AudioAssets::SFX_BUTTON);
        startTransition([this]() {
            engine.startNewRun();
            displayedRoundScore = 0;
            targetRoundScore = 0;
            clearNotifications();
            AudioManager::getInstance().playMusic(AudioAssets::BGM_MENU, true, 0.70f);
        });
        showToast("Run restarted!");
    } else if ((key >= GLFW_KEY_1 && key <= GLFW_KEY_9) || key == GLFW_KEY_0) {
        if (state == "SELECTING_CARDS") {
            int idx = (key == GLFW_KEY_0) ? 9 : (key - GLFW_KEY_1);
            if (idx < static_cast<int>(engine.getHand().size())) {
                try { 
                    engine.toggleSelectCard(idx);
                    AudioManager::getInstance().playSound(AudioAssets::SFX_CARD_SELECT);
                } catch (const GameException& e) { showToast(e.what()); }
            }
        }
    }
}

// ================= RENDER MENU UTAMA =================
void GameWindow::renderMainMenu(float dt) {
    float mx = static_cast<float>(mouseX);
    float my = static_cast<float>(mouseY);
    float time = static_cast<float>(glfwGetTime());

    // 1. ELEMEN KARTU & KOIN BID MELAYANG (FLOATING ASSETS SESUAI GAMBAR REFERENSI)

    // --- SISI KIRI ---
    // A. Kartu Ace of Spades (Melayang & Berotasi Lembut)
    float aceY = 270.0f + sin(time * 1.5f) * 7.0f;
    float aceAngle = -18.0f + sin(time * 1.1f) * 2.5f;
    glPushMatrix();
    glTranslatef(105.0f, aceY, 0.0f);
    glRotatef(aceAngle, 0.0f, 0.0f, 1.0f);
    glScalef(0.85f, 0.85f, 1.0f);
    Card aceOfSpades(Suit::SPADES, Rank::ACE);
    Renderer::drawCard(aceOfSpades, -44.0f, -65.0f, 88.0f, 130.0f, false, 0.35f);
    glPopMatrix();

    // B. Kartu Merah Bersinar (Card Back dengan Neon Glow)
    float redY = 325.0f + sin(time * 1.3f + 1.8f) * 6.5f;
    float redAngle = 14.0f + sin(time * 0.95f + 1.0f) * 2.2f;
    glPushMatrix();
    glTranslatef(190.0f, redY, 0.0f);
    glRotatef(redAngle, 0.0f, 0.0f, 1.0f);
    glScalef(0.82f, 0.82f, 1.0f);
    float redGlow = 0.65f + 0.35f * sin(time * 3.2f);
    Renderer::drawGlowingAura(-44.0f, -65.0f, 88.0f, 130.0f, Color(0.95f, 0.25f, 0.25f), redGlow);
    Renderer::drawCardBack(-44.0f, -65.0f, 88.0f, 130.0f, Color(0.75f, 0.16f, 0.14f));
    glPopMatrix();

    // C. Koin Bid / Casino Chips Sisi Kiri
    // Chip 1 (Hitam - Kiri Atas)
    float c1Y = 145.0f + sin(time * 1.8f + 0.5f) * 5.0f;
    Renderer::drawCasinoChip(90.0f, c1Y, 22.0f, Color(0.18f, 0.20f, 0.22f), Color(0.95f, 0.95f, 0.95f), time * 0.35f);

    // Chip 2 (Biru - Kiri Tengah)
    float c2Y = 195.0f + sin(time * 1.6f + 2.1f) * 5.5f;
    Renderer::drawCasinoChip(195.0f, c2Y, 24.0f, Theme::CHIPS_BLUE, Color(0.95f, 0.95f, 0.95f), -time * 0.25f);

    // Chip 3 (Hijau - Kiri Bawah)
    float c3Y = 445.0f + sin(time * 1.7f + 3.8f) * 6.0f;
    Renderer::drawCasinoChip(130.0f, c3Y, 22.0f, Color(0.15f, 0.55f, 0.35f), Color(0.95f, 0.95f, 0.95f), time * 0.30f);

    // --- SISI KANAN ---
    // D. Kartu Joker Asli Balatro (Melayang & Berotasi Lembut)
    float jokerY = 275.0f + sin(time * 1.4f + 0.8f) * 7.0f;
    float jokerAngle = -10.0f + sin(time * 1.1f + 0.4f) * 2.2f;
    glPushMatrix();
    glTranslatef(820.0f, jokerY, 0.0f);
    glRotatef(jokerAngle, 0.0f, 0.0f, 1.0f);
    glScalef(0.85f, 0.85f, 1.0f);
    float jGlow = 0.55f + 0.30f * sin(time * 2.8f + 1.0f);
    Renderer::drawGlowingAura(-44.0f, -65.0f, 88.0f, 130.0f, Color(0.40f, 0.80f, 1.0f), jGlow);
    Renderer::drawJokerCard("JOKER", "Multipliers", 0, -44.0f, -65.0f, 88.0f, 130.0f, false, 1.0f);
    glPopMatrix();

    // E. Kartu Merah Bersinar Sisi Kanan (Foil Card dengan Neon Glow)
    float foilY = 345.0f + sin(time * 1.6f + 2.8f) * 6.0f;
    float foilAngle = 20.0f + sin(time * 1.25f + 1.8f) * 2.4f;
    glPushMatrix();
    glTranslatef(910.0f, foilY, 0.0f);
    glRotatef(foilAngle, 0.0f, 0.0f, 1.0f);
    glScalef(0.82f, 0.82f, 1.0f);
    float foilGlow = 0.70f + 0.30f * sin(time * 3.2f + 2.0f);
    Renderer::drawGlowingAura(-44.0f, -65.0f, 88.0f, 130.0f, Color(0.95f, 0.30f, 0.20f), foilGlow);
    Renderer::drawCardBack(-44.0f, -65.0f, 88.0f, 130.0f, Color(0.70f, 0.14f, 0.12f));
    glPopMatrix();

    // F. Koin Bid / Casino Chips Sisi Kanan
    // Chip 4 (Hitam - Kanan Atas)
    float c4Y = 150.0f + sin(time * 1.7f + 1.4f) * 5.2f;
    Renderer::drawCasinoChip(925.0f, c4Y, 23.0f, Color(0.18f, 0.20f, 0.22f), Color(0.95f, 0.95f, 0.95f), time * 0.26f);

    // Chip 5 (Merah - Kanan Tengah)
    float c5Y = 185.0f + sin(time * 1.9f + 3.1f) * 4.8f;
    Renderer::drawCasinoChip(835.0f, c5Y, 20.0f, Theme::MULT_RED, Color(0.95f, 0.95f, 0.95f), -time * 0.32f);

    // Chip 6 (Hijau - Kanan Bawah)
    float c6Y = 450.0f + sin(time * 1.5f + 4.2f) * 6.0f;
    Renderer::drawCasinoChip(785.0f, c6Y, 22.0f, Color(0.15f, 0.55f, 0.35f), Color(0.95f, 0.95f, 0.95f), time * 0.22f);


    // 2. JUDUL BALATRO DENGAN EFEK SYNTHWAVE / API PIXEL & 3D ISOMETRIC EXTRUSION
    // Synthwave / Flame ditaruh tepat di BELAKANG tulisan "BALATRO"
    // Pijakan tanah api dibuat sejajar dengan garis dasar (baseline) huruf BALATRO (y = 133.0f).
    // Api dibuat sedikit lebih besar dan menjulang ke atas menembus belakang huruf.
    float flameLeft = 310.0f;
    float flameRight = 714.0f;
    float flameBaseY = 133.0f; // Pijakan tanah sejajar dengan dasar bawah huruf BALATRO

    for (float fx = flameLeft; fx <= flameRight; fx += 4.5f) {
        // Multiplier fade horizontal di tepi kiri dan kanan agar menyatu lembut
        float distEdge = min(fx - flameLeft, flameRight - fx);
        float edgeFade = min(1.0f, distEdge / 55.0f);
        // Smoothstep untuk tepi kiri & kanan
        edgeFade = edgeFade * edgeFade * (3.0f - 2.0f * edgeFade);

        if (edgeFade <= 0.01f) continue;

        // Gelombang api synthwave dinamis
        float w1 = sin(time * 9.5f + fx * 0.09f);
        float w2 = cos(time * 14.5f + fx * 0.15f);
        float w3 = sin(time * 6.0f + fx * 0.04f);
        // Tinggi api diperbesar (menjulang 50px hingga 75px ke atas dari pijakan tanah y=133)
        float flameH = (52.0f + (w1 * 0.55f + w2 * 0.30f + w3 * 0.15f) * 22.0f) * edgeFade;
        if (flameH < 4.0f) flameH = 4.0f;

        // Lapisan 1: Pucuk api merah menyala (soft alpha)
        Renderer::drawRect(fx, flameBaseY - flameH, 4.0f, flameH * 0.32f, Color(0.95f, 0.20f, 0.10f, 0.70f * edgeFade));
        // Lapisan 2: Badan api oranye neon
        Renderer::drawRect(fx, flameBaseY - flameH * 0.68f, 4.0f, flameH * 0.36f, Color(1.0f, 0.52f, 0.06f, 0.85f * edgeFade));
        // Lapisan 3: Inti api kuning emas terang di bagian tengah-bawah
        Renderer::drawRect(fx, flameBaseY - flameH * 0.32f, 4.0f, flameH * 0.24f, Color(1.0f, 0.95f, 0.30f, 0.92f * edgeFade));
        // Lapisan 4: Pijakan tanah/dasar menyala (sejajar tanah baseline)
        Renderer::drawRect(fx, flameBaseY - flameH * 0.08f, 4.0f, flameH * 0.08f, Color(1.0f, 0.85f, 0.25f, 0.50f * edgeFade));
    }

    // Percikan Api / Synthwave Embers Melayang ke Atas dari Pijakan Tanah
    for (int ep = 0; ep < 16; ++ep) {
        float seed = ep * 19.7f;
        float px = 320.0f + fmod(seed * 26.3f, 384.0f);
        float distEdge = min(px - flameLeft, flameRight - px);
        float emberFade = min(1.0f, distEdge / 45.0f);
        float life = fmod(time * 38.0f + seed * 5.7f, 65.0f);
        float py = flameBaseY - life;
        float alpha = max(0.0f, (1.0f - life / 65.0f) * emberFade);
        float sparkW = (ep % 2 == 0) ? 3.0f : 2.0f;
        Color sparkCol = (ep % 3 == 0) ? Color(1.0f, 0.95f, 0.4f, alpha * 0.9f) : Color(1.0f, 0.48f, 0.12f, alpha * 0.85f);
        Renderer::drawRect(px, py, sparkW, sparkW, sparkCol);
    }

    // Extrusion 3D Block BALATRO (Dirender DI DEPAN Synthwave Flame)
    float bScale = 5.8f;
    float bCenter = 512.0f;
    Renderer::drawCenteredText("BALATRO", bCenter + 5.0f, 96.0f, bScale, Color(0.10f, 0.03f, 0.02f, 0.95f));
    Renderer::drawCenteredText("BALATRO", bCenter + 4.0f, 94.0f, bScale, Color(0.12f, 0.04f, 0.02f, 0.95f));
    Renderer::drawCenteredText("BALATRO", bCenter + 3.0f, 92.0f, bScale, Color(0.72f, 0.28f, 0.05f, 1.0f));
    Renderer::drawCenteredText("BALATRO", bCenter + 2.0f, 90.0f, bScale, Color(0.88f, 0.44f, 0.08f, 1.0f));
    Renderer::drawCenteredText("BALATRO", bCenter + 1.0f, 87.0f, bScale, Color(0.98f, 0.78f, 0.15f, 1.0f));
    Renderer::drawCenteredText("BALATRO", bCenter,        85.0f, bScale, Color(1.0f, 0.96f, 0.50f, 0.90f));

    // Tulisan M  I  N  I Beranimasi Lompat Bergantian (Wave / Ombak Setiap Beberapa Saat)
    // Siklus ombak setiap 3.2 detik: selama 1.0 detik melompat berurutan M -> I -> N -> I, lalu diam santai
    float waveCycleTime = 3.2f;
    float cyclePhase = fmod(time, waveCycleTime);
    float letterJumpDur = 0.32f;
    float letterDelay = 0.16f;

    const char miniLetters[4] = { 'M', 'I', 'N', 'I' };
    float letterXs[4] = { 436.0f, 484.0f, 532.0f, 580.0f }; // Terpusat rapi di 512 (jarak 48px)
    float miniBaseY = 162.0f;
    float mScale = 2.8f;

    for (int li = 0; li < 4; ++li) {
        float startT = li * letterDelay;
        float jumpY = 0.0f;
        if (cyclePhase >= startT && cyclePhase < startT + letterJumpDur) {
            float p = (cyclePhase - startT) / letterJumpDur;
            jumpY = -sin(p * 3.14159265f) * 15.0f; // Melompat ke atas setinggi 15px
        }

        string singleLetter(1, miniLetters[li]);
        float lx = letterXs[li];
        float ly = miniBaseY + jumpY;

        // Shadow 3D untuk setiap huruf
        Renderer::drawCenteredText(singleLetter, lx + 2.0f, ly + 3.0f, mScale, Color(0.18f, 0.02f, 0.04f, 0.85f));
        Renderer::drawCenteredText(singleLetter, lx + 1.0f, ly + 1.5f, mScale, Color(0.78f, 0.12f, 0.12f, 1.0f));
        Renderer::drawCenteredText(singleLetter, lx,        ly,        mScale, Theme::MULT_RED);
        Renderer::drawCenteredText(singleLetter, lx - 1.0f, ly - 1.0f, mScale, Color(1.0f, 0.58f, 0.52f, 0.85f));
    }

    // Subtitle "A Poker Roguelike"
    Renderer::drawCenteredText("A Poker Roguelike", 512.0f, 206.0f, 1.35f, Color(0.88f, 0.92f, 0.90f, 0.95f));

    // Garis Aksen Hijau/Teal Bercahaya
    float barW = 340.0f;
    float barX = 512.0f - barW * 0.5f;
    float barY = 232.0f;
    Renderer::drawRect(barX, barY + 1.0f, barW, 2.0f, Color(0.20f, 0.65f, 0.50f, 0.85f));
    Renderer::drawRect(barX + 45.0f, barY, barW - 90.0f, 4.0f, Color(0.40f, 0.95f, 0.75f, 0.45f));
    Renderer::drawRect(510.0f, barY - 1.0f, 4.0f, 6.0f, Color(0.80f, 1.0f, 0.90f));


    // 3. TOMBOL UTAMA (START NEW RUN & QUIT)
    // A. Tombol START NEW RUN dengan Neon Aura Cyan & Partikel Berkelip
    bool hoverRun = isPointInRect(mx, my, 362.0f, 258.0f, 300.0f, 52.0f);
    float runAuraInt = hoverRun ? 1.0f : (0.65f + 0.25f * sin(time * 3.5f));
    Renderer::drawGlowingAura(362.0f, 258.0f, 300.0f, 52.0f, Color(0.25f, 0.75f, 1.0f), runAuraInt);

    // Partikel percikan cyan kecil di sekitar tombol Start
    for (int sp = 0; sp < 8; ++sp) {
        float sparkPhase = time * 2.5f + sp * 0.8f;
        float sx = 356.0f + fmod(sp * 41.3f, 312.0f);
        float sy = 252.0f + fmod(sp * 19.7f, 64.0f) + sin(sparkPhase) * 3.0f;
        float sAlpha = 0.35f + 0.45f * sin(sparkPhase);
        Renderer::drawRect(sx, sy, 3.0f, 3.0f, Color(0.35f, 0.90f, 1.0f, sAlpha));
    }

    Renderer::drawButton("START NEW RUN", 362.0f, 258.0f, 300.0f, 52.0f, Theme::CHIPS_BLUE, Theme::TEXT_WHITE, hoverRun, true);

    // B. Tombol QUIT
    bool hoverQuit = isPointInRect(mx, my, 422.0f, 328.0f, 180.0f, 42.0f);
    Renderer::drawButton("QUIT", 422.0f, 328.0f, 180.0f, 42.0f, Color(0.45f, 0.12f, 0.14f), Theme::TEXT_WHITE, hoverQuit, true);


    // 4. PANEL INFORMASI "HOW TO PLAY / OBJECTIVE" (BINGKAI RETRO CRT DENGAN IKON KOIN)
    float panelW = 780.0f;
    float panelH = 220.0f;
    float panelX = (static_cast<float>(windowWidth) - panelW) * 0.5f;
    float panelY = 410.0f;

    Renderer::drawRect(panelX, panelY, panelW, panelH, Theme::PANEL_BG);
    Renderer::drawRectOutline(panelX, panelY, panelW, panelH, 1.5f, Color(0.20f, 0.45f, 0.38f, 0.85f));

    // Corner Filigree Brackets CRT [ ]
    float bLen = 14.0f;
    float bThick = 2.5f;
    Color bracketCol(0.35f, 0.70f, 0.55f, 0.9f);
    Renderer::drawRect(panelX + 6.0f, panelY + 6.0f, bLen, bThick, bracketCol);
    Renderer::drawRect(panelX + 6.0f, panelY + 6.0f, bThick, bLen, bracketCol);
    Renderer::drawRect(panelX + panelW - 6.0f - bLen, panelY + 6.0f, bLen, bThick, bracketCol);
    Renderer::drawRect(panelX + panelW - 6.0f - bThick, panelY + 6.0f, bThick, bLen, bracketCol);
    Renderer::drawRect(panelX + 6.0f, panelY + panelH - 6.0f - bThick, bLen, bThick, bracketCol);
    Renderer::drawRect(panelX + 6.0f, panelY + panelH - 6.0f - bLen, bThick, bLen, bracketCol);
    Renderer::drawRect(panelX + panelW - 6.0f - bLen, panelY + panelH - 6.0f - bThick, bLen, bThick, bracketCol);
    Renderer::drawRect(panelX + panelW - 6.0f - bThick, panelY + panelH - 6.0f - bLen, bThick, bLen, bracketCol);

    // Judul Panel dengan Ikon Koin Kasino di Kiri & Kanan
    Renderer::drawCasinoChip(512.0f - 165.0f, panelY + 26.0f, 9.0f, Theme::GOLD, Color(1.0f, 1.0f, 1.0f), time * 0.5f);
    Renderer::drawCenteredText("HOW TO PLAY / OBJECTIVE", 512.0f, panelY + 19.0f, 1.55f, Theme::GOLD);
    Renderer::drawCasinoChip(512.0f + 165.0f, panelY + 26.0f, 9.0f, Theme::CHIPS_BLUE, Color(1.0f, 1.0f, 1.0f), -time * 0.5f);

    // Poin-Poin Instruksi Permainan
    float textLeft = panelX + 40.0f;
    Renderer::drawText("- Play poker combinations (1-5 cards) to score Chips x Mult", textLeft, panelY + 55.0f, 1.25f, Theme::TEXT_WHITE);
    Renderer::drawText("- Reach the target score before running out of Hands", textLeft, panelY + 85.0f, 1.25f, Theme::TEXT_WHITE);
    Renderer::drawText("- Defeat Small, Big, and Boss Blinds to advance each Ante!", textLeft, panelY + 115.0f, 1.25f, Theme::TEXT_WHITE);

    // Garis Pemisah & Kontrol
    Renderer::drawRect(textLeft, panelY + 152.0f, panelW - 80.0f, 1.0f, Color(0.25f, 0.45f, 0.4f, 0.6f));
    Renderer::drawText("Controls: Mouse Click | Keys [1-8] | Space/Enter to Play | [D] to Discard", textLeft, panelY + 168.0f, 1.20f, Theme::CHIPS_BLUE);

    // 5. IKON GERIGI (SETTINGS) DI KANAN BAWAH MAIN MENU
    // Dibuat lebih ke atas (y: 665) dan lebih kecil (r: 15) agar tidak terpotong
    float gearCx = 960.0f;
    float gearCy = 665.0f;
    float gearRadius = 15.0f;
    bool isGearHovered = isPointInRect(mx, my, gearCx - 20.0f, gearCy - 20.0f, 40.0f, 40.0f);

    // Berputar lambat dan santai searah jarum jam saat mouse di-hover (45 derajat/detik)
    if (isGearHovered) {
        gearRotationAngle += dt * 45.0f;
        if (gearRotationAngle >= 360.0f) {
            gearRotationAngle -= 360.0f;
        }
    }

    if (isGearHovered) {
        Renderer::drawGlowingAura(gearCx - 18.0f, gearCy - 18.0f, 36.0f, 36.0f, Theme::GOLD, 0.65f);
    }
    Color gearColor = isGearHovered ? Theme::GOLD : Color(0.70f, 0.82f, 0.78f, 0.95f);
    drawGearIcon(gearCx, gearCy, gearRadius, gearRotationAngle, gearColor);
}

// ================= RENDER PEMILIHAN BLIND =================
void GameWindow::renderBlindSelect() {
    float mx = static_cast<float>(mouseX);
    float my = static_cast<float>(mouseY);

    string tooltipTitle = "";
    string tooltipDesc = "";
    float tooltipAnchorX = 0.0f;
    float tooltipAnchorY = 0.0f;

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
            // Update partikel di sekeliling frame blind yang sedang dipilih
            updateFrameParticles(0.016f, cx, cardY, cardW, cardH, borderCol);
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
            float debuffBoxW = 244.0f;
            float debuffBoxH = 75.0f;
            float debuffBoxX = cx + (cardW - debuffBoxW) / 2.0f;
            Renderer::drawRect(debuffBoxX, cardY + 220.0f, debuffBoxW, debuffBoxH, Color(0.22f, 0.05f, 0.05f, 0.95f));
            Renderer::drawRectOutline(debuffBoxX, cardY + 220.0f, debuffBoxW, debuffBoxH, 2.0f, Theme::MULT_RED);
            Renderer::drawCenteredText("BOSS DEBUFF:", cardCenterX, cardY + 228.0f, 1.25f, Theme::MULT_RED);
            Renderer::drawWrappedCenteredText(blind.getDebuffDescription(), cardCenterX, cardY + 248.0f, debuffBoxW - 16.0f, 1.15f, 4.0f, Theme::TEXT_WHITE);
        }

        // Tombol Aksi di Bawah Kartu
        float btnW = 210.0f;
        float btnH = 42.0f;
        float btnX = cx + (cardW - btnW) / 2.0f;
        float btnY = cardY + cardH - 105.0f;

        if (isDefeated) {
            Renderer::drawButton("DEFEATED", btnX, cardY + cardH - 65.0f, btnW, 45.0f, Color(0.2f, 0.35f, 0.25f), Color(0.6f, 0.8f, 0.6f), false, false);
        } else if (isCurrent) {
            bool hoverSelect = isPointInRect(mx, my, btnX, btnY, btnW, btnH);
            Renderer::drawButton("SELECT BLIND", btnX, btnY, btnW, btnH, titleColor, Theme::TEXT_WHITE, hoverSelect, true);

            // Jika Small Blind (0) atau Big Blind (1), tampilkan tombol SKIP BLIND sama persis seperti di Run Info
            if (i < 2) {
                float skipY = btnY + 48.0f;
                TagReward tr = blind.getTagReward();

                float iconBoxW = 46.0f;
                bool hoverIcon = isPointInRect(mx, my, btnX, skipY, iconBoxW, btnH);
                bool hoverBtn = isPointInRect(mx, my, btnX + iconBoxW + 3.0f, skipY, btnW - iconBoxW - 3.0f, btnH);

                if (hoverIcon) {
                    tooltipTitle = tr.name;
                    tooltipDesc = tr.description;
                    tooltipAnchorX = btnX + btnW * 0.5f;
                    tooltipAnchorY = skipY + btnH;
                }

                // Kotak icon tag di kiri
                Renderer::drawRect(btnX, skipY, iconBoxW, btnH, Color(0.95f, 0.95f, 0.95f));
                Renderer::drawRectOutline(btnX, skipY, iconBoxW, btnH, 1.5f, hoverIcon ? Theme::GOLD : Color(0.55f, 0.60f, 0.65f));
                Renderer::drawTagIcon(tr.type, btnX + iconBoxW * 0.5f, skipY + btnH * 0.5f, 26.0f);

                // Tombol biru Skip Reward di kanan (2 baris teks rapi)
                float skipBtnW = btnW - iconBoxW - 3.0f;
                float skipBtnX = btnX + iconBoxW + 3.0f;
                Color btnBg = hoverBtn ? Color(0.22f, 0.55f, 0.82f) : Color(0.12f, 0.40f, 0.65f);
                Renderer::drawRect(skipBtnX, skipY, skipBtnW, btnH, btnBg);
                Renderer::drawRectOutline(skipBtnX, skipY, skipBtnW, btnH, 1.5f, Color(0.08f, 0.25f, 0.45f));
                Renderer::drawCenteredText("Skip", skipBtnX + skipBtnW * 0.5f, skipY + 6.0f, 1.15f, Theme::TEXT_WHITE);
                Renderer::drawCenteredText("Reward", skipBtnX + skipBtnW * 0.5f, skipY + 22.0f, 1.15f, Theme::TEXT_WHITE);
            } else if (i == 2 && (engine.hasVoucher(VoucherType::DIRECTORS_CUT) || engine.hasVoucher(VoucherType::RETCON))) {
                float skipY = btnY + 48.0f;
                bool canReroll = engine.canRerollBoss();
                bool hoverReroll = isPointInRect(mx, my, btnX, skipY, btnW, btnH);
                string rerollLabel = "REROLL BOSS ($10)";
                Renderer::drawButton(rerollLabel, btnX, skipY, btnW, btnH, Color(0.65f, 0.25f, 0.60f), Theme::TEXT_WHITE, hoverReroll, canReroll);
            }
        } else if (isLocked) {
            Renderer::drawButton("LOCKED", btnX, cardY + cardH - 65.0f, btnW, 45.0f, Color(0.15f, 0.2f, 0.18f), Theme::DEBUFF_GRAY, false, false);
        }
    }

    // Render partikel di sekeliling frame blind yang melayang keluar perlahan
    renderFrameParticles();

    // Tombol Kembali ke Menu
    bool hoverBack = isPointInRect(mx, my, 432.0f, 600.0f, 160.0f, 40.0f);
    Renderer::drawButton("MAIN MENU", 432.0f, 600.0f, 160.0f, 40.0f, Color(0.2f, 0.3f, 0.25f), Theme::TEXT_WHITE, hoverBack, true);

    // Panel Tooltip Detail Tag (HANYA muncul tepat di bawah tulisan skip reward saat ikon tag di-hover)
    if (!tooltipTitle.empty()) {
        float ttW = 280.0f;
        float ttH = 68.0f;
        float ttX = tooltipAnchorX - ttW * 0.5f;
        float ttY = tooltipAnchorY + 6.0f;

        // Bayangan tooltip
        Renderer::drawRect(ttX + 4.0f, ttY + 4.0f, ttW, ttH, Color(0.0f, 0.0f, 0.0f, 0.5f));
        // Kotak putih bersih khas Balatro
        Renderer::drawRect(ttX, ttY, ttW, ttH, Color(0.98f, 0.98f, 0.98f));
        Renderer::drawRectOutline(ttX, ttY, ttW, ttH, 2.5f, Color(0.15f, 0.45f, 0.75f));
        Renderer::drawCenteredText(tooltipTitle, ttX + ttW * 0.5f, ttY + 8.0f, 1.35f, Color(0.12f, 0.35f, 0.65f));
        Renderer::drawWrappedCenteredText(tooltipDesc, ttX + ttW * 0.5f, ttY + 30.0f, ttW - 16.0f, 1.05f, 3.0f, Color(0.18f, 0.22f, 0.25f));
    }
}

// ================= RENDER TOKO (THE SHOP) =================
void GameWindow::renderShop() {
    float mx = static_cast<float>(mouseX);
    float my = static_cast<float>(mouseY);

    // 1. Header Toko
    Renderer::drawCenteredText("THE SHOP", 512.0f, 28.0f, 3.2f, Theme::GOLD);
    string moneyStr = "Your Money: $" + to_string(engine.getMoney());
    Renderer::drawCenteredText(moneyStr, 512.0f, 66.0f, 1.6f, Theme::GOLD);

    // 2. Baris Slot Joker Pemain Saat Ini (di atas meja)
    renderJokerSlots();

    // 3. Panel Etalase Toko (Menampung Voucher di Kiri dan Jokers di Kanan)
    float panelX = 70.0f;
    float panelY = 236.0f;
    float panelW = 884.0f;
    float panelH = 345.0f;

    Renderer::drawRect(panelX, panelY, panelW, panelH, Theme::PANEL_BG);
    Renderer::drawRectOutline(panelX, panelY, panelW, panelH, 2.0f, Color(0.2f, 0.4f, 0.35f, 0.8f));

    // A. SEKSI KIRI: VOUCHER PEDESTAL (Lebar ~195px)
    Voucher shopVoucher = engine.getCurrentShopVoucher();
    bool isVoucherSold = engine.isVoucherSoldOut();

    string pedestalTitle = (shopVoucher.type != VoucherType::NONE) ? shopVoucher.name : "VOUCHER";
    Color titleColor = (shopVoucher.tier == 2) ? Theme::GOLD : Theme::TEXT_WHITE;
    Renderer::drawCenteredText(pedestalTitle, panelX + 97.0f, panelY + 16.0f, 1.45f, titleColor);

    float vx = panelX + 50.0f;
    float vy = panelY + 45.0f;
    float vw = 96.0f;
    float vh = 138.0f;

    bool hoverVoucher = isPointInRect(mx, my, vx, vy, vw, vh);

    if (shopVoucher.type != VoucherType::NONE) {
        Renderer::drawVoucherCard(shopVoucher, vx, vy, vw, vh, hoverVoucher && !isVoucherSold, (hoverVoucher && !isVoucherSold) ? 1.05f : 1.0f, isVoucherSold);

        // Harga Voucher & Tombol BUY
        float vPriceY = vy + vh + 8.0f;
        int vCost = engine.getVoucherCost();
        bool canAffordVoucher = (engine.getMoney() >= vCost);

        if (!isVoucherSold) {
            string vCostText = "$" + to_string(vCost);
            Color vPriceCol = canAffordVoucher ? Theme::GOLD : Theme::MULT_RED;
            Renderer::drawCenteredText(vCostText, vx + vw * 0.5f + 1.0f, vPriceY + 1.5f, 1.85f, Color(0.0f, 0.0f, 0.0f, 0.70f));
            Renderer::drawCenteredText(vCostText, vx + vw * 0.5f, vPriceY, 1.85f, vPriceCol);

            float vBtnW = 90.0f;
            float vBtnH = 34.0f;
            float vBtnX = vx + (vw - vBtnW) * 0.5f;
            float vBtnY = vy + vh + 34.0f;
            bool hoverBuyVoucher = isPointInRect(mx, my, vBtnX, vBtnY, vBtnW, vBtnH);
            Renderer::drawButton("BUY", vBtnX, vBtnY, vBtnW, vBtnH, Theme::GOLD, Theme::TEXT_DARK, hoverBuyVoucher, canAffordVoucher);
        } else {
            Renderer::drawCenteredText("SOLD OUT", vx + vw * 0.5f, vPriceY + 6.0f, 1.35f, Theme::MULT_RED);
            float vBtnW = 90.0f;
            float vBtnH = 34.0f;
            float vBtnX = vx + (vw - vBtnW) * 0.5f;
            float vBtnY = vy + vh + 34.0f;
            Renderer::drawButton("SOLD", vBtnX, vBtnY, vBtnW, vBtnH, Color(0.18f, 0.22f, 0.20f), Theme::DEBUFF_GRAY, false, false);
        }
    } else {
        Renderer::drawCenteredText("SOLD OUT", vx + vw * 0.5f, vy + vh * 0.5f - 8.0f, 1.6f, Theme::DEBUFF_GRAY);
        Renderer::drawCenteredText("ALL REDEEMED", vx + vw * 0.5f, vy + vh * 0.5f + 14.0f, 0.95f, Theme::DEBUFF_GRAY);
    }

    // Garis Pemisah Vertikal antara Voucher dan Jokers
    Renderer::drawRect(panelX + 195.0f, panelY + 15.0f, 2.0f, panelH - 30.0f, Color(0.2f, 0.4f, 0.35f, 0.6f));

    // B. SEKSI KANAN: JOKERS FOR SALE (Lebar ~660px)
    float jokersAreaCenterX = panelX + 195.0f + (panelW - 195.0f) * 0.5f;
    Renderer::drawCenteredText("JOKERS FOR SALE", jokersAreaCenterX, panelY + 16.0f, 1.6f, Theme::TEXT_WHITE);

    const auto& inventory = engine.getShopInventory();
    float itemW = 92.0f;
    float itemH = 135.0f;
    float spacing = (inventory.size() > 3) ? 125.0f : ((inventory.size() == 3) ? 135.0f : 150.0f);
    float startX = jokersAreaCenterX - ((inventory.size() - 1) * spacing) * 0.5f - (itemW * 0.5f);

    int hoveredJokerShopIdx = -1;

    for (size_t i = 0; i < inventory.size(); ++i) {
        float jx = startX + i * spacing;
        float jy = panelY + 45.0f;
        bool isHovered = isPointInRect(mx, my, jx, jy, itemW, itemH);
        if (isHovered) hoveredJokerShopIdx = static_cast<int>(i);

        Renderer::drawJokerCard(inventory[i].getName(), inventory[i].getDescription(), 0,
                                jx, jy, itemW, itemH, isHovered, isHovered ? 1.05f : 1.0f);

        int cost = engine.getDiscountedPrice(inventory[i].getCost());
        bool canAfford = (engine.getMoney() >= cost);
        string costText = "$" + to_string(cost);
        float priceY = jy + itemH + 8.0f;
        Color priceCol = canAfford ? Theme::GOLD : Theme::MULT_RED;
        Renderer::drawCenteredText(costText, jx + itemW * 0.5f + 1.0f, priceY + 1.5f, 1.85f, Color(0.0f, 0.0f, 0.0f, 0.70f));
        Renderer::drawCenteredText(costText, jx + itemW * 0.5f, priceY, 1.85f, priceCol);

        float btnW = 90.0f;
        float btnH = 34.0f;
        float btnX = jx + (itemW - btnW) * 0.5f;
        float btnY = jy + itemH + 34.0f;
        bool canBuy = (canAfford && static_cast<int>(engine.getJokers().size()) < engine.getMaxJokers());
        bool hoverBuy = isPointInRect(mx, my, btnX, btnY, btnW, btnH);

        Renderer::drawButton("BUY", btnX, btnY, btnW, btnH, Theme::CHIPS_BLUE, Theme::TEXT_WHITE, hoverBuy, canBuy);
    }

    if (inventory.empty()) {
        Renderer::drawCenteredText("SOLD OUT! REROLL FOR MORE JOKERS.", jokersAreaCenterX, panelY + 140.0f, 1.8f, Theme::DEBUFF_GRAY);
    }

    // 4. Tombol Kontrol Toko: REROLL ($cost) & NEXT ROUND
    float btnY = 602.0f;
    bool canReroll = (engine.getMoney() >= engine.getRerollCost());
    bool hoverReroll = isPointInRect(mx, my, 310.0f, btnY, 180.0f, 48.0f);
    string rerollText = "REROLL ($" + to_string(engine.getRerollCost()) + ")";
    Renderer::drawButton(rerollText, 310.0f, btnY, 180.0f, 48.0f, Theme::MULT_RED, Theme::TEXT_WHITE, hoverReroll, canReroll);

    bool hoverNext = isPointInRect(mx, my, 530.0f, btnY, 180.0f, 48.0f);
    Renderer::drawButton("NEXT ROUND", 530.0f, btnY, 180.0f, 48.0f, Theme::GOLD, Theme::TEXT_DARK, hoverNext, true);

    // 5. TOOLTIPS: DIGAMBAR PALING AKHIR (LAYERING TERTINGGI, TIDAK AKAN PERNAH TERTUTUP MEJA/TOMBOL!)
    // Cek hover pada Joker milik pemain di baris atas
    const auto& ownedJokers = engine.getJokers();
    int maxJokers = engine.getMaxJokers();
    float ownedSlotW = 74.0f;
    float ownedSlotH = 98.0f;
    float ownedSpacing = 18.0f;
    float ownedStartX = 512.0f - (maxJokers * ownedSlotW + (maxJokers - 1) * ownedSpacing) * 0.5f;
    float ownedSlotY = 98.0f;

    int hoveredOwnedIdx = -1;
    for (size_t i = 0; i < ownedJokers.size(); ++i) {
        float sx = ownedStartX + i * (ownedSlotW + ownedSpacing);
        if (isPointInRect(mx, my, sx, ownedSlotY, ownedSlotW, ownedSlotH)) {
            hoveredOwnedIdx = static_cast<int>(i);
            break;
        }
    }

    if (hoveredOwnedIdx >= 0) {
        float sx = ownedStartX + hoveredOwnedIdx * (ownedSlotW + ownedSpacing);
        renderJokerTooltip(ownedJokers[hoveredOwnedIdx], sx, ownedSlotY);
    } else if (hoveredJokerShopIdx >= 0) {
        float jx = startX + hoveredJokerShopIdx * spacing;
        float jy = panelY + 45.0f;
        renderJokerTooltip(inventory[hoveredJokerShopIdx], jx, jy);
    } else if (hoverVoucher && shopVoucher.type != VoucherType::NONE) {
        renderVoucherTooltip(shopVoucher, vx, vy);
    }
}

void GameWindow::renderVoucherTooltip(const Voucher& voucher, float x, float y) {
    float tipW = 280.0f;
    float tipH = 75.0f;
    float tipX = x + 48.0f - tipW * 0.5f;
    tipX = max(15.0f, min(static_cast<float>(windowWidth) - tipW - 15.0f, tipX));
    float tipY = y - tipH - 14.0f;
    if (tipY < 10.0f) tipY = y + 145.0f;

    // 1. Shadow
    Renderer::drawRect(tipX + 4.0f, tipY + 4.0f, tipW, tipH, Color(0.0f, 0.0f, 0.0f, 0.45f));

    // 2. White clean box
    Renderer::drawRect(tipX, tipY, tipW, tipH, Color(0.98f, 0.98f, 0.96f));
    Color borderCol(voucher.accentColor.r, voucher.accentColor.g, voucher.accentColor.b);
    Renderer::drawRectOutline(tipX, tipY, tipW, tipH, 2.0f, (voucher.tier == 2) ? Theme::GOLD : borderCol);

    // 3. Header: Name & Tier
    string header = voucher.name + ((voucher.tier == 2) ? " (Tier II)" : " (Tier I)");
    Color headerCol = (voucher.tier == 2) ? Color(0.75f, 0.45f, 0.10f) : Color(0.15f, 0.35f, 0.55f);
    Renderer::drawCenteredText(header, tipX + tipW * 0.5f, tipY + 8.0f, 1.35f, headerCol);

    // Divider
    Renderer::drawRect(tipX + 15.0f, tipY + 28.0f, tipW - 30.0f, 1.0f, Color(0.80f, 0.80f, 0.80f));

    // 4. Wrapped Description
    Renderer::drawWrappedCenteredText(voucher.description, tipX + tipW * 0.5f, tipY + 36.0f, tipW - 20.0f, 1.05f, 3.0f, Color(0.18f, 0.22f, 0.25f));
}

// ================= RENDER 5 SLOT JOKER DI ATAS MEJA =================
void GameWindow::renderJokerSlots() {
    float mx = static_cast<float>(mouseX);
    float my = static_cast<float>(mouseY);

    const auto& jokers = engine.getJokers();
    int maxJokers = engine.getMaxJokers();

    string state = engine.getCurrentStateName();

    float slotW = (state == "SHOP") ? 74.0f : 66.0f;
    float slotH = (state == "SHOP") ? 98.0f : 86.0f;
    float spacing = (state == "SHOP") ? 18.0f : 12.0f;

    float startX = 310.0f + (680.0f - (maxJokers * slotW + (maxJokers - 1) * spacing)) * 0.5f;
    float slotY = 22.0f;

    if (state == "SHOP") {
        startX = 512.0f - (maxJokers * slotW + (maxJokers - 1) * spacing) * 0.5f;
        slotY = 98.0f;
    }

    for (int i = 0; i < maxJokers; ++i) {
        float sx = startX + i * (slotW + spacing);

        if (i < static_cast<int>(jokers.size())) {
            bool isHovered = !showRunInfoModal && isPointInRect(mx, my, sx, slotY, slotW, slotH);
            float jScale = (i < static_cast<int>(jokerAnimStates.size())) ? jokerAnimStates[i].scale : 1.0f;
            float jGlow = (i < static_cast<int>(jokerAnimStates.size())) ? jokerAnimStates[i].glow : 0.0f;
            if (isHovered) jScale *= 1.05f;

            Renderer::drawJokerCard(jokers[i].getName(), jokers[i].getDescription(), 0,
                                    sx, slotY, slotW, slotH, isHovered, jScale, jGlow);

            // Jika di Shop, tampilkan tombol SELL di bawah slot Joker
            if (state == "SHOP") {
                float sellW = 72.0f;
                float sellH = 26.0f;
                float sellX = sx + (slotW - sellW) * 0.5f;
                float sellY = slotY + slotH + 6.0f;
                bool hoverSell = isPointInRect(mx, my, sellX, sellY, sellW, sellH);
                string sellText = "SELL $" + to_string(jokers[i].getSellValue());
                Renderer::drawButton(sellText, sellX, sellY, sellW, sellH, Color(0.35f, 0.20f, 0.20f), Theme::GOLD, hoverSell, true);
            }

            if (isHovered && state != "SHOP") {
                renderJokerTooltip(jokers[i], sx, slotY);
            }
        } else {
            // Slot kosong
            Renderer::drawRect(sx, slotY, slotW, slotH, Color(0.06f, 0.12f, 0.10f, 0.7f));
            Renderer::drawRectOutline(sx, slotY, slotW, slotH, 1.5f, Color(0.25f, 0.40f, 0.35f, 0.6f));
            Renderer::drawCenteredText("EMPTY", sx + slotW * 0.5f, slotY + slotH * 0.5f - 6.0f, 0.9f, Color(0.4f, 0.5f, 0.45f, 0.8f));
        }
    }
}

void GameWindow::renderJokerTooltip(const Joker& joker, float x, float y) {
    float tipW = 230.0f;
    float maxTextW = tipW - 24.0f;

    // Word wrapping description
    string desc = joker.getDescription();
    float descScale = 1.05f;
    vector<string> descLines;
    istringstream iss(desc);
    string word;
    string currentLine = "";
    while (iss >> word) {
        string testLine = currentLine.empty() ? word : (currentLine + " " + word);
        if (Renderer::getTextWidth(testLine, descScale) <= maxTextW) {
            currentLine = testLine;
        } else {
            if (!currentLine.empty()) descLines.push_back(currentLine);
            currentLine = word;
        }
    }
    if (!currentLine.empty()) descLines.push_back(currentLine);
    if (descLines.empty()) descLines.push_back(desc);

    // Defensive scale check so text never spills outside
    for (const auto& line : descLines) {
        float lw = Renderer::getTextWidth(line, descScale);
        if (lw > maxTextW) {
            descScale *= (maxTextW / lw);
        }
    }

    // Dynamic height based on number of lines
    float tipH = (descLines.size() <= 1) ? 96.0f : (110.0f + (descLines.size() - 2) * 16.0f);
    float tipX = x + 33.0f - tipW * 0.5f;
    tipX = max(20.0f, min(static_cast<float>(windowWidth) - tipW - 20.0f, tipX));
    float tipY = (engine.getCurrentStateName() == "SHOP") ? (y + 134.0f) : (y + 96.0f);
    if (tipY + tipH > static_cast<float>(windowHeight) - 15.0f) {
        tipY = y - tipH - 10.0f; // Muncul di atas jika di bawah mentok layar
    }

    Renderer::drawRect(tipX + 4.0f, tipY + 4.0f, tipW, tipH, Color(0.0f, 0.0f, 0.0f, 0.45f));
    Renderer::drawRect(tipX, tipY, tipW, tipH, Color(0.97f, 0.96f, 0.93f));
    Renderer::drawRectOutline(tipX, tipY, tipW, tipH, 2.5f, Theme::GOLD);

    // Title Badge
    Renderer::drawRect(tipX + 6.0f, tipY + 6.0f, tipW - 12.0f, 22.0f, Theme::MULT_RED);
    float titleScale = 1.25f;
    float maxTitleW = tipW - 20.0f;
    float titleW = Renderer::getTextWidth(joker.getName(), titleScale);
    if (titleW > maxTitleW) {
        titleScale *= (maxTitleW / titleW);
    }
    Renderer::drawCenteredText(joker.getName(), tipX + tipW * 0.5f, tipY + 10.0f, titleScale, Theme::TEXT_WHITE);

    // Divider
    Renderer::drawRect(tipX + 10.0f, tipY + 34.0f, tipW - 20.0f, 1.0f, Color(0.75f, 0.75f, 0.70f));

    // Description Lines
    if (descLines.size() <= 1) {
        Renderer::drawCenteredText(descLines[0], tipX + tipW * 0.5f, tipY + 46.0f, descScale, Theme::TEXT_DARK);
    } else {
        float lineY = tipY + 42.0f;
        for (const auto& line : descLines) {
            Renderer::drawCenteredText(line, tipX + tipW * 0.5f, lineY, descScale, Theme::TEXT_DARK);
            lineY += 16.0f;
        }
    }

    // Rarity Badge
    string rarityStr = (joker.getRarity() == JokerRarity::RARE) ? "RARE" : 
                       ((joker.getRarity() == JokerRarity::UNCOMMON) ? "UNCOMMON" : "COMMON");
    Color rarityCol = (joker.getRarity() == JokerRarity::RARE) ? Theme::MULT_RED : 
                      ((joker.getRarity() == JokerRarity::UNCOMMON) ? Theme::CHIPS_BLUE : Color(0.3f, 0.6f, 0.4f));
    Renderer::drawCenteredText(rarityStr, tipX + tipW * 0.5f, tipY + tipH - 22.0f, 1.0f, rarityCol);
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

    float contentY = 202.0f;
    if (blind.getDebuff() != BossDebuff::NONE) {
        float debuffBoxW = 230.0f;
        float debuffBoxH = 50.0f;
        Renderer::drawRect(40.0f, contentY, debuffBoxW, debuffBoxH, Color(0.25f, 0.05f, 0.05f, 0.9f));
        Renderer::drawRectOutline(40.0f, contentY, debuffBoxW, debuffBoxH, 1.5f, Theme::MULT_RED);
        Renderer::drawText("DEBUFF ACTIVE:", 46.0f, contentY + 6.0f, 1.1f, Theme::MULT_RED);
        Renderer::drawWrappedText(blind.getDebuffDescription(), 46.0f, contentY + 22.0f, debuffBoxW - 12.0f, 1.0f, 2.0f, Theme::TEXT_WHITE);
        contentY += debuffBoxH + 10.0f;
    } else {
        contentY += 6.0f;
    }

    Renderer::drawRect(45.0f, contentY, 220.0f, 2.0f, Color(0.3f, 0.5f, 0.4f, 0.6f));
    contentY += 12.0f;

    // Round Score dengan Nilai Bergulir Halus
    Renderer::drawText("ROUND SCORE:", 45.0f, contentY, 1.35f, Theme::TEXT_WHITE);
    string scoreStr = to_string(displayedRoundScore) + " / " + to_string(blind.getTargetScore());
    Renderer::drawText(scoreStr, 45.0f, contentY + 20.0f, 1.9f, Theme::GOLD);
    contentY += 46.0f;

    // Dynamic Target Progress Bar (Bilah Kemajuan Skor)
    float barX = 45.0f;
    float barY = contentY;
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
    contentY += 20.0f;

    string moneyStr = "Money: $" + to_string(engine.getMoney());
    Renderer::drawText(moneyStr, 45.0f, contentY, 1.7f, Theme::GOLD);
    contentY += 30.0f;

    string handsStr = "Hands: " + to_string(engine.getHandsRemaining());
    Renderer::drawText(handsStr, 45.0f, contentY, 1.7f, Theme::CHIPS_BLUE);
    contentY += 28.0f;

    string discardsStr = "Discards: " + to_string(engine.getDiscardsRemaining());
    Renderer::drawText(discardsStr, 45.0f, contentY, 1.7f, Theme::MULT_RED);
    contentY += 28.0f;

    string deckStr = "Deck: " + to_string(engine.getDeckRemainingCount()) + " cards";
    Renderer::drawText(deckStr, 45.0f, contentY, 1.25f, Theme::TEXT_WHITE);
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
    float centerTargetY = 235.0f;

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

    currentJokerTriggerIndex = 0;
    jokerAnimStates.assign(engine.getJokers().size(), JokerAnimState{0.0f, 1.0f});

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

    // Lerp joker scale and glow back to normal
    for (auto& ja : jokerAnimStates) {
        ja.scale += (1.0f - ja.scale) * min(1.0f, 10.0f * effectiveDt);
        ja.glow += (0.0f - ja.glow) * min(1.0f, 8.0f * effectiveDt);
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

                    AudioManager::getInstance().playSound(AudioAssets::SFX_CHIP_POP);
                    string chipPop = "+" + to_string(chipGained);
                    spawnFloatingText(chipPop, ac.targetX + 44.0f, ac.targetY - 20.0f, Theme::CHIPS_BLUE, 1.8f);

                    // Periksa jika ada Joker yang terpicu langsung oleh kartu ini (misal Greedy, Lusty, Wrathful, Gluttonous)
                    const auto& jokers = engine.getJokers();
                    float slotW = 66.0f;
                    float spacing = 12.0f;
                    float startX = 310.0f + (680.0f - (engine.getMaxJokers() * slotW + (engine.getMaxJokers() - 1) * spacing)) * 0.5f;

                    for (size_t jIdx = 0; jIdx < jokers.size(); ++jIdx) {
                        const auto& joker = jokers[jIdx];
                        bool suitMatches = false;
                        if (joker.getId() == "joker_greedy" && ac.card.getSuit() == Suit::DIAMONDS && !ac.card.isDebuffed()) suitMatches = true;
                        if (joker.getId() == "joker_lusty" && ac.card.getSuit() == Suit::HEARTS && !ac.card.isDebuffed()) suitMatches = true;
                        if (joker.getId() == "joker_wrathful" && ac.card.getSuit() == Suit::SPADES && !ac.card.isDebuffed()) suitMatches = true;
                        if (joker.getId() == "joker_gluttonous" && ac.card.getSuit() == Suit::CLUBS && !ac.card.isDebuffed()) suitMatches = true;

                        if (suitMatches) {
                            if (jIdx < jokerAnimStates.size()) {
                                jokerAnimStates[jIdx].scale = 1.30f;
                                jokerAnimStates[jIdx].glow = 1.5f;
                            }
                            animMult += 3;
                            multBoxScale = 1.35f;
                            AudioManager::getInstance().playSound(AudioAssets::SFX_MULT_POP);
                            float slotX = startX + jIdx * (slotW + spacing);
                            spawnFloatingText("+3 MULT", slotX + slotW * 0.5f, 115.0f, Theme::MULT_RED, 1.8f);
                        }
                    }
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
            animMult = activeScoreResult.baseMult; // Poker Hand base mult
            multBoxScale = 1.35f;
            AudioManager::getInstance().playSound(AudioAssets::SFX_MULT_POP);
            string multPop = "+" + to_string(activeScoreResult.baseMult) + " MULT";
            spawnFloatingText(multPop, 790.0f, 80.0f, Theme::MULT_RED, 2.0f);
        }

        if (scoringTimer >= 0.45f) {
            scoringStep = ScoringStep::JOKER_TRIGGERS;
            scoringTimer = 0.0f;
            currentJokerTriggerIndex = 0;
        }
    } 
    else if (scoringStep == ScoringStep::JOKER_TRIGGERS) {
        const auto& jokers = engine.getJokers();
        float jokerTriggerInterval = 0.38f;

        if (currentJokerTriggerIndex < static_cast<int>(jokers.size())) {
            if (scoringTimer >= jokerTriggerInterval) {
                scoringTimer -= jokerTriggerInterval;

                int jIdx = currentJokerTriggerIndex;
                const auto& joker = jokers[jIdx];

                // Hitung perolehan spesifik dari Joker ini
                vector<Card> selected;
                for (const auto& ac : scoringAnimatedCards) {
                    selected.push_back(ac.card);
                }
                ScoringContext ctx{
                    selected,
                    activeScoreResult.scoringCards,
                    engine.getDeckRemainingCount(),
                    engine.getDiscardsRemaining()
                };

                int jChips = 0, jMult = 0;
                joker.trigger(ctx, jChips, jMult);

                // Hitung posisi visual slot Joker di layar
                float slotW = 66.0f;
                float spacing = 12.0f;
                float startX = 310.0f + (680.0f - (engine.getMaxJokers() * slotW + (engine.getMaxJokers() - 1) * spacing)) * 0.5f;
                float slotX = startX + jIdx * (slotW + spacing);
                float slotY = 22.0f;

                // Jika Joker adalah suit booster (Greedy/Lusty/Wrathful/Gluttonous), efeknya sudah terpicu per kartu
                bool isSuitBooster = (joker.getId() == "joker_greedy" || joker.getId() == "joker_lusty" ||
                                      joker.getId() == "joker_wrathful" || joker.getId() == "joker_gluttonous");

                if (isSuitBooster) {
                    // Berikan feedback visual singkat bahwa kartu suit Joker ini telah berkontribusi
                    if (jMult > 0) {
                        if (jIdx < static_cast<int>(jokerAnimStates.size())) {
                            jokerAnimStates[jIdx].scale = 1.25f;
                            jokerAnimStates[jIdx].glow = 1.2f;
                        }
                    }
                } else {
                    if (jIdx < static_cast<int>(jokerAnimStates.size())) {
                        jokerAnimStates[jIdx].scale = 1.30f;
                        jokerAnimStates[jIdx].glow = 1.5f;
                    }

                    if (jMult > 0) {
                        animMult += jMult;
                        multBoxScale = 1.35f;
                        AudioManager::getInstance().playSound(AudioAssets::SFX_MULT_POP);
                        string jPop = "+" + to_string(jMult) + " MULT";
                        spawnFloatingText(jPop, slotX + slotW * 0.5f, slotY + 95.0f, Theme::MULT_RED, 1.8f);
                    } else if (jChips > 0) {
                        animChips += jChips;
                        chipsBoxScale = 1.35f;
                        AudioManager::getInstance().playSound(AudioAssets::SFX_CHIP_POP);
                        string jPop = "+" + to_string(jChips) + " CHIPS";
                        spawnFloatingText(jPop, slotX + slotW * 0.5f, slotY + 95.0f, Theme::CHIPS_BLUE, 1.8f);
                    } else {
                        // Joker tidak memenuhi syarat scoring saat ini
                        spawnFloatingText("...", slotX + slotW * 0.5f, slotY + 95.0f, Theme::DEBUFF_GRAY, 1.2f);
                    }
                }

                currentJokerTriggerIndex++;
            }
        } else {
            if (scoringTimer >= 0.30f) {
                scoringStep = ScoringStep::FINAL_SLAM;
                scoringTimer = 0.0f;
            }
        }
    }
    else if (scoringStep == ScoringStep::FINAL_SLAM) {
        if (animFinalScore == 0) {
            animFinalScore = activeScoreResult.finalScore;
            chipsBoxScale = 1.35f;
            multBoxScale = 1.35f;
            cameraShake = 8.0f;

            AudioManager::getInstance().playSound(AudioAssets::SFX_SCORE_SLAM);
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
        string stateBefore = engine.getCurrentStateName();
        try {
            engine.playHand();
        } catch (const GameException& e) {
            showToast(e.what());
        }

        string stateAfter = engine.getCurrentStateName();
        displayedRoundScore = engine.getCurrentScore();
        targetRoundScore = engine.getCurrentScore();
        scoringAnimatedCards.clear();
        scoringStep = ScoringStep::IDLE;

        // Jika ronde selesai (ROUND_WON atau GAME_OVER), picu transisi CRT ke layar hasil
        if (stateAfter != stateBefore) {
            // Rendered state adalah state lama sampai layar CRT menutup
            renderedState = stateBefore;
            isTransitioning = true;
            transitionTimer = 0.0f;
            transitionSwapped = true; // State sudah berubah di engine
            pendingTransitionAction = nullptr;
        }
    }
}

void GameWindow::renderActiveScoringDisplay() {
    // 1. Nama Hand Poker di Bawah Baris Joker
    Renderer::drawCenteredText(activeScoreResult.handName, 650.0f, 116.0f, 2.0f, Theme::GOLD);

    // 2. Kotak CHIPS Berdenyut (Biru)
    float bw = 140.0f * chipsBoxScale;
    float bh = 54.0f * chipsBoxScale;
    float bx = 500.0f - bw * 0.5f;
    float by = 168.0f - bh * 0.5f;

    Renderer::drawRect(bx, by, bw, bh, Theme::CHIPS_BLUE);
    Renderer::drawRectOutline(bx, by, bw, bh, 2.5f, (chipsBoxScale > 1.05f) ? Color(1.0f, 1.0f, 1.0f, 1.0f) : Color(1, 1, 1, 0.8f));
    Renderer::drawCenteredText("CHIPS", bx + bw * 0.5f, by + 6.0f, 1.1f, Theme::TEXT_WHITE);
    string cStr = to_string(animChips);
    Renderer::drawCenteredText(cStr, bx + bw * 0.5f, by + 24.0f, 2.2f, Theme::TEXT_WHITE);

    // 3. Simbol Perkalian "X"
    Renderer::drawCenteredText("X", 650.0f, 155.0f, 2.6f, Theme::TEXT_WHITE);

    // 4. Kotak MULT Berdenyut (Merah)
    float mw = 140.0f * multBoxScale;
    float mh = 54.0f * multBoxScale;
    float mx = 800.0f - mw * 0.5f;
    float my = 168.0f - mh * 0.5f;

    Renderer::drawRect(mx, my, mw, mh, Theme::MULT_RED);
    Renderer::drawRectOutline(mx, my, mw, mh, 2.5f, (multBoxScale > 1.05f) ? Color(1.0f, 1.0f, 1.0f, 1.0f) : Color(1, 1, 1, 0.8f));
    Renderer::drawCenteredText("MULT", mx + mw * 0.5f, my + 6.0f, 1.1f, Theme::TEXT_WHITE);
    string mStr = to_string(animMult);
    Renderer::drawCenteredText(mStr, mx + mw * 0.5f, my + 24.0f, 2.2f, Theme::TEXT_WHITE);

    // 5. Total Skor yang Terhitung saat Slam
    if (scoringStep == ScoringStep::FINAL_SLAM || scoringStep == ScoringStep::SCORE_TICK) {
        string totalStr = "SCORED +" + to_string(animFinalScore) + " PTS!";
        Renderer::drawCenteredText(totalStr, 650.0f, 204.0f, 2.0f, Theme::GOLD);
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
    Renderer::drawRect(310.0f, 15.0f, 680.0f, 395.0f, Theme::PANEL_BG);
    Renderer::drawRectOutline(310.0f, 15.0f, 680.0f, 395.0f, 2.0f, Color(0.2f, 0.4f, 0.35f, 0.8f));

    string state = engine.getCurrentStateName();

    if (state == "ROUND_WON") {
        Renderer::drawText("ROUND CLEARED!", 460.0f, 145.0f, 3.2f, Theme::GOLD);
        string winSub = "Target reached! Blind reward $" + to_string(engine.getCurrentBlind().getRewardMoney()) + " earned.";
        Renderer::drawText(winSub, 400.0f, 200.0f, 1.5f, Theme::TEXT_WHITE);
        Renderer::drawText("Click 'CASH OUT' to enter THE SHOP", 440.0f, 235.0f, 1.5f, Theme::CHIPS_BLUE);
        return;
    }

    if (state == "GAME_OVER") {
        Renderer::drawText("GAME OVER!", 480.0f, 145.0f, 3.5f, Theme::MULT_RED);
        Renderer::drawText("You ran out of hands before beating the blind.", 390.0f, 200.0f, 1.5f, Theme::TEXT_WHITE);
        Renderer::drawText("Click 'TRY AGAIN' or return to 'MAIN MENU'", 410.0f, 235.0f, 1.5f, Theme::GOLD);
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

        // 1. Nama Hand di y: 116px
        Renderer::drawCenteredText(handRes.handName, 650.0f, 116.0f, 2.0f, Theme::GOLD);

        // 2. Kotak CHIPS
        Renderer::drawRect(430.0f, 142.0f, 140.0f, 54.0f, Theme::CHIPS_BLUE);
        Renderer::drawRectOutline(430.0f, 142.0f, 140.0f, 54.0f, 2.5f, Color(1,1,1,0.8f));
        Renderer::drawCenteredText("CHIPS", 500.0f, 148.0f, 1.1f, Theme::TEXT_WHITE);
        string cStr = to_string(totalC);
        Renderer::drawCenteredText(cStr, 500.0f, 166.0f, 2.2f, Theme::TEXT_WHITE);

        // 3. Simbol "X"
        Renderer::drawCenteredText("X", 650.0f, 155.0f, 2.6f, Theme::TEXT_WHITE);

        // 4. Kotak MULT
        Renderer::drawRect(730.0f, 142.0f, 140.0f, 54.0f, Theme::MULT_RED);
        Renderer::drawRectOutline(730.0f, 142.0f, 140.0f, 54.0f, 2.5f, Color(1,1,1,0.8f));
        Renderer::drawCenteredText("MULT", 800.0f, 148.0f, 1.1f, Theme::TEXT_WHITE);
        string mStr = to_string(totalM);
        Renderer::drawCenteredText(mStr, 800.0f, 166.0f, 2.2f, Theme::TEXT_WHITE);

        // 5. Perkiraan Skor & Hint
        string estStr = "Estimated Score: " + to_string(estScore);
        Renderer::drawCenteredText(estStr, 650.0f, 214.0f, 1.6f, Theme::TEXT_WHITE);

        string hintStr = to_string(selected.size()) + " card(s) selected (" + 
                         to_string(handRes.scoringCards.size()) + " scoring cards)";
        Renderer::drawCenteredText(hintStr, 650.0f, 246.0f, 1.25f, Color(0.8f, 0.8f, 0.8f, 0.9f));
    } else {
        Renderer::drawCenteredText("SELECT 1 - 5 CARDS", 650.0f, 150.0f, 2.2f, Theme::TEXT_WHITE);
        int maxHand = engine.getMaxHandSize();
        string keyHint = "Click cards below or press [1 - 8] on keyboard";
        if (maxHand >= 10) keyHint = "Click cards below or press [1 - 0] on keyboard";
        else if (maxHand == 9) keyHint = "Click cards below or press [1 - 9] on keyboard";
        Renderer::drawCenteredText(keyHint, 650.0f, 190.0f, 1.35f, Color(0.7f, 0.8f, 0.75f, 0.9f));
        Renderer::drawCenteredText("Press PLAY HAND to score or DISCARD to draw new cards", 650.0f, 222.0f, 1.25f, Color(0.6f, 0.7f, 0.65f, 0.8f));

        const auto& last = engine.getLastScoreResult();
        if (last.finalScore > 0) {
            string lastStr = "Last Hand: " + last.handName + " scored " + to_string(last.finalScore) + " pts!";
            Renderer::drawCenteredText(lastStr, 650.0f, 280.0f, 1.35f, Theme::GOLD);
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
        bool hoverCash = isPointInRect(mx, my, 440.0f, 440.0f, 160.0f, 48.0f);
        bool hoverRunInfo = isPointInRect(mx, my, 620.0f, 440.0f, 140.0f, 48.0f);
        Renderer::drawButton("CASH OUT", 440.0f, 440.0f, 160.0f, 48.0f, Theme::GOLD, Theme::TEXT_DARK, hoverCash, true);
        Renderer::drawButton("RUN INFO", 620.0f, 440.0f, 140.0f, 48.0f, Color(0.20f, 0.48f, 0.45f), Theme::TEXT_WHITE, hoverRunInfo, true);
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
    bool hoverRunInfo = isPointInRect(mx, my, 750.0f, 440.0f, 140.0f, 48.0f);

    Renderer::drawButton("PLAY HAND", 410.0f, 440.0f, 140.0f, 48.0f, Theme::CHIPS_BLUE, Theme::TEXT_WHITE, hoverPlay, canPlay);
    Renderer::drawButton("DISCARD", 580.0f, 440.0f, 140.0f, 48.0f, Theme::MULT_RED, Theme::TEXT_WHITE, hoverDiscard, canDiscard);
    Renderer::drawButton("RUN INFO", 750.0f, 440.0f, 140.0f, 48.0f, Color(0.20f, 0.48f, 0.45f), Theme::TEXT_WHITE, hoverRunInfo, true);
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
    int handLimit = max(8, max(static_cast<int>(hand.size()), engine.getMaxHandSize()));
    float startX, cardW, cardH, spacing, baseY;
    getCardLayout(handLimit, startX, cardW, cardH, spacing, baseY);

    for (size_t i = 0; i < hand.size(); ++i) {
        if (hand[i].isSelected()) {
            AnimatedCard ac(hand[i]);
            ac.currentX = (i < cardAnims.size() && cardAnims[i].initialized) ? cardAnims[i].x : (startX + i * (cardW + spacing));
            ac.currentY = (i < cardAnims.size() && cardAnims[i].initialized) ? cardAnims[i].y : baseY;
            ac.targetX = static_cast<float>(windowWidth) + 180.0f; // Geser ke kanan ke luar layar!
            ac.targetY = ac.currentY;
            discardAnimatedCards.push_back(ac);
        }
    }

    isDiscarding = true;
    discardTimer = 0.0f;
    AudioManager::getInstance().playSound(AudioAssets::SFX_CARD_DISCARD);
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

void GameWindow::getCardLayout(int handLimit, float& startX, float& cardW, float& cardH, float& spacing, float& baseY) const {
    cardH = 130.0f;
    baseY = 530.0f;
    if (handLimit >= 10) {
        startX = 24.0f;
        cardW = 85.0f;
        spacing = 15.0f;
    } else if (handLimit == 9) {
        startX = 32.0f;
        cardW = 88.0f;
        spacing = 21.0f;
    } else {
        startX = 55.0f;
        cardW = 88.0f;
        spacing = 28.0f;
    }
}

void GameWindow::triggerDealAnimation() {
    clearNotifications();
    cardAnims.clear();
    const auto& hand = engine.getHand();
    cardAnims.resize(hand.size());

    int handLimit = max(8, max(static_cast<int>(hand.size()), engine.getMaxHandSize()));
    float startX, cardW, cardH, spacing, baseY;
    getCardLayout(handLimit, startX, cardW, cardH, spacing, baseY);

    for (size_t i = 0; i < hand.size(); ++i) {
        float targetX = startX + i * (cardW + spacing);
        float targetElev = hand[i].isSelected() ? 1.0f : 0.0f;
        cardAnims[i].card = hand[i];
        cardAnims[i].x = targetX;
        cardAnims[i].y = static_cast<float>(windowHeight) + 160.0f;
        cardAnims[i].elevation = targetElev;
        cardAnims[i].delayTimer = i * 0.050f;
        cardAnims[i].initialized = true;
    }
    // Suara deal kartu pertama
    AudioManager::getInstance().playSound(AudioAssets::SFX_CARD_DEAL, 0.45f);
}

void GameWindow::updateCardAnimations(float dt) {
    const auto& hand = engine.getHand();
    int handLimit = max(8, max(static_cast<int>(hand.size()), engine.getMaxHandSize()));
    float startX, cardW, cardH, spacing, baseY;
    getCardLayout(handLimit, startX, cardW, cardH, spacing, baseY);

    // Rekonsiliasi sinkronisasi animasi kartu dengan isi hand berdasarkan identitas kartu
    vector<CardAnim> updatedAnims(hand.size());
    int newlyDrawnCount = 0;

    for (size_t i = 0; i < hand.size(); ++i) {
        bool found = false;
        for (auto& oldAnim : cardAnims) {
            if (oldAnim.initialized && oldAnim.card == hand[i]) {
                updatedAnims[i] = oldAnim;
                found = true;
                break;
            }
        }
        if (!found) {
            // Kartu baru (misal ditarik setelah discard / play hand): animasi meluncur dari bawah!
            float targetX = startX + i * (cardW + spacing);

            updatedAnims[i].card = hand[i];
            updatedAnims[i].x = targetX;
            updatedAnims[i].y = static_cast<float>(windowHeight) + 160.0f;
            updatedAnims[i].elevation = hand[i].isSelected() ? 1.0f : 0.0f;
            updatedAnims[i].delayTimer = newlyDrawnCount * 0.050f;
            updatedAnims[i].initialized = true;
            newlyDrawnCount++;
        }
    }
    cardAnims = updatedAnims;

    float mx = static_cast<float>(mouseX);
    float my = static_cast<float>(mouseY);

    hoveredCardIndex = -1;

    for (size_t i = 0; i < hand.size(); ++i) {
        float targetX = startX + i * (cardW + spacing);

        float curX = cardAnims[i].initialized ? cardAnims[i].x : targetX;
        float curY = cardAnims[i].initialized ? cardAnims[i].y : baseY;
        bool isHovered = (!showRunInfoModal && scoringStep == ScoringStep::IDLE && !isDiscarding) && isPointInRect(mx, my, curX, curY, cardW, cardH);
        if (isHovered) {
            hoveredCardIndex = static_cast<int>(i);
        }

        float targetElev = hand[i].isSelected() ? 1.0f : (isHovered ? 0.35f : 0.0f);
        float targetY = baseY - targetElev * 32.0f;

        if (cardAnims[i].delayTimer > 0.0f) {
            float prevTimer = cardAnims[i].delayTimer;
            cardAnims[i].delayTimer -= dt;
            if (prevTimer > 0.0f && cardAnims[i].delayTimer <= 0.0f) {
                AudioManager::getInstance().playSound(AudioAssets::SFX_CARD_DEAL, 0.45f);
            }
        } else {
            float speed = min(1.0f, 15.0f * dt);
            cardAnims[i].x += (targetX - cardAnims[i].x) * speed;
            cardAnims[i].y += (targetY - cardAnims[i].y) * speed;
            cardAnims[i].elevation += (targetElev - cardAnims[i].elevation) * speed;
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

    int handLimit = max(8, max(static_cast<int>(hand.size()), engine.getMaxHandSize()));
    float startX, cardW, cardH, spacing, baseY;
    getCardLayout(handLimit, startX, cardW, cardH, spacing, baseY);

    const Card& card = hand[hoveredCardIndex];
    float cardX = (hoveredCardIndex < static_cast<int>(cardAnims.size()) && cardAnims[hoveredCardIndex].initialized) ? 
                   cardAnims[hoveredCardIndex].x : (startX + hoveredCardIndex * (cardW + spacing));
    float cardY = (hoveredCardIndex < static_cast<int>(cardAnims.size()) && cardAnims[hoveredCardIndex].initialized) ? 
                   cardAnims[hoveredCardIndex].y : baseY;

    float tipW = 180.0f;
    float tipH = 72.0f;
    float tipX = cardX + (cardW - tipW) * 0.5f;
    tipX = max(15.0f, min(static_cast<float>(windowWidth) - tipW - 15.0f, tipX));
    float tipY = cardY - tipH - 14.0f;
    if (tipY < 10.0f) tipY = cardY + cardH + 10.0f;

    // 1. Bayangan Tooltip
    Renderer::drawRect(tipX + 4.0f, tipY + 4.0f, tipW, tipH, Color(0.0f, 0.0f, 0.0f, 0.38f * hoverTooltipAlpha));

    // 2. Bodi Kotak Putih Bersih
    Renderer::drawRect(tipX, tipY, tipW, tipH, Color(0.97f, 0.97f, 0.95f, hoverTooltipAlpha));
    Renderer::drawRectOutline(tipX, tipY, tipW, tipH, 2.0f, Color(0.25f, 0.32f, 0.30f, hoverTooltipAlpha));

    // 3. Kotak Badge Header Kartu ("Queen of Hearts")
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
    float titleScale = 1.2f;
    float maxTitleW = badgeW - 10.0f;
    float tw = Renderer::getTextWidth(fullTitle, titleScale);
    if (tw > maxTitleW && tw > 0.0f) {
        titleScale *= (maxTitleW / tw);
    }
    Renderer::drawCenteredText(fullTitle, badgeX + badgeW * 0.5f, badgeY + 6.0f, titleScale, 
                               Color(suitCol.r, suitCol.g, suitCol.b, hoverTooltipAlpha));

    // 4. Garis Pembatas
    Renderer::drawRect(tipX + 10.0f, tipY + 36.0f, tipW - 20.0f, 1.0f, Color(0.78f, 0.80f, 0.78f, hoverTooltipAlpha));

    // 5. Bagian Bawah: "+X chips" atau "DEBUFFED (0 chips)"
    float maxBottomW = tipW - 16.0f;
    if (card.isDebuffed()) {
        string debuffText = "DEBUFFED (0 chips)";
        float debuffScale = 1.05f;
        float dw = Renderer::getTextWidth(debuffText, debuffScale);
        if (dw > maxBottomW && dw > 0.0f) {
            debuffScale *= (maxBottomW / dw);
        }
        Renderer::drawCenteredText(debuffText, tipX + tipW * 0.5f, tipY + 46.0f, debuffScale, 
                                   Color(Theme::MULT_RED.r, Theme::MULT_RED.g, Theme::MULT_RED.b, hoverTooltipAlpha));
    } else {
        string chipStr = "+" + to_string(card.getBaseChips()) + " chips";
        float chipScale = 1.35f;
        float cw = Renderer::getTextWidth(chipStr, chipScale);
        if (cw > maxBottomW && cw > 0.0f) {
            chipScale *= (maxBottomW / cw);
        }
        Renderer::drawCenteredText(chipStr, tipX + tipW * 0.5f, tipY + 46.0f, chipScale, 
                                   Color(Theme::CHIPS_BLUE.r, Theme::CHIPS_BLUE.g, Theme::CHIPS_BLUE.b, hoverTooltipAlpha));
    }
}

void GameWindow::renderCards() {
    const auto& hand = engine.getHand();
    int handLimit = max(8, max(static_cast<int>(hand.size()), engine.getMaxHandSize()));
    float startX, cardW, cardH, spacing, baseY;
    getCardLayout(handLimit, startX, cardW, cardH, spacing, baseY);

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

        bool isHovered = (!showRunInfoModal && scoringStep == ScoringStep::IDLE && !isDiscarding) && isPointInRect(mx, my, cx, cy, cardW, cardH);

        Renderer::drawCard(hand[i], cx, cy, cardW, cardH, isHovered, elev);

        string keyNum = (i == 9) ? "[0]" : ("[" + to_string(i + 1) + "]");
        float keyTextW = Renderer::getTextWidth(keyNum, 1.1f);
        Renderer::drawText(keyNum, cx + (cardW - keyTextW) * 0.5f, baseY + cardH + 12.0f, 1.1f, Color(0.6f, 0.7f, 0.65f, 0.8f));
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
    if (!showRunInfoModal) {
        renderCardTooltip(0.016f); // Update dan render tooltip hanya saat modal tertutup
    }
    renderFloatingTexts();
}

// ================= EFEK PARTIKEL FRAME BLIND TERPILIH =================
void GameWindow::updateFrameParticles(float dt, float fx, float fy, float fw, float fh, Color col) {
    frameParticleTimer += dt;
    // Spawn partikel baru secara berkala mengelilingi 4 sisi frame
    if (frameParticleTimer >= 0.05f) {
        frameParticleTimer = 0.0f;

        // Pilih sisi acak dari frame: 0 = atas, 1 = kanan, 2 = bawah, 3 = kiri
        for (int p = 0; p < 2; ++p) {
            FrameParticle fp;
            int side = rand() % 4;
            float speed = 12.0f + static_cast<float>(rand() % 16); // Partikel bergerak lambat ke arah luar

            if (side == 0) { // Atas: gerak ke atas sedikit menyebar
                fp.x = fx + static_cast<float>(rand() % static_cast<int>(fw));
                fp.y = fy;
                fp.vx = ((rand() % 21) - 10) * 1.5f;
                fp.vy = -speed;
            } else if (side == 1) { // Kanan: gerak ke kanan
                fp.x = fx + fw;
                fp.y = fy + static_cast<float>(rand() % static_cast<int>(fh));
                fp.vx = speed;
                fp.vy = ((rand() % 21) - 10) * 1.5f;
            } else if (side == 2) { // Bawah: gerak ke bawah
                fp.x = fx + static_cast<float>(rand() % static_cast<int>(fw));
                fp.y = fy + fh;
                fp.vx = ((rand() % 21) - 10) * 1.5f;
                fp.vy = speed;
            } else { // Kiri: gerak ke kiri
                fp.x = fx;
                fp.y = fy + static_cast<float>(rand() % static_cast<int>(fh));
                fp.vx = -speed;
                fp.vy = ((rand() % 21) - 10) * 1.5f;
            }

            fp.size = 2.5f + static_cast<float>(rand() % 3);
            fp.maxLife = 0.65f + static_cast<float>(rand() % 40) / 100.0f; // Muncul sebentar lalu fade menghilang
            fp.life = fp.maxLife;
            fp.color = col;

            frameParticles.push_back(fp);
        }
    }

    // Update posisi dan life partikel
    for (auto it = frameParticles.begin(); it != frameParticles.end();) {
        it->life -= dt;
        if (it->life <= 0.0f) {
            it = frameParticles.erase(it);
        } else {
            it->x += it->vx * dt;
            it->y += it->vy * dt;
            ++it;
        }
    }
}

void GameWindow::renderFrameParticles() {
    for (const auto& fp : frameParticles) {
        float lifeRatio = max(0.0f, fp.life / fp.maxLife);
        float alpha = lifeRatio * lifeRatio; // Fade out halus
        Color pCol(fp.color.r, fp.color.g, fp.color.b, alpha * 0.9f);
        Renderer::drawRect(fp.x, fp.y, fp.size, fp.size, pCol);
        // Core glow kecil di tengah
        Renderer::drawRect(fp.x + 0.5f, fp.y + 0.5f, max(1.0f, fp.size - 1.0f), max(1.0f, fp.size - 1.0f), Color(1.0f, 1.0f, 1.0f, alpha * 0.75f));
    }
}

// ================= EFEK TRANSISI TV JADUL MATI/NYALA (2 GARIS PUTIH BERTEMU DI TENGAH) =================
void GameWindow::startTransition(std::function<void()> action) {
    if (isTransitioning) return; // Cegah spam transisi bersamaan
    isTransitioning = true;
    transitionTimer = 0.0f;
    transitionSwapped = false;
    pendingTransitionAction = action;
    renderedState = engine.getCurrentStateName();
    AudioManager::getInstance().playSound(AudioAssets::SFX_CRT_OFF, 0.65f);
}

void GameWindow::updateTransition(float dt) {
    if (!isTransitioning) return;

    transitionTimer += dt;
    float halfTime = transitionDuration * 0.5f;

    // Tepat di tengah waktu transisi saat layar tertutup hitam total dan kedua garis bertemu di tengah:
    // Eksekusi aksi perpindahan layar / scene
    if (!transitionSwapped && transitionTimer >= halfTime) {
        if (pendingTransitionAction) {
            pendingTransitionAction();
            pendingTransitionAction = nullptr;
        }
        renderedState = engine.getCurrentStateName();
        transitionSwapped = true;
    }

    if (transitionTimer >= transitionDuration) {
        isTransitioning = false;
        transitionTimer = 0.0f;
        transitionSwapped = false;
        renderedState = engine.getCurrentStateName();
    }
}

void GameWindow::renderCrtTransition() {
    if (!isTransitioning) return;

    float t = transitionTimer / transitionDuration; // 0.0 s/d 1.0
    float ww = static_cast<float>(windowWidth);
    float wh = static_cast<float>(windowHeight);
    float halfH = wh * 0.5f;

    // Fase 1 (t: 0.0 s/d 0.5): TV MATI / MENUTUP (Kelopak mata menutup)
    // - Tutup hitam atas turun dari y=0 ke y=halfH
    // - Tutup hitam bawah naik dari y=wh ke y=halfH
    // - Garis putih 1 menempel tepat di ujung bawah tutup hitam atas
    // - Garis putih 2 menempel tepat di ujung atas tutup hitam bawah
    // Keduanya bergerak mendekat dan bertemu tepat di sumbu tengah (y = halfH)!
    if (t <= 0.5f) {
        float p = t / 0.5f; // 0.0 -> 1.0
        float ease = p * p; // Easing in akselerasi penutupan
        float curtainH = halfH * ease;
        float lineThick = max(3.0f, 6.0f * (1.0f - ease * 0.5f));

        // 1. Tutup hitam atas
        if (curtainH > 0.0f) {
            Renderer::drawRect(0.0f, 0.0f, ww, curtainH, Color(0.0f, 0.0f, 0.0f, 1.0f));
        }
        // Garis putih 1 (menempel pada ujung bawah tutup hitam atas)
        Renderer::drawRect(0.0f, curtainH - lineThick * 0.5f, ww, lineThick, Color(1.0f, 1.0f, 1.0f, 0.98f));

        // 2. Tutup hitam bawah
        if (curtainH > 0.0f) {
            Renderer::drawRect(0.0f, wh - curtainH, ww, curtainH, Color(0.0f, 0.0f, 0.0f, 1.0f));
        }
        // Garis putih 2 (menempel pada ujung atas tutup hitam bawah)
        Renderer::drawRect(0.0f, wh - curtainH - lineThick * 0.5f, ww, lineThick, Color(1.0f, 1.0f, 1.0f, 0.98f));

        // Ketika hampir bertemu di tengah (p > 0.85), garis menyatu dengan kilatan phosphor terang
        if (p > 0.85f) {
            float glowAlpha = (p - 0.85f) / 0.15f;
            Renderer::drawRect(0.0f, halfH - 4.0f, ww, 8.0f, Color(0.9f, 0.98f, 1.0f, glowAlpha));
        }
    }
    // Fase 2 (t: 0.5 s/d 1.0): TV NYALA / MEMBUKA (Kelopak mata membuka layar baru yang sudah berganti)
    // - Dari pertemuan di tengah (y=halfH), kedua garis putih dan tutup hitam membuka kembali ke atas dan bawah
    else {
        float p = (t - 0.5f) / 0.5f; // 0.0 -> 1.0
        float ease = sin(p * 3.14159265f * 0.5f); // Smooth ease out pembukaan
        float openH = halfH * ease;
        float curtainH = halfH - openH;
        float lineThick = max(2.5f, 5.0f * (1.0f - ease));

        // 1. Sisa tutup hitam atas menyusut ke atas
        if (curtainH > 0.0f) {
            Renderer::drawRect(0.0f, 0.0f, ww, curtainH, Color(0.0f, 0.0f, 0.0f, 1.0f));
        }
        // Garis putih 1 membuka ke atas
        float lineAlpha = max(0.0f, (1.0f - ease * 0.8f));
        Renderer::drawRect(0.0f, curtainH - lineThick * 0.5f, ww, lineThick, Color(1.0f, 1.0f, 1.0f, lineAlpha));

        // 2. Sisa tutup hitam bawah menyusut ke bawah
        if (curtainH > 0.0f) {
            Renderer::drawRect(0.0f, wh - curtainH, ww, curtainH, Color(0.0f, 0.0f, 0.0f, 1.0f));
        }
        // Garis putih 2 membuka ke bawah
        Renderer::drawRect(0.0f, wh - curtainH - lineThick * 0.5f, ww, lineThick, Color(1.0f, 1.0f, 1.0f, lineAlpha));

        // Kilatan phosphor lembut saat TV pertama kali menyala kembali
        float flashAlpha = (1.0f - p) * 0.30f;
        if (flashAlpha > 0.01f) {
            Renderer::drawRect(0.0f, 0.0f, ww, wh, Color(0.85f, 0.98f, 1.0f, flashAlpha));
        }
    }
}

void GameWindow::renderUI(float dt) {
    glClearColor(0.06f, 0.14f, 0.11f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Update Audio Engine (Fade-in / Fade-out musik & buffer update)
    AudioManager::getInstance().update(dt);

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

    // Update timer transisi TV CRT (eksekusi pergantian state tepat di tengah penutupan)
    updateTransition(dt);

    // Gunakan renderedState saat transisi agar tampilan lama tetap terlihat hingga tertutup total,
    // lalu berganti ke tampilan baru saat transisi membuka kembali.
    string displayState = isTransitioning ? renderedState : engine.getCurrentStateName();

    if (displayState == "MAIN_MENU") {
        renderMainMenu(dt);
    } else if (displayState == "BLIND_SELECT") {
        updateFloatingTexts(dt);
        renderBlindSelect();
        renderFloatingTexts();
    } else if (displayState == "SHOP") {
        renderShop();
    } else {
        updateCardAnimations(dt);
        updateScoringSequence(dt);
        updateDiscardSequence(dt);
        updateFloatingTexts(dt);
        renderRoundGameplay();
    }

    // Modal Run Info (Renders on top of gameplay)
    if (showRunInfoModal) {
        renderRunInfoModal();
    }

    // Modal Settings (Renders on top of screen)
    if (showSettingsModal) {
        renderSettingsModal();
    }

    // Reset Modelview matrix untuk CRT Transition & Toast notification
    glLoadIdentity();

    // Efek transisi layar TV jadul mati & nyala (2 garis putih bertemu di tengah)
    if (isTransitioning) {
        renderCrtTransition();
    }

    if (toastTimer > 0.0f) {
        float alpha = min(1.0f, toastTimer);
        Renderer::drawToast(toastMessage, static_cast<float>(windowWidth), static_cast<float>(windowHeight), alpha);
    }
}

void GameWindow::renderRunInfoModal() {
    float mx = static_cast<float>(mouseX);
    float my = static_cast<float>(mouseY);

    float modalW = 540.0f;
    float modalH = 580.0f;
    float modalX = (static_cast<float>(windowWidth) - modalW) * 0.5f;
    float modalY = (static_cast<float>(windowHeight) - modalH) * 0.5f;

    // 1. Dim latar belakang
    Renderer::drawRect(0.0f, 0.0f, static_cast<float>(windowWidth), static_cast<float>(windowHeight), Color(0.0f, 0.0f, 0.0f, 0.65f));

    // 2. Kotak Modal Utama
    Renderer::drawRect(modalX, modalY, modalW, modalH, Color(0.20f, 0.25f, 0.28f, 0.98f));
    Renderer::drawRectOutline(modalX, modalY, modalW, modalH, 3.5f, Color(0.40f, 0.50f, 0.54f));
    Renderer::drawRectOutline(modalX + 5.0f, modalY + 5.0f, modalW - 10.0f, modalH - 10.0f, 1.0f, Color(0.28f, 0.35f, 0.38f));

    // 3. Tab Bar di Bagian Atas
    float tab1X = modalX + 45.0f;
    float tabY = modalY + 28.0f;
    float tab1W = 145.0f;
    float tabH = 34.0f;

    float tab2X = tab1X + tab1W + 15.0f;
    float tab2W = 130.0f;

    float tab3X = tab2X + tab2W + 15.0f;
    float tab3W = 145.0f;

    // Hitung posisi panah merah penunjuk tab aktif
    float arrowCenterX = tab1X + tab1W * 0.5f;
    if (runInfoActiveTab == 1) {
        arrowCenterX = tab2X + tab2W * 0.5f;
    } else if (runInfoActiveTab == 2) {
        arrowCenterX = tab3X + tab3W * 0.5f;
    }

    // Indikator panah merah ke bawah di atas tab yang aktif
    glColor4f(0.88f, 0.32f, 0.28f, 1.0f);
    glBegin(GL_TRIANGLES);
        glVertex2f(arrowCenterX - 7.0f, tabY - 14.0f);
        glVertex2f(arrowCenterX + 7.0f, tabY - 14.0f);
        glVertex2f(arrowCenterX, tabY - 4.0f);
    glEnd();

    // Tab 1: Poker Hands
    bool tab1Active = (runInfoActiveTab == 0);
    Renderer::drawRect(tab1X, tabY, tab1W, tabH, tab1Active ? Color(0.88f, 0.32f, 0.28f) : Color(0.78f, 0.30f, 0.26f, 0.7f));
    Renderer::drawRectOutline(tab1X, tabY, tab1W, tabH, tab1Active ? 2.0f : 1.5f, tab1Active ? Color(0.60f, 0.20f, 0.18f) : Color(0.50f, 0.20f, 0.18f));
    Renderer::drawCenteredText("Poker Hands", tab1X + tab1W * 0.5f, tabY + 8.0f, 1.4f, tab1Active ? Theme::TEXT_WHITE : Color(0.9f, 0.9f, 0.9f, 0.8f));

    // Tab 2: Blinds
    bool tab2Active = (runInfoActiveTab == 1);
    Renderer::drawRect(tab2X, tabY, tab2W, tabH, tab2Active ? Color(0.88f, 0.32f, 0.28f) : Color(0.78f, 0.30f, 0.26f, 0.7f));
    Renderer::drawRectOutline(tab2X, tabY, tab2W, tabH, tab2Active ? 2.0f : 1.5f, tab2Active ? Color(0.60f, 0.20f, 0.18f) : Color(0.50f, 0.20f, 0.18f));
    Renderer::drawCenteredText("Blinds", tab2X + tab2W * 0.5f, tabY + 8.0f, 1.4f, tab2Active ? Theme::TEXT_WHITE : Color(0.9f, 0.9f, 0.9f, 0.8f));

    // Tab 3: Vouchers
    bool tab3Active = (runInfoActiveTab == 2);
    Renderer::drawRect(tab3X, tabY, tab3W, tabH, tab3Active ? Color(0.88f, 0.32f, 0.28f) : Color(0.78f, 0.30f, 0.26f, 0.7f));
    Renderer::drawRectOutline(tab3X, tabY, tab3W, tabH, tab3Active ? 2.0f : 1.5f, tab3Active ? Color(0.60f, 0.20f, 0.18f) : Color(0.50f, 0.20f, 0.18f));
    Renderer::drawCenteredText("Vouchers", tab3X + tab3W * 0.5f, tabY + 8.0f, 1.4f, tab3Active ? Theme::TEXT_WHITE : Color(0.9f, 0.9f, 0.9f, 0.8f));

    // KONTEN TAB:
    if (runInfoActiveTab == 1) {
        renderRunInfoBlindsTab(modalX, modalY, modalW, modalH, mx, my);
    } else if (runInfoActiveTab == 2) {
        renderRunInfoVouchersTab(modalX, modalY, modalW, modalH, mx, my);
    } else {
        // TAB 0: Poker Hands (Default)
        const HandType handsOrder[9] = {
            HandType::STRAIGHT_FLUSH,
            HandType::FOUR_OF_A_KIND,
            HandType::FULL_HOUSE,
            HandType::FLUSH,
            HandType::STRAIGHT,
            HandType::THREE_OF_A_KIND,
            HandType::TWO_PAIR,
            HandType::PAIR,
            HandType::HIGH_CARD
        };

        float tableStartY = modalY + 76.0f;
        float rowW = 490.0f;
        float rowH = 40.0f;
        float rowGap = 5.0f;
        float rx = modalX + (modalW - rowW) * 0.5f;

        hoveredRunInfoRow = -1;

        for (int i = 0; i < 9; ++i) {
            float ry = tableStartY + i * (rowH + rowGap);
            bool isHovered = isPointInRect(mx, my, rx, ry, rowW, rowH);
            if (isHovered) {
                hoveredRunInfoRow = i;
            }

            Color rowBg = isHovered ? Color(0.28f, 0.36f, 0.39f) : Color(0.16f, 0.20f, 0.23f);
            Renderer::drawRect(rx, ry, rowW, rowH, rowBg);
            Renderer::drawRectOutline(rx, ry, rowW, rowH, 1.0f, Color(0.30f, 0.38f, 0.42f));

            HandInfo info = getHandBaseInfo(handsOrder[i]);

            // Kapsul Level "lvl.1" di kiri
            float lvlX = rx + 8.0f;
            float lvlY = ry + 6.0f;
            Renderer::drawRect(lvlX, lvlY, 54.0f, 28.0f, Color(0.96f, 0.96f, 0.96f));
            Renderer::drawRectOutline(lvlX, lvlY, 54.0f, 28.0f, 1.5f, Color(0.70f, 0.72f, 0.75f));
            Renderer::drawCenteredText("lvl.1", lvlX + 27.0f, lvlY + 7.0f, 1.35f, Color(0.18f, 0.22f, 0.24f));

            // Nama Hand Poker
            Renderer::drawText(info.name, rx + 75.0f, ry + 12.0f, 1.4f, Theme::TEXT_WHITE);

            // Kapsul Gabungan Chips & Mult (Biru + Merah)
            float capsuleX = rx + 275.0f;
            float capsuleY = ry + 6.0f;
            float capsuleW = 135.0f;
            float capsuleH = 28.0f;

            // Bagian Kiri (Chips - Biru)
            Renderer::drawRect(capsuleX, capsuleY, 56.0f, capsuleH, Theme::CHIPS_BLUE);
            Renderer::drawCenteredText(to_string(info.baseChips), capsuleX + 28.0f, capsuleY + 6.0f, 1.4f, Theme::TEXT_WHITE);

            // Separator Tengah ('X')
            Renderer::drawRect(capsuleX + 56.0f, capsuleY, 18.0f, capsuleH, Color(0.88f, 0.28f, 0.24f));
            Renderer::drawCenteredText("X", capsuleX + 65.0f, capsuleY + 6.0f, 1.35f, Theme::TEXT_WHITE);

            // Bagian Kanan (Mult - Merah)
            Renderer::drawRect(capsuleX + 74.0f, capsuleY, 61.0f, capsuleH, Theme::MULT_RED);
            Renderer::drawCenteredText(to_string(info.baseMult), capsuleX + 104.5f, capsuleY + 6.0f, 1.4f, Theme::TEXT_WHITE);

            // Garis batas luar kapsul
            Renderer::drawRectOutline(capsuleX, capsuleY, capsuleW, capsuleH, 1.5f, Color(0.12f, 0.15f, 0.18f));

            // Kotak Statistik "# [count]" di kanan
            float cntX = rx + 422.0f;
            float cntY = ry + 6.0f;
            float cntW = 58.0f;
            float cntH = 28.0f;
            Renderer::drawRect(cntX, cntY, cntW, cntH, Color(0.12f, 0.16f, 0.18f));
            Renderer::drawRectOutline(cntX, cntY, cntW, cntH, 1.0f, Color(0.24f, 0.30f, 0.34f));

            Renderer::drawText("#", cntX + 8.0f, cntY + 6.0f, 1.4f, Color(0.70f, 0.75f, 0.80f));
            int count = engine.getHandPlayedCount(handsOrder[i]);
            Renderer::drawText(to_string(count), cntX + 26.0f, cntY + 6.0f, 1.4f, Color(0.98f, 0.75f, 0.15f));
        }

        // Popup Tooltip Detail saat Hover (Poker Hands)
        if (hoveredRunInfoRow >= 0 && hoveredRunInfoRow < 9) {
            float ttW = 460.0f;
            float ttH = 135.0f;
            float ttX = modalX + (modalW - ttW) * 0.5f;

            float hoveredRowY = tableStartY + hoveredRunInfoRow * (rowH + rowGap);
            float targetY = hoveredRowY + rowH + 4.0f;

            if (targetY + ttH > modalY + modalH - 56.0f) {
                targetY = hoveredRowY - ttH - 4.0f;
            }

            renderPokerHandDetailTooltip(handsOrder[hoveredRunInfoRow], ttX, targetY);
        }
    }

    // 5. Tombol Back di Paling Bawah Modal
    float backX = modalX + 25.0f;
    float backY = modalY + modalH - 50.0f;
    float backW = 490.0f;
    float backH = 38.0f;
    bool hoverBack = isPointInRect(mx, my, backX, backY, backW, backH);
    Color backBg = hoverBack ? Color(0.95f, 0.60f, 0.10f) : Color(0.85f, 0.50f, 0.05f);
    Renderer::drawButton("Back", backX, backY, backW, backH, backBg, Theme::TEXT_WHITE, hoverBack, true);
}

void GameWindow::renderRunInfoBlindsTab(float modalX, float modalY, float modalW, float modalH, float mx, float my) {
    (void)modalH;
    // 3 Kolom Kartu Vertikal (Small Blind, Big Blind, Boss Blind)
    float colW = 152.0f;
    float colH = 430.0f;
    float colGap = 16.0f;
    float startX = modalX + (modalW - (3 * colW + 2 * colGap)) * 0.5f;
    float startY = modalY + 74.0f;

    // Untuk tooltip tag yang di-hover
    string tooltipTitle = "";
    string tooltipDesc = "";
    float tooltipAnchorX = 0.0f;
    float tooltipAnchorY = 0.0f;

    for (int i = 0; i < 3; ++i) {
        float cx = startX + i * (colW + colGap);
        float cy = startY;
        Blind b = engine.getBlindByIndex(i);
        bool isCurrent = engine.isBlindCurrent(i);
        bool isDefeated = engine.isBlindDefeated(i);

        // Container utama kolom kartu
        Renderer::drawRect(cx, cy, colW, colH, Color(0.14f, 0.18f, 0.20f, 0.95f));
        Renderer::drawRectOutline(cx, cy, colW, colH, 2.0f, Color(0.28f, 0.36f, 0.40f));

        // 1. Badge Status di Bagian Atas: Current / Upcoming / Defeated
        float badgeW = colW - 20.0f;
        float badgeH = 26.0f;
        float badgeX = cx + 10.0f;
        float badgeY = cy + 10.0f;
        Color badgeBg = isDefeated ? Color(0.22f, 0.30f, 0.25f) : (isCurrent ? Color(0.88f, 0.28f, 0.24f) : Color(0.92f, 0.60f, 0.10f));
        string badgeText = isDefeated ? "Defeated" : (isCurrent ? "Current" : "Upcoming");
        Renderer::drawRect(badgeX, badgeY, badgeW, badgeH, badgeBg);
        Renderer::drawRectOutline(badgeX, badgeY, badgeW, badgeH, 1.2f, Color(0.12f, 0.15f, 0.18f));
        Renderer::drawCenteredText(badgeText, badgeX + badgeW * 0.5f, badgeY + 5.0f, 1.35f, Theme::TEXT_WHITE);

        // 2. Kapsul Header Nama Blind
        float nameY = cy + 42.0f;
        Color namePillBg = (i == 2) ? Color(0.68f, 0.15f, 0.12f) : (i == 1 ? Color(0.65f, 0.45f, 0.05f) : Color(0.12f, 0.38f, 0.65f));
        Renderer::drawRect(cx + 8.0f, nameY, colW - 16.0f, 26.0f, namePillBg);
        Renderer::drawRectOutline(cx + 8.0f, nameY, colW - 16.0f, 26.0f, 1.2f, Color(0.08f, 0.10f, 0.12f));
        Renderer::drawCenteredText(b.getName(), cx + colW * 0.5f, nameY + 6.0f, 1.2f, Theme::TEXT_WHITE);

        // 3. Chip Bulat Kasino (Polos tanpa teks duplikat di dalamnya, dengan motif bintang emas di tengah)
        float chipCenterY = cy + 115.0f;
        Color chipCol = (i == 2) ? Color(0.78f, 0.22f, 0.18f) : (i == 1 ? Color(0.92f, 0.68f, 0.15f) : Color(0.20f, 0.45f, 0.85f));
        Renderer::drawCasinoChip(cx + colW * 0.5f, chipCenterY, 32.0f, chipCol, Color(1, 1, 1, 0.9f), 0.0f);

        // 4. Kotak Target Skor & Reward
        float infoBoxY = cy + 158.0f;
        float infoBoxH = (i == 2) ? 140.0f : 85.0f;
        Renderer::drawRect(cx + 8.0f, infoBoxY, colW - 16.0f, infoBoxH, Color(0.09f, 0.12f, 0.14f, 0.9f));
        Renderer::drawRectOutline(cx + 8.0f, infoBoxY, colW - 16.0f, infoBoxH, 1.0f, Color(0.22f, 0.28f, 0.32f));

        if (i == 2) {
            // Deskripsi boss debuff (Jelas & terbaca dengan scale 1.0f, 2-3 baris rapi)
            Renderer::drawWrappedCenteredText(b.getDebuffDescription(), cx + colW * 0.5f, infoBoxY + 8.0f, colW - 20.0f, 1.0f, 3.0f, Theme::TEXT_WHITE);
            Renderer::drawRect(cx + 14.0f, infoBoxY + 54.0f, colW - 28.0f, 1.0f, Color(0.3f, 0.35f, 0.4f, 0.4f));

            Renderer::drawCenteredText("Score at least", cx + colW * 0.5f, infoBoxY + 62.0f, 1.05f, Color(0.85f, 0.88f, 0.90f));
            string tStr = to_string(b.getTargetScore());
            Renderer::drawCenteredText(tStr, cx + colW * 0.5f, infoBoxY + 82.0f, 2.0f, Theme::MULT_RED);
            Renderer::drawCenteredText("Reward: $$$$$+", cx + colW * 0.5f, infoBoxY + 112.0f, 1.15f, Theme::GOLD);
        } else {
            Renderer::drawCenteredText("Score at least", cx + colW * 0.5f, infoBoxY + 8.0f, 1.05f, Color(0.85f, 0.88f, 0.90f));
            string tStr = to_string(b.getTargetScore());
            Renderer::drawCenteredText(tStr, cx + colW * 0.5f, infoBoxY + 28.0f, 2.0f, Theme::MULT_RED);
            string rStr = (i == 0) ? "Reward: $$$+" : "Reward: $$$$+";
            Renderer::drawCenteredText(rStr, cx + colW * 0.5f, infoBoxY + 58.0f, 1.15f, Theme::GOLD);

            // 5. Teks "or"
            Renderer::drawCenteredText("or", cx + colW * 0.5f, cy + 258.0f, 1.35f, Theme::TEXT_WHITE);

            // 6. Tombol Skip Reward Tag Capsule
            TagReward tag = b.getTagReward();
            float tagBoxW = colW - 16.0f;
            float tagBoxH = 42.0f;
            float tagBoxX = cx + 8.0f;
            float tagBoxY = cy + 280.0f;

            // Kotak icon di sebelah kiri
            float iconBoxW = 42.0f;
            bool hoverIconOnly = isPointInRect(mx, my, tagBoxX, tagBoxY, iconBoxW, tagBoxH);
            bool hoverBtnOnly = isPointInRect(mx, my, tagBoxX + iconBoxW + 3.0f, tagBoxY, tagBoxW - iconBoxW - 3.0f, tagBoxH);

            // Tooltip HANYA terpicu ketika cursor mengarah ke IKONNYA (bukan ke tombol skip)
            if (hoverIconOnly) {
                tooltipTitle = tag.name;
                tooltipDesc = tag.description;
                tooltipAnchorX = tagBoxX + tagBoxW * 0.5f;
                tooltipAnchorY = tagBoxY + tagBoxH;
            }

            Renderer::drawRect(tagBoxX, tagBoxY, iconBoxW, tagBoxH, Color(0.95f, 0.95f, 0.95f));
            Renderer::drawRectOutline(tagBoxX, tagBoxY, iconBoxW, tagBoxH, 1.5f, hoverIconOnly ? Theme::GOLD : Color(0.55f, 0.60f, 0.65f));
            Renderer::drawTagIcon(tag.type, tagBoxX + iconBoxW * 0.5f, tagBoxY + tagBoxH * 0.5f, 26.0f);

            // Bagian tombol biru di sebelah kanan ikon: Teks "Skip" & "Reward" dalam 2 baris rapi
            float btnW = tagBoxW - iconBoxW - 3.0f;
            float btnX = tagBoxX + iconBoxW + 3.0f;
            Color btnBg = hoverBtnOnly ? Color(0.22f, 0.55f, 0.82f) : Color(0.12f, 0.40f, 0.65f);
            Renderer::drawRect(btnX, tagBoxY, btnW, tagBoxH, btnBg);
            Renderer::drawRectOutline(btnX, tagBoxY, btnW, tagBoxH, 1.5f, Color(0.08f, 0.25f, 0.45f));
            Renderer::drawCenteredText("Skip", btnX + btnW * 0.5f, tagBoxY + 6.0f, 1.15f, Theme::TEXT_WHITE);
            Renderer::drawCenteredText("Reward", btnX + btnW * 0.5f, tagBoxY + 22.0f, 1.15f, Theme::TEXT_WHITE);
        }
    }

    // Panel Tooltip Detail Tag (Muncul TEPAT DI BAWAH tulisan Skip Reward, tidak menempel pada mouse)
    if (!tooltipTitle.empty()) {
        float ttW = 280.0f;
        float ttH = 68.0f;
        float ttX = tooltipAnchorX - ttW * 0.5f;
        float ttY = tooltipAnchorY + 6.0f;

        // Cegah keluar modal jika terlalu ke pinggir
        if (ttX < modalX + 10.0f) ttX = modalX + 10.0f;
        if (ttX + ttW > modalX + modalW - 10.0f) ttX = modalX + modalW - 10.0f - ttW;

        // Bayangan tooltip
        Renderer::drawRect(ttX + 4.0f, ttY + 4.0f, ttW, ttH, Color(0.0f, 0.0f, 0.0f, 0.55f));
        // Kotak putih bersih berbingkai biru khas Balatro
        Renderer::drawRect(ttX, ttY, ttW, ttH, Color(0.98f, 0.98f, 0.98f));
        Renderer::drawRectOutline(ttX, ttY, ttW, ttH, 2.5f, Color(0.15f, 0.45f, 0.75f));
        Renderer::drawCenteredText(tooltipTitle, ttX + ttW * 0.5f, ttY + 8.0f, 1.35f, Color(0.12f, 0.35f, 0.65f));
        Renderer::drawWrappedCenteredText(tooltipDesc, ttX + ttW * 0.5f, ttY + 30.0f, ttW - 16.0f, 1.05f, 3.0f, Color(0.18f, 0.22f, 0.25f));
    }
}

void GameWindow::renderRunInfoVouchersTab(float modalX, float modalY, float modalW, float modalH, float mx, float my) {
    (void)modalH;
    const set<VoucherType>& redeemed = engine.getRedeemedVouchers();

    if (redeemed.empty()) {
        // Tampilan ketika belum ada voucher yang ditebus (persis referensi gambar)
        Renderer::drawCenteredText("No vouchers redeemed this run", modalX + modalW * 0.5f, modalY + modalH * 0.5f - 15.0f, 1.7f, Theme::TEXT_WHITE);
        return;
    }

    // Tampilan ketika ada voucher yang ditebus
    Renderer::drawCenteredText("Vouchers redeemed this run", modalX + modalW * 0.5f, modalY + 80.0f, 1.6f, Theme::TEXT_WHITE);

    // Baki gelap (Dark Tray Container)
    float trayX = modalX + 25.0f;
    float trayY = modalY + 110.0f;
    float trayW = modalW - 50.0f; // 490.0f
    float trayH = 390.0f;

    Renderer::drawRect(trayX, trayY, trayW, trayH, Color(0.11f, 0.14f, 0.16f, 0.95f));
    Renderer::drawRectOutline(trayX, trayY, trayW, trayH, 2.0f, Color(0.24f, 0.30f, 0.35f));

    // Daftar pasangan voucher Balatro (Tier 1 & Tier 2)
    struct VoucherPairInfo {
        VoucherType t1;
        VoucherType t2;
    };
    const VoucherPairInfo allPairs[8] = {
        { VoucherType::OVERSTOCK,      VoucherType::OVERSTOCK_PLUS },
        { VoucherType::CLEARANCE_SALE, VoucherType::LIQUIDATION },
        { VoucherType::REROLL_SURPLUS, VoucherType::REROLL_GLUT },
        { VoucherType::GRABBER,        VoucherType::NACHO_TONG },
        { VoucherType::WASTEFUL,       VoucherType::RECYCLOMANCY },
        { VoucherType::PAINT_BRUSH,    VoucherType::PALETTE },
        { VoucherType::BLANK,          VoucherType::ANTIMATTER },
        { VoucherType::DIRECTORS_CUT,  VoucherType::RETCON }
    };

    vector<VoucherPairInfo> activePairs;
    for (const auto& p : allPairs) {
        if (redeemed.find(p.t1) != redeemed.end() || redeemed.find(p.t2) != redeemed.end()) {
            activePairs.push_back(p);
        }
    }

    // Ukuran kartu voucher di dalam baki Run Info
    float cardW = 76.0f;
    float cardH = 114.0f;
    float overlapOffsetX = 18.0f;
    float overlapOffsetY = 8.0f;
    float slotW = cardW + overlapOffsetX; // ~94.0f
    float colGap = 20.0f;

    int totalPairs = static_cast<int>(activePairs.size());
    int maxCols = 4;
    int numRows = (totalPairs > maxCols) ? 2 : 1;

    // Untuk tooltip voucher yang di-hover
    Voucher hoveredVoucher;
    bool hasHoveredVoucher = false;
    float hoverTipX = 0.0f;
    float hoverTipY = 0.0f;

    for (int r = 0; r < numRows; ++r) {
        int startIdx = r * maxCols;
        int endIdx = min(totalPairs, (r + 1) * maxCols);
        int countInRow = endIdx - startIdx;
        if (countInRow <= 0) break;

        float rowWidth = countInRow * slotW + (countInRow - 1) * colGap;
        float rowStartX = trayX + (trayW - rowWidth) * 0.5f;
        float rowStartY = (numRows == 1) 
            ? (trayY + (trayH - (cardH + overlapOffsetY)) * 0.5f)
            : (trayY + 45.0f + r * (cardH + overlapOffsetY + 45.0f));

        for (int i = 0; i < countInRow; ++i) {
            int pairIdx = startIdx + i;
            const auto& p = activePairs[pairIdx];

            float slotX = rowStartX + i * (slotW + colGap);
            float slotY = rowStartY;

            bool hasT1 = (redeemed.find(p.t1) != redeemed.end());
            bool hasT2 = (redeemed.find(p.t2) != redeemed.end());

            if (hasT1 && hasT2) {
                // Tier 1 di belakang (kiri-atas)
                float t1X = slotX;
                float t1Y = slotY;
                // Tier 2 di depan (kanan-bawah tumpang tindih ala Balatro)
                float t2X = slotX + overlapOffsetX;
                float t2Y = slotY + overlapOffsetY;

                Voucher v1 = VoucherDatabase::getVoucher(p.t1);
                Voucher v2 = VoucherDatabase::getVoucher(p.t2);

                // Hover check: cek T2 dulu karena ada di atas layer
                bool hoverT2 = isPointInRect(mx, my, t2X, t2Y, cardW, cardH);
                bool hoverT1 = !hoverT2 && isPointInRect(mx, my, t1X, t1Y, cardW, cardH);

                // Gambar T1
                Renderer::drawVoucherCard(v1, t1X, t1Y, cardW, cardH, hoverT1, 1.0f, false);
                // Gambar T2
                Renderer::drawVoucherCard(v2, t2X, t2Y, cardW, cardH, hoverT2, 1.0f, false);

                if (hoverT2) {
                    hoveredVoucher = v2;
                    hasHoveredVoucher = true;
                    hoverTipX = t2X;
                    hoverTipY = t2Y;
                } else if (hoverT1) {
                    hoveredVoucher = v1;
                    hasHoveredVoucher = true;
                    hoverTipX = t1X;
                    hoverTipY = t1Y;
                }
            } else if (hasT1) {
                // Hanya Tier 1
                float t1X = slotX + overlapOffsetX * 0.5f;
                float t1Y = slotY + overlapOffsetY * 0.5f;
                Voucher v1 = VoucherDatabase::getVoucher(p.t1);
                bool hoverT1 = isPointInRect(mx, my, t1X, t1Y, cardW, cardH);

                Renderer::drawVoucherCard(v1, t1X, t1Y, cardW, cardH, hoverT1, 1.0f, false);

                if (hoverT1) {
                    hoveredVoucher = v1;
                    hasHoveredVoucher = true;
                    hoverTipX = t1X;
                    hoverTipY = t1Y;
                }
            } else if (hasT2) {
                // Hanya Tier 2 (jika terjadi)
                float t2X = slotX + overlapOffsetX * 0.5f;
                float t2Y = slotY + overlapOffsetY * 0.5f;
                Voucher v2 = VoucherDatabase::getVoucher(p.t2);
                bool hoverT2 = isPointInRect(mx, my, t2X, t2Y, cardW, cardH);

                Renderer::drawVoucherCard(v2, t2X, t2Y, cardW, cardH, hoverT2, 1.0f, false);

                if (hoverT2) {
                    hoveredVoucher = v2;
                    hasHoveredVoucher = true;
                    hoverTipX = t2X;
                    hoverTipY = t2Y;
                }
            }
        }
    }

    // Render tooltip jika ada kartu voucher yang di-hover
    if (hasHoveredVoucher && hoveredVoucher.type != VoucherType::NONE) {
        renderVoucherTooltip(hoveredVoucher, hoverTipX, hoverTipY);
    }
}

void GameWindow::renderPokerHandDetailTooltip(HandType type, float x, float y) {
    float ttW = 460.0f;
    float ttH = 135.0f;

    // Bayangan tooltip
    Renderer::drawRect(x + 5.0f, y + 6.0f, ttW, ttH, Color(0.0f, 0.0f, 0.0f, 0.45f));

    // Kotak putih berbingkai merah khas Balatro
    Renderer::drawRect(x, y, ttW, ttH, Color(0.98f, 0.98f, 0.98f));
    Renderer::drawRectOutline(x, y, ttW, ttH, 3.5f, Color(0.88f, 0.32f, 0.28f));

    string desc;
    vector<Card> sampleCards;

    switch (type) {
        case HandType::STRAIGHT_FLUSH:
            desc = "5 cards in a row with the same suit";
            sampleCards = {
                Card(Suit::HEARTS, Rank::JACK),
                Card(Suit::HEARTS, Rank::TEN),
                Card(Suit::HEARTS, Rank::NINE),
                Card(Suit::HEARTS, Rank::EIGHT),
                Card(Suit::HEARTS, Rank::SEVEN)
            };
            break;
        case HandType::FOUR_OF_A_KIND:
            desc = "4 cards with the same rank";
            sampleCards = {
                Card(Suit::SPADES, Rank::ACE),
                Card(Suit::HEARTS, Rank::ACE),
                Card(Suit::DIAMONDS, Rank::ACE),
                Card(Suit::CLUBS, Rank::ACE),
                Card(Suit::SPADES, Rank::KING)
            };
            break;
        case HandType::FULL_HOUSE:
            desc = "3 cards with the same rank and a matching pair";
            sampleCards = {
                Card(Suit::HEARTS, Rank::KING),
                Card(Suit::DIAMONDS, Rank::KING),
                Card(Suit::SPADES, Rank::KING),
                Card(Suit::CLUBS, Rank::EIGHT),
                Card(Suit::DIAMONDS, Rank::EIGHT)
            };
            break;
        case HandType::FLUSH:
            desc = "5 cards with the same suit";
            sampleCards = {
                Card(Suit::SPADES, Rank::ACE),
                Card(Suit::SPADES, Rank::JACK),
                Card(Suit::SPADES, Rank::EIGHT),
                Card(Suit::SPADES, Rank::SIX),
                Card(Suit::SPADES, Rank::THREE)
            };
            break;
        case HandType::STRAIGHT:
            desc = "5 cards in a row (consecutive ranks)";
            // Contoh persis gambar referensi Balatro: J Wajik, 10 Keriting, 9 Keriting, 8 Sekop, 7 Hati
            sampleCards = {
                Card(Suit::DIAMONDS, Rank::JACK),
                Card(Suit::CLUBS, Rank::TEN),
                Card(Suit::CLUBS, Rank::NINE),
                Card(Suit::SPADES, Rank::EIGHT),
                Card(Suit::HEARTS, Rank::SEVEN)
            };
            break;
        case HandType::THREE_OF_A_KIND:
            desc = "3 cards with the same rank";
            sampleCards = {
                Card(Suit::HEARTS, Rank::QUEEN),
                Card(Suit::DIAMONDS, Rank::QUEEN),
                Card(Suit::CLUBS, Rank::QUEEN),
                Card(Suit::SPADES, Rank::NINE),
                Card(Suit::HEARTS, Rank::FOUR)
            };
            break;
        case HandType::TWO_PAIR:
            desc = "2 pairs of cards with different ranks";
            sampleCards = {
                Card(Suit::SPADES, Rank::JACK),
                Card(Suit::HEARTS, Rank::JACK),
                Card(Suit::DIAMONDS, Rank::SEVEN),
                Card(Suit::CLUBS, Rank::SEVEN),
                Card(Suit::HEARTS, Rank::TWO)
            };
            break;
        case HandType::PAIR:
            desc = "2 cards with the same rank";
            sampleCards = {
                Card(Suit::DIAMONDS, Rank::TEN),
                Card(Suit::SPADES, Rank::TEN),
                Card(Suit::HEARTS, Rank::KING),
                Card(Suit::CLUBS, Rank::EIGHT),
                Card(Suit::SPADES, Rank::FOUR)
            };
            break;
        case HandType::HIGH_CARD:
        default:
            desc = "The highest card in your hand";
            sampleCards = {
                Card(Suit::SPADES, Rank::ACE),
                Card(Suit::HEARTS, Rank::KING),
                Card(Suit::CLUBS, Rank::NINE),
                Card(Suit::DIAMONDS, Rank::SIX),
                Card(Suit::SPADES, Rank::THREE)
            };
            break;
    }

    // Auto-fit skala teks agar tidak pernah meluap/keluar dari kotak
    float textScale = 1.15f;
    float maxTextW = ttW - 36.0f;
    float actualW = Renderer::getTextWidth(desc, textScale);
    if (actualW > maxTextW) {
        textScale *= (maxTextW / actualW);
    }

    Renderer::drawCenteredText(desc, x + ttW * 0.5f, y + 13.0f, textScale, Color(0.20f, 0.32f, 0.34f));

    // Baris 5 Kartu Mini dengan lengkungan alami (organic fan)
    float scale = 0.58f;
    float miniW = 88.0f * scale; // 51.0f
    float miniH = 130.0f * scale; // 75.4f
    float miniSpacing = 10.0f;
    float totalCardsW = 5 * miniW + 4 * miniSpacing;
    float startCardsX = x + (ttW - totalCardsW) * 0.5f;
    float cardsCenterY = y + 36.0f + miniH * 0.5f;

    float fanAngles[5] = { -3.5f, -1.8f, 0.0f, 1.8f, 3.5f };

    for (size_t i = 0; i < sampleCards.size(); ++i) {
        float cx = startCardsX + i * (miniW + miniSpacing) + miniW * 0.5f;

        glPushMatrix();
        glTranslatef(cx, cardsCenterY, 0.0f);
        glRotatef(fanAngles[i], 0.0f, 0.0f, 1.0f);
        glScalef(scale, scale, 1.0f);
        Renderer::drawCard(sampleCards[i], -44.0f, -65.0f, 88.0f, 130.0f, false, 0.0f);
        glPopMatrix();
    }
}

void GameWindow::drawGearIcon(float cx, float cy, float radius, float angle, Color col) {
    glPushMatrix();
    glTranslatef(cx, cy, 0.0f);
    glRotatef(angle, 0.0f, 0.0f, 1.0f);

    float rHub = radius * 0.70f;
    float rOuter = radius * 1.18f;
    float rHole = radius * 0.32f;
    const int numTeeth = 8;
    float dAngle = 6.2831853f / numTeeth;
    float halfTooth = 0.16f; // ~9 degrees half-width

    // 1. Drop Shadow Lembut
    glColor4f(0.0f, 0.0f, 0.0f, 0.35f);
    glBegin(GL_TRIANGLES);
    for (int i = 0; i < numTeeth; ++i) {
        float a = i * dAngle;
        float ax1 = cos(a - halfTooth * 0.75f) * rOuter + 1.5f;
        float ay1 = sin(a - halfTooth * 0.75f) * rOuter + 2.0f;
        float ax2 = cos(a + halfTooth * 0.75f) * rOuter + 1.5f;
        float ay2 = sin(a + halfTooth * 0.75f) * rOuter + 2.0f;
        float bx1 = cos(a - halfTooth * 1.15f) * rHub + 1.5f;
        float by1 = sin(a - halfTooth * 1.15f) * rHub + 2.0f;
        float bx2 = cos(a + halfTooth * 1.15f) * rHub + 1.5f;
        float by2 = sin(a + halfTooth * 1.15f) * rHub + 2.0f;

        glVertex2f(bx1, by1);
        glVertex2f(ax1, ay1);
        glVertex2f(ax2, ay2);

        glVertex2f(bx1, by1);
        glVertex2f(ax2, ay2);
        glVertex2f(bx2, by2);
    }
    glEnd();

    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(1.5f, 2.0f);
    for (int s = 0; s <= 24; ++s) {
        float a = s * (6.2831853f / 24.0f);
        glVertex2f(1.5f + cos(a) * rHub, 2.0f + sin(a) * rHub);
    }
    glEnd();

    // 2. Gigi Gerigi (8 Cogs / Teeth)
    glColor4f(col.r, col.g, col.b, col.a);
    glBegin(GL_TRIANGLES);
    for (int i = 0; i < numTeeth; ++i) {
        float a = i * dAngle;
        float ax1 = cos(a - halfTooth * 0.75f) * rOuter;
        float ay1 = sin(a - halfTooth * 0.75f) * rOuter;
        float ax2 = cos(a + halfTooth * 0.75f) * rOuter;
        float ay2 = sin(a + halfTooth * 0.75f) * rOuter;
        float bx1 = cos(a - halfTooth * 1.15f) * rHub;
        float by1 = sin(a - halfTooth * 1.15f) * rHub;
        float bx2 = cos(a + halfTooth * 1.15f) * rHub;
        float by2 = sin(a + halfTooth * 1.15f) * rHub;

        glVertex2f(bx1, by1);
        glVertex2f(ax1, ay1);
        glVertex2f(ax2, ay2);

        glVertex2f(bx1, by1);
        glVertex2f(ax2, ay2);
        glVertex2f(bx2, by2);
    }
    glEnd();

    // 3. Badan Roda Gigi (Wheel Disc)
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(0.0f, 0.0f);
    for (int s = 0; s <= 24; ++s) {
        float a = s * (6.2831853f / 24.0f);
        glVertex2f(cos(a) * rHub, sin(a) * rHub);
    }
    glEnd();

    // 4. Lingkaran Aksen Emas/Cerah di dalam hub
    glColor4f(min(1.0f, col.r * 1.30f), min(1.0f, col.g * 1.30f), min(1.0f, col.b * 1.30f), col.a);
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
    for (int s = 0; s < 20; ++s) {
        float a = s * (6.2831853f / 20.0f);
        glVertex2f(cos(a) * (rHub * 0.65f), sin(a) * (rHub * 0.65f));
    }
    glEnd();

    // 5. Lubang Tengah Gerigi (Center Hole)
    glColor4f(0.08f, 0.14f, 0.12f, 1.0f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(0.0f, 0.0f);
    for (int s = 0; s <= 20; ++s) {
        float a = s * (6.2831853f / 20.0f);
        glVertex2f(cos(a) * rHole, sin(a) * rHole);
    }
    glEnd();

    glPopMatrix();
}

void GameWindow::renderSettingsModal() {
    float mx = static_cast<float>(mouseX);
    float my = static_cast<float>(mouseY);

    float modalW = 440.0f;
    float modalH = 310.0f;
    float modalX = (static_cast<float>(windowWidth) - modalW) * 0.5f;
    float modalY = (static_cast<float>(windowHeight) - modalH) * 0.5f;

    // 1. Overlay Gelap Layar Belakang
    Renderer::drawRect(0.0f, 0.0f, static_cast<float>(windowWidth), static_cast<float>(windowHeight), Color(0.0f, 0.0f, 0.0f, 0.68f));

    // 2. Panel Background & Bingkai CRT Balatro
    Renderer::drawRect(modalX, modalY, modalW, modalH, Theme::PANEL_BG);
    Renderer::drawRectOutline(modalX, modalY, modalW, modalH, 2.0f, Theme::GOLD);

    // Corner Filigree Brackets CRT [ ]
    float bLen = 14.0f;
    float bThick = 2.5f;
    Color bracketCol = Theme::GOLD;
    Renderer::drawRect(modalX + 6.0f, modalY + 6.0f, bLen, bThick, bracketCol);
    Renderer::drawRect(modalX + 6.0f, modalY + 6.0f, bThick, bLen, bracketCol);
    Renderer::drawRect(modalX + modalW - 6.0f - bLen, modalY + 6.0f, bLen, bThick, bracketCol);
    Renderer::drawRect(modalX + modalW - 6.0f - bThick, modalY + 6.0f, bThick, bLen, bracketCol);
    Renderer::drawRect(modalX + 6.0f, modalY + modalH - 6.0f - bThick, bLen, bThick, bracketCol);
    Renderer::drawRect(modalX + 6.0f, modalY + modalH - 6.0f - bLen, bThick, bLen, bracketCol);
    Renderer::drawRect(modalX + modalW - 6.0f - bLen, modalY + modalH - 6.0f - bThick, bLen, bThick, bracketCol);
    Renderer::drawRect(modalX + modalW - 6.0f - bThick, modalY + modalH - 6.0f - bLen, bThick, bLen, bracketCol);

    // 3. Judul Modal "SETTINGS"
    Renderer::drawCenteredText("SETTINGS", modalX + modalW * 0.5f, modalY + 18.0f, 1.9f, Theme::GOLD);
    drawGearIcon(modalX + 60.0f, modalY + 28.0f, 11.0f, gearRotationAngle, Theme::GOLD);
    drawGearIcon(modalX + modalW - 60.0f, modalY + 28.0f, 11.0f, -gearRotationAngle, Theme::GOLD);

    // Divider Line
    Renderer::drawRect(modalX + 25.0f, modalY + 52.0f, modalW - 50.0f, 1.5f, Color(0.25f, 0.50f, 0.45f, 0.70f));

    // 4. SLIDER BGM VOLUME
    float bgmVol = AudioManager::getInstance().getBGMVolume();
    float trackX = modalX + 35.0f;
    float trackW = modalW - 70.0f;
    float trackH = 14.0f;

    Renderer::drawText("MUSIC (BGM)", trackX, modalY + 75.0f, 1.35f, Theme::TEXT_WHITE);
    string bgmPct = to_string(static_cast<int>(round(bgmVol * 100.0f))) + "%";
    float bgmPctW = Renderer::getTextWidth(bgmPct, 1.35f);
    Renderer::drawText(bgmPct, trackX + trackW - bgmPctW, modalY + 75.0f, 1.35f, Theme::CHIPS_BLUE);

    float bgmTrackY = modalY + 103.0f;
    // Track Groove
    Renderer::drawRect(trackX, bgmTrackY, trackW, trackH, Color(0.08f, 0.14f, 0.12f, 1.0f));
    Renderer::drawRectOutline(trackX, bgmTrackY, trackW, trackH, 1.2f, Color(0.25f, 0.45f, 0.40f, 0.85f));

    // Fill
    float bgmFillW = trackW * bgmVol;
    if (bgmFillW > 0.0f) {
        Renderer::drawRect(trackX, bgmTrackY, bgmFillW, trackH, Theme::CHIPS_BLUE);
    }

    // Knob Handle
    float bgmKnobX = trackX + bgmFillW;
    float bgmKnobY = bgmTrackY + trackH * 0.5f;
    float knobW = 14.0f;
    float knobH = 24.0f;
    Color bgmKnobCol = isDraggingBgmSlider ? Theme::GOLD : Theme::TEXT_WHITE;
    Renderer::drawRect(bgmKnobX - knobW * 0.5f + 1.5f, bgmKnobY - knobH * 0.5f + 2.0f, knobW, knobH, Color(0.0f, 0.0f, 0.0f, 0.4f)); // shadow
    Renderer::drawRect(bgmKnobX - knobW * 0.5f, bgmKnobY - knobH * 0.5f, knobW, knobH, bgmKnobCol);
    Renderer::drawRectOutline(bgmKnobX - knobW * 0.5f, bgmKnobY - knobH * 0.5f, knobW, knobH, 1.5f, Color(0.12f, 0.20f, 0.18f, 0.9f));
    // Knob grip lines
    Renderer::drawRect(bgmKnobX - 2.0f, bgmKnobY - 6.0f, 1.0f, 12.0f, Color(0.4f, 0.4f, 0.4f, 0.8f));
    Renderer::drawRect(bgmKnobX + 1.0f, bgmKnobY - 6.0f, 1.0f, 12.0f, Color(0.4f, 0.4f, 0.4f, 0.8f));

    // 5. SLIDER SFX VOLUME
    float sfxVol = AudioManager::getInstance().getSFXVolume();
    float sfxTrackY = modalY + 173.0f;

    Renderer::drawText("SOUND (SFX)", trackX, modalY + 145.0f, 1.35f, Theme::TEXT_WHITE);
    string sfxPct = to_string(static_cast<int>(round(sfxVol * 100.0f))) + "%";
    float sfxPctW = Renderer::getTextWidth(sfxPct, 1.35f);
    Renderer::drawText(sfxPct, trackX + trackW - sfxPctW, modalY + 145.0f, 1.35f, Theme::MULT_RED);

    // Track Groove
    Renderer::drawRect(trackX, sfxTrackY, trackW, trackH, Color(0.08f, 0.14f, 0.12f, 1.0f));
    Renderer::drawRectOutline(trackX, sfxTrackY, trackW, trackH, 1.2f, Color(0.25f, 0.45f, 0.40f, 0.85f));

    // Fill
    float sfxFillW = trackW * sfxVol;
    if (sfxFillW > 0.0f) {
        Renderer::drawRect(trackX, sfxTrackY, sfxFillW, trackH, Theme::MULT_RED);
    }

    // Knob Handle
    float sfxKnobX = trackX + sfxFillW;
    float sfxKnobY = sfxTrackY + trackH * 0.5f;
    Color sfxKnobCol = isDraggingSfxSlider ? Theme::GOLD : Theme::TEXT_WHITE;
    Renderer::drawRect(sfxKnobX - knobW * 0.5f + 1.5f, sfxKnobY - knobH * 0.5f + 2.0f, knobW, knobH, Color(0.0f, 0.0f, 0.0f, 0.4f)); // shadow
    Renderer::drawRect(sfxKnobX - knobW * 0.5f, sfxKnobY - knobH * 0.5f, knobW, knobH, sfxKnobCol);
    Renderer::drawRectOutline(sfxKnobX - knobW * 0.5f, sfxKnobY - knobH * 0.5f, knobW, knobH, 1.5f, Color(0.12f, 0.20f, 0.18f, 0.9f));
    // Knob grip lines
    Renderer::drawRect(sfxKnobX - 2.0f, sfxKnobY - 6.0f, 1.0f, 12.0f, Color(0.4f, 0.4f, 0.4f, 0.8f));
    Renderer::drawRect(sfxKnobX + 1.0f, sfxKnobY - 6.0f, 1.0f, 12.0f, Color(0.4f, 0.4f, 0.4f, 0.8f));

    // 6. TOMBOL CLOSE
    float closeW = 160.0f;
    float closeH = 40.0f;
    float closeX = modalX + (modalW - closeW) * 0.5f;
    float closeY = modalY + modalH - 58.0f;
    bool hoverClose = isPointInRect(mx, my, closeX, closeY, closeW, closeH);
    Renderer::drawButton("CLOSE", closeX, closeY, closeW, closeH, Color(0.18f, 0.35f, 0.30f), Theme::TEXT_WHITE, hoverClose, true);
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

        // Update real-time slider dragging jika sedang aktif
        if (showSettingsModal) {
            float modalW = 440.0f;
            float modalX = (static_cast<float>(windowWidth) - modalW) * 0.5f;
            float trackX = modalX + 35.0f;
            float trackW = modalW - 70.0f;

            if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
                if (isDraggingBgmSlider) {
                    float newVol = (static_cast<float>(mouseX) - trackX) / trackW;
                    if (newVol < 0.0f) newVol = 0.0f;
                    if (newVol > 1.0f) newVol = 1.0f;
                    AudioManager::getInstance().setBGMVolume(newVol);
                } else if (isDraggingSfxSlider) {
                    float newVol = (static_cast<float>(mouseX) - trackX) / trackW;
                    if (newVol < 0.0f) newVol = 0.0f;
                    if (newVol > 1.0f) newVol = 1.0f;
                    AudioManager::getInstance().setSFXVolume(newVol);
                }
            } else {
                isDraggingBgmSlider = false;
                isDraggingSfxSlider = false;
            }
        }

        renderUI(dt);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}

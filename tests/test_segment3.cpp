#include <iostream>
#include <cassert>
#include <memory>
#include <vector>

// Header & Implementasi self-contained agar kompatibel dengan tombol F5 di VS Code
#include "../include/core/Card.h"
#include "../include/core/Deck.h"
#include "../include/core/DeckFactory.h"
#include "../include/core/Exceptions.h"
#include "../include/core/PokerHand.h"
#include "../include/core/PokerEvaluator.h"
#include "../include/core/ScoringEngine.h"
#include "../include/core/Blind.h"
#include "../include/core/GameState.h"
#include "../include/core/GameEngine.h"

#include "../src/core/Card.cpp"
#include "../src/core/Deck.cpp"
#include "../src/core/DeckFactory.cpp"
#include "../src/core/PokerEvaluator.cpp"
#include "../src/core/ScoringEngine.cpp"
#include "../src/core/Blind.cpp"
#include "../src/core/GameState.cpp"
#include "../src/core/GameEngine.cpp"

using namespace std;

void printHeader(const string& title) {
    cout << "\n========================================\n";
    cout << " [TEST] " << title << "\n";
    cout << "========================================\n";
}

void testBlindAndAnteScaling() {
    printHeader("Test 1: Sistem Blind & Ante Progression");
    BlindManager manager;

    assert(manager.getCurrentAnte() == 1);
    assert(manager.getCurrentBlindIndex() == 0);
    Blind sb = manager.getCurrentBlind();
    assert(sb.getType() == BlindType::SMALL_BLIND);
    assert(sb.getTargetScore() == 300);
    assert(sb.getRewardMoney() == 3);
    cout << "  - Ante 1 Small Blind: Target " << sb.getTargetScore() << " | Reward $" << sb.getRewardMoney() << " OK\n";

    manager.advanceBlind();
    Blind bb = manager.getCurrentBlind();
    assert(bb.getType() == BlindType::BIG_BLIND);
    assert(bb.getTargetScore() == 450);
    assert(bb.getRewardMoney() == 4);
    cout << "  - Ante 1 Big Blind: Target " << bb.getTargetScore() << " | Reward $" << bb.getRewardMoney() << " OK\n";

    manager.advanceBlind();
    Blind boss = manager.getCurrentBlind();
    assert(boss.getType() == BlindType::BOSS_BLIND);
    assert(boss.getTargetScore() == 600);
    assert(boss.getRewardMoney() == 5);
    cout << "  - Ante 1 Boss Blind: Target " << boss.getTargetScore() << " | Debuff: " << boss.getDebuffDescription() << " OK\n";

    manager.advanceBlind();
    assert(manager.getCurrentAnte() == 2);
    assert(manager.getCurrentBlindIndex() == 0);
    Blind ante2Sb = manager.getCurrentBlind();
    assert(ante2Sb.getTargetScore() == 800);
    cout << "  - Naik ke Ante 2 Small Blind: Target berskala naik ke " << ante2Sb.getTargetScore() << " OK\n";

    cout << ">> [PASS] Test 1 Selesai!\n";
}

void testStatePatternAndPlayerActions() {
    printHeader("Test 2: State Pattern (Menu -> Blind Select -> In-Game -> Discard/Play)");
    GameEngine engine;

    // Awal instansiasi harus berada di MAIN_MENU
    assert(engine.getCurrentStateName() == "MAIN_MENU");
    cout << "  - State awal engine: MAIN_MENU OK\n";

    // Mulai run baru -> masuk ke BLIND_SELECT
    engine.startNewRun();
    assert(engine.getCurrentStateName() == "BLIND_SELECT");
    assert(engine.isBlindCurrent(0)); // Small Blind aktif
    assert(engine.isBlindLocked(1));  // Big Blind terkunci
    assert(engine.isBlindLocked(2));  // Boss Blind terkunci
    cout << "  - Start New Run: Berpindah ke BLIND_SELECT (Small Blind aktif, yang lain terkunci) OK\n";

    // Pilih blind -> masuk ke ronde (SELECTING_CARDS)
    engine.selectBlind();
    assert(engine.getCurrentStateName() == "SELECTING_CARDS");
    assert(engine.getHand().size() == 8);
    assert(engine.getHandsRemaining() == 4);
    assert(engine.getDiscardsRemaining() == 3);
    assert(engine.getCurrentScore() == 0);
    cout << "  - Select Blind: Memulai ronde, Hand Size = 8, Hands = 4, Discards = 3 OK\n";

    // 1. Uji seleksi kartu (Maksimal 5 kartu)
    engine.toggleSelectCard(0);
    engine.toggleSelectCard(1);
    engine.toggleSelectCard(2);
    assert(engine.getSelectedCount() == 3);

    engine.toggleSelectCard(3);
    engine.toggleSelectCard(4);
    assert(engine.getSelectedCount() == 5);
    
    bool overlimitCaught = false;
    try {
        engine.toggleSelectCard(5);
    } catch (const InvalidCardCountException& e) {
        overlimitCaught = true;
    }
    assert(overlimitCaught);
    cout << "  - Guard batas 5 kartu bekerja sempurna!\n";

    // Unselect kartu 3 & 4
    engine.toggleSelectCard(3);
    engine.toggleSelectCard(4);

    // 2. Uji Discard
    engine.discard();
    assert(engine.getDiscardsRemaining() == 2);
    assert(engine.getHand().size() == 8);
    assert(engine.getSelectedCount() == 0);
    cout << "  - Discard berhasil: Sisa Discard = 2, Hand penuh kembali (8 kartu) OK\n";

    // 3. Uji Play Hand
    engine.toggleSelectCard(0);
    engine.toggleSelectCard(1);
    int initialScore = engine.getCurrentScore();
    engine.playHand();
    
    assert(engine.getHandsRemaining() == 3);
    assert(engine.getCurrentScore() > initialScore);
    assert(engine.getHand().size() == 8);
    cout << "  - Play Hand berhasil: Hands tersisa = 3, Skor bertambah jadi " << engine.getCurrentScore() << " OK\n";

    cout << ">> [PASS] Test 2 Selesai!\n";
}

void testWinAndCashOutTransitions() {
    printHeader("Test 3: Transisi State Menang (RoundWon) & Cash Out ke Blind Select");
    GameEngine engine;
    engine.startNewRun();
    engine.selectBlind();

    cout << "  - Memainkan tangan hingga mencapai target ronde...\n";
    while (engine.getCurrentScore() < engine.getCurrentBlind().getTargetScore() && engine.getHandsRemaining() > 0) {
        engine.toggleSelectCard(0);
        engine.playHand();
    }

    if (engine.getCurrentScore() >= engine.getCurrentBlind().getTargetScore()) {
        assert(engine.getCurrentStateName() == "ROUND_WON");
        cout << "  - Target tercapai! State berubah menjadi: ROUND_WON\n";

        // Cash out reward
        int oldMoney = engine.getMoney();
        int reward = engine.getCurrentBlind().getRewardMoney();
        engine.cashOut();

        assert(engine.getCurrentStateName() == "BLIND_SELECT");
        assert(engine.getMoney() == oldMoney + reward);
        assert(engine.isBlindDefeated(0)); // Small Blind selesai dikalahkan
        assert(engine.isBlindCurrent(1));  // Big Blind sekarang aktif!
        cout << "  - Cash out berhasil! Uang bertambah $" << reward << " dan kembali ke BLIND_SELECT (Big Blind aktif) OK\n";
    }

    cout << ">> [PASS] Test 3 Selesai!\n";
}

int main() {
    cout << "\n==========================================================\n";
    cout << "   BALATRO MINI - SEGMEN 3 UNIT TEST (C++)                \n";
    cout << "   Menu -> Blind Selection -> In-Game Round Flow          \n";
    cout << "==========================================================\n";

    try {
        testBlindAndAnteScaling();
        testStatePatternAndPlayerActions();
        testWinAndCashOutTransitions();

        cout << "\n==========================================================\n";
        cout << "   SEMUA PENGUJIAN SEGMEN 3 LOLOS 100%!                   \n";
        cout << "==========================================================\n\n";
    } catch (const exception& e) {
        cerr << "\n[FATAL UNCAUGHT EXCEPTION] " << e.what() << "\n";
        return 1;
    }

    return 0;
}

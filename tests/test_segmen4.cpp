#include <iostream>
#include <cassert>
#include "../include/core/GameEngine.h"
#include "../include/core/Joker.h"
#include "../include/core/Exceptions.h"

using namespace std;

void testJokerScoringEffects() {
    cout << "\n========================================" << endl;
    cout << " [TEST] Test 1: Efek Strategy Scoring Joker" << endl;
    cout << "========================================" << endl;

    // Persiapkan kartu uji: Pair of Hearts (Ace of Hearts, Ace of Diamonds)
    Card aH(Suit::HEARTS, Rank::ACE);
    Card aD(Suit::DIAMONDS, Rank::ACE);
    vector<Card> played = { aH, aD };
    vector<Card> scoring = { aH, aD };

    ScoringContext ctx{ played, scoring, 40, 2 };

    // 1. Classic Joker (+4 Mult)
    FlatMultEffect classicJoker(4);
    int chips = 10;
    int mult = 2;
    classicJoker.applyEffect(ctx, chips, mult);
    assert(mult == 6);
    cout << "  - Classic Joker (+4 Mult): OK (Mult: 2 -> " << mult << ")" << endl;

    // 2. Lusty Joker (+3 Mult per Heart)
    SuitMultEffect lustyJoker(Suit::HEARTS, 3);
    lustyJoker.applyEffect(ctx, chips, mult);
    assert(mult == 9); // Ada 1 Heart di scoring cards (+3)
    cout << "  - Lusty Joker (+3 Mult per Heart): OK (Mult: 6 -> " << mult << ")" << endl;

    // 3. Blue Joker (+2 Chips per sisa kartu di deck: 40 kartu = +80 Chips)
    DeckCountChipsEffect blueJoker(2);
    blueJoker.applyEffect(ctx, chips, mult);
    assert(chips == 90);
    cout << "  - Blue Joker (+2 Chips x 40 sisa kartu deck): OK (Chips: 10 -> " << chips << ")" << endl;

    // 4. Banner (+30 Chips per sisa discard: 2 sisa = +60 Chips)
    DiscardChipsEffect banner(30);
    banner.applyEffect(ctx, chips, mult);
    assert(chips == 150);
    cout << "  - Banner (+30 Chips x 2 sisa discards): OK (Chips: 90 -> " << chips << ")" << endl;

    cout << ">> [PASS] Test 1 Selesai!" << endl;
}

void testShopSystemAndStateTransition() {
    cout << "\n========================================" << endl;
    cout << " [TEST] Test 2: Shop State, Pembelian, & Penjualan" << endl;
    cout << "========================================" << endl;

    GameEngine engine;
    engine.startNewRun();

    // Tambahkan Joker kuat untuk memastikan ronde langsung menang
    engine.addJoker(Joker("dev_joker", "Cheat Joker", "+100 Mult", 0, JokerRarity::RARE, make_shared<FlatMultEffect>(100)));

    engine.selectBlind();

    // Naikkan uang untuk keperluan tes
    engine.addMoney(20);
    assert(engine.getMoney() == 24);

    // Kalahkan ronde dalam 1x playHand (karena ada +100 Mult, skor > 1000)
    engine.toggleSelectCard(0);
    engine.toggleSelectCard(1);
    engine.playHand();

    assert(engine.getCurrentStateName() == "ROUND_WON");
    cout << "  - Ronde selesai, state: ROUND_WON OK" << endl;

    // Cash Out harus mengarahkan pemain ke SHOP
    engine.cashOut();
    assert(engine.getCurrentStateName() == "SHOP");
    cout << "  - Cash out berhasil, transisi ke state: SHOP OK" << endl;

    const auto& shop = engine.getShopInventory();
    assert(!shop.empty());
    cout << "  - Toko terisi " << shop.size() << " Joker siap beli. Contoh: " << shop[0].getName() << " ($" << shop[0].getCost() << ")" << endl;

    // Beli Joker pertama
    int startMoney = engine.getMoney();
    int itemCost = shop[0].getCost();
    string boughtName = shop[0].getName();
    engine.buyJoker(0);

    assert(engine.getMoney() == startMoney - itemCost);
    assert(engine.getJokers().size() == 2); // 1 dev_joker + 1 yang baru dibeli
    assert(engine.getJokers().back().getName() == boughtName);
    cout << "  - Pembelian berhasil! Uang tersisa: $" << engine.getMoney() << ", Joker tersimpan di slot OK" << endl;

    // Jual Joker yang baru dibeli (index 1)
    int sellVal = engine.getJokers().back().getSellValue();
    int moneyBeforeSell = engine.getMoney();
    engine.sellJoker(1);
    assert(engine.getJokers().size() == 1);
    assert(engine.getMoney() == moneyBeforeSell + sellVal);
    cout << "  - Penjualan berhasil! Joker terjual $" << sellVal << ", saldo kembali: $" << engine.getMoney() << " OK" << endl;

    // Reroll toko
    int moneyBeforeReroll = engine.getMoney();
    engine.rerollShop();
    assert(engine.getMoney() == moneyBeforeReroll - engine.getRerollCost());
    assert(!engine.getShopInventory().empty());
    cout << "  - Reroll berhasil! Saldo berkurang $" << engine.getRerollCost() << ", stok toko diperbarui OK" << endl;

    // Leave Shop -> Kembali ke Blind Select
    engine.leaveShop();
    assert(engine.getCurrentStateName() == "BLIND_SELECT");
    cout << "  - Leave shop berhasil! Kembali ke BLIND_SELECT OK" << endl;

    cout << ">> [PASS] Test 2 Selesai!" << endl;
}

void testJokerSlotBoundaryAndDefensiveCoding() {
    cout << "\n========================================" << endl;
    cout << " [TEST] Test 3: Batas Maksimal 5 Slot Joker & Defensive Guard" << endl;
    cout << "========================================" << endl;

    GameEngine engine;
    engine.startNewRun();
    
    // Tambahkan 5 Joker manual hingga penuh
    vector<Joker> all = JokerFactory::createAllJokers();
    for (int i = 0; i < 5; ++i) {
        engine.addJoker(all[i]);
    }
    assert(engine.getJokers().size() == 5);
    cout << "  - 5 Slot Joker terisi penuh OK" << endl;

    // Simulasikan masuk ke Shop
    engine.addMoney(100);
    engine.selectBlind();
    while (engine.getCurrentScore() < engine.getCurrentBlind().getTargetScore()) {
        engine.toggleSelectCard(0);
        engine.playHand();
    }
    engine.cashOut();
    assert(engine.getCurrentStateName() == "SHOP");

    // Mencoba membeli Joker ke-6 saat slot sudah penuh (Harus melempar exception)
    bool slotFullCaught = false;
    try {
        engine.buyJoker(0);
    } catch (const GameException& e) {
        slotFullCaught = true;
        cout << "  - [BERHASIL DITANGKAP] " << e.what() << endl;
    }
    assert(slotFullCaught);
    cout << "  - Guard kapasitas 5 Joker bekerja sempurna OK" << endl;

    cout << ">> [PASS] Test 3 Selesai!" << endl;
}

int main() {
    cout << "==========================================================" << endl;
    cout << "   BALATRO MINI - SEGMEN 4 UNIT TEST (C++)                " << endl;
    cout << "   Pola Desain: Strategy Pattern & Shop State System      " << endl;
    cout << "==========================================================" << endl;

    testJokerScoringEffects();
    testShopSystemAndStateTransition();
    testJokerSlotBoundaryAndDefensiveCoding();

    cout << "\n==========================================================" << endl;
    cout << "   SEMUA PENGUJIAN SEGMEN 4 LOLOS 100%!                   " << endl;
    cout << "==========================================================" << endl;

    return 0;
}

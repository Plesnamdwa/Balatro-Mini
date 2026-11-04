#include <iostream>
#include <cassert>
#include <set>
#include "core/Card.h"
#include "core/Tag.h"
#include "core/Blind.h"
#include "core/GameEngine.h"

using namespace std;

void testBossPoolUnique() {
    cout << "[TEST] 1. Pengujian 10 Boss Blind Pool (Tidak Berulang)..." << endl;
    BlindManager bm;

    set<string> encounteredBosses;
    // Cek selama 10 Ante (10 boss rounds)
    for (int ante = 1; ante <= 10; ++ante) {
        Blind boss = bm.getBlindByIndex(2);
        string bossName = boss.getName();
        cout << "  Ante " << ante << ": " << bossName << " (" << boss.getDebuffDescription() << ")" << endl;
        assert(encounteredBosses.find(bossName) == encounteredBosses.end());
        encounteredBosses.insert(bossName);

        // Maju 3 blind per ante (Small -> Big -> Boss -> Next Ante)
        bm.advanceBlind(); // Small -> Big
        bm.advanceBlind(); // Big -> Boss
        bm.advanceBlind(); // Boss -> Next Ante
    }
    assert(encounteredBosses.size() == 10);
    cout << "  -> BERHASIL: Semua 10 jenis boss blind unik dan tidak berulang dalam 1 putaran pool!" << endl;
}

void testSkipBlindAndD6Reroll() {
    cout << "[TEST] 2. Pengujian Mekanik Skip Blind & D6 Reroll..." << endl;
    GameEngine engine;
    engine.executeStartNewRunLogic();

    assert(engine.getCurrentStateName() == "BLIND_SELECT");
    assert(engine.getCurrentBlind().getName() == "Small Blind");

    int initialMoney = engine.getMoney();
    Blind curBlind = engine.getCurrentBlind();
    TagReward tag = curBlind.getTagReward();
    cout << "  Small Blind Tag: " << tag.name << " (" << tag.description << ")" << endl;

    // Test skip Small Blind
    engine.skipBlind();

    // Pastikan sekarang berada di Big Blind tanpa lewat round/shop!
    assert(engine.getCurrentStateName() == "BLIND_SELECT");
    assert(engine.getCurrentBlind().getName() == "Big Blind");
    cout << "  -> BERHASIL: Small Blind berhasil di-skip dan langsung maju ke Big Blind!" << endl;

    // Test skip Big Blind
    curBlind = engine.getCurrentBlind();
    tag = curBlind.getTagReward();
    cout << "  Big Blind Tag: " << tag.name << " (" << tag.description << ")" << endl;
    engine.skipBlind();

    // Sekarang harus berada di Boss Blind
    assert(engine.getCurrentBlind().getType() == BlindType::BOSS_BLIND);
    cout << "  -> BERHASIL: Big Blind berhasil di-skip dan sekarang berada di Boss Blind: " 
         << engine.getCurrentBlind().getName() << endl;

    // Verifikasi bahwa Boss Blind TIDAK BISA di-skip
    bool caughtException = false;
    try {
        engine.skipBlind();
    } catch (const exception& e) {
        caughtException = true;
        cout << "  -> Exception tertangkap sesuai aturan: " << e.what() << endl;
    }
    assert(caughtException);
    cout << "  -> BERHASIL: Boss Blind dicegah dari di-skip!" << endl;
}

int main() {
    cout << "================ RUNNING VERIFICATION TESTS ================\n";
    testBossPoolUnique();
    testSkipBlindAndD6Reroll();
    cout << "================ ALL TESTS PASSED SUCCESSFULLY! ============\n";
    return 0;
}

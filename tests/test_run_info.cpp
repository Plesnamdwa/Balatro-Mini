#include <iostream>
#include <cassert>
#include "../include/core/GameEngine.h"

using namespace std;

int main() {
    cout << "==========================================================" << endl;
    cout << "   BALATRO MINI - RUN INFO & HAND TRACKING UNIT TEST     " << endl;
    cout << "==========================================================" << endl;

    GameEngine engine;
    engine.startNewRun();
    engine.selectBlind();

    // Verify all 9 hand types initial count is 0
    assert(engine.getHandPlayedCount(HandType::HIGH_CARD) == 0);
    assert(engine.getHandPlayedCount(HandType::PAIR) == 0);
    assert(engine.getHandPlayedCount(HandType::TWO_PAIR) == 0);
    assert(engine.getHandPlayedCount(HandType::THREE_OF_A_KIND) == 0);
    assert(engine.getHandPlayedCount(HandType::STRAIGHT) == 0);
    assert(engine.getHandPlayedCount(HandType::FLUSH) == 0);
    assert(engine.getHandPlayedCount(HandType::FULL_HOUSE) == 0);
    assert(engine.getHandPlayedCount(HandType::FOUR_OF_A_KIND) == 0);
    assert(engine.getHandPlayedCount(HandType::STRAIGHT_FLUSH) == 0);
    cout << " [PASS] Inisialisasi awal statistik handPlayedCount = 0 untuk seluruh 9 kombinasi." << endl;

    // Select 1 card and play it (High Card)
    engine.toggleSelectCard(0);
    assert(engine.getSelectedCount() == 1);
    engine.playHand();

    // Verify High Card count incremented to 1
    assert(engine.getHandPlayedCount(HandType::HIGH_CARD) == 1);
    assert(engine.getHandPlayedCount(HandType::PAIR) == 0);
    cout << " [PASS] Memainkan 1 kartu berhasil meningkatkan statistik High Card menjadi 1." << endl;

    // Restart run and verify counts are cleared
    engine.restartRun();
    assert(engine.getHandPlayedCount(HandType::HIGH_CARD) == 0);
    cout << " [PASS] Reset/Restart run berhasil membersihkan statistik kembali ke 0." << endl;

    cout << "==========================================================" << endl;
    cout << "   SEMUA TEST RUN INFO STATISTIK LOLOS 100%!              " << endl;
    cout << "==========================================================" << endl;
    return 0;
}

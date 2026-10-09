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

#include "../src/core/Card.cpp"
#include "../src/core/Deck.cpp"
#include "../src/core/DeckFactory.cpp"
#include "../src/core/PokerEvaluator.cpp"
#include "../src/core/ScoringEngine.cpp"

using namespace std;

void printHeader(const string& title) {
    cout << "\n========================================\n";
    cout << " [TEST] " << title << "\n";
    cout << "========================================\n";
}

void testPokerEvaluations() {
    printHeader("Test 1: Klasifikasi 9 Kombinasi Poker Balatro");
    StandardPokerEvaluator evaluator;

    // 1. High Card (1 kartu atau 5 kartu acak tanpa kombinasi)
    vector<Card> highCardHand = { Card(Suit::SPADES, Rank::ACE) };
    auto resHC = evaluator.evaluate(highCardHand);
    assert(resHC.handType == HandType::HIGH_CARD);
    assert(resHC.scoringCards.size() == 1);
    cout << "  - High Card (A [S]): OK\n";

    // 2. Pair (Dua kartu rank sama + kicker)
    vector<Card> pairHand = {
        Card(Suit::HEARTS, Rank::EIGHT),
        Card(Suit::CLUBS, Rank::EIGHT),
        Card(Suit::DIAMONDS, Rank::TWO)
    };
    auto resPair = evaluator.evaluate(pairHand);
    assert(resPair.handType == HandType::PAIR);
    assert(resPair.scoringCards.size() == 2); // Hanya dua kartu 8 yang dihitung
    cout << "  - Pair (8 [H], 8 [C] + 2 [D]): OK (Hanya 8 yang menjadi scoring card)\n";

    // 3. Two Pair
    vector<Card> twoPairHand = {
        Card(Suit::HEARTS, Rank::TEN),
        Card(Suit::CLUBS, Rank::TEN),
        Card(Suit::DIAMONDS, Rank::FOUR),
        Card(Suit::SPADES, Rank::FOUR)
    };
    auto resTwoPair = evaluator.evaluate(twoPairHand);
    assert(resTwoPair.handType == HandType::TWO_PAIR);
    assert(resTwoPair.scoringCards.size() == 4);
    cout << "  - Two Pair (10-10 & 4-4): OK\n";

    // 4. Three of a Kind
    vector<Card> tripsHand = {
        Card(Suit::HEARTS, Rank::KING),
        Card(Suit::CLUBS, Rank::KING),
        Card(Suit::DIAMONDS, Rank::KING),
        Card(Suit::SPADES, Rank::THREE)
    };
    auto resTrips = evaluator.evaluate(tripsHand);
    assert(resTrips.handType == HandType::THREE_OF_A_KIND);
    assert(resTrips.scoringCards.size() == 3);
    cout << "  - Three of a Kind (K-K-K): OK\n";

    // 5. Straight (Standar: 5-6-7-8-9)
    vector<Card> straightHand = {
        Card(Suit::HEARTS, Rank::FIVE),
        Card(Suit::CLUBS, Rank::SIX),
        Card(Suit::DIAMONDS, Rank::SEVEN),
        Card(Suit::SPADES, Rank::EIGHT),
        Card(Suit::HEARTS, Rank::NINE)
    };
    auto resStraight = evaluator.evaluate(straightHand);
    assert(resStraight.handType == HandType::STRAIGHT);
    assert(resStraight.scoringCards.size() == 5);
    cout << "  - Standard Straight (5-6-7-8-9): OK\n";

    // Straight Ace-Low (A-2-3-4-5)
    vector<Card> aceLowHand = {
        Card(Suit::HEARTS, Rank::ACE),
        Card(Suit::CLUBS, Rank::TWO),
        Card(Suit::DIAMONDS, Rank::THREE),
        Card(Suit::SPADES, Rank::FOUR),
        Card(Suit::HEARTS, Rank::FIVE)
    };
    auto resAceLow = evaluator.evaluate(aceLowHand);
    assert(resAceLow.handType == HandType::STRAIGHT);
    cout << "  - Ace-Low Straight (A-2-3-4-5): OK\n";

    // 6. Flush (5 kartu satu suit acak rank)
    vector<Card> flushHand = {
        Card(Suit::SPADES, Rank::TWO),
        Card(Suit::SPADES, Rank::SIX),
        Card(Suit::SPADES, Rank::NINE),
        Card(Suit::SPADES, Rank::JACK),
        Card(Suit::SPADES, Rank::ACE)
    };
    auto resFlush = evaluator.evaluate(flushHand);
    assert(resFlush.handType == HandType::FLUSH);
    assert(resFlush.scoringCards.size() == 5);
    cout << "  - Flush (5 kartu Spades): OK\n";

    // 7. Full House (3 Queen + 2 Dua)
    vector<Card> fullHouseHand = {
        Card(Suit::HEARTS, Rank::QUEEN),
        Card(Suit::CLUBS, Rank::QUEEN),
        Card(Suit::DIAMONDS, Rank::QUEEN),
        Card(Suit::SPADES, Rank::TWO),
        Card(Suit::HEARTS, Rank::TWO)
    };
    auto resFH = evaluator.evaluate(fullHouseHand);
    assert(resFH.handType == HandType::FULL_HOUSE);
    assert(resFH.scoringCards.size() == 5);
    cout << "  - Full House (Q-Q-Q + 2-2): OK\n";

    // 8. Four of a Kind (4 Ace)
    vector<Card> quadsHand = {
        Card(Suit::HEARTS, Rank::ACE),
        Card(Suit::CLUBS, Rank::ACE),
        Card(Suit::DIAMONDS, Rank::ACE),
        Card(Suit::SPADES, Rank::ACE),
        Card(Suit::HEARTS, Rank::SEVEN)
    };
    auto resQuads = evaluator.evaluate(quadsHand);
    assert(resQuads.handType == HandType::FOUR_OF_A_KIND);
    assert(resQuads.scoringCards.size() == 4);
    cout << "  - Four of a Kind (A-A-A-A): OK\n";

    // 9. Straight Flush (10-J-Q-K-A Hearts)
    vector<Card> sfHand = {
        Card(Suit::HEARTS, Rank::TEN),
        Card(Suit::HEARTS, Rank::JACK),
        Card(Suit::HEARTS, Rank::QUEEN),
        Card(Suit::HEARTS, Rank::KING),
        Card(Suit::HEARTS, Rank::ACE)
    };
    auto resSF = evaluator.evaluate(sfHand);
    assert(resSF.handType == HandType::STRAIGHT_FLUSH);
    assert(resSF.scoringCards.size() == 5);
    cout << "  - Straight Flush (10-J-Q-K-A Hearts): OK\n";

    cout << ">> [PASS] Test 1 Selesai!\n";
}

void testScoringFormula() {
    printHeader("Test 2: Perhitungan Matematis Scoring (Chips x Mult)");
    ScoringEngine engine;

    // Contoh: Pair of Aces (A [H], A [S])
    // Pair Base: 10 Chips, 2 Mult
    // Card Chips: Ace = 11, Ace = 11 -> Total Card Chips = 22
    // Total Chips = 10 + 22 = 32 Chips
    // Total Mult = 2 Mult
    // Final Score = 32 x 2 = 64
    vector<Card> pairOfAces = {
        Card(Suit::HEARTS, Rank::ACE),
        Card(Suit::SPADES, Rank::ACE)
    };
    ScoreResult res = engine.calculateScore(pairOfAces);
    cout << "  - Hand: " << res.handName << "\n";
    cout << "  - Base Chips: " << res.baseChips << ", Card Chips: " << res.cardChips << " -> Total Chips: " << res.totalChips << "\n";
    cout << "  - Base Mult: " << res.baseMult << " -> Total Mult: " << res.totalMult << "\n";
    cout << "  - Skor Akhir: " << res.finalScore << " (" << res.totalChips << " x " << res.totalMult << ")\n";
    assert(res.totalChips == 32);
    assert(res.totalMult == 2);
    assert(res.finalScore == 64);
    cout << "  - Perhitungan Pair of Aces: OK (Skor = 64)\n";

    // Contoh: Full House dengan bonus modifier (+10 Chips, +2 Mult)
    // Three Kings (10x3 = 30) + Pair of Tens (10x2 = 20) -> Card Chips = 50
    // Full House Base: 40 Chips, 4 Mult
    // Bonus: +10 Chips, +2 Mult
    // Total Chips = 40 + 50 + 10 = 100 Chips
    // Total Mult = 4 + 2 = 6 Mult
    // Final Score = 100 x 6 = 600
    vector<Card> fullHouse = {
        Card(Suit::HEARTS, Rank::KING),
        Card(Suit::CLUBS, Rank::KING),
        Card(Suit::SPADES, Rank::KING),
        Card(Suit::HEARTS, Rank::TEN),
        Card(Suit::DIAMONDS, Rank::TEN)
    };
    ScoreResult fhRes = engine.calculateScore(fullHouse, 10, 2);
    cout << "\n  - Hand: " << fhRes.handName << " (Dengan Bonus +10 Chips, +2 Mult)\n";
    cout << "  - Total Chips: " << fhRes.totalChips << ", Total Mult: " << fhRes.totalMult << "\n";
    cout << "  - Skor Akhir: " << fhRes.finalScore << " (Harus 600)\n";
    assert(fhRes.totalChips == 100);
    assert(fhRes.totalMult == 6);
    assert(fhRes.finalScore == 600);
    cout << "  - Perhitungan Full House + Bonus: OK (Skor = 600)\n";

    cout << ">> [PASS] Test 2 Selesai!\n";
}

void testErrorHandlingScoring() {
    printHeader("Test 3: Error Handling & Boundary Check Evaluator");
    ScoringEngine engine;

    // Test 1: Evaluasi 0 kartu harus melempar InvalidCardCountException
    bool emptyCaught = false;
    try {
        cout << "  - Menguji evaluasi 0 kartu (Kosong)...\n";
        vector<Card> emptyCards;
        engine.calculateScore(emptyCards);
    } catch (const InvalidCardCountException& e) {
        emptyCaught = true;
        cout << "  - [BERHASIL DITANGKAP] " << e.what() << "\n";
    }
    assert(emptyCaught);

    // Test 2: Evaluasi lebih dari 5 kartu harus melempar InvalidCardCountException
    bool overlimitCaught = false;
    try {
        cout << "  - Menguji evaluasi 6 kartu (Melebihi batas maksimal 5)...\n";
        vector<Card> sixCards = {
            Card(Suit::HEARTS, Rank::TWO),
            Card(Suit::HEARTS, Rank::THREE),
            Card(Suit::HEARTS, Rank::FOUR),
            Card(Suit::HEARTS, Rank::FIVE),
            Card(Suit::HEARTS, Rank::SIX),
            Card(Suit::HEARTS, Rank::SEVEN)
        };
        engine.calculateScore(sixCards);
    } catch (const InvalidCardCountException& e) {
        overlimitCaught = true;
        cout << "  - [BERHASIL DITANGKAP] " << e.what() << "\n";
    }
    assert(overlimitCaught);

    cout << ">> [PASS] Test 3 Selesai! Validasi jumlah kartu terlindungi dengan aman.\n";
}

int main() {
    cout << "\n======================================================\n";
    cout << "   BALATRO MINI - SEGMEN 2 UNIT TEST (C++)            \n";
    cout << "   Pola Desain: Strategy Pattern & Chips x Mult Logic \n";
    cout << "======================================================\n";

    try {
        testPokerEvaluations();
        testScoringFormula();
        testErrorHandlingScoring();

        cout << "\n======================================================\n";
        cout << "   SEMUA PENGUJIAN SEGMEN 2 LOLOS 100%!               \n";
        cout << "======================================================\n\n";
    } catch (const exception& e) {
        cerr << "\n[FATAL UNCAUGHT EXCEPTION] " << e.what() << "\n";
        return 1;
    }

    return 0;
}

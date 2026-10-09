#include <iostream>
#include <cassert>
#include <memory>
#include <vector>

// Memasukkan header & implementasi agar dapat di-compile langsung dengan tombol F5 (Single-File Run) di VS Code
#include "../include/core/Card.h"
#include "../include/core/Deck.h"
#include "../include/core/DeckFactory.h"
#include "../include/core/Exceptions.h"

#include "../src/core/Card.cpp"
#include "../src/core/Deck.cpp"
#include "../src/core/DeckFactory.cpp"

using namespace std;

void printHeader(const string& title) {
    cout << "\n========================================\n";
    cout << " [TEST] " << title << "\n";
    cout << "========================================\n";
}

void testCardProperties() {
    printHeader("Test 1: Properti Kartu & Base Chips");
    
    Card c2(Suit::HEARTS, Rank::TWO);
    Card c10(Suit::DIAMONDS, Rank::TEN);
    Card cJ(Suit::CLUBS, Rank::JACK);
    Card cQ(Suit::SPADES, Rank::QUEEN);
    Card cK(Suit::HEARTS, Rank::KING);
    Card cA(Suit::SPADES, Rank::ACE);

    assert(c2.getBaseChips() == 2);
    assert(c10.getBaseChips() == 10);
    assert(cJ.getBaseChips() == 10);
    assert(cQ.getBaseChips() == 10);
    assert(cK.getBaseChips() == 10);
    assert(cA.getBaseChips() == 11);

    cout << "  - Rank 2-10 Chips OK\n";
    cout << "  - Face Cards (J, Q, K = 10 Chips) OK\n";
    cout << "  - Ace (11 Chips) OK\n";

    // Test selection toggle
    assert(!cA.isSelected());
    cA.toggleSelected();
    assert(cA.isSelected());
    cA.toggleSelected();
    assert(!cA.isSelected());
    cout << "  - Selection Toggle OK\n";

    // Test debuff
    assert(cA.getBaseChips() == 11);
    cA.setDebuffed(true);
    assert(cA.getBaseChips() == 0);
    cout << "  - Debuff Mekanik (Chips jadi 0) OK\n";

    cout << ">> [PASS] Test 1 Selesai!\n";
}

void testFactoryAndDeck() {
    printHeader("Test 2: Factory Pattern & Manajemen Deck");

    unique_ptr<IDeckFactory> factory = make_unique<StandardDeckFactory>();
    Deck deck = factory->createDeck();

    assert(deck.getRemainingCount() == 52);
    assert(deck.getDiscardCount() == 0);
    assert(!deck.isEmpty());
    cout << "  - StandardDeckFactory berhasil membuat 52 kartu standar.\n";

    // Test draw hand (Balatro standard: 8 cards)
    vector<Card> hand = deck.drawMultiple(8);
    assert(hand.size() == 8);
    assert(deck.getRemainingCount() == 44);
    cout << "  - Draw 8 kartu untuk tangan pemain: OK (Sisa di deck: 44 kartu)\n";

    cout << "  - Contoh kartu yang ditarik: ";
    for (const auto& c : hand) {
        cout << "[" << c.getFullName() << "] ";
    }
    cout << "\n";

    // Test discard 3 cards
    vector<Card> toDiscard = { hand[0], hand[1], hand[2] };
    deck.discardMultiple(toDiscard);
    assert(deck.getDiscardCount() == 3);
    cout << "  - Discard 3 kartu: OK (Tumpukan discard: 3 kartu)\n";

    // Test recycle discards
    deck.recycleDiscardsIntoDeck();
    assert(deck.getDiscardCount() == 0);
    assert(deck.getRemainingCount() == 47);
    cout << "  - Recycle Discard ke Deck: OK (Total kartu di deck kembali 47)\n";

    cout << ">> [PASS] Test 2 Selesai!\n";
}

void testErrorHandlingDeckExhaustion() {
    printHeader("Test 3: Error Handling & Exception saat Deck Kosong");

    vector<Card> miniList = {
        Card(Suit::HEARTS, Rank::ACE),
        Card(Suit::SPADES, Rank::KING)
    };

    CustomDeckFactory miniFactory(miniList);
    Deck miniDeck = miniFactory.createDeck();

    assert(miniDeck.getRemainingCount() == 2);
    Card c1 = miniDeck.drawCard();
    Card c2 = miniDeck.drawCard();
    assert(c1.getRank() == Rank::KING);
    assert(c2.getRank() == Rank::ACE);
    assert(miniDeck.isEmpty());
    cout << "  - Mini deck (2 kartu) ditarik hingga kosong.\n";

    // Mencoba draw kartu ke-3 dari deck yang kosong harus melempar DeckEmptyException
    bool exceptionCaught = false;
    try {
        cout << "  - Mencoba draw kartu dari deck kosong...\n";
        miniDeck.drawCard();
    } catch (const DeckEmptyException& e) {
        exceptionCaught = true;
        cout << "  - [BERHASIL DITANGKAP] " << e.what() << "\n";
    } catch (const exception& e) {
        cout << "  - [ERROR LAIN] " << e.what() << "\n";
    }

    assert(exceptionCaught);
    cout << ">> [PASS] Test 3 Selesai! DeckEmptyException tertangani dengan aman.\n";
}

int main() {
    cout << "\n==================================================\n";
    cout << "   BALATRO MINI - SEGMEN 1 UNIT TEST (C++)        \n";
    cout << "   Pola Desain: Factory Method & Defensive Coding \n";
    cout << "==================================================\n";

    try {
        testCardProperties();
        testFactoryAndDeck();
        testErrorHandlingDeckExhaustion();

        cout << "\n==================================================\n";
        cout << "   SEMUA PENGUJIAN SEGMEN 1 LOLOS 100%!           \n";
        cout << "==================================================\n\n";
    } catch (const exception& e) {
        cerr << "\n[FATAL UNCAUGHT EXCEPTION] " << e.what() << "\n";
        return 1;
    }

    return 0;
}

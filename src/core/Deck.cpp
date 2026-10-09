#include "../../include/core/Deck.h"
#include "../../include/core/Exceptions.h"
#include <algorithm>
#include <random>
#include <chrono>

using namespace std;

Deck::Deck() {}

Deck::Deck(const vector<Card>& initialCards) 
    : cards(initialCards) {}

void Deck::shuffle() {
    // Fisher-Yates shuffle menggunakan Mersenne Twister dengan seed entropy
    unsigned seed = static_cast<unsigned>(chrono::system_clock::now().time_since_epoch().count());
    mt19937 g(seed);
    ::shuffle(cards.begin(), cards.end(), g);
}

Card Deck::drawCard() {
    if (cards.empty()) {
        throw DeckEmptyException();
    }
    Card topCard = cards.back();
    cards.pop_back();
    topCard.setSelected(false);
    return topCard;
}

vector<Card> Deck::drawMultiple(int count) {
    if (count <= 0) {
        return {};
    }
    vector<Card> drawn;
    drawn.reserve(count);
    for (int i = 0; i < count; ++i) {
        if (cards.empty()) {
            throw DeckEmptyException();
        }
        drawn.push_back(drawCard());
    }
    return drawn;
}

void Deck::discard(const Card& card) {
    Card c = card;
    c.setSelected(false);
    discardPile.push_back(c);
}

void Deck::discardMultiple(const vector<Card>& discardedCards) {
    for (const auto& c : discardedCards) {
        discard(c);
    }
}

void Deck::recycleDiscardsIntoDeck() {
    for (const auto& c : discardPile) {
        cards.push_back(c);
    }
    discardPile.clear();
    shuffle();
}

int Deck::getRemainingCount() const {
    return static_cast<int>(cards.size());
}

int Deck::getDiscardCount() const {
    return static_cast<int>(discardPile.size());
}

bool Deck::isEmpty() const {
    return cards.empty();
}

const vector<Card>& Deck::getCards() const {
    return cards;
}

const vector<Card>& Deck::getDiscardPile() const {
    return discardPile;
}

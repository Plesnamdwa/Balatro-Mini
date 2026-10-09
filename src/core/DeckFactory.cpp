#include "../../include/core/DeckFactory.h"

using namespace std;

Deck StandardDeckFactory::createDeck() const {
    vector<Card> standard52;
    standard52.reserve(52);

    const Suit suits[] = { Suit::HEARTS, Suit::DIAMONDS, Suit::CLUBS, Suit::SPADES };
    const Rank ranks[] = {
        Rank::TWO, Rank::THREE, Rank::FOUR, Rank::FIVE,
        Rank::SIX, Rank::SEVEN, Rank::EIGHT, Rank::NINE,
        Rank::TEN, Rank::JACK, Rank::QUEEN, Rank::KING, Rank::ACE
    };

    for (Suit s : suits) {
        for (Rank r : ranks) {
            standard52.emplace_back(s, r);
        }
    }

    Deck deck(standard52);
    deck.shuffle();
    return deck;
}

CustomDeckFactory::CustomDeckFactory(const vector<Card>& cards)
    : customCards(cards) {}

Deck CustomDeckFactory::createDeck() const {
    Deck deck(customCards);
    return deck;
}

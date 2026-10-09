#ifndef DECK_H
#define DECK_H

#include "Card.h"
#include <vector>

using namespace std;

class Deck {
private:
    vector<Card> cards;
    vector<Card> discardPile;

public:
    Deck();
    explicit Deck(const vector<Card>& initialCards);

    void shuffle();
    Card drawCard();
    vector<Card> drawMultiple(int count);

    void discard(const Card& card);
    void discardMultiple(const vector<Card>& discardedCards);

    void recycleDiscardsIntoDeck();

    int getRemainingCount() const;
    int getDiscardCount() const;
    bool isEmpty() const;

    const vector<Card>& getCards() const;
    const vector<Card>& getDiscardPile() const;
};

#endif // DECK_H

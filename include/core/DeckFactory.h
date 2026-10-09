#ifndef DECK_FACTORY_H
#define DECK_FACTORY_H

#include "Deck.h"
#include <vector>
#include <memory>

using namespace std;

// Abstract Factory Interface (Pola Desain: Factory Method)
class IDeckFactory {
public:
    virtual ~IDeckFactory() = default;
    virtual Deck createDeck() const = 0;
};

// Concrete Factory: Menghasilkan 52 kartu standar (4 suits x 13 ranks)
class StandardDeckFactory : public IDeckFactory {
public:
    Deck createDeck() const override;
};

// Concrete Factory: Menghasilkan Custom Deck (opsional untuk testing / deck modifikasi)
class CustomDeckFactory : public IDeckFactory {
private:
    vector<Card> customCards;
public:
    explicit CustomDeckFactory(const vector<Card>& cards);
    Deck createDeck() const override;
};

#endif // DECK_FACTORY_H

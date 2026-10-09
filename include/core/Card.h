#ifndef CARD_H
#define CARD_H

#include <string>

using namespace std;

enum class Suit {
    HEARTS,     // Hati (♥)
    DIAMONDS,   // Wajik (♦)
    CLUBS,      // Keriting (♣)
    SPADES      // Sekop (♠)
};

enum class Rank {
    TWO = 2,
    THREE = 3,
    FOUR = 4,
    FIVE = 5,
    SIX = 6,
    SEVEN = 7,
    EIGHT = 8,
    NINE = 9,
    TEN = 10,
    JACK = 11,
    QUEEN = 12,
    KING = 13,
    ACE = 14
};

class Card {
private:
    Suit suit;
    Rank rank;
    int baseChips;
    bool selected;
    bool debuffed; // Untuk mekanik Boss Blind di masa depan

public:
    Card(Suit s, Rank r);

    Suit getSuit() const;
    Rank getRank() const;
    int getBaseChips() const;
    
    bool isSelected() const;
    void setSelected(bool sel);
    void toggleSelected();

    bool isDebuffed() const;
    void setDebuffed(bool debuff);

    string getSuitSymbol() const;
    string getRankString() const;
    string getFullName() const;

    // Operator kesetaraan untuk pencarian dan komparasi
    bool operator==(const Card& other) const;
    bool operator<(const Card& other) const;
};

#endif // CARD_H

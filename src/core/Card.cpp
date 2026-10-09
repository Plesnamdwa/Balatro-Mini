#include "../../include/core/Card.h"

using namespace std;

Card::Card(Suit s, Rank r) 
    : suit(s), rank(r), selected(false), debuffed(false) {
    // Balatro standard base chips per rank:
    // 2-10 bernilai rank itu sendiri, J/Q/K bernilai 10, Ace bernilai 11
    int rankVal = static_cast<int>(rank);
    if (rankVal <= 10) {
        baseChips = rankVal;
    } else if (rank == Rank::ACE) {
        baseChips = 11;
    } else { // Jack, Queen, King
        baseChips = 10;
    }
}

Suit Card::getSuit() const { return suit; }
Rank Card::getRank() const { return rank; }
int Card::getBaseChips() const { 
    return debuffed ? 0 : baseChips; 
}

bool Card::isSelected() const { return selected; }
void Card::setSelected(bool sel) { selected = sel; }
void Card::toggleSelected() { selected = !selected; }

bool Card::isDebuffed() const { return debuffed; }
void Card::setDebuffed(bool debuff) { debuffed = debuff; }

string Card::getSuitSymbol() const {
    switch (suit) {
        case Suit::HEARTS:   return "[H]"; // ♥
        case Suit::DIAMONDS: return "[D]"; // ♦
        case Suit::CLUBS:    return "[C]"; // ♣
        case Suit::SPADES:   return "[S]"; // ♠
    }
    return "?";
}

string Card::getRankString() const {
    switch (rank) {
        case Rank::TWO:   return "2";
        case Rank::THREE: return "3";
        case Rank::FOUR:  return "4";
        case Rank::FIVE:  return "5";
        case Rank::SIX:   return "6";
        case Rank::SEVEN: return "7";
        case Rank::EIGHT: return "8";
        case Rank::NINE:  return "9";
        case Rank::TEN:   return "10";
        case Rank::JACK:  return "J";
        case Rank::QUEEN: return "Q";
        case Rank::KING:  return "K";
        case Rank::ACE:   return "A";
    }
    return "?";
}

string Card::getFullName() const {
    return getRankString() + " " + getSuitSymbol();
}

bool Card::operator==(const Card& other) const {
    return suit == other.suit && rank == other.rank;
}

bool Card::operator<(const Card& other) const {
    if (rank != other.rank) {
        return static_cast<int>(rank) < static_cast<int>(other.rank);
    }
    return static_cast<int>(suit) < static_cast<int>(other.suit);
}

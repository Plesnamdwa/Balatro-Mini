#ifndef POKER_HAND_H
#define POKER_HAND_H

#include <string>

using namespace std;

enum class HandType {
    HIGH_CARD,
    PAIR,
    TWO_PAIR,
    THREE_OF_A_KIND,
    STRAIGHT,
    FLUSH,
    FULL_HOUSE,
    FOUR_OF_A_KIND,
    STRAIGHT_FLUSH
};

struct HandInfo {
    HandType type;
    string name;
    int baseChips;
    int baseMult;
};

// Helper untuk mendapatkan base chips & mult resmi standar Balatro
inline HandInfo getHandBaseInfo(HandType type) {
    switch (type) {
        case HandType::STRAIGHT_FLUSH:
            return { type, "Straight Flush", 100, 8 };
        case HandType::FOUR_OF_A_KIND:
            return { type, "Four of a Kind", 60, 7 };
        case HandType::FULL_HOUSE:
            return { type, "Full House", 40, 4 };
        case HandType::FLUSH:
            return { type, "Flush", 35, 4 };
        case HandType::STRAIGHT:
            return { type, "Straight", 30, 4 };
        case HandType::THREE_OF_A_KIND:
            return { type, "Three of a Kind", 30, 3 };
        case HandType::TWO_PAIR:
            return { type, "Two Pair", 20, 2 };
        case HandType::PAIR:
            return { type, "Pair", 10, 2 };
        case HandType::HIGH_CARD:
        default:
            return { HandType::HIGH_CARD, "High Card", 5, 1 };
    }
}

#endif // POKER_HAND_H

#include "../../include/core/PokerEvaluator.h"
#include <algorithm>
#include <map>

using namespace std;

bool StandardPokerEvaluator::isFlush(const vector<Card>& cards) const {
    if (cards.size() != 5) return false;
    Suit firstSuit = cards[0].getSuit();
    for (size_t i = 1; i < cards.size(); ++i) {
        if (cards[i].getSuit() != firstSuit) {
            return false;
        }
    }
    return true;
}

bool StandardPokerEvaluator::isStraight(const vector<Card>& cards, bool& aceLow) const {
    aceLow = false;
    if (cards.size() != 5) return false;

    // Kumpulkan nilai numerik rank dan urutkan menaik
    vector<int> ranks;
    for (const auto& c : cards) {
        ranks.push_back(static_cast<int>(c.getRank()));
    }
    sort(ranks.begin(), ranks.end());

    // Cek apakah ada rank kembar (Straight tidak boleh punya kartu kembar)
    for (size_t i = 1; i < ranks.size(); ++i) {
        if (ranks[i] == ranks[i - 1]) return false;
    }

    // Kasus Straight Standar (misal: 2-3-4-5-6, 10-J-Q-K-A)
    bool standardStraight = true;
    for (size_t i = 1; i < ranks.size(); ++i) {
        if (ranks[i] != ranks[i - 1] + 1) {
            standardStraight = false;
            break;
        }
    }
    if (standardStraight) return true;

    // Kasus Ace-Low / Steel Straight (A, 2, 3, 4, 5) -> Nilai di vector: [2, 3, 4, 5, 14]
    if (ranks[0] == 2 && ranks[1] == 3 && ranks[2] == 4 && ranks[3] == 5 && ranks[4] == 14) {
        aceLow = true;
        return true;
    }

    return false;
}

EvaluationResult StandardPokerEvaluator::evaluate(const vector<Card>& playedCards) const {
    if (playedCards.empty() || playedCards.size() > 5) {
        throw InvalidCardCountException(static_cast<int>(playedCards.size()));
    }

    // Salin kartu dan urutkan berdasarkan rank menurun (kartu terbesar di awal)
    vector<Card> sortedCards = playedCards;
    sort(sortedCards.begin(), sortedCards.end(), [](const Card& a, const Card& b) {
        return static_cast<int>(a.getRank()) > static_cast<int>(b.getRank());
    });

    // Petakan kartu berdasarkan rank untuk mendeteksi Pair, Three of Kind, Four of Kind
    map<int, vector<Card>> rankGroups;
    for (const auto& c : sortedCards) {
        rankGroups[static_cast<int>(c.getRank())].push_back(c);
    }

    bool aceLow = false;
    bool hasFlush = isFlush(sortedCards);
    bool hasStraight = isStraight(sortedCards, aceLow);

    // 1. STRAIGHT FLUSH (5 kartu sama suit dan berurutan)
    if (hasFlush && hasStraight) {
        return { HandType::STRAIGHT_FLUSH, "Straight Flush", sortedCards };
    }

    // Cari grup kartu kembar (4-of-kind, 3-of-kind, pairs)
    vector<vector<Card>> quads;
    vector<vector<Card>> triples;
    vector<vector<Card>> pairs;

    for (auto& [rankVal, group] : rankGroups) {
        if (group.size() == 4) {
            quads.push_back(group);
        } else if (group.size() == 3) {
            triples.push_back(group);
        } else if (group.size() == 2) {
            pairs.push_back(group);
        }
    }

    // Urutkan grup berdasarkan nilai rank menurun agar kartu bernilai lebih tinggi diprioritaskan
    auto sortByRank = [](const vector<Card>& a, const vector<Card>& b) {
        return static_cast<int>(a[0].getRank()) > static_cast<int>(b[0].getRank());
    };
    sort(triples.begin(), triples.end(), sortByRank);
    sort(pairs.begin(), pairs.end(), sortByRank);

    // 2. FOUR OF A KIND
    if (!quads.empty()) {
        return { HandType::FOUR_OF_A_KIND, "Four of a Kind", quads[0] };
    }

    // 3. FULL HOUSE (3 of a kind + Pair)
    if (!triples.empty() && !pairs.empty()) {
        vector<Card> scoring = triples[0];
        scoring.insert(scoring.end(), pairs[0].begin(), pairs[0].end());
        return { HandType::FULL_HOUSE, "Full House", scoring };
    }

    // 4. FLUSH
    if (hasFlush) {
        return { HandType::FLUSH, "Flush", sortedCards };
    }

    // 5. STRAIGHT
    if (hasStraight) {
        return { HandType::STRAIGHT, "Straight", sortedCards };
    }

    // 6. THREE OF A KIND
    if (!triples.empty()) {
        return { HandType::THREE_OF_A_KIND, "Three of a Kind", triples[0] };
    }

    // 7. TWO PAIR
    if (pairs.size() >= 2) {
        vector<Card> scoring = pairs[0];
        scoring.insert(scoring.end(), pairs[1].begin(), pairs[1].end());
        return { HandType::TWO_PAIR, "Two Pair", scoring };
    }

    // 8. PAIR
    if (pairs.size() == 1) {
        return { HandType::PAIR, "Pair", pairs[0] };
    }

    // 9. HIGH CARD (Hanya kartu rank tertinggi yang menjadi scoring card)
    return { HandType::HIGH_CARD, "High Card", { sortedCards[0] } };
}

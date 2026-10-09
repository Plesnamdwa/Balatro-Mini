#ifndef POKER_EVALUATOR_H
#define POKER_EVALUATOR_H

#include "Card.h"
#include "PokerHand.h"
#include "Exceptions.h"
#include <vector>
#include <memory>

using namespace std;

struct EvaluationResult {
    HandType handType;
    string handName;
    vector<Card> scoringCards; // Kartu yang berkontribusi membentuk kombinasi poker
};

// Pola Desain: Strategy Pattern Interface
class IPokerEvaluatorStrategy {
public:
    virtual ~IPokerEvaluatorStrategy() = default;
    virtual EvaluationResult evaluate(const vector<Card>& playedCards) const = 0;
};

// Implementasi Evaluator Standar Poker Balatro
class StandardPokerEvaluator : public IPokerEvaluatorStrategy {
public:
    EvaluationResult evaluate(const vector<Card>& playedCards) const override;

private:
    bool isFlush(const vector<Card>& cards) const;
    bool isStraight(const vector<Card>& cards, bool& aceLow) const;
};

#endif // POKER_EVALUATOR_H

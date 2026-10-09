#ifndef SCORING_ENGINE_H
#define SCORING_ENGINE_H

#include "Card.h"
#include "PokerEvaluator.h"
#include <vector>
#include <memory>

using namespace std;

struct ScoreResult {
    HandType handType;
    string handName;
    
    int baseChips;
    int cardChips;
    int bonusChips;
    int totalChips;

    int baseMult;
    int bonusMult;
    int totalMult;

    long long finalScore; // Chips x Mult

    vector<Card> scoringCards;
    vector<Card> allPlayedCards;
};

class ScoringEngine {
private:
    unique_ptr<IPokerEvaluatorStrategy> evaluator;

public:
    ScoringEngine();
    explicit ScoringEngine(unique_ptr<IPokerEvaluatorStrategy> customEvaluator);

    // Ganti strategi evaluator saat runtime (Fitur utama Strategy Pattern)
    void setEvaluatorStrategy(unique_ptr<IPokerEvaluatorStrategy> newEvaluator);

    // Hitung total skor dari kartu yang dimainkan pemain (1-5 kartu)
    ScoreResult calculateScore(const vector<Card>& playedCards, 
                               int additionalChips = 0, 
                               int additionalMult = 0) const;
};

#endif // SCORING_ENGINE_H

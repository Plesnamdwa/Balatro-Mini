#include "../../include/core/ScoringEngine.h"

using namespace std;

ScoringEngine::ScoringEngine() 
    : evaluator(make_unique<StandardPokerEvaluator>()) {}

ScoringEngine::ScoringEngine(unique_ptr<IPokerEvaluatorStrategy> customEvaluator)
    : evaluator(move(customEvaluator)) {}

void ScoringEngine::setEvaluatorStrategy(unique_ptr<IPokerEvaluatorStrategy> newEvaluator) {
    evaluator = move(newEvaluator);
}

ScoreResult ScoringEngine::calculateScore(const vector<Card>& playedCards, 
                                          int additionalChips, 
                                          int additionalMult) const {
    // Evaluasi kombinasi tangan poker menggunakan strategy
    EvaluationResult eval = evaluator->evaluate(playedCards);
    HandInfo baseInfo = getHandBaseInfo(eval.handType);

    // Hitung akumulasi chip dari kartu-kartu yang berkontribusi (scoring cards)
    int sumCardChips = 0;
    for (const auto& card : eval.scoringCards) {
        sumCardChips += card.getBaseChips();
    }

    int totalChips = baseInfo.baseChips + sumCardChips + additionalChips;
    int totalMult = baseInfo.baseMult + additionalMult;

    // Pastikan tidak ada nilai negatif
    if (totalChips < 0) totalChips = 0;
    if (totalMult < 1) totalMult = 1;

    long long finalScore = static_cast<long long>(totalChips) * totalMult;

    ScoreResult result;
    result.handType = eval.handType;
    result.handName = eval.handName;
    result.baseChips = baseInfo.baseChips;
    result.cardChips = sumCardChips;
    result.bonusChips = additionalChips;
    result.totalChips = totalChips;
    result.baseMult = baseInfo.baseMult;
    result.bonusMult = additionalMult;
    result.totalMult = totalMult;
    result.finalScore = finalScore;
    result.scoringCards = eval.scoringCards;
    result.allPlayedCards = playedCards;

    return result;
}

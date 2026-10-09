#include "../../include/core/Blind.h"

using namespace std;

Blind::Blind(BlindType t, int target, int reward, BossDebuff deb)
    : type(t), targetScore(target), rewardMoney(reward), debuff(deb) {
    switch (type) {
        case BlindType::SMALL_BLIND:
            name = "Small Blind";
            break;
        case BlindType::BIG_BLIND:
            name = "Big Blind";
            break;
        case BlindType::BOSS_BLIND:
            name = "Boss Blind";
            break;
    }
}

BlindType Blind::getType() const { return type; }
int Blind::getTargetScore() const { return targetScore; }
int Blind::getRewardMoney() const { return rewardMoney; }
BossDebuff Blind::getDebuff() const { return debuff; }
string Blind::getName() const { return name; }

string Blind::getDebuffDescription() const {
    switch (debuff) {
        case BossDebuff::DEBUFF_HEARTS:
            return "All Heart cards 0 Chips";
        case BossDebuff::DEBUFF_SPADES:
            return "All Spade cards 0 Chips";
        case BossDebuff::MINUS_HAND_SIZE:
            return "-1 Hand Size (7 cards)";
        case BossDebuff::NONE:
        default:
            return "No debuff";
    }
}

void Blind::applyDebuffToCards(vector<Card>& cards) const {
    for (auto& card : cards) {
        if (debuff == BossDebuff::DEBUFF_HEARTS && card.getSuit() == Suit::HEARTS) {
            card.setDebuffed(true);
        } else if (debuff == BossDebuff::DEBUFF_SPADES && card.getSuit() == Suit::SPADES) {
            card.setDebuffed(true);
        } else {
            card.setDebuffed(false);
        }
    }
}

// ---------------- BlindManager ----------------

BlindManager::BlindManager() 
    : currentAnte(1), currentBlindIndex(0) {}

int BlindManager::getCurrentAnte() const { return currentAnte; }
int BlindManager::getCurrentBlindIndex() const { return currentBlindIndex; }

int BlindManager::calculateBaseTarget(int ante) {
    switch (ante) {
        case 1: return 300;
        case 2: return 800;
        case 3: return 2000;
        case 4: return 5000;
        default: return 5000 * (ante - 3) * 2;
    }
}

Blind BlindManager::getBlindByIndex(int index) const {
    int baseTarget = calculateBaseTarget(currentAnte);

    if (index == 0) {
        return Blind(BlindType::SMALL_BLIND, baseTarget, 3);
    } else if (index == 1) {
        int bigTarget = static_cast<int>(baseTarget * 1.5);
        return Blind(BlindType::BIG_BLIND, bigTarget, 4);
    } else {
        int bossTarget = baseTarget * 2;
        BossDebuff bossDebuff = (currentAnte % 2 == 1) ? BossDebuff::DEBUFF_HEARTS : BossDebuff::DEBUFF_SPADES;
        return Blind(BlindType::BOSS_BLIND, bossTarget, 5, bossDebuff);
    }
}

Blind BlindManager::getCurrentBlind() const {
    return getBlindByIndex(currentBlindIndex);
}

bool BlindManager::isBlindDefeated(int index) const {
    return index < currentBlindIndex;
}

bool BlindManager::isBlindCurrent(int index) const {
    return index == currentBlindIndex;
}

bool BlindManager::isBlindLocked(int index) const {
    return index > currentBlindIndex;
}

void BlindManager::advanceBlind() {
    currentBlindIndex++;
    if (currentBlindIndex > 2) {
        currentBlindIndex = 0;
        currentAnte++;
    }
}

void BlindManager::reset() {
    currentAnte = 1;
    currentBlindIndex = 0;
}

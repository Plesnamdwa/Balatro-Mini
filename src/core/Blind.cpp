#include "../../include/core/Blind.h"
#include <algorithm>
#include <random>
#include <chrono>

using namespace std;

Blind::Blind(BlindType t, int target, int reward, BossDebuff deb, const string& customName, const TagReward& tag)
    : type(t), targetScore(target), rewardMoney(reward), debuff(deb), tagReward(tag) {
    if (!customName.empty()) {
        name = customName;
    } else {
        switch (type) {
            case BlindType::SMALL_BLIND:
                name = "Small Blind";
                break;
            case BlindType::BIG_BLIND:
                name = "Big Blind";
                break;
            case BlindType::BOSS_BLIND:
                name = "The Hook";
                break;
        }
    }
}

BlindType Blind::getType() const { return type; }
int Blind::getTargetScore() const { return targetScore; }
int Blind::getRewardMoney() const { return rewardMoney; }
BossDebuff Blind::getDebuff() const { return debuff; }
string Blind::getName() const { return name; }
TagReward Blind::getTagReward() const { return tagReward; }
void Blind::setTagReward(const TagReward& tag) { tagReward = tag; }

string Blind::getDebuffDescription() const {
    switch (debuff) {
        case BossDebuff::THE_HOOK:
            return "Discards 2 random cards per hand played";
        case BossDebuff::THE_GOAD:
            return "All Spades cards are debuffed (0 Chips)";
        case BossDebuff::THE_HEAD:
            return "All Hearts cards are debuffed (0 Chips)";
        case BossDebuff::THE_CLUB:
            return "All Clubs cards are debuffed (0 Chips)";
        case BossDebuff::THE_WINDOW:
            return "All Diamonds cards are debuffed (0 Chips)";
        case BossDebuff::THE_MANACLE:
            return "-1 Hand Size (7 cards max)";
        case BossDebuff::THE_PILLAR:
            return "Target score increased (+25%)";
        case BossDebuff::THE_NEEDLE:
            return "Play only 1 hand this round";
        case BossDebuff::THE_WATER:
            return "Start with 0 discards this round";
        case BossDebuff::THE_PSYCHIC:
            return "Must play exactly 5 cards to score";
        case BossDebuff::NONE:
        default:
            return "No debuff";
    }
}

void Blind::applyDebuffToCards(vector<Card>& cards) const {
    for (auto& card : cards) {
        if (debuff == BossDebuff::THE_HEAD && card.getSuit() == Suit::HEARTS) {
            card.setDebuffed(true);
        } else if (debuff == BossDebuff::THE_GOAD && card.getSuit() == Suit::SPADES) {
            card.setDebuffed(true);
        } else if (debuff == BossDebuff::THE_CLUB && card.getSuit() == Suit::CLUBS) {
            card.setDebuffed(true);
        } else if (debuff == BossDebuff::THE_WINDOW && card.getSuit() == Suit::DIAMONDS) {
            card.setDebuffed(true);
        } else {
            card.setDebuffed(false);
        }
    }
}

// ---------------- BlindManager ----------------

BlindManager::BlindManager() 
    : currentAnte(1), currentBlindIndex(0), bossPoolIndex(0), currentAnteBoss(BossDebuff::THE_HOOK) {
    initializeBossPool();
    generateAnteBlinds();
}

void BlindManager::initializeBossPool() {
    bossPool = {
        BossDebuff::THE_HOOK,
        BossDebuff::THE_GOAD,
        BossDebuff::THE_HEAD,
        BossDebuff::THE_CLUB,
        BossDebuff::THE_WINDOW,
        BossDebuff::THE_MANACLE,
        BossDebuff::THE_PILLAR,
        BossDebuff::THE_NEEDLE,
        BossDebuff::THE_WATER,
        BossDebuff::THE_PSYCHIC
    };

    // Acak menggunakan seed waktu acak (Fisher-Yates)
    unsigned seed = static_cast<unsigned>(chrono::system_clock::now().time_since_epoch().count());
    shuffle(bossPool.begin(), bossPool.end(), default_random_engine(seed));
    bossPoolIndex = 0;
}

void BlindManager::generateAnteBlinds() {
    // Pastikan pool di-shuffle kembali jika ke-10 boss sudah digunakan habis
    if (bossPoolIndex >= bossPool.size()) {
        initializeBossPool();
    }

    currentAnteBoss = bossPool[bossPoolIndex++];

    // Generate tags untuk Small & Big Blind
    smallBlindTag = TagFactory::getRandomTag();
    bigBlindTag = TagFactory::getRandomTag();

    // Pastikan jika tag sama, coba bedakan agar menarik jika memungkinkan
    if (smallBlindTag.type == bigBlindTag.type) {
        bigBlindTag = TagFactory::getRandomTag();
    }
}

void BlindManager::rerollCurrentBoss() {
    // Ambil boss berikutnya dari pool yang belum muncul
    if (bossPoolIndex >= bossPool.size()) {
        initializeBossPool();
    }
    currentAnteBoss = bossPool[bossPoolIndex++];
}

int BlindManager::getCurrentAnte() const { return currentAnte; }
int BlindManager::getCurrentBlindIndex() const { return currentBlindIndex; }

int BlindManager::calculateBaseTarget(int ante) {
    switch (ante) {
        case 1: return 300;
        case 2: return 800;
        case 3: return 2000;
        case 4: return 5000;
        case 5: return 11000;
        case 6: return 20000;
        case 7: return 35000;
        case 8: return 50000;
        default: return 50000 * (ante - 7);
    }
}

static string getBossNameString(BossDebuff debuff) {
    switch (debuff) {
        case BossDebuff::THE_HOOK: return "The Hook";
        case BossDebuff::THE_GOAD: return "The Goad";
        case BossDebuff::THE_HEAD: return "The Head";
        case BossDebuff::THE_CLUB: return "The Club";
        case BossDebuff::THE_WINDOW: return "The Window";
        case BossDebuff::THE_MANACLE: return "The Manacle";
        case BossDebuff::THE_PILLAR: return "The Pillar";
        case BossDebuff::THE_NEEDLE: return "The Needle";
        case BossDebuff::THE_WATER: return "The Water";
        case BossDebuff::THE_PSYCHIC: return "The Psychic";
        default: return "Boss Blind";
    }
}

Blind BlindManager::getBlindByIndex(int index) const {
    int baseTarget = calculateBaseTarget(currentAnte);

    if (index == 0) {
        return Blind(BlindType::SMALL_BLIND, baseTarget, 3, BossDebuff::NONE, "Small Blind", smallBlindTag);
    } else if (index == 1) {
        int bigTarget = static_cast<int>(baseTarget * 1.5);
        return Blind(BlindType::BIG_BLIND, bigTarget, 4, BossDebuff::NONE, "Big Blind", bigBlindTag);
    } else {
        int bossTarget = baseTarget * 2;
        if (currentAnteBoss == BossDebuff::THE_PILLAR) {
            bossTarget = static_cast<int>(bossTarget * 1.25); // Pillar +25% score target
        }
        string bossName = getBossNameString(currentAnteBoss);
        return Blind(BlindType::BOSS_BLIND, bossTarget, 5, currentAnteBoss, bossName, TagReward());
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
        generateAnteBlinds();
    }
}

void BlindManager::reset() {
    currentAnte = 1;
    currentBlindIndex = 0;
    initializeBossPool();
    generateAnteBlinds();
}

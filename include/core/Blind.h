#ifndef BLIND_H
#define BLIND_H

#include "Card.h"
#include "Tag.h"
#include <string>
#include <vector>

using namespace std;

enum class BlindType {
    SMALL_BLIND,
    BIG_BLIND,
    BOSS_BLIND
};

enum class BossDebuff {
    NONE,
    THE_HOOK,    // Discards 2 random cards per hand played
    THE_GOAD,    // All Spade cards are debuffed (0 Chips)
    THE_HEAD,    // All Heart cards are debuffed (0 Chips)
    THE_CLUB,    // All Club cards are debuffed (0 Chips)
    THE_WINDOW,  // All Diamond cards are debuffed (0 Chips)
    THE_MANACLE, // -1 Hand Size (7 cards)
    THE_PILLAR,  // Boss target score increased (+25%)
    THE_NEEDLE,  // Play only 1 hand this round
    THE_WATER,   // Start with 0 discards this round
    THE_PSYCHIC  // Must play exactly 5 cards to score
};

class Blind {
private:
    BlindType type;
    int targetScore;
    int rewardMoney;
    BossDebuff debuff;
    string name;
    TagReward tagReward;

public:
    Blind(BlindType t, int target, int reward, BossDebuff deb = BossDebuff::NONE, const string& customName = "", const TagReward& tag = TagReward());

    BlindType getType() const;
    int getTargetScore() const;
    int getRewardMoney() const;
    BossDebuff getDebuff() const;
    string getName() const;
    string getDebuffDescription() const;
    TagReward getTagReward() const;
    void setTagReward(const TagReward& tag);

    void applyDebuffToCards(vector<Card>& cards) const;
};

class BlindManager {
private:
    int currentAnte;
    int currentBlindIndex; // 0 = Small, 1 = Big, 2 = Boss

    // Pool 10 Boss Blind (No repeat bag shuffle)
    vector<BossDebuff> bossPool;
    size_t bossPoolIndex;
    BossDebuff currentAnteBoss;

    // Tags untuk Ante saat ini
    TagReward smallBlindTag;
    TagReward bigBlindTag;

    void initializeBossPool();

public:
    BlindManager();

    int getCurrentAnte() const;
    int getCurrentBlindIndex() const;

    Blind getCurrentBlind() const;
    Blind getBlindByIndex(int index) const;

    bool isBlindDefeated(int index) const;
    bool isBlindCurrent(int index) const;
    bool isBlindLocked(int index) const;

    void advanceBlind();
    void generateAnteBlinds();
    void rerollCurrentBoss(); // Dipanggil jika D6 Tag diaktifkan

    static int calculateBaseTarget(int ante);
    void reset();
};

#endif // BLIND_H

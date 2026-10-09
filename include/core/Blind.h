#ifndef BLIND_H
#define BLIND_H

#include "Card.h"
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
    DEBUFF_HEARTS,  // Kartu Hearts tidak memberikan chips
    DEBUFF_SPADES,  // Kartu Spades tidak memberikan chips
    MINUS_HAND_SIZE // Hand size berkurang 1 kartu (misal 8 -> 7)
};

class Blind {
private:
    BlindType type;
    int targetScore;
    int rewardMoney;
    BossDebuff debuff;
    string name;

public:
    Blind(BlindType t, int target, int reward, BossDebuff deb = BossDebuff::NONE);

    BlindType getType() const;
    int getTargetScore() const;
    int getRewardMoney() const;
    BossDebuff getDebuff() const;
    string getName() const;
    string getDebuffDescription() const;

    void applyDebuffToCards(vector<Card>& cards) const;
};

class BlindManager {
private:
    int currentAnte;
    int currentBlindIndex; // 0 = Small, 1 = Big, 2 = Boss

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
    static int calculateBaseTarget(int ante);
    void reset();
};

#endif // BLIND_H

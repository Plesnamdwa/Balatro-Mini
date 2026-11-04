#ifndef GAME_ENGINE_H
#define GAME_ENGINE_H

#include "Card.h"
#include "Deck.h"
#include "DeckFactory.h"
#include "Blind.h"
#include "ScoringEngine.h"
#include "GameState.h"
#include "Joker.h"
#include "Voucher.h"
#include "Exceptions.h"
#include <vector>
#include <memory>
#include <map>
#include <set>

using namespace std;

class GameEngine {
private:
    Deck deck;
    vector<Card> hand;
    int maxHandSize;
    int handsRemaining;
    int discardsRemaining;
    int currentScore;
    int money;

    BlindManager blindManager;
    ScoringEngine scoringEngine;
    ScoreResult lastScoreResult;

    // Sistem Joker & Toko
    vector<Joker> jokers;
    int maxJokers;
    vector<Joker> shopInventory;
    int rerollCost;

    // Sistem Voucher
    set<VoucherType> redeemedVouchers;
    Voucher currentShopVoucher;
    bool isShopVoucherSoldOut;
    int voucherShopAnte;
    int bossRerollsUsedInAnte;

    // Statistik Permainan (Run Info: Jumlah kombinasi poker yang pernah dimainkan)
    map<HandType, int> handPlayedCounts;

    unique_ptr<IGameState> currentState;

    void drawUpToHandSize();

public:
    GameEngine();

    // State Pattern Handler
    void changeState(unique_ptr<IGameState> newState);
    string getCurrentStateName() const;

    // Aksi Alur Permainan (Mendelegasikan ke currentState)
    void startNewRun();
    void selectBlind();
    void skipBlind();
    void playHand();
    void discard();
    void toggleSelectCard(int index);
    void cashOut();
    void buyJoker(int shopIndex);
    void sellJoker(int jokerIndex);
    void rerollShop();
    void leaveShop();
    void buyVoucher();
    void rerollBoss();
    void toMainMenu();
    void restartRun();

    // Logika Internal yang dipanggil oleh State
    void executeStartNewRunLogic();
    void executeSelectBlindLogic();
    void executeSkipBlindLogic();
    void executePlayHandLogic();
    void executeDiscardLogic();
    void executeToggleSelectLogic(int index);
    void executeCashOutLogic();
    void executeBuyJokerLogic(int shopIndex);
    void executeSellJokerLogic(int jokerIndex);
    void executeRerollShopLogic();
    void executeLeaveShopLogic();
    void executeBuyVoucherLogic();
    void executeRerollBossLogic();
    void onEnterShop();
    void generateShopVoucher();
    int getEffectiveShopSlots() const;
    int getDiscountedPrice(int basePrice) const;
    void refreshShopInventory();

    // Getters & Helpers
    const vector<Card>& getHand() const;
    vector<Card> getSelectedCards() const;
    int getSelectedCount() const;
    
    int getHandsRemaining() const;
    int getDiscardsRemaining() const;
    int getMaxHandSize() const;
    int getCurrentScore() const;
    int getMoney() const;
    Blind getCurrentBlind() const;
    Blind getBlindByIndex(int index) const;
    bool isBlindDefeated(int index) const;
    bool isBlindCurrent(int index) const;
    bool isBlindLocked(int index) const;
    int getCurrentAnte() const;
    const ScoreResult& getLastScoreResult() const;
    int getDeckRemainingCount() const;
    int getDiscardPileCount() const;

    const vector<Joker>& getJokers() const;
    int getMaxJokers() const;
    const vector<Joker>& getShopInventory() const;
    int getRerollCost() const;
    int getHandPlayedCount(HandType type) const;

    // Voucher Getters
    bool hasVoucher(VoucherType type) const;
    const set<VoucherType>& getRedeemedVouchers() const;
    const Voucher& getCurrentShopVoucher() const;
    bool isVoucherSoldOut() const;
    int getVoucherCost() const;
    bool canRerollBoss() const;
    int getBossRerollCost() const;

    void addMoney(int amount);
    void addJoker(const Joker& j);
};

#endif // GAME_ENGINE_H

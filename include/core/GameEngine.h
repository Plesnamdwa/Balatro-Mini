#ifndef GAME_ENGINE_H
#define GAME_ENGINE_H

#include "Card.h"
#include "Deck.h"
#include "DeckFactory.h"
#include "Blind.h"
#include "ScoringEngine.h"
#include "GameState.h"
#include "Joker.h"
#include "Exceptions.h"
#include <vector>
#include <memory>

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
    void playHand();
    void discard();
    void toggleSelectCard(int index);
    void cashOut();
    void buyJoker(int shopIndex);
    void sellJoker(int jokerIndex);
    void rerollShop();
    void leaveShop();
    void toMainMenu();

    // Logika Internal yang dipanggil oleh State
    void executeStartNewRunLogic();
    void executeSelectBlindLogic();
    void executePlayHandLogic();
    void executeDiscardLogic();
    void executeToggleSelectLogic(int index);
    void executeCashOutLogic();
    void executeBuyJokerLogic(int shopIndex);
    void executeSellJokerLogic(int jokerIndex);
    void executeRerollShopLogic();
    void executeLeaveShopLogic();
    void refreshShopInventory();

    // Getters & Helpers
    const vector<Card>& getHand() const;
    vector<Card> getSelectedCards() const;
    int getSelectedCount() const;
    
    int getHandsRemaining() const;
    int getDiscardsRemaining() const;
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

    void addMoney(int amount);
    void addJoker(const Joker& j);
};

#endif // GAME_ENGINE_H

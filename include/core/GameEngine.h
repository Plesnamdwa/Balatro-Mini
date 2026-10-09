#ifndef GAME_ENGINE_H
#define GAME_ENGINE_H

#include "Card.h"
#include "Deck.h"
#include "DeckFactory.h"
#include "Blind.h"
#include "ScoringEngine.h"
#include "GameState.h"
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
    void toMainMenu();

    // Logika Internal yang dipanggil oleh State
    void executeStartNewRunLogic();
    void executeSelectBlindLogic();
    void executePlayHandLogic();
    void executeDiscardLogic();
    void executeToggleSelectLogic(int index);
    void executeCashOutLogic();

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

    void addMoney(int amount);
};

#endif // GAME_ENGINE_H

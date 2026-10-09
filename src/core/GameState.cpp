#include "../../include/core/GameState.h"
#include "../../include/core/GameEngine.h"
#include "../../include/core/Exceptions.h"

using namespace std;

// Implementasi default IGameState (Menolak aksi yang tidak sesuai dengan state aktif)
void IGameState::startNewRun(GameEngine& /*engine*/) {
    throw InvalidActionException("Start New Run", "Cannot start new run from state: " + getStateName());
}

void IGameState::selectBlind(GameEngine& /*engine*/) {
    throw InvalidActionException("Select Blind", "Cannot select blind from state: " + getStateName());
}

void IGameState::playHand(GameEngine& /*engine*/) {
    throw InvalidActionException("Play Hand", "Cannot play hand from state: " + getStateName());
}

void IGameState::discard(GameEngine& /*engine*/) {
    throw InvalidActionException("Discard", "Cannot discard from state: " + getStateName());
}

void IGameState::toggleSelectCard(GameEngine& /*engine*/, int /*index*/) {
    throw InvalidActionException("Select Card", "Cannot select card from state: " + getStateName());
}

void IGameState::cashOut(GameEngine& /*engine*/) {
    throw InvalidActionException("Cash Out", "Cannot cash out from state: " + getStateName());
}

void IGameState::toMainMenu(GameEngine& engine) {
    engine.changeState(make_unique<MainMenuState>());
}

// ================= 1. MainMenuState =================
void MainMenuState::startNewRun(GameEngine& engine) {
    engine.executeStartNewRunLogic();
}

// ================= 2. BlindSelectState =================
void BlindSelectState::selectBlind(GameEngine& engine) {
    engine.executeSelectBlindLogic();
}

void BlindSelectState::toMainMenu(GameEngine& engine) {
    engine.changeState(make_unique<MainMenuState>());
}

// ================= 3. SelectingCardState =================
void SelectingCardState::playHand(GameEngine& engine) {
    engine.executePlayHandLogic();
}

void SelectingCardState::discard(GameEngine& engine) {
    engine.executeDiscardLogic();
}

void SelectingCardState::toggleSelectCard(GameEngine& engine, int index) {
    engine.executeToggleSelectLogic(index);
}

// ================= 4. RoundWonState =================
void RoundWonState::onEnter(GameEngine& /*engine*/) {
    // Reward uang langsung disiapkan
}

void RoundWonState::cashOut(GameEngine& engine) {
    engine.executeCashOutLogic();
}

// ================= 5. GameOverState =================
void GameOverState::startNewRun(GameEngine& engine) {
    engine.executeStartNewRunLogic();
}

void GameOverState::toMainMenu(GameEngine& engine) {
    engine.changeState(make_unique<MainMenuState>());
}

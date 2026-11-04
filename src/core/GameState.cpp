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

void IGameState::buyJoker(GameEngine& /*engine*/, int /*shopIndex*/) {
    throw InvalidActionException("Buy Joker", "Cannot buy joker from state: " + getStateName());
}

void IGameState::sellJoker(GameEngine& /*engine*/, int /*jokerIndex*/) {
    throw InvalidActionException("Sell Joker", "Cannot sell joker from state: " + getStateName());
}

void IGameState::rerollShop(GameEngine& /*engine*/) {
    throw InvalidActionException("Reroll Shop", "Cannot reroll shop from state: " + getStateName());
}

void IGameState::leaveShop(GameEngine& /*engine*/) {
    throw InvalidActionException("Leave Shop", "Cannot leave shop from state: " + getStateName());
}

void IGameState::toMainMenu(GameEngine& engine) {
    engine.changeState(make_unique<MainMenuState>());
}

void IGameState::skipBlind(GameEngine& /*engine*/) {
    throw InvalidActionException("Skip Blind", "Cannot skip blind from state: " + getStateName());
}

void IGameState::buyVoucher(GameEngine& /*engine*/) {
    throw InvalidActionException("Buy Voucher", "Cannot buy voucher from state: " + getStateName());
}

void IGameState::rerollBoss(GameEngine& /*engine*/) {
    throw InvalidActionException("Reroll Boss", "Cannot reroll boss from state: " + getStateName());
}

// ================= 1. MainMenuState =================
void MainMenuState::startNewRun(GameEngine& engine) {
    engine.executeStartNewRunLogic();
}

// ================= 2. BlindSelectState =================
void BlindSelectState::selectBlind(GameEngine& engine) {
    engine.executeSelectBlindLogic();
}

void BlindSelectState::skipBlind(GameEngine& engine) {
    engine.executeSkipBlindLogic();
}

void BlindSelectState::toMainMenu(GameEngine& engine) {
    engine.changeState(make_unique<MainMenuState>());
}

void BlindSelectState::rerollBoss(GameEngine& engine) {
    engine.executeRerollBossLogic();
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

// ================= 5. ShopState =================
void ShopState::onEnter(GameEngine& engine) {
    engine.onEnterShop();
}

void ShopState::buyJoker(GameEngine& engine, int shopIndex) {
    engine.executeBuyJokerLogic(shopIndex);
}

void ShopState::sellJoker(GameEngine& engine, int jokerIndex) {
    engine.executeSellJokerLogic(jokerIndex);
}

void ShopState::rerollShop(GameEngine& engine) {
    engine.executeRerollShopLogic();
}

void ShopState::leaveShop(GameEngine& engine) {
    engine.executeLeaveShopLogic();
}

void ShopState::buyVoucher(GameEngine& engine) {
    engine.executeBuyVoucherLogic();
}

// ================= 6. GameOverState =================
void GameOverState::startNewRun(GameEngine& engine) {
    engine.executeStartNewRunLogic();
}

void GameOverState::toMainMenu(GameEngine& engine) {
    engine.changeState(make_unique<MainMenuState>());
}

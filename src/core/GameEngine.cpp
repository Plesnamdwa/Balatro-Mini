#include "../../include/core/GameEngine.h"
#include <algorithm>

using namespace std;

GameEngine::GameEngine()
    : maxHandSize(8), handsRemaining(4), discardsRemaining(3), currentScore(0), money(4) {
    toMainMenu();
}

void GameEngine::changeState(unique_ptr<IGameState> newState) {
    currentState = move(newState);
    if (currentState) {
        currentState->onEnter(*this);
    }
}

string GameEngine::getCurrentStateName() const {
    return currentState ? currentState->getStateName() : "UNKNOWN";
}

void GameEngine::drawUpToHandSize() {
    int needed = maxHandSize - static_cast<int>(hand.size());
    if (needed > 0) {
        if (deck.getRemainingCount() < needed) {
            deck.recycleDiscardsIntoDeck();
        }
        
        int toDraw = min(needed, deck.getRemainingCount());
        for (int i = 0; i < toDraw; ++i) {
            hand.push_back(deck.drawCard());
        }
    }
    getCurrentBlind().applyDebuffToCards(hand);
}

void GameEngine::toMainMenu() {
    changeState(make_unique<MainMenuState>());
}

void GameEngine::executeStartNewRunLogic() {
    StandardDeckFactory factory;
    deck = factory.createDeck();
    hand.clear();
    blindManager.reset();
    money = 4;
    lastScoreResult = {};
    currentScore = 0;

    changeState(make_unique<BlindSelectState>());
}

void GameEngine::executeSelectBlindLogic() {
    currentScore = 0;
    handsRemaining = 4;
    discardsRemaining = 3;

    // Bersihkan tangan dan recycle kartu ke deck
    for (const auto& c : hand) {
        deck.discard(c);
    }
    hand.clear();
    deck.recycleDiscardsIntoDeck();

    Blind blind = getCurrentBlind();
    maxHandSize = (blind.getDebuff() == BossDebuff::MINUS_HAND_SIZE) ? 7 : 8;

    drawUpToHandSize();
    changeState(make_unique<SelectingCardState>());
}

void GameEngine::executeCashOutLogic() {
    // Tambahkan uang dari reward Blind
    addMoney(getCurrentBlind().getRewardMoney());
    // Lanjut ke blind berikutnya (Small -> Big -> Boss -> Ante++)
    blindManager.advanceBlind();
    // Kembali ke layar pemilihan Blind
    changeState(make_unique<BlindSelectState>());
}

void GameEngine::executeToggleSelectLogic(int index) {
    if (index < 0 || index >= static_cast<int>(hand.size())) {
        throw GameException("Card index " + to_string(index) + " out of bounds!");
    }

    if (!hand[index].isSelected()) {
        if (getSelectedCount() >= 5) {
            throw InvalidCardCountException(6, 5);
        }
        hand[index].setSelected(true);
    } else {
        hand[index].setSelected(false);
    }
}

void GameEngine::executePlayHandLogic() {
    if (handsRemaining <= 0) {
        changeState(make_unique<GameOverState>());
        throw InvalidActionException("Play Hand", "No hands remaining!");
    }

    vector<Card> selected = getSelectedCards();
    if (selected.empty()) {
        throw InvalidCardCountException(0, 5);
    }
    if (selected.size() > 5) {
        throw InvalidCardCountException(static_cast<int>(selected.size()), 5);
    }

    // 1. Hitung skor
    lastScoreResult = scoringEngine.calculateScore(selected);
    currentScore += lastScoreResult.finalScore;
    handsRemaining--;

    // 2. Buang kartu yang dimainkan
    vector<Card> remainingHand;
    for (const auto& c : hand) {
        if (c.isSelected()) {
            deck.discard(c);
        } else {
            remainingHand.push_back(c);
        }
    }
    hand = remainingHand;

    // 3. Tarik kartu baru
    drawUpToHandSize();

    // 4. Evaluasi Kondisi Menang / Kalah
    if (currentScore >= getCurrentBlind().getTargetScore()) {
        changeState(make_unique<RoundWonState>());
    } else if (handsRemaining <= 0) {
        changeState(make_unique<GameOverState>());
    }
}

void GameEngine::executeDiscardLogic() {
    if (discardsRemaining <= 0) {
        throw InvalidActionException("Discard", "No discards remaining!");
    }

    vector<Card> selected = getSelectedCards();
    if (selected.empty()) {
        throw InvalidCardCountException(0, 5);
    }
    if (selected.size() > 5) {
        throw InvalidCardCountException(static_cast<int>(selected.size()), 5);
    }

    discardsRemaining--;

    vector<Card> remainingHand;
    for (const auto& c : hand) {
        if (c.isSelected()) {
            deck.discard(c);
        } else {
            remainingHand.push_back(c);
        }
    }
    hand = remainingHand;

    drawUpToHandSize();
}

// Delegasi Aksi Pemain ke State Pattern
void GameEngine::startNewRun() {
    if (currentState) currentState->startNewRun(*this);
}

void GameEngine::selectBlind() {
    if (currentState) currentState->selectBlind(*this);
}

void GameEngine::playHand() {
    if (currentState) currentState->playHand(*this);
}

void GameEngine::discard() {
    if (currentState) currentState->discard(*this);
}

void GameEngine::toggleSelectCard(int index) {
    if (currentState) currentState->toggleSelectCard(*this, index);
}

void GameEngine::cashOut() {
    if (currentState) currentState->cashOut(*this);
}

// Getters & Helpers
const vector<Card>& GameEngine::getHand() const { return hand; }

vector<Card> GameEngine::getSelectedCards() const {
    vector<Card> selected;
    for (const auto& c : hand) {
        if (c.isSelected()) {
            selected.push_back(c);
        }
    }
    return selected;
}

int GameEngine::getSelectedCount() const {
    int count = 0;
    for (const auto& c : hand) {
        if (c.isSelected()) count++;
    }
    return count;
}

int GameEngine::getHandsRemaining() const { return handsRemaining; }
int GameEngine::getDiscardsRemaining() const { return discardsRemaining; }
int GameEngine::getCurrentScore() const { return currentScore; }
int GameEngine::getMoney() const { return money; }

Blind GameEngine::getCurrentBlind() const {
    return blindManager.getCurrentBlind();
}

Blind GameEngine::getBlindByIndex(int index) const {
    return blindManager.getBlindByIndex(index);
}

bool GameEngine::isBlindDefeated(int index) const {
    return blindManager.isBlindDefeated(index);
}

bool GameEngine::isBlindCurrent(int index) const {
    return blindManager.isBlindCurrent(index);
}

bool GameEngine::isBlindLocked(int index) const {
    return blindManager.isBlindLocked(index);
}

int GameEngine::getCurrentAnte() const {
    return blindManager.getCurrentAnte();
}

const ScoreResult& GameEngine::getLastScoreResult() const {
    return lastScoreResult;
}

int GameEngine::getDeckRemainingCount() const {
    return deck.getRemainingCount();
}

int GameEngine::getDiscardPileCount() const {
    return deck.getDiscardCount();
}

void GameEngine::addMoney(int amount) {
    money += amount;
}

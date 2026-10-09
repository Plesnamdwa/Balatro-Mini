#include "../../include/core/GameEngine.h"
#include <algorithm>
#include <random>

using namespace std;

GameEngine::GameEngine()
    : maxHandSize(8), handsRemaining(4), discardsRemaining(3), currentScore(0), money(4),
      maxJokers(5), rerollCost(5) {
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
    jokers.clear();
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
    // Berpindah ke layar Toko (The Shop)
    changeState(make_unique<ShopState>());
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

    // 1. Evaluasi dasar poker
    ScoreResult baseResult = scoringEngine.calculateScore(selected);

    // 2. Evaluasi Efek Strategy dari Jokers
    int bonusChips = 0;
    int bonusMult = 0;
    ScoringContext ctx{
        selected,
        baseResult.scoringCards,
        deck.getRemainingCount(),
        discardsRemaining
    };

    for (const auto& joker : jokers) {
        joker.trigger(ctx, bonusChips, bonusMult);
    }

    // 3. Hitung skor akhir dengan sinergi Joker
    lastScoreResult = scoringEngine.calculateScore(selected, bonusChips, bonusMult);
    currentScore += lastScoreResult.finalScore;
    handsRemaining--;

    // 4. Buang kartu yang dimainkan
    vector<Card> remainingHand;
    for (const auto& c : hand) {
        if (c.isSelected()) {
            deck.discard(c);
        } else {
            remainingHand.push_back(c);
        }
    }
    hand = remainingHand;

    // 5. Tarik kartu baru
    drawUpToHandSize();

    // 6. Evaluasi Kondisi Menang / Kalah
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

void GameEngine::refreshShopInventory() {
    shopInventory.clear();
    vector<Joker> catalog = JokerFactory::createAllJokers();

    // Saring Joker yang sudah dimiliki pemain agar tidak duplikat
    vector<Joker> available;
    for (const auto& j : catalog) {
        bool owned = false;
        for (const auto& oj : jokers) {
            if (oj.getId() == j.getId()) {
                owned = true;
                break;
            }
        }
        if (!owned) {
            available.push_back(j);
        }
    }

    // Acak dan pilih hingga 3 Joker
    static random_device rd;
    static mt19937 g(rd());
    shuffle(available.begin(), available.end(), g);

    int count = min(3, static_cast<int>(available.size()));
    for (int i = 0; i < count; ++i) {
        shopInventory.push_back(available[i]);
    }
}

void GameEngine::executeBuyJokerLogic(int shopIndex) {
    if (shopIndex < 0 || shopIndex >= static_cast<int>(shopInventory.size())) {
        throw GameException("Invalid shop item index!");
    }
    if (static_cast<int>(jokers.size()) >= maxJokers) {
        throw GameException("Joker slots full! Max " + to_string(maxJokers) + " Jokers.");
    }

    const Joker& target = shopInventory[shopIndex];
    if (money < target.getCost()) {
        throw GameException("Not enough money! Cost: $" + to_string(target.getCost()));
    }

    money -= target.getCost();
    jokers.push_back(target);
    shopInventory.erase(shopInventory.begin() + shopIndex);
}

void GameEngine::executeSellJokerLogic(int jokerIndex) {
    if (jokerIndex < 0 || jokerIndex >= static_cast<int>(jokers.size())) {
        throw GameException("Invalid joker index to sell!");
    }

    int sellVal = jokers[jokerIndex].getSellValue();
    money += sellVal;
    jokers.erase(jokers.begin() + jokerIndex);
}

void GameEngine::executeRerollShopLogic() {
    if (money < rerollCost) {
        throw GameException("Not enough money to reroll! Cost: $" + to_string(rerollCost));
    }
    money -= rerollCost;
    refreshShopInventory();
}

void GameEngine::executeLeaveShopLogic() {
    changeState(make_unique<BlindSelectState>());
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

void GameEngine::buyJoker(int shopIndex) {
    if (currentState) currentState->buyJoker(*this, shopIndex);
}

void GameEngine::sellJoker(int jokerIndex) {
    if (currentState) currentState->sellJoker(*this, jokerIndex);
}

void GameEngine::rerollShop() {
    if (currentState) currentState->rerollShop(*this);
}

void GameEngine::leaveShop() {
    if (currentState) currentState->leaveShop(*this);
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

const vector<Joker>& GameEngine::getJokers() const { return jokers; }
int GameEngine::getMaxJokers() const { return maxJokers; }
const vector<Joker>& GameEngine::getShopInventory() const { return shopInventory; }
int GameEngine::getRerollCost() const { return rerollCost; }

void GameEngine::addMoney(int amount) {
    money += amount;
}

void GameEngine::addJoker(const Joker& j) {
    if (static_cast<int>(jokers.size()) < maxJokers) {
        jokers.push_back(j);
    }
}

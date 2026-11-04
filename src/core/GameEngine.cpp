#include "../../include/core/GameEngine.h"
#include <algorithm>
#include <random>

using namespace std;

GameEngine::GameEngine()
    : maxHandSize(8), handsRemaining(4), discardsRemaining(3), currentScore(0), money(4),
      maxJokers(5), rerollCost(5), isShopVoucherSoldOut(false), voucherShopAnte(0), bossRerollsUsedInAnte(0) {
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
    handPlayedCounts.clear();
    redeemedVouchers.clear();
    isShopVoucherSoldOut = false;
    voucherShopAnte = 0;
    bossRerollsUsedInAnte = 0;
    generateShopVoucher();

    blindManager.reset();
    money = 4;
    lastScoreResult = {};
    currentScore = 0;

    changeState(make_unique<BlindSelectState>());
}

void GameEngine::executeSelectBlindLogic() {
    currentScore = 0;

    int baseHands = 4;
    if (hasVoucher(VoucherType::GRABBER)) baseHands += 1;
    if (hasVoucher(VoucherType::NACHO_TONG)) baseHands += 1;
    handsRemaining = baseHands;

    int baseDiscards = 3;
    if (hasVoucher(VoucherType::WASTEFUL)) baseDiscards += 1;
    if (hasVoucher(VoucherType::RECYCLOMANCY)) baseDiscards += 1;
    discardsRemaining = baseDiscards;

    Blind blind = getCurrentBlind();

    // Aturan khusus Boss Blind saat mulai ronde
    if (blind.getDebuff() == BossDebuff::THE_NEEDLE) {
        handsRemaining = 1; // The Needle: Hanya 1 hand sepanjang ronde
    }
    if (blind.getDebuff() == BossDebuff::THE_WATER) {
        discardsRemaining = 0; // The Water: 0 discards
    }

    int bonusSize = 0;
    if (hasVoucher(VoucherType::PAINT_BRUSH)) bonusSize += 1;
    if (hasVoucher(VoucherType::PALETTE)) bonusSize += 1;
    maxHandSize = ((blind.getDebuff() == BossDebuff::THE_MANACLE) ? 7 : 8) + bonusSize;

    // Bersihkan tangan dan recycle kartu ke deck
    for (const auto& c : hand) {
        deck.discard(c);
    }
    hand.clear();
    deck.recycleDiscardsIntoDeck();

    drawUpToHandSize();
    changeState(make_unique<SelectingCardState>());
}

void GameEngine::executeSkipBlindLogic() {
    int curIdx = blindManager.getCurrentBlindIndex();
    // Boss blind tidak boleh di-skip (hanya Small index 0 atau Big index 1)
    if (curIdx >= 2) {
        throw InvalidActionException("Skip Blind", "Boss Blind cannot be skipped!");
    }

    Blind currentB = getCurrentBlind();
    TagReward tag = currentB.getTagReward();

    // Berikan reward tag
    switch (tag.type) {
        case TagType::D6_REROLL_TAG:
            blindManager.rerollCurrentBoss();
            break;
        case TagType::SPEED_TAG:
            addMoney(15);
            break;
        case TagType::HANDY_TAG: {
            int totalHandsPlayed = 0;
            for (const auto& kv : handPlayedCounts) {
                totalHandsPlayed += kv.second;
            }
            addMoney(max(1, totalHandsPlayed));
            break;
        }
        case TagType::GARBAGE_TAG:
            addMoney(max(3, discardsRemaining * 2));
            break;
        case TagType::ECONOMY_TAG: {
            int bonus = min(20, max(6, money));
            addMoney(bonus);
            break;
        }
        case TagType::COUPON_TAG:
            addMoney(10);
            break;
    }

    // Maju ke blind berikutnya tanpa main ronde, tanpa reward uang round, tanpa shop
    blindManager.advanceBlind();
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

    Blind currentB = getCurrentBlind();

    // 1. Evaluasi dasar poker
    ScoreResult baseResult = scoringEngine.calculateScore(selected);

    // The Psychic: Harus memainkan tepat 5 kartu untuk menghasilkan skor!
    if (currentB.getDebuff() == BossDebuff::THE_PSYCHIC && selected.size() < 5) {
        baseResult.finalScore = 0;
        baseResult.baseChips = 0;
        baseResult.baseMult = 0;
        baseResult.handName = "Psychic Debuffed! (0 PTS)";
    }

    // 2. Evaluasi Efek Strategy dari Jokers (jika tidak didebuff The Psychic)
    int bonusChips = 0;
    int bonusMult = 0;
    if (currentB.getDebuff() != BossDebuff::THE_PSYCHIC || selected.size() == 5) {
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
    } else {
        lastScoreResult = baseResult;
    }

    currentScore += lastScoreResult.finalScore;
    handsRemaining--;
    handPlayedCounts[baseResult.handType]++;

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

    // The Hook: Membuang 2 kartu acak dari sisa tangan ke discard pile!
    if (currentB.getDebuff() == BossDebuff::THE_HOOK && !hand.empty()) {
        int discardsCount = min(2, static_cast<int>(hand.size()));
        for (int i = 0; i < discardsCount; ++i) {
            int randIdx = rand() % hand.size();
            deck.discard(hand[randIdx]);
            hand.erase(hand.begin() + randIdx);
        }
    }

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

int GameEngine::getEffectiveShopSlots() const {
    int slots = 2; // Default menjual 2 Joker
    if (hasVoucher(VoucherType::OVERSTOCK)) slots += 1; // Jadi 3
    if (hasVoucher(VoucherType::OVERSTOCK_PLUS)) slots += 1; // Jadi 4
    return slots;
}

int GameEngine::getDiscountedPrice(int basePrice) const {
    if (hasVoucher(VoucherType::LIQUIDATION)) {
        return max(1, (basePrice + 1) / 2); // Diskon 50%
    } else if (hasVoucher(VoucherType::CLEARANCE_SALE)) {
        return max(1, static_cast<int>(ceil(basePrice * 0.75f))); // Diskon 25%
    }
    return basePrice;
}

void GameEngine::generateShopVoucher() {
    vector<Voucher> eligible = VoucherDatabase::getEligibleVouchers(redeemedVouchers);
    if (eligible.empty()) {
        currentShopVoucher = Voucher(); // NONE
        return;
    }
    static random_device rd;
    static mt19937 g(rd());
    shuffle(eligible.begin(), eligible.end(), g);
    currentShopVoucher = eligible[0];
}

void GameEngine::onEnterShop() {
    if (voucherShopAnte != getCurrentAnte()) {
        voucherShopAnte = getCurrentAnte();
        isShopVoucherSoldOut = false;
        bossRerollsUsedInAnte = 0;
        generateShopVoucher();
    }
    refreshShopInventory();
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

    // Acak dan pilih sejumlah slot etalase toko yang efektif
    static random_device rd;
    static mt19937 g(rd());
    shuffle(available.begin(), available.end(), g);

    int count = min(getEffectiveShopSlots(), static_cast<int>(available.size()));
    for (int i = 0; i < count; ++i) {
        shopInventory.push_back(available[i]);
    }
}

void GameEngine::executeBuyJokerLogic(int shopIndex) {
    if (shopIndex < 0 || shopIndex >= static_cast<int>(shopInventory.size())) {
        throw GameException("Invalid shop item index!");
    }
    if (static_cast<int>(jokers.size()) >= getMaxJokers()) {
        throw GameException("Joker slots full! Max " + to_string(getMaxJokers()) + " Jokers.");
    }

    const Joker& target = shopInventory[shopIndex];
    int effectiveCost = getDiscountedPrice(target.getCost());
    if (money < effectiveCost) {
        throw GameException("Not enough money! Cost: $" + to_string(effectiveCost));
    }

    money -= effectiveCost;
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
    int cost = getRerollCost();
    if (money < cost) {
        throw GameException("Not enough money to reroll! Cost: $" + to_string(cost));
    }
    money -= cost;
    refreshShopInventory();
}

void GameEngine::executeBuyVoucherLogic() {
    if (isShopVoucherSoldOut || currentShopVoucher.type == VoucherType::NONE) {
        throw GameException("Voucher is sold out for this Ante!");
    }
    int cost = getVoucherCost();
    if (money < cost) {
        throw GameException("Not enough money to buy voucher! Cost: $" + to_string(cost));
    }

    money -= cost;
    redeemedVouchers.insert(currentShopVoucher.type);
    isShopVoucherSoldOut = true;

    // Jika Overstock / Overstock Plus dibeli, tambahkan Joker ekstra langsung ke etalase jika masih muat
    if (currentShopVoucher.type == VoucherType::OVERSTOCK || currentShopVoucher.type == VoucherType::OVERSTOCK_PLUS) {
        int targetSlots = getEffectiveShopSlots();
        if (static_cast<int>(shopInventory.size()) < targetSlots) {
            vector<Joker> catalog = JokerFactory::createAllJokers();
            for (const auto& j : catalog) {
                bool alreadyInShop = false;
                for (const auto& sj : shopInventory) {
                    if (sj.getId() == j.getId()) { alreadyInShop = true; break; }
                }
                bool alreadyOwned = false;
                for (const auto& oj : jokers) {
                    if (oj.getId() == j.getId()) { alreadyOwned = true; break; }
                }
                if (!alreadyInShop && !alreadyOwned) {
                    shopInventory.push_back(j);
                    if (static_cast<int>(shopInventory.size()) >= targetSlots) break;
                }
            }
        }
    }
}

bool GameEngine::canRerollBoss() const {
    if (blindManager.getCurrentBlindIndex() != 2) return false;
    if (hasVoucher(VoucherType::RETCON)) {
        return money >= 10;
    } else if (hasVoucher(VoucherType::DIRECTORS_CUT)) {
        return (bossRerollsUsedInAnte == 0) && (money >= 10);
    }
    return false;
}

int GameEngine::getBossRerollCost() const {
    return 10;
}

void GameEngine::executeRerollBossLogic() {
    if (!hasVoucher(VoucherType::DIRECTORS_CUT) && !hasVoucher(VoucherType::RETCON)) {
        throw GameException("Requires Director's Cut or Retcon voucher to reroll Boss Blind!");
    }
    if (money < 10) {
        throw GameException("Not enough money to reroll Boss! Cost: $10");
    }
    if (bossRerollsUsedInAnte > 0 && !hasVoucher(VoucherType::RETCON)) {
        throw GameException("Director's Cut allows only 1 Boss reroll per Ante!");
    }

    money -= 10;
    bossRerollsUsedInAnte++;
    blindManager.rerollCurrentBoss();
}

void GameEngine::executeLeaveShopLogic() {
    changeState(make_unique<BlindSelectState>());
}

// Delegasi Aksi Pemain ke State Pattern
void GameEngine::startNewRun() {
    if (currentState) currentState->startNewRun(*this);
}

void GameEngine::restartRun() {
    executeStartNewRunLogic();
}

void GameEngine::selectBlind() {
    if (currentState) currentState->selectBlind(*this);
}

void GameEngine::skipBlind() {
    if (currentState) currentState->skipBlind(*this);
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

void GameEngine::buyVoucher() {
    if (currentState) currentState->buyVoucher(*this);
}

void GameEngine::rerollBoss() {
    if (currentState) currentState->rerollBoss(*this);
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

int GameEngine::getMaxHandSize() const {
    return maxHandSize;
}

const vector<Joker>& GameEngine::getJokers() const { return jokers; }

int GameEngine::getMaxJokers() const { 
    return hasVoucher(VoucherType::ANTIMATTER) ? 6 : 5; 
}

const vector<Joker>& GameEngine::getShopInventory() const { return shopInventory; }

int GameEngine::getRerollCost() const { 
    int cost = 5;
    if (hasVoucher(VoucherType::REROLL_SURPLUS)) cost -= 2;
    if (hasVoucher(VoucherType::REROLL_GLUT)) cost -= 2;
    return max(1, cost);
}

bool GameEngine::hasVoucher(VoucherType type) const {
    return redeemedVouchers.find(type) != redeemedVouchers.end();
}

const set<VoucherType>& GameEngine::getRedeemedVouchers() const {
    return redeemedVouchers;
}

const Voucher& GameEngine::getCurrentShopVoucher() const {
    return currentShopVoucher;
}

bool GameEngine::isVoucherSoldOut() const {
    return isShopVoucherSoldOut || (currentShopVoucher.type == VoucherType::NONE);
}

int GameEngine::getVoucherCost() const {
    return getDiscountedPrice(currentShopVoucher.baseCost);
}

void GameEngine::addMoney(int amount) {
    money += amount;
}

void GameEngine::addJoker(const Joker& j) {
    if (static_cast<int>(jokers.size()) < getMaxJokers()) {
        jokers.push_back(j);
    }
}

int GameEngine::getHandPlayedCount(HandType type) const {
    auto it = handPlayedCounts.find(type);
    return (it != handPlayedCounts.end()) ? it->second : 0;
}

#ifndef GAME_STATE_H
#define GAME_STATE_H

#include <string>
#include <memory>
#include <vector>

using namespace std;

class GameEngine;

// Interface State Pattern
class IGameState {
public:
    virtual ~IGameState() = default;
    virtual string getStateName() const = 0;
    
    virtual void onEnter(GameEngine& /*engine*/) {}
    virtual void startNewRun(GameEngine& /*engine*/);
    virtual void selectBlind(GameEngine& /*engine*/);
    virtual void playHand(GameEngine& /*engine*/);
    virtual void discard(GameEngine& /*engine*/);
    virtual void toggleSelectCard(GameEngine& /*engine*/, int /*index*/);
    virtual void cashOut(GameEngine& /*engine*/);
    virtual void buyJoker(GameEngine& /*engine*/, int /*shopIndex*/);
    virtual void sellJoker(GameEngine& /*engine*/, int /*jokerIndex*/);
    virtual void rerollShop(GameEngine& /*engine*/);
    virtual void leaveShop(GameEngine& /*engine*/);
    virtual void toMainMenu(GameEngine& /*engine*/);
};

// 1. State Tampilan Awal / Menu Utama (Title Screen)
class MainMenuState : public IGameState {
public:
    string getStateName() const override { return "MAIN_MENU"; }
    void startNewRun(GameEngine& engine) override;
};

// 2. State Pemilihan Blind (Small, Big, Boss Blind)
class BlindSelectState : public IGameState {
public:
    string getStateName() const override { return "BLIND_SELECT"; }
    void selectBlind(GameEngine& engine) override;
    void toMainMenu(GameEngine& engine) override;
};

// 3. State Pemain Sedang Memilih Kartu (Fase Aktif Ronde Permainan)
class SelectingCardState : public IGameState {
public:
    string getStateName() const override { return "SELECTING_CARDS"; }
    void playHand(GameEngine& engine) override;
    void discard(GameEngine& engine) override;
    void toggleSelectCard(GameEngine& engine, int index) override;
};

// 4. State Ronde Menang (Target Skor Tercapai -> Cash Out ke Shop)
class RoundWonState : public IGameState {
public:
    string getStateName() const override { return "ROUND_WON"; }
    void onEnter(GameEngine& engine) override;
    void cashOut(GameEngine& engine) override;
};

// 5. State Toko (The Shop: Beli/Jual Joker, Reroll)
class ShopState : public IGameState {
public:
    string getStateName() const override { return "SHOP"; }
    void onEnter(GameEngine& engine) override;
    void buyJoker(GameEngine& engine, int shopIndex) override;
    void sellJoker(GameEngine& engine, int jokerIndex) override;
    void rerollShop(GameEngine& engine) override;
    void leaveShop(GameEngine& engine) override;
};

// 6. State Kalah / Game Over
class GameOverState : public IGameState {
public:
    string getStateName() const override { return "GAME_OVER"; }
    void toMainMenu(GameEngine& engine) override;
    void startNewRun(GameEngine& engine) override;
};

#endif // GAME_STATE_H

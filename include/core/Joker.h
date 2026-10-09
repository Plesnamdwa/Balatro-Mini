#ifndef JOKER_H
#define JOKER_H

#include "Card.h"
#include <string>
#include <vector>
#include <memory>

using namespace std;

// Konteks data saat scoring berlangsung untuk dievaluasi oleh Joker
struct ScoringContext {
    const vector<Card>& playedCards;
    const vector<Card>& scoringCards;
    int remainingDeckCount;
    int remainingDiscards;
};

enum class JokerRarity {
    COMMON,
    UNCOMMON,
    RARE
};

// Interface Strategy Pattern untuk Efek Kemampuan Joker
class IJokerEffect {
public:
    virtual ~IJokerEffect() = default;
    virtual void applyEffect(const ScoringContext& ctx, int& inOutChips, int& inOutMult) const = 0;
};

// Kelas Data Joker
class Joker {
private:
    string id;
    string name;
    string description;
    int cost;
    JokerRarity rarity;
    shared_ptr<IJokerEffect> effect;

public:
    Joker(string id, string name, string description, int cost, JokerRarity rarity, shared_ptr<IJokerEffect> effect)
        : id(move(id)), name(move(name)), description(move(description)),
          cost(cost), rarity(rarity), effect(move(effect)) {}

    const string& getId() const { return id; }
    const string& getName() const { return name; }
    const string& getDescription() const { return description; }
    int getCost() const { return cost; }
    int getSellValue() const { return max(1, cost / 2); }
    JokerRarity getRarity() const { return rarity; }

    void trigger(const ScoringContext& ctx, int& chips, int& mult) const {
        if (effect) {
            effect->applyEffect(ctx, chips, mult);
        }
    }
};

// ================= DAFTAR IMPLEMENTASI STRATEGY EFEK JOKER =================

// 1. Classic Joker: +4 Flat Mult
class FlatMultEffect : public IJokerEffect {
private:
    int multBonus;
public:
    explicit FlatMultEffect(int bonus = 4) : multBonus(bonus) {}
    void applyEffect(const ScoringContext& /*ctx*/, int& /*chips*/, int& inOutMult) const override {
        inOutMult += multBonus;
    }
};

// 2. Suit Booster Effect: +3 Mult per kartu suit tertentu yang scoring
class SuitMultEffect : public IJokerEffect {
private:
    Suit targetSuit;
    int multPerCard;
public:
    SuitMultEffect(Suit suit, int bonus = 3) : targetSuit(suit), multPerCard(bonus) {}
    void applyEffect(const ScoringContext& ctx, int& /*chips*/, int& inOutMult) const override {
        int count = 0;
        for (const auto& c : ctx.scoringCards) {
            if (!c.isDebuffed() && c.getSuit() == targetSuit) {
                count++;
            }
        }
        inOutMult += (count * multPerCard);
    }
};

// 3. Blue Joker: +2 Chips per sisa kartu di dalam deck
class DeckCountChipsEffect : public IJokerEffect {
private:
    int chipsPerCard;
public:
    explicit DeckCountChipsEffect(int bonus = 2) : chipsPerCard(bonus) {}
    void applyEffect(const ScoringContext& ctx, int& inOutChips, int& /*mult*/) const override {
        inOutChips += (ctx.remainingDeckCount * chipsPerCard);
    }
};

// 4. Banner: +30 Chips per sisa discard yang belum terpakai
class DiscardChipsEffect : public IJokerEffect {
private:
    int chipsPerDiscard;
public:
    explicit DiscardChipsEffect(int bonus = 30) : chipsPerDiscard(bonus) {}
    void applyEffect(const ScoringContext& ctx, int& inOutChips, int& /*mult*/) const override {
        inOutChips += (ctx.remainingDiscards * chipsPerDiscard);
    }
};

// Factory untuk membuat katalog Joker
class JokerFactory {
public:
    static vector<Joker> createAllJokers() {
        vector<Joker> list;
        list.emplace_back("joker_classic", "Joker", "+4 Mult", 4, JokerRarity::COMMON, 
                          make_shared<FlatMultEffect>(4));
        list.emplace_back("joker_greedy", "Greedy Joker", "Played Diamond cards give +3 Mult when scored", 5, JokerRarity::COMMON, 
                          make_shared<SuitMultEffect>(Suit::DIAMONDS, 3));
        list.emplace_back("joker_lusty", "Lusty Joker", "Played Heart cards give +3 Mult when scored", 5, JokerRarity::COMMON, 
                          make_shared<SuitMultEffect>(Suit::HEARTS, 3));
        list.emplace_back("joker_wrathful", "Wrathful Joker", "Played Spade cards give +3 Mult when scored", 5, JokerRarity::COMMON, 
                          make_shared<SuitMultEffect>(Suit::SPADES, 3));
        list.emplace_back("joker_gluttonous", "Gluttonous Joker", "Played Club cards give +3 Mult when scored", 5, JokerRarity::COMMON, 
                          make_shared<SuitMultEffect>(Suit::CLUBS, 3));
        list.emplace_back("joker_blue", "Blue Joker", "+2 Chips for each card remaining in deck", 5, JokerRarity::COMMON, 
                          make_shared<DeckCountChipsEffect>(2));
        list.emplace_back("joker_banner", "Banner", "+30 Chips for each remaining discard", 6, JokerRarity::COMMON, 
                          make_shared<DiscardChipsEffect>(30));
        return list;
    }
};

#endif // JOKER_H

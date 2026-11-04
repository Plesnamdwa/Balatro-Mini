#ifndef VOUCHER_H
#define VOUCHER_H

#include <string>
#include <vector>
#include <set>

struct VoucherColor {
    float r, g, b, a;
    VoucherColor(float red = 1.0f, float green = 1.0f, float blue = 1.0f, float alpha = 1.0f)
        : r(red), g(green), b(blue), a(alpha) {}
};

enum class VoucherType {
    NONE = 0,
    // Pair 1: Hands
    GRABBER,
    NACHO_TONG,
    // Pair 2: Discards
    WASTEFUL,
    RECYCLOMANCY,
    // Pair 3: Hand Size
    PAINT_BRUSH,
    PALETTE,
    // Pair 4: Joker Slots
    BLANK,
    ANTIMATTER,
    // Pair 5: Reroll Cost
    REROLL_SURPLUS,
    REROLL_GLUT,
    // Pair 6: Store Discount
    CLEARANCE_SALE,
    LIQUIDATION,
    // Pair 7: Overstock
    OVERSTOCK,
    OVERSTOCK_PLUS,
    // Pair 8: Boss Reroll
    DIRECTORS_CUT,
    RETCON
};

struct Voucher {
    VoucherType type;
    std::string name;
    std::string description;
    int tier;                    // 1 atau 2
    VoucherType prerequisite;    // NONE jika Tier 1, tipe Tier 1 jika Tier 2
    int baseCost;                // Default 10
    VoucherColor accentColor;

    Voucher() 
        : type(VoucherType::NONE), name("None"), description(""), tier(1), 
          prerequisite(VoucherType::NONE), baseCost(10), accentColor(VoucherColor(0.8f, 0.8f, 0.8f)) {}

    Voucher(VoucherType t, const std::string& n, const std::string& d, int tr, 
            VoucherType prereq, int cost, VoucherColor col)
        : type(t), name(n), description(d), tier(tr), 
          prerequisite(prereq), baseCost(cost), accentColor(col) {}

    bool operator==(const Voucher& other) const {
        return type == other.type;
    }
};

class VoucherDatabase {
public:
    static const std::vector<Voucher>& getAllVouchers();
    static Voucher getVoucher(VoucherType type);
    static std::vector<Voucher> getEligibleVouchers(const std::set<VoucherType>& redeemed);
};

#endif // VOUCHER_H

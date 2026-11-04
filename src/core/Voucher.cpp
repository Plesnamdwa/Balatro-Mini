#include "../../include/core/Voucher.h"

using namespace std;

static const vector<Voucher> ALL_VOUCHERS = {
    // Pair 1: Hands
    Voucher(VoucherType::GRABBER, "Grabber", 
            "Permanently gain +1 hand per round.", 
            1, VoucherType::NONE, 10, VoucherColor(0.25f, 0.55f, 0.85f)),
    Voucher(VoucherType::NACHO_TONG, "Nacho Tong", 
            "Permanently gain an additional +1 hand per round (+2 total).", 
            2, VoucherType::GRABBER, 10, VoucherColor(0.15f, 0.40f, 0.75f)),

    // Pair 2: Discards
    Voucher(VoucherType::WASTEFUL, "Wasteful", 
            "Permanently gain +1 discard per round.", 
            1, VoucherType::NONE, 10, VoucherColor(0.85f, 0.30f, 0.28f)),
    Voucher(VoucherType::RECYCLOMANCY, "Recyclomancy", 
            "Permanently gain an additional +1 discard per round (+2 total).", 
            2, VoucherType::WASTEFUL, 10, VoucherColor(0.75f, 0.20f, 0.20f)),

    // Pair 3: Hand Size
    Voucher(VoucherType::PAINT_BRUSH, "Paint Brush", 
            "Permanently gain +1 hand size (draw 9 cards).", 
            1, VoucherType::NONE, 10, VoucherColor(0.20f, 0.70f, 0.65f)),
    Voucher(VoucherType::PALETTE, "Palette", 
            "Permanently gain an additional +1 hand size (+2 total, draw 10 cards).", 
            2, VoucherType::PAINT_BRUSH, 10, VoucherColor(0.12f, 0.55f, 0.50f)),

    // Pair 4: Joker Slots
    Voucher(VoucherType::BLANK, "Blank", 
            "Does nothing?", 
            1, VoucherType::NONE, 10, VoucherColor(0.90f, 0.90f, 0.92f)),
    Voucher(VoucherType::ANTIMATTER, "Antimatter", 
            "Permanently gain +1 Joker slot (maximum 6 Jokers).", 
            2, VoucherType::BLANK, 10, VoucherColor(0.55f, 0.25f, 0.75f)),

    // Pair 5: Reroll Cost
    Voucher(VoucherType::REROLL_SURPLUS, "Reroll Surplus", 
            "Rerolls in shop cost $2 less (reduces base cost to $3).", 
            1, VoucherType::NONE, 10, VoucherColor(0.88f, 0.65f, 0.20f)),
    Voucher(VoucherType::REROLL_GLUT, "Reroll Glut", 
            "Rerolls in shop cost an additional $2 less (reduces cost to $1).", 
            2, VoucherType::REROLL_SURPLUS, 10, VoucherColor(0.95f, 0.75f, 0.25f)),

    // Pair 6: Store Discount
    Voucher(VoucherType::CLEARANCE_SALE, "Clearance Sale", 
            "All cards and items in shop are 25% off.", 
            1, VoucherType::NONE, 10, VoucherColor(0.85f, 0.50f, 0.20f)),
    Voucher(VoucherType::LIQUIDATION, "Liquidation", 
            "All cards and items in shop are 50% off.", 
            2, VoucherType::CLEARANCE_SALE, 10, VoucherColor(0.92f, 0.40f, 0.15f)),

    // Pair 7: Overstock
    Voucher(VoucherType::OVERSTOCK, "Overstock", 
            "+1 card slot available in shop (sells 3 Jokers).", 
            1, VoucherType::NONE, 10, VoucherColor(0.25f, 0.68f, 0.38f)),
    Voucher(VoucherType::OVERSTOCK_PLUS, "Overstock Plus", 
            "+1 additional card slot in shop (sells 4 Jokers).", 
            2, VoucherType::OVERSTOCK, 10, VoucherColor(0.18f, 0.52f, 0.28f)),

    // Pair 8: Boss Reroll
    Voucher(VoucherType::DIRECTORS_CUT, "Director's Cut", 
            "Reroll Boss Blind 1 time per Ante for $10 in Choose Blind screen.", 
            1, VoucherType::NONE, 10, VoucherColor(0.70f, 0.35f, 0.65f)),
    Voucher(VoucherType::RETCON, "Retcon", 
            "Reroll Boss Blind unlimited times for $10 per roll.", 
            2, VoucherType::DIRECTORS_CUT, 10, VoucherColor(0.82f, 0.25f, 0.75f))
};

const vector<Voucher>& VoucherDatabase::getAllVouchers() {
    return ALL_VOUCHERS;
}

Voucher VoucherDatabase::getVoucher(VoucherType type) {
    for (const auto& v : ALL_VOUCHERS) {
        if (v.type == type) return v;
    }
    return Voucher();
}

vector<Voucher> VoucherDatabase::getEligibleVouchers(const set<VoucherType>& redeemed) {
    vector<Voucher> eligible;
    for (const auto& v : ALL_VOUCHERS) {
        // Jika sudah ditebus/dibeli, tidak boleh muncul lagi
        if (redeemed.find(v.type) != redeemed.end()) continue;

        if (v.tier == 1) {
            // Tier 1 selalu eligible selama belum dibeli
            eligible.push_back(v);
        } else if (v.tier == 2) {
            // Tier 2 hanya eligible jika Tier 1-nya sudah ditebus
            if (redeemed.find(v.prerequisite) != redeemed.end()) {
                eligible.push_back(v);
            }
        }
    }
    return eligible;
}

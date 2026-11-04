#include "../../include/core/Tag.h"
#include <cstdlib>

TagReward TagFactory::createTag(TagType type) {
    switch (type) {
        case TagType::D6_REROLL_TAG:
            return TagReward(TagType::D6_REROLL_TAG, "D6 Tag", "Rerolls upcoming Boss Blind", "[D6]");
        case TagType::SPEED_TAG:
            return TagReward(TagType::SPEED_TAG, "Speed Tag", "Gives +$15 instant cash", "[$$]");
        case TagType::HANDY_TAG:
            return TagReward(TagType::HANDY_TAG, "Handy Tag", "Gives +$1 per played hand this run", "[HAND]");
        case TagType::GARBAGE_TAG:
            return TagReward(TagType::GARBAGE_TAG, "Garbage Tag", "Gives +$1 per remaining discard", "[BIN]");
        case TagType::ECONOMY_TAG:
            return TagReward(TagType::ECONOMY_TAG, "Economy Tag", "Doubles current cash (max +$20)", "[X2]");
        case TagType::COUPON_TAG:
            return TagReward(TagType::COUPON_TAG, "Coupon Tag", "Gives +$10 shop cash bonus", "[TAG]");
        default:
            return TagReward(TagType::SPEED_TAG, "Speed Tag", "Gives +$15 instant cash", "[$$]");
    }
}

TagReward TagFactory::getRandomTag() {
    int r = rand() % 6;
    return createTag(static_cast<TagType>(r));
}

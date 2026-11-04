#ifndef TAG_H
#define TAG_H

#include <string>

using namespace std;

enum class TagType {
    D6_REROLL_TAG, // Dadu: Reroll upcoming Boss Blind
    SPEED_TAG,     // +$15 cash instan
    HANDY_TAG,     // +$1 cash per played hand run ini
    GARBAGE_TAG,   // +$1 cash per unused discard saat ini (min +$3)
    ECONOMY_TAG,   // Gandakan uang pemain (maks +$20, min +$6)
    COUPON_TAG     // +$10 cash belanja
};

struct TagReward {
    TagType type;
    string name;
    string description;
    string iconSymbol; // "[D6]", "[$15]", "[HAND]", dll

    TagReward(TagType t = TagType::SPEED_TAG, 
              const string& n = "Speed Tag", 
              const string& desc = "Gives +$15 instant cash", 
              const string& icon = "[$15]")
        : type(t), name(n), description(desc), iconSymbol(icon) {}
};

class TagFactory {
public:
    static TagReward createTag(TagType type);
    static TagReward getRandomTag();
};

#endif // TAG_H

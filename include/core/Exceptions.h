#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include <stdexcept>
#include <string>

using namespace std;

// Base Exception untuk semua error internal game
class GameException : public runtime_error {
public:
    explicit GameException(const string& message)
        : runtime_error("[Game Error] " + message) {}
};

// Exception ketika mencoba mengambil kartu dari deck yang sudah kosong
class DeckEmptyException : public GameException {
public:
    DeckEmptyException()
        : GameException("Deck is empty! Cannot draw more cards.") {}
};

// Exception ketika jumlah kartu yang dipilih tidak valid (misal: 0 atau > 5)
class InvalidCardCountException : public GameException {
public:
    explicit InvalidCardCountException(int count, int maxCount = 5)
        : GameException("Invalid card selection count: " + to_string(count) + 
                        " (Allowed: 1 to " + to_string(maxCount) + ")") {}
};

// Exception ketika kartu yang direferensikan tidak ada di tangan pemain
class CardNotFoundException : public GameException {
public:
    explicit CardNotFoundException(const string& cardInfo)
        : GameException("Card not found in player's hand: " + cardInfo) {}
};

// Exception ketika aksi tidak diizinkan pada state tertentu
class InvalidActionException : public GameException {
public:
    explicit InvalidActionException(const string& actionName, const string& reason)
        : GameException("Action '" + actionName + "' rejected: " + reason) {}
};

#endif // EXCEPTIONS_H

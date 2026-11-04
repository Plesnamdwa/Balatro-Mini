#include <GLFW/glfw3.h>
#include <iostream>

#include "../include/ui/GameWindow.h"

// Sertakan implementasi agar dapat di-compile langsung dengan tombol F5
// (Single-File Task) di VS Code
#include "audio/AudioManager.cpp"
#include "core/Blind.cpp"
#include "core/Card.cpp"
#include "core/Deck.cpp"
#include "core/DeckFactory.cpp"
#include "core/GameEngine.cpp"
#include "core/GameState.cpp"
#include "core/PokerEvaluator.cpp"
#include "core/ScoringEngine.cpp"
#include "core/Tag.cpp"
#include "core/Voucher.cpp"
#include "ui/GameWindow.cpp"
#include "ui/PixelFont.cpp"
#include "ui/Renderer.cpp"

using namespace std;

int main() {
  GameWindow app(1024, 720, "Balatro Mini");

  if (!app.init()) {
    cerr << "[FATAL] Gagal membuka jendela game!\n";
    return 1;
  }

  app.run();

  cout << "\nTerima kasih telah bermain Balatro Mini!\n";
  return 0;
}

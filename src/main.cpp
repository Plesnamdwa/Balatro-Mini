#include <GLFW/glfw3.h>
#include <iostream>

#include "../include/ui/GameWindow.h"

// Sertakan implementasi agar dapat di-compile langsung dengan tombol F5 (Single-File Task) di VS Code
#include "core/Card.cpp"
#include "core/Deck.cpp"
#include "core/DeckFactory.cpp"
#include "core/PokerEvaluator.cpp"
#include "core/ScoringEngine.cpp"
#include "core/Blind.cpp"
#include "core/GameState.cpp"
#include "core/GameEngine.cpp"
#include "ui/PixelFont.cpp"
#include "ui/Renderer.cpp"
#include "ui/GameWindow.cpp"

using namespace std;

int main() {
    cout << "==================================================\n";
    cout << "               BALATRO MINI                       \n";
    cout << "==================================================\n";
    cout << " Kontrol Permainan:\n";
    cout << " - Klik Kiri pada Kartu: Pilih / Batalkan Kartu (Maks 5)\n";
    cout << " - Klik Tombol 'PLAY HAND' atau tekan SPACE / ENTER\n";
    cout << " - Klik Tombol 'DISCARD' atau tekan tombol 'D'\n";
    cout << " - Tekan tombol '1' - '8' pada keyboard untuk memilih kartu\n";
    cout << " - Tekan tombol 'R' untuk me-restart run kapan saja\n";
    cout << " - Tekan ESC untuk kembali ke menu / keluar\n";
    cout << "==================================================\n";

    GameWindow app(1024, 720, "Balatro Mini");
    
    if (!app.init()) {
        cerr << "[FATAL] Gagal membuka jendela game!\n";
        return 1;
    }

    app.run();

    cout << "\nTerima kasih telah bermain Balatro Mini!\n";
    return 0;
}

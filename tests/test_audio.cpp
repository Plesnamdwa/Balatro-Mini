#include <iostream>
#include <cassert>
#include <cmath>
#include "../include/audio/AudioManager.h"

using namespace std;

int main() {
    cout << "==========================================================\n";
    cout << "   BALATRO MINI - AUDIO ENGINE & SETTINGS UNIT TEST       \n";
    cout << "==========================================================\n\n";

    auto& audio = AudioManager::getInstance();

    cout << "[TEST 1] Inisialisasi Audio Engine..." << endl;
    bool ok = audio.init();
    cout << " >> Init result: " << (ok ? "SUCCESS" : "FAILED (No audio hardware / dummy mode)") << endl;

    cout << "[TEST 2] Cek File Eksistensi BGM Menu & Game..." << endl;
    bool menuExists = AudioManager::fileExists("assets/audio/bgm/music_menu.mp3");
    bool gameExists = AudioManager::fileExists("assets/audio/bgm/music_game.mp3");
    cout << " >> menuExists: " << (menuExists ? "TRUE" : "FALSE") << ", gameExists: " << (gameExists ? "TRUE" : "FALSE") << endl;
    if (!menuExists || !gameExists) {
        cout << " >> Checking with backslash..." << endl;
        bool menuBS = AudioManager::fileExists("assets\\audio\\bgm\\music_menu.mp3");
        cout << " >> menuBS: " << (menuBS ? "TRUE" : "FALSE") << endl;
    }
    assert(menuExists && "music_menu.mp3 harus ada di assets/audio/bgm/");
    assert(gameExists && "music_game.mp3 harus ada di assets/audio/bgm/");
    cout << " >> [PASS] File music_menu.mp3 dan music_game.mp3 terdeteksi." << endl;

    cout << "[TEST 3] Pengaturan Slider Volume BGM & SFX...\n";
    audio.setBGMVolume(0.50f);
    assert(fabs(audio.getBGMVolume() - 0.50f) < 0.001f);
    audio.setSFXVolume(0.80f);
    assert(fabs(audio.getSFXVolume() - 0.80f) < 0.001f);

    // Clamp boundaries
    audio.setBGMVolume(-0.2f);
    assert(fabs(audio.getBGMVolume() - 0.0f) < 0.001f);
    audio.setBGMVolume(1.5f);
    assert(fabs(audio.getBGMVolume() - 1.0f) < 0.001f);
    cout << " >> [PASS] Volume slider & clamping bekerja akurat.\n\n";

    cout << "[TEST 4] Simulasi Fade Transition BGM...\n";
    audio.playMusic(AudioAssets::BGM_MENU, true, 0.70f, 0.4f);
    for (int i = 0; i < 10; ++i) {
        audio.update(0.05f);
    }
    // Switch to gameplay music
    audio.playMusic(AudioAssets::BGM_GAMEPLAY, true, 0.65f, 0.4f);
    for (int i = 0; i < 20; ++i) {
        audio.update(0.05f);
    }
    cout << " >> [PASS] Fade update berjalan mulus tanpa crash.\n\n";

    cout << "[TEST 5] Pengujian Procedural SFX Audio Synthesizer Fallback...\n";
    // Uji coba memutar semua sound key yang belum memiliki file .mp3 / .wav di disk
    audio.playSound(AudioAssets::SFX_BUTTON, 1.0f);
    audio.playSound(AudioAssets::SFX_CARD_SELECT, 1.0f);
    audio.playSound(AudioAssets::SFX_CARD_DEAL, 0.8f);
    audio.playSound(AudioAssets::SFX_CARD_DISCARD, 0.9f);
    audio.playSound(AudioAssets::SFX_CHIP_POP, 1.0f);
    audio.playSound(AudioAssets::SFX_MULT_POP, 1.0f);
    audio.playSound(AudioAssets::SFX_SCORE_SLAM, 1.0f);
    audio.playSound(AudioAssets::SFX_CRT_OFF, 0.65f);
    audio.playSound(AudioAssets::SFX_WIN_CASHOUT, 1.0f);
    audio.playSound(AudioAssets::SFX_BLIND_SELECT, 1.0f);
    // Verifikasi pemutaran beruntun (multi-channel buffer reuse)
    for (int k = 0; k < 10; ++k) {
        audio.playSound(AudioAssets::SFX_CHIP_POP, 0.5f);
    }
    cout << " >> [PASS] Seluruh 10 Procedural SFX disintesis dan diputar tanpa crash.\n\n";

    audio.shutdown();
    cout << "==========================================================\n";
    cout << "   SEMUA TEST AUDIO & SETTINGS LOLOS 100%!                \n";
    cout << "==========================================================\n";
    return 0;
}

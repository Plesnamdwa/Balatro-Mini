#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include <string>
#include <vector>
#include <map>
#include <memory>

using namespace std;

class AudioManager {
private:
    bool initialized;
    float masterVolume;
    float bgmVolume;
    float sfxVolume;

    string currentBgmPath;
    bool isBgmPlaying;

    struct Impl;
    unique_ptr<Impl> pImpl;

    AudioManager();
    ~AudioManager();

public:
    // Singleton Instance
    static AudioManager& getInstance();

    // Inisialisasi & Shutdown Audio Engine
    bool init();
    void shutdown();

    // Pemutaran Musik Latar Belakang (BGM)
    void playBGM(const string& filePath, bool loop = true, float volume = 0.7f);
    void stopBGM();
    void pauseBGM();
    void resumeBGM();

    // Pemutaran Efek Suara (SFX) Multi-Channel Concurrent
    void playSFX(const string& filePath, float volume = 1.0f);

    // Pengaturan Volume (0.0f s/d 1.0f)
    void setMasterVolume(float volume);
    void setBGMVolume(float volume);
    void setSFXVolume(float volume);

    float getMasterVolume() const { return masterVolume; }
    float getBGMVolume() const { return bgmVolume; }
    float getSFXVolume() const { return sfxVolume; }

    // Pemutaran Musik dengan Fade Out & Fade In Otomatis
    void fadeToBGM(const string& filePath, bool loop = true, float targetVolume = 0.7f, float fadeDuration = 0.8f);
    void update(float dt);

    // Helper pemutar otomatis dengan pengecekan format .wav / .mp3 / .ogg
    void playSound(const string& soundKey, float volume = 1.0f);
    void playMusic(const string& musicKey, bool loop = true, float volume = 0.7f, float fadeDuration = 0.8f);

    // Generator & Pemutar Efek Suara Procedural Sintetis
    void playProceduralSFX(const string& soundKey, float volume = 1.0f);

    // Helper cek keberadaan file audio
    static bool fileExists(const string& path);
};

// Konstanta Path Aset Audio Standar
namespace AudioAssets {
    const string BGM_MENU        = "assets/audio/bgm/music_menu";
    const string BGM_GAMEPLAY    = "assets/audio/bgm/music_game";
    const string BGM_SHOP        = "assets/audio/bgm/music_game";

    const string SFX_BUTTON      = "assets/audio/sfx/button_click";
    const string SFX_CARD_SELECT = "assets/audio/sfx/card_select";
    const string SFX_CARD_DEAL   = "assets/audio/sfx/card_deal";
    const string SFX_CARD_DISCARD= "assets/audio/sfx/card_discard";
    const string SFX_CHIP_POP    = "assets/audio/sfx/chip_pop";
    const string SFX_MULT_POP    = "assets/audio/sfx/mult_trigger";
    const string SFX_SCORE_SLAM  = "assets/audio/sfx/score_slam";
    const string SFX_CRT_OFF     = "assets/audio/sfx/crt_tv";
    const string SFX_WIN_CASHOUT = "assets/audio/sfx/win_cashout";
    const string SFX_BLIND_SELECT= "assets/audio/sfx/blind_select";
}

#endif // AUDIO_MANAGER_H

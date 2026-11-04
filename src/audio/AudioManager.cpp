#define _HAS_STD_BYTE 0
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define MINIAUDIO_IMPLEMENTATION
#include "../../include/audio/miniaudio.h"
#include "../../include/audio/AudioManager.h"
#include <iostream>
#include <fstream>
#include <algorithm>

using namespace std;

// PIMPL Implementation untuk mengisolasi struktur miniaudio
struct AudioManager::Impl {
    ma_engine engine;
    ma_sound bgmSound;
    bool bgmLoaded = false;

    // Pool untuk suara SFX agar multi-channel tidak tumpang tindih
    static const int MAX_SFX_SOUNDS = 24;
    ma_sound sfxSounds[MAX_SFX_SOUNDS];
    bool sfxActive[MAX_SFX_SOUNDS];
    ma_audio_buffer proceduralBuffers[MAX_SFX_SOUNDS];
    bool isProceduralSlot[MAX_SFX_SOUNDS];
    int nextSfxSlot = 0;

    // Cache PCM sample data untuk procedural SFX agar tidak perlu di-generate ulang
    map<string, vector<float>> proceduralCache;

    // Fade state
    enum class FadeState { NONE, FADING_OUT, FADING_IN };
    FadeState fadeState = FadeState::NONE;
    float fadeTimer = 0.0f;
    float fadeDuration = 0.8f;
    float currentBgmTargetVol = 0.7f;
    string nextBgmPath = "";
    bool nextBgmLoop = true;
    float nextBgmTargetVol = 0.7f;

    bool engineInitialized = false;

    Impl() {
        for (int i = 0; i < MAX_SFX_SOUNDS; ++i) {
            sfxActive[i] = false;
            isProceduralSlot[i] = false;
        }
    }

    ~Impl() {
        cleanup();
    }

    void cleanup() {
        if (bgmLoaded) {
            ma_sound_stop(&bgmSound);
            ma_sound_uninit(&bgmSound);
            bgmLoaded = false;
        }
        for (int i = 0; i < MAX_SFX_SOUNDS; ++i) {
            if (sfxActive[i]) {
                ma_sound_stop(&sfxSounds[i]);
                ma_sound_uninit(&sfxSounds[i]);
                if (isProceduralSlot[i]) {
                    ma_audio_buffer_uninit(&proceduralBuffers[i]);
                    isProceduralSlot[i] = false;
                }
                sfxActive[i] = false;
            }
        }
        if (engineInitialized) {
            ma_engine_uninit(&engine);
            engineInitialized = false;
        }
    }
};

AudioManager::AudioManager()
    : initialized(false), masterVolume(1.0f), bgmVolume(0.75f), sfxVolume(0.85f),
      currentBgmPath(""), isBgmPlaying(false), pImpl(make_unique<Impl>()) {}

AudioManager::~AudioManager() {
    shutdown();
}

AudioManager& AudioManager::getInstance() {
    static AudioManager instance;
    return instance;
}

static string getExeDir() {
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    string p(buffer);
    size_t last = p.find_last_of("\\/");
    if (last != string::npos) return p.substr(0, last);
    return ".";
}

bool AudioManager::fileExists(const string& path) {
    if (path.empty()) return false;
    ifstream f(path.c_str(), ios::binary);
    return f.good();
}

// Mencari path asset yang benar-benar bisa dibuka.
// Urutan: cwd -> folder exe -> parent folder exe (hingga 4 tingkat).
// Penting karena exe bisa berada di root (balatro_mini.exe) ATAU di src/ (main.exe via F5/Code Runner),
// sedangkan folder assets/ selalu berada di root project.
static string findAssetPath(const string& relPath) {
    if (relPath.empty()) return "";
    if (AudioManager::fileExists(relPath)) return relPath;

    string dir = getExeDir();
    for (int depth = 0; depth <= 4; ++depth) {
        string candidate = dir + "\\" + relPath;
        if (AudioManager::fileExists(candidate)) return candidate;

        size_t last = dir.find_last_of("\\/");
        if (last == string::npos) break;
        dir = dir.substr(0, last);
    }
    return "";
}

bool AudioManager::init() {
    if (initialized) return true;

    ma_engine_config config = ma_engine_config_init();
    ma_result result = ma_engine_init(&config, &pImpl->engine);
    if (result != MA_SUCCESS) {
        cerr << "[AUDIO] Gagal inisialisasi audio engine miniaudio! Error: " << result << "\n";
        initialized = false;
        return false;
    }

    initialized = true;
    pImpl->engineInitialized = true;
    ma_engine_set_volume(&pImpl->engine, masterVolume);
    cout << "[AUDIO] AudioManager initialized successfully (miniaudio engine active)." << endl;
    return true;
}

void AudioManager::shutdown() {
    if (!initialized) return;
    pImpl->cleanup();
    initialized = false;
    cout << "[AUDIO] AudioManager shutdown." << endl;
}

void AudioManager::playBGM(const string& filePath, bool loop, float volume) {
    if (!initialized && !init()) return;

    // Jika lagu yang sama sudah sedang berputar, tidak perlu restart dari awal
    if (isBgmPlaying && currentBgmPath == filePath) {
        return;
    }

    if (!fileExists(filePath)) {
        cout << "[AUDIO] Info: File BGM '" << filePath << "' belum ditemukan. Melewati BGM." << endl;
        return;
    }

    // Unload BGM sebelumnya jika ada
    if (pImpl->bgmLoaded) {
        ma_sound_stop(&pImpl->bgmSound);
        ma_sound_uninit(&pImpl->bgmSound);
        pImpl->bgmLoaded = false;
        isBgmPlaying = false;
    }

    // Coba inisialisasi file audio (flag 0 untuk decoding stabil)
    ma_result res = ma_sound_init_from_file(&pImpl->engine, filePath.c_str(), 
                                           0, nullptr, nullptr, &pImpl->bgmSound);
    if (res != MA_SUCCESS) {
        // Fallback coba dengan MA_SOUND_FLAG_STREAM
        res = ma_sound_init_from_file(&pImpl->engine, filePath.c_str(), 
                                      MA_SOUND_FLAG_STREAM, nullptr, nullptr, &pImpl->bgmSound);
    }

    if (res != MA_SUCCESS) {
        cerr << "[AUDIO] Gagal memuat BGM dari '" << filePath << "'! Error: " << res << endl;
        return;
    }

    pImpl->bgmLoaded = true;
    currentBgmPath = filePath;
    ma_sound_set_looping(&pImpl->bgmSound, loop ? MA_TRUE : MA_FALSE);
    ma_sound_set_volume(&pImpl->bgmSound, volume * bgmVolume);
    ma_sound_start(&pImpl->bgmSound);
    isBgmPlaying = true;
    cout << "[AUDIO] Memutar BGM: " << filePath << " (Vol: " << (volume * bgmVolume) << ", Loop: " << (loop ? "Yes" : "No") << ")" << endl;
}

void AudioManager::stopBGM() {
    if (!initialized || !pImpl->bgmLoaded) return;
    ma_sound_stop(&pImpl->bgmSound);
    isBgmPlaying = false;
    currentBgmPath = "";
}

void AudioManager::pauseBGM() {
    if (!initialized || !pImpl->bgmLoaded || !isBgmPlaying) return;
    ma_sound_stop(&pImpl->bgmSound);
    isBgmPlaying = false;
}

void AudioManager::resumeBGM() {
    if (!initialized || !pImpl->bgmLoaded || isBgmPlaying) return;
    ma_sound_start(&pImpl->bgmSound);
    isBgmPlaying = true;
}

void AudioManager::playSFX(const string& filePath, float volume) {
    if (!initialized && !init()) return;

    if (!fileExists(filePath)) {
        // Fallback lembut jika pengguna belum menaruh file SFX
        return;
    }

    // Cari slot SFX di ring buffer
    int slot = pImpl->nextSfxSlot;
    pImpl->nextSfxSlot = (pImpl->nextSfxSlot + 1) % Impl::MAX_SFX_SOUNDS;

    if (pImpl->sfxActive[slot]) {
        ma_sound_stop(&pImpl->sfxSounds[slot]);
        ma_sound_uninit(&pImpl->sfxSounds[slot]);
        if (pImpl->isProceduralSlot[slot]) {
            ma_audio_buffer_uninit(&pImpl->proceduralBuffers[slot]);
            pImpl->isProceduralSlot[slot] = false;
        }
        pImpl->sfxActive[slot] = false;
    }

    ma_result res = ma_sound_init_from_file(&pImpl->engine, filePath.c_str(), 
                                           MA_SOUND_FLAG_DECODE, nullptr, nullptr, &pImpl->sfxSounds[slot]);
    if (res == MA_SUCCESS) {
        pImpl->sfxActive[slot] = true;
        ma_sound_set_volume(&pImpl->sfxSounds[slot], volume * sfxVolume);
        ma_sound_start(&pImpl->sfxSounds[slot]);
    }
}

// ==============================================================================
// PROCEDURAL AUDIO SYNTHESIZER
// Menghasilkan efek suara dinamis Balatro-style secara matematis (sine, chirp, noise, pop)
// saat file audio fisik (.wav / .mp3 / .ogg) belum disediakan di folder sfx.
// ==============================================================================
static vector<float> synthesizeSFX(const string& soundKey) {
    const int sampleRate = 44100;
    const float PI = 3.14159265358979323846f;
    vector<float> buffer;

    // Normalisasi soundKey ke lowercase untuk pencocokan kata kunci fleksibel
    string key = soundKey;
    for (char& c : key) c = (char)tolower(c);

    if (key.find("button") != string::npos) {
        // SFX_BUTTON: Klik kayu/UI lembut (Soft Wood/UI Click)
        // Pitch drop cepat 750 Hz -> 320 Hz, durasi ~45ms
        float duration = 0.045f;
        int totalFrames = static_cast<int>(sampleRate * duration);
        buffer.resize(totalFrames);
        float phase = 0.0f;
        for (int i = 0; i < totalFrames; ++i) {
            float t = (float)i / totalFrames;
            float freq = 750.0f - (430.0f * t);
            phase += (2.0f * PI * freq) / sampleRate;
            float env = 1.0f - t;
            env = env * env; // exponential decay
            buffer[i] = sin(phase) * env * 0.45f;
        }
    }
    else if (key.find("card_select") != string::npos) {
        // SFX_CARD_SELECT: Sentuhan kartu/jentikan jernih (Card Select Snap)
        // Chirp naik 480 Hz -> 880 Hz, durasi ~55ms
        float duration = 0.055f;
        int totalFrames = static_cast<int>(sampleRate * duration);
        buffer.resize(totalFrames);
        float phase = 0.0f;
        for (int i = 0; i < totalFrames; ++i) {
            float t = (float)i / totalFrames;
            float freq = 480.0f + (400.0f * t);
            phase += (2.0f * PI * freq) / sampleRate;
            float env = 1.0f - t;
            env = env * env;
            // Paduan nada fundamental dan harmonik oktav lembut
            buffer[i] = (sin(phase) * 0.7f + sin(phase * 2.0f) * 0.3f) * env * 0.50f;
        }
    }
    else if (key.find("card_deal") != string::npos) {
        // SFX_CARD_DEAL: Desiran gesekan kartu kertas cepat (Card Deal Swoosh)
        // Filtered white noise dengan envelope segitiga cepat, durasi ~70ms
        float duration = 0.070f;
        int totalFrames = static_cast<int>(sampleRate * duration);
        buffer.resize(totalFrames);
        float filterState = 0.0f;
        for (int i = 0; i < totalFrames; ++i) {
            float t = (float)i / totalFrames;
            float white = ((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f;
            // Low-pass filter sederhana
            filterState += (white - filterState) * 0.28f;
            float env = (t < 0.2f) ? (t / 0.2f) : ((1.0f - t) / 0.8f);
            buffer[i] = filterState * env * 0.40f;
        }
    }
    else if (key.find("card_discard") != string::npos) {
        // SFX_CARD_DISCARD: Kibasan buang kartu (Card Discard Flutter)
        // Chirp turun 520 Hz -> 180 Hz dipadu noise gesekan, durasi ~95ms
        float duration = 0.095f;
        int totalFrames = static_cast<int>(sampleRate * duration);
        buffer.resize(totalFrames);
        float phase = 0.0f;
        float noiseState = 0.0f;
        for (int i = 0; i < totalFrames; ++i) {
            float t = (float)i / totalFrames;
            float freq = 520.0f - (340.0f * t);
            phase += (2.0f * PI * freq) / sampleRate;
            float white = ((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f;
            noiseState += (white - noiseState) * 0.20f;
            float env = (1.0f - t) * (1.0f - t);
            buffer[i] = (sin(phase) * 0.6f + noiseState * 0.4f) * env * 0.45f;
        }
    }
    else if (key.find("chip_pop") != string::npos) {
        // SFX_CHIP_POP: Letupan Chip Balatro yang ceria (Satisfying Chip Pop)
        // Sine ping beresonansi pada 920 Hz meluruh ke 580 Hz, durasi ~80ms
        float duration = 0.080f;
        int totalFrames = static_cast<int>(sampleRate * duration);
        buffer.resize(totalFrames);
        float phase = 0.0f;
        for (int i = 0; i < totalFrames; ++i) {
            float t = (float)i / totalFrames;
            float freq = 920.0f - (340.0f * t * t);
            phase += (2.0f * PI * freq) / sampleRate;
            float env = (t < 0.05f) ? (t / 0.05f) : exp(-t * 6.5f);
            buffer[i] = (sin(phase) + 0.25f * sin(phase * 2.0f)) * env * 0.55f;
        }
    }
    else if (key.find("mult") != string::npos) {
        // SFX_MULT_POP: Letupan Pengali Merah dramatis (Punchy Mult Trigger)
        // Chirp naik-tajam 320 Hz -> 840 Hz dengan distorsi lembut, durasi ~110ms
        float duration = 0.110f;
        int totalFrames = static_cast<int>(sampleRate * duration);
        buffer.resize(totalFrames);
        float phase = 0.0f;
        for (int i = 0; i < totalFrames; ++i) {
            float t = (float)i / totalFrames;
            float freq = 320.0f + (520.0f * sqrt(t));
            phase += (2.0f * PI * freq) / sampleRate;
            float env = (t < 0.1f) ? (t / 0.1f) : exp(-t * 4.5f);
            float s = sin(phase) * 1.2f;
            // Soft-saturation clipping untuk punchy tone
            s = max(-1.0f, min(1.0f, s));
            buffer[i] = s * env * 0.55f;
        }
    }
    else if (key.find("score_slam") != string::npos) {
        // SFX_SCORE_SLAM: Dentuman Total Skor Masuk (Heavy Score Slam)
        // Sub-bass 150 Hz meluncur ke 45 Hz + noise impact, durasi ~280ms
        float duration = 0.280f;
        int totalFrames = static_cast<int>(sampleRate * duration);
        buffer.resize(totalFrames);
        float phase = 0.0f;
        float noiseState = 0.0f;
        for (int i = 0; i < totalFrames; ++i) {
            float t = (float)i / totalFrames;
            float freq = 150.0f * (1.0f - 0.70f * t);
            phase += (2.0f * PI * freq) / sampleRate;
            float white = ((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f;
            noiseState += (white - noiseState) * 0.15f;
            float env = exp(-t * 5.0f);
            float noiseEnv = exp(-t * 18.0f); // noise punch hanya di awal
            float s = (sin(phase) * 0.75f + noiseState * noiseEnv * 0.5f) * env;
            buffer[i] = s * 0.65f;
        }
    }
    else if (key.find("crt") != string::npos) {
        // SFX_CRT_OFF: Efek Televisi Jadul Mati / Nyala (CRT De-energize Zap)
        // High-pitched zap 2400 Hz meluncur turun drastis ke 60 Hz, durasi ~180ms
        float duration = 0.180f;
        int totalFrames = static_cast<int>(sampleRate * duration);
        buffer.resize(totalFrames);
        float phase = 0.0f;
        for (int i = 0; i < totalFrames; ++i) {
            float t = (float)i / totalFrames;
            float freq = 2400.0f * exp(-t * 8.0f) + 60.0f;
            phase += (2.0f * PI * freq) / sampleRate;
            float env = (t < 0.02f) ? (t / 0.02f) : exp(-t * 5.0f);
            buffer[i] = sin(phase) * env * 0.50f;
        }
    }
    else if (key.find("cashout") != string::npos || key.find("win") != string::npos) {
        // SFX_WIN_CASHOUT: Arpeggio Kemenangan Koin Kasir (Coin Cash Out Arpeggio)
        // Nada C5 (523Hz) -> E5 (659Hz) -> G5 (784Hz) -> C6 (1046Hz), durasi ~300ms
        float duration = 0.320f;
        int totalFrames = static_cast<int>(sampleRate * duration);
        buffer.resize(totalFrames);
        float notes[4] = { 523.25f, 659.25f, 783.99f, 1046.50f };
        int framesPerNote = totalFrames / 4;
        for (int n = 0; n < 4; ++n) {
            float phase = 0.0f;
            float freq = notes[n];
            for (int f = 0; f < framesPerNote; ++f) {
                int idx = n * framesPerNote + f;
                if (idx >= totalFrames) break;
                float tNote = (float)f / framesPerNote;
                phase += (2.0f * PI * freq) / sampleRate;
                float env = (tNote < 0.1f) ? (tNote / 0.1f) : exp(-tNote * 4.0f);
                buffer[idx] = (sin(phase) * 0.7f + sin(phase * 2.0f) * 0.3f) * env * 0.55f;
            }
        }
    }
    else if (key.find("blind") != string::npos) {
        // SFX_BLIND_SELECT: Seleksi Blind Menantang (Bold Blind Lock-In)
        // Dua nada power chord F#3 (185Hz) + C#4 (277Hz), durasi ~140ms
        float duration = 0.140f;
        int totalFrames = static_cast<int>(sampleRate * duration);
        buffer.resize(totalFrames);
        float phase1 = 0.0f, phase2 = 0.0f;
        for (int i = 0; i < totalFrames; ++i) {
            float t = (float)i / totalFrames;
            phase1 += (2.0f * PI * 185.0f) / sampleRate;
            phase2 += (2.0f * PI * 277.0f) / sampleRate;
            float env = (t < 0.08f) ? (t / 0.08f) : exp(-t * 5.5f);
            buffer[i] = (sin(phase1) * 0.5f + sin(phase2) * 0.5f) * env * 0.55f;
        }
    }
    else {
        // Default Generic Click: 600 Hz -> 200 Hz, durasi ~40ms
        float duration = 0.040f;
        int totalFrames = static_cast<int>(sampleRate * duration);
        buffer.resize(totalFrames);
        float phase = 0.0f;
        for (int i = 0; i < totalFrames; ++i) {
            float t = (float)i / totalFrames;
            float freq = 600.0f - (400.0f * t);
            phase += (2.0f * PI * freq) / sampleRate;
            float env = 1.0f - t;
            buffer[i] = sin(phase) * (env * env) * 0.40f;
        }
    }

    return buffer;
}

void AudioManager::playProceduralSFX(const string& soundKey, float volume) {
    if (!initialized && !init()) return;

    // Cek apakah sample PCM sudah ada di cache, jika belum lakukan sintesis
    auto it = pImpl->proceduralCache.find(soundKey);
    if (it == pImpl->proceduralCache.end()) {
        pImpl->proceduralCache[soundKey] = synthesizeSFX(soundKey);
        it = pImpl->proceduralCache.find(soundKey);
    }

    const vector<float>& pcmData = it->second;
    if (pcmData.empty()) return;

    // Cari slot SFX di ring buffer
    int slot = pImpl->nextSfxSlot;
    pImpl->nextSfxSlot = (pImpl->nextSfxSlot + 1) % Impl::MAX_SFX_SOUNDS;

    if (pImpl->sfxActive[slot]) {
        ma_sound_stop(&pImpl->sfxSounds[slot]);
        ma_sound_uninit(&pImpl->sfxSounds[slot]);
        if (pImpl->isProceduralSlot[slot]) {
            ma_audio_buffer_uninit(&pImpl->proceduralBuffers[slot]);
            pImpl->isProceduralSlot[slot] = false;
        }
        pImpl->sfxActive[slot] = false;
    }

    ma_audio_buffer_config bufConfig = ma_audio_buffer_config_init(
        ma_format_f32, 1, pcmData.size(), pcmData.data(), nullptr);

    ma_result res = ma_audio_buffer_init(&bufConfig, &pImpl->proceduralBuffers[slot]);
    if (res != MA_SUCCESS) {
        return;
    }

    res = ma_sound_init_from_data_source(&pImpl->engine, &pImpl->proceduralBuffers[slot],
                                         0, nullptr, &pImpl->sfxSounds[slot]);
    if (res == MA_SUCCESS) {
        pImpl->sfxActive[slot] = true;
        pImpl->isProceduralSlot[slot] = true;
        ma_sound_set_volume(&pImpl->sfxSounds[slot], volume * sfxVolume);
        ma_sound_start(&pImpl->sfxSounds[slot]);
    } else {
        ma_audio_buffer_uninit(&pImpl->proceduralBuffers[slot]);
    }
}

void AudioManager::setMasterVolume(float volume) {
    masterVolume = max(0.0f, min(1.0f, volume));
    if (initialized) {
        ma_engine_set_volume(&pImpl->engine, masterVolume);
    }
}

void AudioManager::setBGMVolume(float volume) {
    bgmVolume = max(0.0f, min(1.0f, volume));
    if (initialized && pImpl->bgmLoaded && pImpl->fadeState == Impl::FadeState::NONE) {
        ma_sound_set_volume(&pImpl->bgmSound, pImpl->currentBgmTargetVol * bgmVolume);
    }
}

void AudioManager::setSFXVolume(float volume) {
    sfxVolume = max(0.0f, min(1.0f, volume));
}

// Mencari file dengan urutan prioritas (tanpa ekstensi) -> .mp3 -> .wav -> .ogg
// Hasilnya adalah path yang benar-benar bisa dibuka (absolut jika perlu).
static string resolveAudioPath(const string& baseKey) {
    if (baseKey.empty()) return "";

    const char* exts[] = { "", ".mp3", ".wav", ".ogg" };
    for (const char* ext : exts) {
        string found = findAssetPath(baseKey + ext);
        if (!found.empty()) return found;
    }
    return "";
}

void AudioManager::playSound(const string& soundKey, float volume) {
    string resolved = resolveAudioPath(soundKey);
    if (!resolved.empty()) {
        playSFX(resolved, volume);
    } else {
        // PROSEDURAL FALLBACK: Jika file fisik tidak ditemukan, langsung sintesiskan efek suara
        playProceduralSFX(soundKey, volume);
    }
}

void AudioManager::fadeToBGM(const string& filePath, bool loop, float targetVolume, float fadeDuration) {
    if (!initialized && !init()) return;

    if (currentBgmPath == filePath && isBgmPlaying) {
        return; // Sudah memainkan file yang sama
    }

    if (!pImpl->bgmLoaded || !isBgmPlaying) {
        // Jika belum ada musik bermain (pertama kali game dinyalakan),
        // langsung putar dengan target volume penuh agar terdengar jelas seketika!
        playBGM(filePath, loop, targetVolume);
        pImpl->fadeState = Impl::FadeState::NONE;
        pImpl->fadeTimer = 0.0f;
        pImpl->currentBgmTargetVol = targetVolume;
        return;
    }

    // Jika sedang bermain musik lain, fade out dulu lalu sambung musik baru
    pImpl->nextBgmPath = filePath;
    pImpl->nextBgmLoop = loop;
    pImpl->nextBgmTargetVol = targetVolume;
    pImpl->fadeState = Impl::FadeState::FADING_OUT;
    pImpl->fadeTimer = 0.0f;
    pImpl->fadeDuration = max(0.1f, fadeDuration);
}

void AudioManager::update(float dt) {
    if (!initialized || pImpl->fadeState == Impl::FadeState::NONE) return;

    pImpl->fadeTimer += dt;
    float progress = min(1.0f, pImpl->fadeTimer / pImpl->fadeDuration);

    if (pImpl->fadeState == Impl::FadeState::FADING_OUT) {
        // Turunkan volume secara mulus (fade out)
        float curVol = (1.0f - progress) * pImpl->currentBgmTargetVol * bgmVolume;
        if (pImpl->bgmLoaded) {
            ma_sound_set_volume(&pImpl->bgmSound, max(0.0f, curVol));
        }

        if (progress >= 1.0f) {
            // Selesai fade out, ganti ke BGM baru
            string nextPath = pImpl->nextBgmPath;
            bool nextLoop = pImpl->nextBgmLoop;
            float nextVol = pImpl->nextBgmTargetVol;
            pImpl->nextBgmPath = "";

            if (!nextPath.empty()) {
                playBGM(nextPath, nextLoop, 0.0f);
                pImpl->fadeState = Impl::FadeState::FADING_IN;
                pImpl->fadeTimer = 0.0f;
                pImpl->currentBgmTargetVol = nextVol;
            } else {
                stopBGM();
                pImpl->fadeState = Impl::FadeState::NONE;
            }
        }
    } else if (pImpl->fadeState == Impl::FadeState::FADING_IN) {
        // Naikkan volume secara mulus (fade in)
        float curVol = progress * pImpl->currentBgmTargetVol * bgmVolume;
        if (pImpl->bgmLoaded) {
            ma_sound_set_volume(&pImpl->bgmSound, curVol);
        }

        if (progress >= 1.0f) {
            pImpl->fadeState = Impl::FadeState::NONE;
            if (pImpl->bgmLoaded) {
                ma_sound_set_volume(&pImpl->bgmSound, pImpl->currentBgmTargetVol * bgmVolume);
            }
        }
    }
}

void AudioManager::playMusic(const string& musicKey, bool loop, float volume, float fadeDuration) {
    string resolved = resolveAudioPath(musicKey);
    if (!resolved.empty()) {
        fadeToBGM(resolved, loop, volume, fadeDuration);
    } else {
        cerr << "[AUDIO] PERINGATAN: BGM '" << musicKey << "' tidak ditemukan (cwd/exe dir: " << getExeDir() << ")" << endl;
    }
}

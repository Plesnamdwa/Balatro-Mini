# Balatro Mini (C++ & OpenGL)

Sebuah implementasi game poker roguelike yang terinspirasi oleh **Balatro**, dibangun menggunakan **C++17** dan **OpenGL 2D (GLFW)** dengan penerapan estetika **Low-Fidelity**, **Interaktivitas Penuh**, **Robust Error Handling**, serta **Pola Desain (Design Patterns)** berorientasi objek yang kuat.

---

## 🎮 Fitur Utama

- **Formula Skor Khas Balatro**: $(\text{Base Chips} + \sum \text{Card Chips}) \times (\text{Base Mult} + \text{Bonus Mult})$.
- **9 Kombinasi Poker Resmi**:
  - *High Card*, *Pair*, *Two Pair*, *Three of a Kind*, *Straight* (termasuk *Ace-Low Straight*), *Flush*, *Full House*, *Four of a Kind*, *Straight Flush*.
  - Pemisahan *Scoring Cards* (hanya kartu pembentuk kombinasi yang menyumbang Chips).
- **Alur Siklus Ronde & Blind**:
  - **Small Blind**: Target dasar ($1.0\times$), Reward $\$3$.
  - **Big Blind**: Target lebih tinggi ($1.5\times$), Reward $\$4$.
  - **Boss Blind**: Target berat ($2.0\times$), Reward $\$5$, dengan **Efek Debuff** aktif (misal: *All Heart cards debuffed / 0 Chips* atau *-1 Hand Size*).
- **Ante Progression**: Mengalahkan Boss Blind menaikkan level **Ante (1 s.d. 8)** dengan skala target skor yang meningkat.
- **Tampilan Low-Fidelity & Interaktif**:
  - Meja kasino *Felt-Green* retro 60 FPS.
  - Kartu bergerak naik (*elevation*) saat dipilih (maksimal 5 kartu).
  - *Live Poker Hand Preview* (menampilkan kombinasi kartu dan estimasi skor Chips $\times$ Mult sebelum dimainkan).
  - Tipografi retro menggunakan *Monochrome 8x8 Bitmap Pixel Font* mandiri.
- **Robust Error Handling**:
  - Sistem *Guard Clauses* dan *Custom Exception Hierarchy*.
  - Menolak aksi ilegal (seleksi $>5$ kartu, bermain saat kuota habis, discard saat kuota habis) dengan banner notifikasi retro (*Toast*) tanpa *crash* atau *freeze*.

---

## 🏛️ Pola Desain (Design Patterns) yang Diterapkan

Proyek ini dirancang sebagai studi kasus arsitektur perangkat lunak yang bersih:

1. **Factory Method Pattern (`IDeckFactory`, `StandardDeckFactory`)**:
   - Memisahkan pembuatan 52 kartu standar poker dan pengacakan *Fisher-Yates* dari struktur data `Deck`.
2. **Strategy Pattern (`IPokerEvaluatorStrategy`, `StandardPokerEvaluator`)**:
   - Algoritma evaluasi kombinasi poker dipisahkan menjadi strategi yang dapat ditukar secara dinamis saat runtime oleh `ScoringEngine`.
3. **State Pattern (`IGameState`, `MainMenuState`, `BlindSelectState`, `SelectingCardState`, `RoundWonState`, `GameOverState`)**:
   - Mengelola alur permainan secara modular dari Menu Utama $\rightarrow$ Pilih Blind $\rightarrow$ Main Kartu $\rightarrow$ Cash Out $\rightarrow$ Game Over.
4. **Defensive Programming & Exception Hierarchy (`GameException`)**:
   - Hierarki error terstruktur (`DeckEmptyException`, `InvalidCardCountException`, `InvalidActionException`) untuk menjamin kestabilan runtime.

---

## 📂 Struktur Direktori

```text
balatro-mini/
├── include/
│   ├── core/           # Business logic & Design Patterns (C++ Murni)
│   │   ├── Card.h
│   │   ├── Deck.h
│   │   ├── DeckFactory.h
│   │   ├── PokerHand.h
│   │   ├── PokerEvaluator.h
│   │   ├── ScoringEngine.h
│   │   ├── Blind.h
│   │   ├── GameState.h
│   │   ├── GameEngine.h
│   │   └── Exceptions.h
│   ├── ui/             # Presentation layer (OpenGL / GLFW)
│   │   ├── PixelFont.h
│   │   ├── Renderer.h
│   │   └── GameWindow.h
│   └── GLFW/           # Header library GLFW
├── src/
│   ├── core/           # Implementasi modul core
│   ├── ui/             # Implementasi renderer, font, dan window
│   └── main.cpp        # Entry point aplikasi
├── tests/              # Unit test per segmen
│   ├── test_segment1.cpp
│   ├── test_segment2.cpp
│   └── test_segment3.cpp
├── lib/                # Static library GLFW (libglfw3.a)
├── run_game.bat        # Launcher instan
└── build_and_run.bat   # Skrip kompilasi otomatis
```

---

## 🚀 Cara Menjalankan Game

### Opsi 1: Menggunakan Release Portable (Tanpa Install Compiler)
1. Unduh file `Balatro-Mini-v1.0.0-Windows.zip` dari tab **Releases**.
2. Ekstrak zip ke folder pilihan Anda.
3. Klik ganda `run_game.bat` atau `balatro_mini.exe`.

### Opsi 2: Kompilasi dari Source Code (MinGW g++)
Pastikan compiler `g++` (MinGW-w64) tersedia di sistem Anda, lalu jalankan:
```cmd
cd balatro-mini
build_and_run.bat
```

Atau menggunakan perintah kompilasi langsung:
```cmd
g++ -std=c++17 -Wall -Wextra -Iinclude src/main.cpp -Llib -lglfw3 -lopengl32 -lgdi32 -o balatro_mini.exe
```

---

## ⌨️ Kontrol Permainan

| Aksi | Mouse | Keyboard Shortcut |
| :--- | :--- | :--- |
| **Pilih / Batalkan Kartu** | Klik Kiri pada Kartu | Tombol `1` sampai `8` |
| **Play Hand** | Klik Tombol `PLAY HAND` | `Space` atau `Enter` |
| **Discard (Buang Kartu)** | Klik Tombol `DISCARD` | Tombol `D` |
| **Lanjut / Cash Out** | Klik Tombol `CASH OUT` | `Space` atau `Enter` |
| **Restart Run** | Klik Tombol `RESTART` | Tombol `R` |
| **Keluar / Menu** | Klik Tombol `MAIN MENU` | `ESC` |

---

## 📜 Lisensi
Dibuat untuk keperluan akademik / tugas Desain Pattern. Lisensi bersifat open-source untuk tujuan pembelajaran.

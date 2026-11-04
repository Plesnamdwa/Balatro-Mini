# PANDUAN IMPOR AUDIO (BGM & SFX) - BALATRO MINI

Sistem audio game telah siap 100%! Anda dapat memasukkan file musik atau efek suara berkualitas tinggi (format `.wav`, `.mp3`, atau `.ogg`) ke dalam folder ini.

---

## 1. Musik Latar Belakang (BGM)
Folder: `assets/audio/bgm/`

Taruh file musik dengan salah satu nama berikut:
- `music_menu.wav` / `music_menu.mp3` / `music_menu.ogg` : Musik saat di Menu Utama & Layar Choose Blind
- `music_game.wav` / `music_game.mp3` / `music_game.ogg` : Musik saat ronde bermain kartu aktif
- `music_shop.wav` / `music_shop.mp3` / `music_shop.ogg` : Musik saat berada di toko (The Shop)

*(Game akan otomatis mendeteksi format .wav, .mp3, atau .ogg dan memutarnya secara looping halus).*

---

## 2. Efek Suara (SFX)
Folder: `assets/audio/sfx/`

Taruh file efek suara dengan salah satu nama berikut:
- `button_click.wav` / `.mp3` : Suara saat tombol UI ditekan (Start, Play, Discard, dll)
- `card_select.wav` / `.mp3`  : Suara saat memilih / melepas kartu
- `card_deal.wav` / `.mp3`    : Suara kartu meluncur masuk dari bawah layar
- `card_discard.wav` / `.mp3` : Suara kartu meluncur dibuang ke samping kanan
- `chip_pop.wav` / `.mp3`     : Suara punchy pop scoring bertahap (+Chips kartu)
- `mult_trigger.wav` / `.mp3` : Suara pengali Mult merah berdenyut
- `score_slam.wav` / `.mp3`   : Suara tubrukan skor akhir & screen shake
- `crt_tv.wav` / `.mp3`       : Suara perpindahan layar TV tabung retro
- `blind_select.wav` / `.mp3` : Suara memilih tantangan Blind
- `win_cashout.wav` / `.mp3`  : Suara koin saat menang ronde / Cash Out

---

## Catatan Penting
- Jika suatu file belum Anda masukkan, game **TIDAK AKAN CRASH** (game akan melewatinya dengan aman).
- Game mendukung multi-channel playback sehingga suara kartu yang beruntun tidak akan memotong musik BGM Anda!

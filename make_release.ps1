$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.IO.Compression.FileSystem

$base = "D:\VSCode\Antigravity\Desain Pattern\balatro-mini"
$pkg = "$base\Balatro-Mini-Release"
$zipV110 = "$base\Balatro-Mini-v1.1.0-Windows.zip"
$zipGeneric = "$base\Balatro-Mini-Windows.zip"
$hasilZipDir = "D:\VSCode\Antigravity\Desain Pattern\Hasil Zip"

Write-Host ">>> Membersihkan folder sementara..." -ForegroundColor Cyan
if (Test-Path $pkg) { Remove-Item $pkg -Recurse -Force }
New-Item -ItemType Directory -Path $pkg | Out-Null

# 1. Salin executable utama
Write-Host ">>> Menyalin balatro_mini.exe..." -ForegroundColor Cyan
Copy-Item "$base\balatro_mini.exe" -Destination "$pkg\balatro_mini.exe" -Force

# 2. Salin folder assets (BGM & SFX)
Write-Host ">>> Menyalin folder assets (Audio & SFX)..." -ForegroundColor Cyan
Copy-Item "$base\assets" -Destination "$pkg\assets" -Recurse -Force

# 3. Buat runner batch script mandiri
$runnerScript = @"
@echo off
cd /d "%~dp0"
echo ==================================================
echo       MENJALANKAN BALATRO MINI (C++ OPENGL)
echo ==================================================
start "" "%~dp0balatro_mini.exe"
"@
Set-Content -Path "$pkg\run_game.bat" -Value $runnerScript

# 4. Buat file panduan README.txt
$readmeText = @"
==================================================
                 BALATRO MINI
==================================================
Versi    : v1.1.0 (Windows x64 Standalone Release)
Bahasa   : C++17 (OpenGL 2D Low-Fidelity)
Platform : Windows 10 / 11 (64-bit)

CARA MENJALANKAN:
1. Ekstrak seluruh isi file ZIP ini ke folder pilihan Anda.
2. Klik ganda 'run_game.bat' atau 'balatro_mini.exe'.
(Pastikan folder 'assets' berada di sebelah balatro_mini.exe agar musik dan SFX aktif)

KONTROL PERMAINAN:
- Klik Kiri pada Kartu: Pilih / Batalkan kartu (Maksimal 5 kartu)
- Tombol 'PLAY HAND'   : Mainkan kartu terpilih untuk kalkulasi skor
- Tombol 'DISCARD'     : Buang kartu terpilih dan ambil kartu baru
- Tombol 'CASH OUT'    : Ambil hadiah uang dan lanjut ke Toko / Blind berikutnya
- Keyboard [1] - [8]   : Shortcut memilih kartu 1 sampai 8
- Keyboard [9]         : Shortcut memilih kartu ke-9 (jika memiliki voucher Paint Brush)
- Keyboard [0]         : Shortcut memilih kartu ke-10 (jika memiliki voucher Palette)
- Space / Enter        : Shortcut Play Hand / Select Blind / Cash Out
- Tombol 'D'           : Shortcut Discard kartu
- Tombol 'R'           : Shortcut Restart Run
- Tombol 'ESC'         : Buka Run Info / Kembali ke Menu Utama

FITUR LENGKAP:
- Scoring Balatro Akurat: (Base Chips + Card Chips) x (Base Mult + Joker Mult)
- 9 Kombinasi Poker Resmi (High Card s.d. Straight Flush) dengan Live Preview
- Siklus Ante 1 - 8 & 3 Jenis Blind (Small Blind, Big Blind, Boss Blind)
- Boss Blind Debuff (The Heart, The Spade, Hand Size reduction, dll.)
- Sistem Skip Blind dengan Hadiah Tag & Info Hover
- Toko (Shop): Beli Joker, Voucher, Reroll item, dan Jual (Sell) Joker
- 16 Vouchers Unik (8 Tier 1 & 8 Tier 2) dengan efek pasif permanen
- Tata letak kartu dinamis: Otomatis merapat rapi saat kapasitas tangan bertambah ke 9 dan 10 kartu
- Menu Run Info Modal: Tab Poker Hands, Blinds Tracker, dan Koleksi Voucher
- Audio & Retro SFX: Backsound BGM (Menu, Gameplay, Shop) & Sound Effect interaktif
- Desain Pattern OOP Bersih: State Machine, Strategy, Factory Method, Singleton, Defensive Guards
==================================================
"@
Set-Content -Path "$pkg\README.txt" -Value $readmeText -Encoding UTF8

# 5. Salin README.md proyek jika ada
if (Test-Path "$base\README.md") {
    Copy-Item "$base\README.md" -Destination "$pkg\README.md" -Force
}

Start-Sleep -Milliseconds 500

# 6. Kompresi ke file ZIP menggunakan .NET ZipFile
Write-Host ">>> Mengompresi paket ke file ZIP..." -ForegroundColor Cyan
if (Test-Path $zipV110) { Remove-Item $zipV110 -Force }
if (Test-Path $zipGeneric) { Remove-Item $zipGeneric -Force }

[System.IO.Compression.ZipFile]::CreateFromDirectory($pkg, $zipV110, [System.IO.Compression.CompressionLevel]::Optimal, $false)
Copy-Item $zipV110 -Destination $zipGeneric -Force

# 7. Salin juga ke folder Hasil Zip jika ada
if (Test-Path $hasilZipDir) {
    Copy-Item $zipV110 -Destination "$hasilZipDir\Balatro-Mini-v1.1.0-Windows.zip" -Force
    Copy-Item $zipGeneric -Destination "$hasilZipDir\Balatro-Mini-Windows.zip" -Force
    Write-Host ">>> Berhasil disalin ke: $hasilZipDir" -ForegroundColor Green
}

# 8. Bersihkan folder sementara
Remove-Item $pkg -Recurse -Force

Write-Host ">>> Sukses! File rilis siap diunggah ke GitHub Release:" -ForegroundColor Green
Get-Item $zipV110, $zipGeneric | Select-Object Name, Length, LastWriteTime

$ErrorActionPreference = "Stop"

$base = "D:\VSCode\Antigravity\Desain Pattern\balatro-mini"
$pkg = "$base\Balatro-Mini-v1.0.0-Windows"
$zipFile = "$base\Balatro-Mini-v1.0.0-Windows.zip"
$hasilZip = "D:\VSCode\Antigravity\Desain Pattern\Hasil Zip\Balatro-Mini-v1.0.0-Windows.zip"

if (Test-Path $pkg) { Remove-Item $pkg -Recurse -Force }
New-Item -ItemType Directory -Path $pkg | Out-Null

Copy-Item "$base\balatro_mini.exe" -Destination "$pkg\balatro_mini.exe" -Force
Copy-Item "$base\run_game.bat" -Destination "$pkg\run_game.bat" -Force

$readmeText = @"
==================================================
                 BALATRO MINI
==================================================
Versi    : v1.0.0 (Windows x64 Standalone)
Bahasa   : C++17 (OpenGL 2D Low-Fidelity)

CARA MENJALANKAN:
1. Ekstrak file zip ini ke folder mana saja.
2. Klik ganda 'run_game.bat' atau 'balatro_mini.exe'.

KONTROL:
- Klik Kiri pada Kartu: Pilih / Batalkan (Maksimal 5 kartu)
- Tombol 'PLAY HAND'   : Mainkan kartu untuk menghitung skor
- Tombol 'DISCARD'     : Buang kartu dan tarik pengganti dari deck
- Tombol 'CASH OUT'    : Ambil hadiah uang dan lanjut ke Blind berikutnya
- Keyboard [1] - [8]   : Shortcut memilih kartu 1 sampai 8
- Space / Enter        : Shortcut Play Hand / Select Blind / Cash Out
- Tombol 'D'           : Shortcut Discard
- Tombol 'R'           : Shortcut Restart Run
- Tombol 'ESC'         : Kembali ke Menu Utama / Keluar

FITUR GAME:
- Formula Balatro : Chips x Mult
- 9 Kombinasi Poker Resmi (High Card s.d. Straight Flush)
- Siklus Ante & Blind: Small Blind, Big Blind, dan Boss Blind
- Efek Boss Debuff (The Heart, The Spade, Hand Size reduction)
- Low-Fidelity Retro Typography & Responsive UI
- Robust Error Handling & Action Boundary Guards
==================================================
"@

Set-Content -Path "$pkg\README.txt" -Value $readmeText

if (Test-Path $zipFile) { Remove-Item $zipFile -Force }

Compress-Archive -Path "$pkg\*" -DestinationPath $zipFile -Force

if (Test-Path "D:\VSCode\Antigravity\Desain Pattern\Hasil Zip") {
    Copy-Item $zipFile -Destination $hasilZip -Force
}

Remove-Item $pkg -Recurse -Force

Get-Item $zipFile | Select-Object Name, Length, LastWriteTime

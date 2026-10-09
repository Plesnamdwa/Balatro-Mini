$srcDir = "D:\VSCode\Antigravity\Desain Pattern\balatro-mini"
$staging = "$srcDir\release_staging"
$zipPath = "$srcDir\Balatro-Mini-v1.0.0-Windows.zip"
$hasilZipDir = "D:\VSCode\Antigravity\Desain Pattern\Hasil Zip"

if (Test-Path $staging) {
    Remove-Item $staging -Recurse -Force
}
New-Item -ItemType Directory -Force -Path $staging | Out-Null

Copy-Item "$srcDir\balatro_mini.exe" -Destination "$staging\balatro_mini.exe" -Force
Copy-Item "$srcDir\run_game.bat" -Destination "$staging\run_game.bat" -Force

$readmeContent = @"
==================================================
                 BALATRO MINI
==================================================
Versi    : v1.0.0 (Release Standalone)
Bahasa   : C++17 (OpenGL 2D Low-Fidelity)
Platform : Windows (x64)

CARA MENJALANKAN:
- Cukup klik ganda 'run_game.bat' atau 'balatro_mini.exe'

KONTROL PERMAINAN:
- Klik Kiri pada Kartu: Memilih / Membatalkan pilihan kartu (Maks 5 kartu)
- Tombol 'PLAY HAND'   : Memainkan kartu terpilih untuk menghitung skor
- Tombol 'DISCARD'     : Membuang kartu terpilih dan menarik kartu baru dari deck
- Tombol 'CASH OUT'    : Mengambil hadiah uang dan lanjut ke Blind berikutnya
- Keyboard [1] - [8]   : Shortcut memilih kartu 1 sampai 8
- Space / Enter        : Shortcut Play Hand / Select Blind / Cash Out
- Tombol 'D'           : Shortcut Discard
- Tombol 'R'           : Shortcut Restart Run
- Tombol 'ESC'         : Kembali ke Menu / Keluar Game

FITUR UTAMA:
- Formula Skor Khas Balatro: Chips x Mult
- 9 Kombinasi Poker Resmi (High Card hingga Straight Flush)
- Siklus Blind: Small Blind, Big Blind, dan Boss Blind dengan efek Debuff
- Sistem Ante Progression (Ante 1 - 8)
- UI Retro Low-Fidelity yang Ringan & Interaktif
- Robust Error Handling & Action Boundary Guards
==================================================
"@

Set-Content -Path "$staging\README.txt" -Value $readmeContent

if (Test-Path $zipPath) {
    Remove-Item $zipPath -Force
}

Compress-Archive -Path "$staging\*" -DestinationPath $zipPath -Force

if (Test-Path $hasilZipDir) {
    Copy-Item $zipPath -Destination "$hasilZipDir\Balatro-Mini-v1.0.0-Windows.zip" -Force
}

Remove-Item $staging -Recurse -Force

Get-Item $zipPath | Select-Object Name, Length, LastWriteTime

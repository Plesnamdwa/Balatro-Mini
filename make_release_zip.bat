@echo off
echo ==================================================
echo   MEMBUAT RELEASE ZIP BALATRO MINI v1.0.0
echo ==================================================

set DEST_DIR=release_package
if exist %DEST_DIR% rmdir /s /q %DEST_DIR%
mkdir %DEST_DIR%

copy /Y balatro_mini.exe %DEST_DIR%\balatro_mini.exe > nul
copy /Y run_game.bat %DEST_DIR%\run_game.bat > nul

(
echo ==================================================
echo                  BALATRO MINI
echo ==================================================
echo Versi    : v1.0.0 (Release Standalone^)
echo Bahasa   : C++17 (OpenGL 2D Low-Fidelity^)
echo Platform : Windows (x64^)
echo.
echo CARA MENJALANKAN:
echo - Cukup klik ganda 'run_game.bat' atau 'balatro_mini.exe'
echo.
echo KONTROL PERMAINAN:
echo - Klik Kiri pada Kartu: Memilih / Membatalkan kartu (Maks 5^)
echo - Tombol 'PLAY HAND'   : Mainkan kartu terpilih untuk skor
echo - Tombol 'DISCARD'     : Buang kartu dan tarik kartu baru
echo - Tombol 'CASH OUT'    : Ambil hadiah uang dan lanjut
echo - Keyboard [1] - [8]   : Shortcut memilih kartu 1 sampai 8
echo - Space / Enter        : Shortcut Play Hand / Select / Cash Out
echo - Tombol 'D'           : Shortcut Discard
echo - Tombol 'R'           : Shortcut Restart Run
echo - Tombol 'ESC'         : Kembali ke Menu / Keluar Game
echo.
echo FITUR UTAMA:
echo - Formula Skor Balatro : Chips x Mult
echo - 9 Kombinasi Poker Resmi (High Card s.d. Straight Flush^)
echo - Siklus Blind: Small, Big, dan Boss Blind dengan Debuff
echo - Sistem Ante Progression (Ante 1 - 8^)
echo - UI Retro Low-Fidelity Ringan & Interaktif
echo - Robust Error Handling & Action Boundary Guards
echo ==================================================
) > %DEST_DIR%\README.txt

if exist Balatro-Mini-v1.0.0-Windows.zip del /f /q Balatro-Mini-v1.0.0-Windows.zip

powershell -Command "Compress-Archive -Path '%DEST_DIR%\*' -DestinationPath 'Balatro-Mini-v1.0.0-Windows.zip' -Force"

if exist "D:\VSCode\Antigravity\Desain Pattern\Hasil Zip" (
    copy /Y Balatro-Mini-v1.0.0-Windows.zip "D:\VSCode\Antigravity\Desain Pattern\Hasil Zip\Balatro-Mini-v1.0.0-Windows.zip" > nul
)

rmdir /s /q %DEST_DIR%

echo.
echo [OK] Paket Release Berhasil Dibuat:
dir Balatro-Mini-v1.0.0-Windows.zip | findstr "Balatro-Mini"

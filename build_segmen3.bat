@echo off
echo ==================================================
echo   MENGOMPILASI SEGMEN 3 (BALATRO MINI C++)
echo ==================================================

if not exist bin mkdir bin

g++ -std=c++17 -Wall -Wextra -Iinclude tests/test_segment3.cpp -o tests/test_segment3.exe

if %errorlevel% neq 0 (
    echo [ERROR] Kompilasi Gagal! Periksa kode C++.
    exit /b %errorlevel%
)

copy /Y tests\test_segment3.exe bin\test_segment3.exe > nul

echo [OK] Kompilasi Berhasil! Menjalankan tests/test_segment3.exe...
echo.
tests\test_segment3.exe

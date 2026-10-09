@echo off
echo ==================================================
echo   MENGOMPILASI SEGMEN 2 (BALATRO MINI C++)
echo ==================================================

if not exist bin mkdir bin

g++ -std=c++17 -Wall -Wextra -Iinclude tests/test_segment2.cpp -o tests/test_segment2.exe

if %errorlevel% neq 0 (
    echo [ERROR] Kompilasi Gagal! Periksa kode C++.
    exit /b %errorlevel%
)

copy /Y tests\test_segment2.exe bin\test_segment2.exe > nul

echo [OK] Kompilasi Berhasil! Menjalankan tests/test_segment2.exe...
echo.
tests\test_segment2.exe

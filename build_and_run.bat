@echo off
echo ==================================================
echo   MENGOMPILASI BALATRO MINI (C++ OPENGL)
echo ==================================================

if not exist bin mkdir bin

g++ -std=c++17 -Wall -Wextra -Iinclude src/main.cpp -Llib -lglfw3 -lopengl32 -lgdi32 -o balatro_mini.exe

if %errorlevel% neq 0 (
    echo [ERROR] Kompilasi Gagal! Periksa kode C++.
    pause
    exit /b %errorlevel%
)

copy /Y balatro_mini.exe src\main.exe > nul
copy /Y balatro_mini.exe bin\balatro_mini.exe > nul

echo [OK] Kompilasi Berhasil! Menjalankan game Balatro Mini...
echo.
start balatro_mini.exe

@echo off
echo ==================================================
echo       MENJALANKAN BALATRO MINI (C++ OPENGL)
echo ==================================================

if not exist balatro_mini.exe (
    echo [INFO] balatro_mini.exe belum dibuat, mengompilasi sekarang...
    g++ -std=c++17 -Wall -Wextra -Iinclude src/main.cpp -Llib -lglfw3 -lopengl32 -lgdi32 -o balatro_mini.exe
)

echo [OK] Membuka jendela game...
start balatro_mini.exe

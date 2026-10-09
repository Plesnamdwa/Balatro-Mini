#ifndef PIXEL_FONT_H
#define PIXEL_FONT_H

#include <string>
#include <vector>

using namespace std;

// 8x8 Monochrome Bitmap Font untuk Low-Fidelity Retro Typography
// Meng-cover ASCII standar: Angka 0-9, Huruf A-Z, Simbol Balatro, dll.
namespace PixelFont {
    // 8 bytes per glyph (baris 0 sampai 7)
    extern const unsigned char FONT_DATA[128][8];

    // Helper untuk merender karakter tunggal sebagai kumpulan piksel kotak
    // x, y: koordinat kiri atas
    // scale: ukuran per pixel (misal 2 = karakter berukuran 16x16 pixel)
    // drawPixelCallback: fungsi untuk menggambar kotak pixel
    template<typename Func>
    inline void renderText(const string& text, float startX, float startY, float scale, Func drawPixel) {
        float curX = startX;
        float curY = startY;

        for (char c : text) {
            if (c == '\n') {
                curX = startX;
                curY += 9.0f * scale; // Spasi antar baris
                continue;
            }

            unsigned char uChar = static_cast<unsigned char>(c);
            if (uChar > 127) uChar = '?';

            const unsigned char* glyph = FONT_DATA[uChar];

            for (int row = 0; row < 8; ++row) {
                unsigned char rowBits = glyph[row];
                for (int col = 0; col < 8; ++col) {
                    // Cek apakah bit ke-col menyala (MSB di kiri)
                    if (rowBits & (0x80 >> col)) {
                        float px = curX + col * scale;
                        float py = curY + row * scale;
                        drawPixel(px, py, scale, scale);
                    }
                }
            }

            // Maju ke karakter berikutnya (lebar 8 + 1 spasi)
            curX += 8.0f * scale;
        }
    }
}

#endif // PIXEL_FONT_H

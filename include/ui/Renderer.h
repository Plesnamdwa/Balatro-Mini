#ifndef RENDERER_H
#define RENDERER_H

#include "../core/Card.h"
#include <string>

using namespace std;

struct Color {
    float r, g, b, a;
    Color(float red = 1.0f, float green = 1.0f, float blue = 1.0f, float alpha = 1.0f)
        : r(red), g(green), b(blue), a(alpha) {}
};

namespace Theme {
    const Color BG_GREEN(0.10f, 0.22f, 0.18f);        // Felt Green Meja Casino
    const Color PANEL_BG(0.06f, 0.12f, 0.10f, 0.92f); // Panel gelap
    const Color CHIPS_BLUE(0.12f, 0.60f, 0.95f);      // Chips Biru Balatro
    const Color MULT_RED(0.92f, 0.22f, 0.20f);        // Mult Merah Balatro
    const Color GOLD(0.98f, 0.78f, 0.15f);            // Aksen Emas / Uang
    const Color CARD_BG(0.96f, 0.95f, 0.92f);         // Kertas Kartu Ivory
    const Color CARD_BORDER(0.20f, 0.20f, 0.20f);
    const Color CARD_SELECTED(1.0f, 0.85f, 0.20f);    // Highlight Kartu Terpilih
    const Color TEXT_DARK(0.12f, 0.12f, 0.12f);
    const Color TEXT_WHITE(0.95f, 0.95f, 0.95f);
    const Color DEBUFF_GRAY(0.45f, 0.45f, 0.45f);
    const Color TOAST_RED(0.85f, 0.15f, 0.20f, 0.95f);
}

class Renderer {
public:
    static void initGL(int windowWidth, int windowHeight);
    static void updateViewport(int windowWidth, int windowHeight);

    // Latar Belakang Animasi & Efek CRT
    static void drawAnimatedBackground(float windowW, float windowH, float time);

    // Primitif 2D
    static void drawRect(float x, float y, float w, float h, Color color);
    static void drawRectOutline(float x, float y, float w, float h, float thickness, Color color);
    static void drawText(const string& text, float x, float y, float scale, Color color);
    static void drawTextInverted(const string& text, float rightX, float bottomY, float scale, Color color);

    // Helper Tipografi
    static float getTextWidth(const string& text, float scale);
    static void drawCenteredText(const string& text, float centerX, float y, float scale, Color color);

    // Komponen Game Balatro
    static void drawCard(const Card& card, float x, float y, float w, float h, bool isHovered, float elevation = 0.0f);
    static void drawJokerCard(const string& name, const string& desc, int cost, float x, float y, float w, float h, bool isHovered, float scale = 1.0f);
    static void drawSuitIcon(Suit suit, float x, float y, float size, Color color);
    static void drawSuitIconInverted(Suit suit, float rightX, float bottomY, float size, Color color);
    static void drawButton(const string& text, float x, float y, float w, float h, 
                           Color bg, Color textCol, bool isHovered, bool isEnabled);
    static void drawToast(const string& message, float windowW, float windowH, float alpha);

private:
    static void drawPixelArtGrid(float startX, float startY, float pixelSize,
                                const char* const grid[], int rows, int cols,
                                Suit suit, bool debuffed);
    static void drawAceEmblem(float boxX, float boxY, float boxW, float boxH, Suit suit, bool debuffed);
};

#endif // RENDERER_H

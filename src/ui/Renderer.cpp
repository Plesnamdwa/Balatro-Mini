#include <GLFW/glfw3.h>
#include "../../include/ui/Renderer.h"
#include "../../include/ui/PixelFont.h"
#include <algorithm>
#include <cmath>

using namespace std;

// Pixel Art Bitmaps untuk Royal Face Cards (14 kolom x 16 baris)
// Simbol: . = kosong, B = Outline gelap, W = Putih/Zirah, G = Emas, S = Kulit,
//         R = Ruby/Merah, U = Biru, H = Rambut, C = Warna Suit Kartu
static const char* JACK_PIXELS[16] = {
    "....RRRR......",
    "...RRR........",
    "...GGGGGG.....",
    "..GBWWWWWB....",
    "..GBWBBWWB....",
    "..GSSSSSSB....",
    "..GSBSBSGB....",
    "...GSSSSB.....",
    "..BBWWWWBB....",
    ".BGGWCCWGG....",
    "BGGWWCCWWGGB..",
    "BGWWCCCCWWGB..",
    "BGWWCCCCWWGB..",
    ".BGGWWWWGGB...",
    "..BBGGGGGB....",
    "....BBBB......"
};

static const char* QUEEN_PIXELS[16] = {
    "..G.G.G.G.....",
    "..GRGRGRG.....",
    "..GGGGGGG.....",
    ".HHGGGGGSHH...",
    "HHSSSSSSSHH...",
    "HHSBSSBSHHH...",
    "HHSSSSSSSHH...",
    "HHHSSRSSHHH...",
    ".HHSSSSSHHH...",
    "..BWWWWWWB....",
    ".BGRRWWWRGB...",
    "BGRRCCCRRRGB..",
    "BGRRCCCCRRGB..",
    "BGRRCCCCRRGB..",
    ".BGGGGGGGGB...",
    "..BBBBBBBB...."
};

static const char* KING_PIXELS[16] = {
    "...GG.GG......",
    "..GGRGGGRG....",
    "..GGGGGGGG....",
    ".HHGGGGGSHH...",
    "HHSSSSSSSHH...",
    "HHSBSSBSHHH...",
    "HHSSSSSSSHH...",
    "HHHBBBBBHHH...",
    ".HHBBBBBBHH...",
    "..WBBBBBWW....",
    ".WWBWWWBWWB...",
    "BGRRCCCCRRGB..",
    "BGRRCCCCRRGB..",
    "BGRRCCCCRRGB..",
    ".BGGGGGGGGB...",
    "..BBBBBBBB...."
};

void Renderer::initGL(int windowWidth, int windowHeight) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    updateViewport(windowWidth, windowHeight);
}

void Renderer::updateViewport(int windowWidth, int windowHeight) {
    glViewport(0, 0, windowWidth, windowHeight);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, static_cast<double>(windowWidth), static_cast<double>(windowHeight), 0.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void Renderer::drawAnimatedBackground(float windowW, float windowH, float time) {
    const int cols = 32;
    const int rows = 24;
    float cellW = windowW / static_cast<float>(cols);
    float cellH = windowH / static_cast<float>(rows);

    float cx = windowW * 0.5f;
    float cy = windowH * 0.5f;

    auto getColor = [&](float px, float py) -> Color {
        float dx = (px - cx) / 360.0f;
        float dy = (py - cy) / 360.0f;
        float dist = sqrt(dx * dx + dy * dy);
        float angle = atan2(dy, dx);

        // Balatro iconic hypnotic swirl formula
        float swirl = sin(dist * 4.0f - time * 0.8f + angle * 2.5f);
        float wave = sin(px * 0.007f + time * 0.35f) * cos(py * 0.007f - time * 0.25f);
        float val = 0.65f * swirl + 0.35f * wave;

        float t = (val + 1.0f) * 0.5f; // 0.0 to 1.0
        float r = 0.06f + 0.08f * t;
        float g = 0.14f + 0.15f * t;
        float b = 0.11f + 0.12f * t;

        // Soft CRT vignette on edges
        float edgeDist = sqrt(pow((px - cx) / (windowW * 0.5f), 2) + pow((py - cy) / (windowH * 0.5f), 2));
        float vignette = max(0.0f, 1.0f - 0.35f * edgeDist * edgeDist);

        return Color(r * vignette, g * vignette, b * vignette, 1.0f);
    };

    glBegin(GL_QUADS);
    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < cols; ++x) {
            float x0 = x * cellW;
            float y0 = y * cellH;
            float x1 = (x + 1) * cellW;
            float y1 = (y + 1) * cellH;

            Color c00 = getColor(x0, y0);
            Color c10 = getColor(x1, y0);
            Color c11 = getColor(x1, y1);
            Color c01 = getColor(x0, y1);

            glColor4f(c00.r, c00.g, c00.b, c00.a); glVertex2f(x0, y0);
            glColor4f(c10.r, c10.g, c10.b, c10.a); glVertex2f(x1, y0);
            glColor4f(c11.r, c11.g, c11.b, c11.a); glVertex2f(x1, y1);
            glColor4f(c01.r, c01.g, c01.b, c01.a); glVertex2f(x0, y1);
        }
    }
    glEnd();

    // Subtle CRT Scanlines
    glBegin(GL_LINES);
    glColor4f(0.0f, 0.0f, 0.0f, 0.06f);
    for (float sy = 0.0f; sy < windowH; sy += 3.0f) {
        glVertex2f(0.0f, sy);
        glVertex2f(windowW, sy);
    }
    glEnd();
}

void Renderer::drawRect(float x, float y, float w, float h, Color color) {
    glColor4f(color.r, color.g, color.b, color.a);
    glBegin(GL_QUADS);
        glVertex2f(x, y);
        glVertex2f(x + w, y);
        glVertex2f(x + w, y + h);
        glVertex2f(x, y + h);
    glEnd();
}

void Renderer::drawRectOutline(float x, float y, float w, float h, float thickness, Color color) {
    drawRect(x, y, w, thickness, color);
    drawRect(x, y + h - thickness, w, thickness, color);
    drawRect(x, y, thickness, h, color);
    drawRect(x + w - thickness, y, thickness, h, color);
}

void Renderer::drawText(const string& text, float x, float y, float scale, Color color) {
    glColor4f(color.r, color.g, color.b, color.a);
    glBegin(GL_QUADS);
    PixelFont::renderText(text, x, y, scale, [](float px, float py, float pw, float ph) {
        glVertex2f(px, py);
        glVertex2f(px + pw, py);
        glVertex2f(px + pw, py + ph);
        glVertex2f(px, py + ph);
    });
    glEnd();
}

float Renderer::getTextWidth(const string& text, float scale) {
    return static_cast<float>(text.length()) * 8.0f * scale;
}

void Renderer::drawCenteredText(const string& text, float centerX, float y, float scale, Color color) {
    float tw = getTextWidth(text, scale);
    drawText(text, centerX - (tw / 2.0f), y, scale, color);
}

void Renderer::drawPixelArtGrid(float startX, float startY, float pixelSize,
                                const char* const grid[], int rows, int cols,
                                Suit suit, bool debuffed) {
    Color suitCol = debuffed ? Theme::DEBUFF_GRAY : 
                    ((suit == Suit::HEARTS || suit == Suit::DIAMONDS) ? Theme::MULT_RED : Theme::TEXT_DARK);

    glBegin(GL_QUADS);
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            char ch = grid[r][c];
            if (ch == '.') continue;

            Color col;
            switch (ch) {
                case 'B': col = Color(0.12f, 0.12f, 0.14f); break;
                case 'W': col = Color(0.92f, 0.94f, 0.96f); break;
                case 'G': col = Theme::GOLD; break;
                case 'S': col = Color(0.96f, 0.78f, 0.64f); break;
                case 'R': col = Theme::MULT_RED; break;
                case 'U': col = Theme::CHIPS_BLUE; break;
                case 'H': col = Color(0.52f, 0.32f, 0.16f); break;
                case 'C': col = suitCol; break;
                default:  col = Color(0.5f, 0.5f, 0.5f); break;
            }

            if (debuffed) {
                col = Theme::DEBUFF_GRAY;
            }

            float px = startX + c * pixelSize;
            float py = startY + r * pixelSize;

            glColor4f(col.r, col.g, col.b, col.a);
            glVertex2f(px, py);
            glVertex2f(px + pixelSize, py);
            glVertex2f(px + pixelSize, py + pixelSize);
            glVertex2f(px, py + pixelSize);
        }
    }
    glEnd();
}

void Renderer::drawAceEmblem(float boxX, float boxY, float boxW, float boxH, Suit suit, bool debuffed) {
    Color suitCol = debuffed ? Theme::DEBUFF_GRAY : 
                    ((suit == Suit::HEARTS || suit == Suit::DIAMONDS) ? Theme::MULT_RED : Theme::TEXT_DARK);
    Color goldCol = debuffed ? Theme::DEBUFF_GRAY : Theme::GOLD;

    // 4 Corner Filigree Brackets
    float bLen = 6.0f;
    float bThick = 2.0f;
    drawRect(boxX + 3.0f, boxY + 3.0f, bLen, bThick, goldCol);
    drawRect(boxX + 3.0f, boxY + 3.0f, bThick, bLen, goldCol);
    drawRect(boxX + boxW - 3.0f - bLen, boxY + 3.0f, bLen, bThick, goldCol);
    drawRect(boxX + boxW - 3.0f - bThick, boxY + 3.0f, bThick, bLen, goldCol);
    drawRect(boxX + 3.0f, boxY + boxH - 3.0f - bThick, bLen, bThick, goldCol);
    drawRect(boxX + 3.0f, boxY + boxH - 3.0f - bLen, bThick, bLen, goldCol);
    drawRect(boxX + boxW - 3.0f - bLen, boxY + boxH - 3.0f - bThick, bLen, bThick, goldCol);
    drawRect(boxX + boxW - 3.0f - bThick, boxY + boxH - 3.0f - bLen, bThick, bLen, goldCol);

    // 4 Diamond Pips (top, bottom, left, right)
    float cx = boxX + boxW * 0.5f;
    float cy = boxY + boxH * 0.5f;
    drawRect(cx - 2.0f, boxY + 6.0f, 4.0f, 4.0f, goldCol);
    drawRect(cx - 2.0f, boxY + boxH - 10.0f, 4.0f, 4.0f, goldCol);
    drawRect(boxX + 6.0f, cy - 2.0f, 4.0f, 4.0f, goldCol);
    drawRect(boxX + boxW - 10.0f, cy - 2.0f, 4.0f, 4.0f, goldCol);

    // Grand Suit Symbol in the Center
    string sym = Card(suit, Rank::ACE).getSuitSymbol();
    float symW = getTextWidth(sym, 3.0f);
    drawText(sym, cx - symW * 0.5f, cy - 12.0f, 3.0f, suitCol);
}

void Renderer::drawCard(const Card& card, float x, float y, float w, float h, bool isHovered, float elevation) {
    float renderY = y;

    // 1. Bayangan kartu dinamis (bereaksi terhadap elevasi/hover)
    float shadowDist = 4.0f + 7.0f * elevation;
    float shadowAlpha = max(0.16f, 0.36f - 0.12f * elevation);
    drawRect(x + shadowDist * 0.6f, renderY + shadowDist, w, h, Color(0.0f, 0.0f, 0.0f, shadowAlpha));

    // 2. Kertas kartu
    Color bodyColor = card.isDebuffed() ? Color(0.85f, 0.85f, 0.85f) : Theme::CARD_BG;
    drawRect(x, renderY, w, h, bodyColor);

    // 3. Border
    if (card.isSelected()) {
        drawRectOutline(x, renderY, w, h, 4.0f, Theme::CARD_SELECTED);
    } else if (isHovered) {
        drawRectOutline(x, renderY, w, h, 3.0f, Color(1.0f, 1.0f, 1.0f, 0.85f));
    } else {
        drawRectOutline(x, renderY, w, h, 2.0f, Theme::CARD_BORDER);
    }

    // 4. Warna Teks & Simbol
    Color suitColor;
    if (card.isDebuffed()) {
        suitColor = Theme::DEBUFF_GRAY;
    } else if (card.getSuit() == Suit::HEARTS || card.getSuit() == Suit::DIAMONDS) {
        suitColor = Theme::MULT_RED;
    } else {
        suitColor = Theme::TEXT_DARK;
    }

    // 5. Pojok Kiri Atas
    string rankStr = card.getRankString();
    drawText(rankStr, x + 8.0f, renderY + 8.0f, 2.0f, suitColor);
    
    string suitStr = card.getSuitSymbol();
    drawText(suitStr, x + 8.0f, renderY + 28.0f, 1.5f, suitColor);

    // 6. Area Tengah: Pixel Art Face Cards vs Grand Ace vs Number Cards
    Rank rank = card.getRank();
    if (rank == Rank::JACK || rank == Rank::QUEEN || rank == Rank::KING) {
        float boxW = 54.0f;
        float boxH = 68.0f;
        float boxX = x + (w - boxW) * 0.5f;
        float boxY = renderY + 26.0f;

        // Bingkai potret kerajaan
        drawRect(boxX, boxY, boxW, boxH, Color(0.92f, 0.90f, 0.85f));
        drawRectOutline(boxX, boxY, boxW, boxH, 1.5f, Color(0.72f, 0.67f, 0.55f));

        float gridStartX = boxX + (boxW - 42.0f) * 0.5f;
        float gridStartY = boxY + (boxH - 48.0f) * 0.5f;

        if (rank == Rank::JACK) {
            drawPixelArtGrid(gridStartX, gridStartY, 3.0f, JACK_PIXELS, 16, 14, card.getSuit(), card.isDebuffed());
        } else if (rank == Rank::QUEEN) {
            drawPixelArtGrid(gridStartX, gridStartY, 3.0f, QUEEN_PIXELS, 16, 14, card.getSuit(), card.isDebuffed());
        } else if (rank == Rank::KING) {
            drawPixelArtGrid(gridStartX, gridStartY, 3.0f, KING_PIXELS, 16, 14, card.getSuit(), card.isDebuffed());
        }
    } else if (rank == Rank::ACE) {
        float boxW = 54.0f;
        float boxH = 68.0f;
        float boxX = x + (w - boxW) * 0.5f;
        float boxY = renderY + 26.0f;

        drawRect(boxX, boxY, boxW, boxH, Color(0.94f, 0.92f, 0.88f));
        drawRectOutline(boxX, boxY, boxW, boxH, 1.5f, Color(0.78f, 0.72f, 0.55f));
        drawAceEmblem(boxX, boxY, boxW, boxH, card.getSuit(), card.isDebuffed());
    } else {
        // Kartu Angka (2 - 10)
        float boxW = 46.0f;
        float boxH = 58.0f;
        float boxX = x + (w - boxW) * 0.5f;
        float boxY = renderY + 30.0f;

        drawRect(boxX, boxY, boxW, boxH, Color(0.93f, 0.92f, 0.88f, 0.6f));
        drawRectOutline(boxX, boxY, boxW, boxH, 1.0f, Color(0.80f, 0.78f, 0.74f, 0.8f));

        float symW = getTextWidth(suitStr, 2.6f);
        drawText(suitStr, boxX + (boxW - symW) * 0.5f, boxY + (boxH - 20.0f) * 0.5f, 2.6f, suitColor);
    }

    // 7. Base Chips Pojok Kanan Bawah
    string chipStr = "+" + to_string(card.getBaseChips());
    drawText(chipStr, x + w - 38.0f, renderY + h - 22.0f, 1.5f, Theme::CHIPS_BLUE);

    // 8. Label DEBUFF jika aktif
    if (card.isDebuffed()) {
        drawRect(x, renderY + h - 34.0f, w, 14.0f, Color(0.2f, 0.2f, 0.2f, 0.85f));
        drawText("DEBUFF", x + 12.0f, renderY + h - 32.0f, 1.0f, Theme::MULT_RED);
    }
}

void Renderer::drawButton(const string& text, float x, float y, float w, float h, 
                          Color bg, Color textCol, bool isHovered, bool isEnabled) {
    Color finalBg = bg;
    Color finalTxt = textCol;

    if (!isEnabled) {
        finalBg = Color(0.22f, 0.25f, 0.25f, 0.85f);
        finalTxt = Color(0.50f, 0.50f, 0.50f, 0.9f);
    } else if (isHovered) {
        finalBg.r = min(1.0f, finalBg.r + 0.15f);
        finalBg.g = min(1.0f, finalBg.g + 0.15f);
        finalBg.b = min(1.0f, finalBg.b + 0.15f);
    }

    drawRect(x + 3.0f, y + 3.0f, w, h, Color(0.0f, 0.0f, 0.0f, 0.4f));
    drawRect(x, y, w, h, finalBg);
    drawRectOutline(x, y, w, h, 2.0f, isHovered && isEnabled ? Color(1,1,1,0.9f) : Color(0,0,0,0.5f));

    float textW = getTextWidth(text, 1.6f);
    float textX = x + (w - textW) / 2.0f;
    float textY = y + (h - 13.0f) / 2.0f;
    drawText(text, textX, textY, 1.6f, finalTxt);
}

void Renderer::drawToast(const string& message, float windowW, float /*windowH*/, float alpha) {
    if (message.empty() || alpha <= 0.01f) return;

    float toastW = getTextWidth(message, 1.5f) + 40.0f;
    float toastH = 40.0f;
    float toastX = (windowW - toastW) / 2.0f;
    float toastY = 30.0f;

    drawRect(toastX, toastY, toastW, toastH, Color(0.1f, 0.02f, 0.02f, 0.92f * alpha));
    drawRectOutline(toastX, toastY, toastW, toastH, 2.5f, Color(0.95f, 0.25f, 0.2f, alpha));
    drawText(message, toastX + 20.0f, toastY + 13.0f, 1.5f, Color(1.0f, 0.9f, 0.9f, alpha));
}

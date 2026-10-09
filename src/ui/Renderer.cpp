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
    "..G.GG.G......",
    ".GGRGGGRGG....",
    "..GGGGGGGG....",
    ".HHGGGGGGHH...",
    "HHSSSSSSSHH...",
    "HHSBSSBSHHH...",
    "HHSSSSSSSHH...",
    "HHHSBBBSHHH...",
    ".HHBBBBBBHH...",
    "..WBBBBBBW....",
    ".WWBBBBBBWW...",
    "BGRRCCCCRRGB..",
    "BGRRCCCCRRGB..",
    "BGRRCCCCRRGB..",
    ".BGGGGGGGGB...",
    "..BBBBBBBB...."
};

// 14x16 Pixel Art Bitmap untuk Topi & Wajah Joker Khas Balatro
static const char* JOKER_PIXELS[16] = {
    "...RR...UU....",
    "..RRRR.UUUU...",
    ".RR.RRGUU.UU..",
    ".RR.RRGUU.UU..",
    "..RRRR.UUUU...",
    "...GGGGGGGG...",
    "..GSSSSSSSSG..",
    "..GSBSGSBSGG..",
    "..GSSSSSSSSG..",
    "..GSSSRRSSSG..",
    "...GSSSSSSG...",
    "...GGGGGGGG...",
    "..RRWWGGWWUU..",
    ".RRRWWGGWWUUU.",
    ".RRRWWGGWWUUU.",
    "..RRWWGGWWUU.."
};

// 11x11 Pixel Art Bitmaps untuk 4 Simbol Kartu (Suits)
// Simbol: . = transparan, X = piksel aktif berwarna
static const char* SUIT_HEARTS[11] = {
    ".XXX...XXX.",
    "XXXXX.XXXXX",
    "XXXXXXXXXXX",
    "XXXXXXXXXXX",
    "XXXXXXXXXXX",
    ".XXXXXXXXX.",
    "..XXXXXXX..",
    "...XXXXX...",
    "....XXX....",
    ".....X.....",
    "..........."
};

static const char* SUIT_DIAMONDS[11] = {
    ".....X.....",
    "....XXX....",
    "...XXXXX...",
    "..XXXXXXX..",
    ".XXXXXXXXX.",
    "XXXXXXXXXXX",
    ".XXXXXXXXX.",
    "..XXXXXXX..",
    "...XXXXX...",
    "....XXX....",
    ".....X....."
};

static const char* SUIT_SPADES[11] = {
    ".....X.....",
    "....XXX....",
    "...XXXXX...",
    "..XXXXXXX..",
    ".XXXXXXXXX.",
    "XXXXXXXXXXX",
    "XXXXXXXXXXX",
    ".XXXX.XXXX.",
    "..XX.X.XX..",
    "....XXX....",
    "...XXXXX..."
};

static const char* SUIT_CLUBS[11] = {
    "....XXX....",
    "...XXXXX...",
    "...XXXXX...",
    "....XXX....",
    ".XXX...XXX.",
    "XXXXX.XXXXX",
    "XXXXXXXXXXX",
    "XXXXXXXXXXX",
    ".XXX.X.XXX.",
    "....XXX....",
    "...XXXXX..."
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

void Renderer::drawTextInverted(const string& text, float rightX, float bottomY, float scale, Color color) {
    glColor4f(color.r, color.g, color.b, color.a);
    glBegin(GL_QUADS);
    float curRight = rightX;
    // Iterate string in normal order, but each glyph rendered upside-down and right-to-left
    for (char c : text) {
        unsigned char uChar = static_cast<unsigned char>(c);
        if (uChar > 127) uChar = '?';
        const unsigned char* glyph = PixelFont::FONT_DATA[uChar];

        for (int row = 0; row < 8; ++row) {
            unsigned char rowBits = glyph[row];
            for (int col = 0; col < 8; ++col) {
                if (rowBits & (0x80 >> col)) {
                    // Rotated 180 degrees: col -> (7 - col), row -> (7 - row)
                    float px = curRight - (8 - (7 - col)) * scale;
                    float py = bottomY - (8 - (7 - row)) * scale;
                    glVertex2f(px, py);
                    glVertex2f(px + scale, py);
                    glVertex2f(px + scale, py + scale);
                    glVertex2f(px, py + scale);
                }
            }
        }
        curRight -= 8.0f * scale;
    }
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

void Renderer::drawSuitIcon(Suit suit, float x, float y, float size, Color color) {
    const char* const* map = nullptr;
    switch (suit) {
        case Suit::HEARTS:   map = SUIT_HEARTS; break;
        case Suit::DIAMONDS: map = SUIT_DIAMONDS; break;
        case Suit::SPADES:   map = SUIT_SPADES; break;
        case Suit::CLUBS:    map = SUIT_CLUBS; break;
    }
    if (!map) return;

    float pixelSize = size / 11.0f;
    glColor4f(color.r, color.g, color.b, color.a);
    glBegin(GL_QUADS);
    for (int r = 0; r < 11; ++r) {
        for (int c = 0; c < 11; ++c) {
            if (map[r][c] == 'X') {
                float px = x + c * pixelSize;
                float py = y + r * pixelSize;
                glVertex2f(px, py);
                glVertex2f(px + pixelSize, py);
                glVertex2f(px + pixelSize, py + pixelSize);
                glVertex2f(px, py + pixelSize);
            }
        }
    }
    glEnd();
}

void Renderer::drawSuitIconInverted(Suit suit, float rightX, float bottomY, float size, Color color) {
    const char* const* map = nullptr;
    switch (suit) {
        case Suit::HEARTS:   map = SUIT_HEARTS; break;
        case Suit::DIAMONDS: map = SUIT_DIAMONDS; break;
        case Suit::SPADES:   map = SUIT_SPADES; break;
        case Suit::CLUBS:    map = SUIT_CLUBS; break;
    }
    if (!map) return;

    float pixelSize = size / 11.0f;
    glColor4f(color.r, color.g, color.b, color.a);
    glBegin(GL_QUADS);
    for (int r = 0; r < 11; ++r) {
        for (int c = 0; c < 11; ++c) {
            if (map[r][c] == 'X') {
                // Rotated 180 degrees: col -> (10 - c), row -> (10 - r)
                float px = rightX - (11 - (10 - c)) * pixelSize;
                float py = bottomY - (11 - (10 - r)) * pixelSize;
                glVertex2f(px, py);
                glVertex2f(px + pixelSize, py);
                glVertex2f(px + pixelSize, py + pixelSize);
                glVertex2f(px, py + pixelSize);
            }
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

    // Grand Pixel Art Suit Icon di Tengah Ace
    float iconSize = 34.0f;
    drawSuitIcon(suit, cx - iconSize * 0.5f, cy - iconSize * 0.5f, iconSize, suitCol);
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

    // 5. Area Tengah: Pixel Art Face Cards vs Grand Ace vs Number Cards
    Rank rank = card.getRank();
    if (rank == Rank::JACK || rank == Rank::QUEEN || rank == Rank::KING) {
        float boxW = 50.0f;
        float boxH = 66.0f;
        float boxX = x + (w - boxW) * 0.5f;
        float boxY = renderY + 32.0f;

        // Bingkai potret kerajaan
        drawRect(boxX, boxY, boxW, boxH, Color(0.92f, 0.90f, 0.85f));
        drawRectOutline(boxX, boxY, boxW, boxH, 1.5f, Color(0.72f, 0.67f, 0.55f));

        float gridStartX = boxX + (boxW - 42.0f) * 0.5f;
        float gridStartY = boxY + (boxH - 48.0f) * 0.5f - 2.0f;

        if (rank == Rank::JACK) {
            drawPixelArtGrid(gridStartX, gridStartY, 3.0f, JACK_PIXELS, 16, 14, card.getSuit(), card.isDebuffed());
        } else if (rank == Rank::QUEEN) {
            drawPixelArtGrid(gridStartX, gridStartY, 3.0f, QUEEN_PIXELS, 16, 14, card.getSuit(), card.isDebuffed());
        } else if (rank == Rank::KING) {
            drawPixelArtGrid(gridStartX, gridStartY, 3.0f, KING_PIXELS, 16, 14, card.getSuit(), card.isDebuffed());
        }

        // Mini suit icon aksen di bawah potret kerajaan
        drawSuitIcon(card.getSuit(), boxX + (boxW - 10.0f) * 0.5f, boxY + boxH - 12.0f, 10.0f, suitColor);
    } else if (rank == Rank::ACE) {
        float boxW = 50.0f;
        float boxH = 66.0f;
        float boxX = x + (w - boxW) * 0.5f;
        float boxY = renderY + 32.0f;

        drawRect(boxX, boxY, boxW, boxH, Color(0.94f, 0.92f, 0.88f));
        drawRectOutline(boxX, boxY, boxW, boxH, 1.5f, Color(0.78f, 0.72f, 0.55f));
        drawAceEmblem(boxX, boxY, boxW, boxH, card.getSuit(), card.isDebuffed());
    } else {
        // Kartu Angka (2 - 10): Pixel Art Suit Icon Besar di Tengah
        float boxW = 44.0f;
        float boxH = 56.0f;
        float boxX = x + (w - boxW) * 0.5f;
        float boxY = renderY + 37.0f;

        drawRect(boxX, boxY, boxW, boxH, Color(0.93f, 0.92f, 0.88f, 0.6f));
        drawRectOutline(boxX, boxY, boxW, boxH, 1.0f, Color(0.80f, 0.78f, 0.74f, 0.8f));

        float iconSize = 30.0f;
        drawSuitIcon(card.getSuit(), boxX + (boxW - iconSize) * 0.5f, boxY + (boxH - iconSize) * 0.5f, iconSize, suitColor);
    }

    // 6. Pojok Kiri Atas: Rank & Pixel Art Suit Icon (Hierarki di atas frame, posisi dinaikkan)
    string rankStr = card.getRankString();
    drawText(rankStr, x + 6.0f, renderY + 5.0f, 1.9f, suitColor);
    drawSuitIcon(card.getSuit(), x + 7.0f, renderY + 22.0f, 12.0f, suitColor);

    // 7. Pojok Kanan Bawah: Inverted Suit Icon & Rank (Hierarki di atas frame, posisi dinaikkan)
    drawSuitIconInverted(card.getSuit(), x + w - 7.0f, renderY + h - 22.0f, 12.0f, suitColor);
    drawTextInverted(rankStr, x + w - 6.0f, renderY + h - 5.0f, 1.9f, suitColor);

    // 8. Label DEBUFF jika aktif
    if (card.isDebuffed()) {
        drawRect(x, renderY + h - 34.0f, w, 14.0f, Color(0.2f, 0.2f, 0.2f, 0.85f));
        drawText("DEBUFF", x + 12.0f, renderY + h - 32.0f, 1.0f, Theme::MULT_RED);
    }
}

void Renderer::drawJokerCard(const string& name, const string& /*desc*/, int cost, 
                             float x, float y, float w, float h, bool isHovered, float scale) {
    float actualW = w * scale;
    float actualH = h * scale;
    float rx = x - (actualW - w) * 0.5f;
    float ry = y - (actualH - h) * 0.5f;

    // 1. Drop shadow
    float shadowDist = isHovered ? 8.0f : 4.0f;
    drawRect(rx + shadowDist * 0.6f, ry + shadowDist, actualW, actualH, Color(0.0f, 0.0f, 0.0f, 0.35f));

    // 2. Joker Card Body (Krem Elegan khas kartu tarot/joker Balatro)
    drawRect(rx, ry, actualW, actualH, Color(0.95f, 0.94f, 0.88f));

    // 3. Border (Merah/Emas cerah saat hover)
    if (isHovered) {
        drawRectOutline(rx, ry, actualW, actualH, 3.5f, Theme::GOLD);
    } else {
        drawRectOutline(rx, ry, actualW, actualH, 2.0f, Color(0.40f, 0.22f, 0.20f));
    }

    // 4. Header Badge Joker
    float badgeW = actualW - 12.0f;
    float badgeH = 20.0f;
    float badgeX = rx + 6.0f;
    float badgeY = ry + 6.0f;
    drawRect(badgeX, badgeY, badgeW, badgeH, Color(0.85f, 0.20f, 0.20f, 0.90f));
    drawRectOutline(badgeX, badgeY, badgeW, badgeH, 1.0f, Color(1.0f, 0.85f, 0.40f));
    drawCenteredText("JOKER", badgeX + badgeW * 0.5f, badgeY + 4.0f, 1.1f, Theme::TEXT_WHITE);

    // 5. Potret Piksel Art Joker di Tengah
    float boxW = actualW - 20.0f;
    float boxH = actualH - 52.0f;
    float boxX = rx + 10.0f;
    float boxY = ry + 30.0f;
    drawRect(boxX, boxY, boxW, boxH, Color(0.88f, 0.87f, 0.82f));
    drawRectOutline(boxX, boxY, boxW, boxH, 1.2f, Color(0.70f, 0.68f, 0.60f));

    float gridStartX = boxX + (boxW - 42.0f) * 0.5f;
    float gridStartY = boxY + (boxH - 48.0f) * 0.5f;
    drawPixelArtGrid(gridStartX, gridStartY, 3.0f, JOKER_PIXELS, 16, 14, Suit::HEARTS, false);

    // 6. Label Nama Joker di Bawah Potret
    float nameScale = (name.length() > 10) ? 0.95f : 1.1f;
    drawCenteredText(name, rx + actualW * 0.5f, ry + actualH - 18.0f, nameScale, Theme::TEXT_DARK);

    // 7. Harga Beli (Jika ada dalam konteks toko)
    if (cost > 0) {
        string costStr = "$" + to_string(cost);
        drawText(costStr, rx + actualW - 24.0f, ry + 8.0f, 1.1f, Theme::GOLD);
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

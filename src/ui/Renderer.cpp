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

void Renderer::drawWrappedCenteredText(const string& text, float centerX, float y, float maxWidth, float scale, float lineSpacing, Color color) {
    vector<string> lines;
    string curLine = "";
    string word = "";

    auto flushWord = [&](const string& w) {
        string testLine = curLine.empty() ? w : (curLine + " " + w);
        if (getTextWidth(testLine, scale) <= maxWidth) {
            curLine = testLine;
        } else {
            if (!curLine.empty()) lines.push_back(curLine);
            curLine = w;
        }
    };

    for (size_t i = 0; i < text.length(); ++i) {
        char ch = text[i];
        if (ch == ' ' || ch == '\n') {
            if (!word.empty()) {
                flushWord(word);
                word = "";
            }
            if (ch == '\n') {
                if (!curLine.empty()) lines.push_back(curLine);
                curLine = "";
            }
        } else {
            word += ch;
        }
    }
    if (!word.empty()) {
        flushWord(word);
    }
    if (!curLine.empty()) {
        lines.push_back(curLine);
    }

    float curY = y;
    for (const auto& line : lines) {
        drawCenteredText(line, centerX, curY, scale, color);
        curY += (8.0f * scale) + lineSpacing;
    }
}

void Renderer::drawWrappedText(const string& text, float startX, float startY, float maxWidth, float scale, float lineSpacing, Color color) {
    vector<string> lines;
    string curLine = "";
    string word = "";

    auto flushWord = [&](const string& w) {
        string testLine = curLine.empty() ? w : (curLine + " " + w);
        if (getTextWidth(testLine, scale) <= maxWidth) {
            curLine = testLine;
        } else {
            if (!curLine.empty()) lines.push_back(curLine);
            curLine = w;
        }
    };

    for (size_t i = 0; i < text.length(); ++i) {
        char ch = text[i];
        if (ch == ' ' || ch == '\n') {
            if (!word.empty()) {
                flushWord(word);
                word = "";
            }
            if (ch == '\n') {
                if (!curLine.empty()) lines.push_back(curLine);
                curLine = "";
            }
        } else {
            word += ch;
        }
    }
    if (!word.empty()) {
        flushWord(word);
    }
    if (!curLine.empty()) {
        lines.push_back(curLine);
    }

    float curY = startY;
    for (const auto& line : lines) {
        drawText(line, startX, curY, scale, color);
        curY += (8.0f * scale) + lineSpacing;
    }
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

void Renderer::drawJokerCard(const string& name, const string& /*desc*/, int /*cost*/, 
                             float x, float y, float w, float h, bool isHovered, float scale, float glowIntensity) {
    float actualW = w * scale;
    float actualH = h * scale;
    float rx = x - (actualW - w) * 0.5f;
    float ry = y - (actualH - h) * 0.5f;

    // 0. Efek Aura Menyala saat Joker Aktif / Terpicu
    if (glowIntensity > 0.01f) {
        drawGlowingAura(rx, ry, actualW, actualH, Theme::GOLD, glowIntensity * 1.5f);
    }

    // 1. Drop shadow
    float shadowDist = isHovered ? 6.0f : 3.0f;
    drawRect(rx + shadowDist * 0.6f, ry + shadowDist, actualW, actualH, Color(0.0f, 0.0f, 0.0f, 0.35f));

    // 2. Joker Card Body (Krem Elegan khas kartu tarot/joker Balatro)
    drawRect(rx, ry, actualW, actualH, Color(0.95f, 0.94f, 0.88f));

    // 3. Border (Merah/Emas cerah saat hover atau saat terpicu aktif)
    if (glowIntensity > 0.1f) {
        drawRectOutline(rx, ry, actualW, actualH, 3.0f, Color(1.0f, 0.95f, 0.35f, 1.0f));
    } else if (isHovered) {
        drawRectOutline(rx, ry, actualW, actualH, 2.5f, Theme::GOLD);
    } else {
        drawRectOutline(rx, ry, actualW, actualH, 1.5f, Color(0.40f, 0.22f, 0.20f));
    }

    // 4. Header Badge Joker
    float badgeW = actualW - 8.0f;
    float badgeH = (actualH < 100.0f) ? 14.0f : 18.0f;
    float badgeX = rx + 4.0f;
    float badgeY = ry + 4.0f;
    drawRect(badgeX, badgeY, badgeW, badgeH, Color(0.85f, 0.20f, 0.20f, 0.90f));
    drawRectOutline(badgeX, badgeY, badgeW, badgeH, 1.0f, Color(1.0f, 0.85f, 0.40f));
    float badgeTextScale = (actualH < 100.0f) ? 0.9f : 1.05f;
    drawCenteredText("JOKER", badgeX + badgeW * 0.5f, badgeY + ((actualH < 100.0f) ? 2.5f : 4.0f), badgeTextScale, Theme::TEXT_WHITE);

    // 5. Potret Piksel Art Joker di Tengah
    float boxW = actualW - 14.0f;
    float boxH = actualH - ((actualH < 100.0f) ? 36.0f : 48.0f);
    float boxX = rx + 7.0f;
    float boxY = ry + badgeH + 6.0f;
    drawRect(boxX, boxY, boxW, boxH, Color(0.88f, 0.87f, 0.82f));
    drawRectOutline(boxX, boxY, boxW, boxH, 1.0f, Color(0.70f, 0.68f, 0.60f));

    float pixelSize = (actualH < 100.0f) ? 2.2f : 2.8f;
    float gridStartX = boxX + (boxW - 14.0f * pixelSize) * 0.5f;
    float gridStartY = boxY + (boxH - 16.0f * pixelSize) * 0.5f;
    drawPixelArtGrid(gridStartX, gridStartY, pixelSize, JOKER_PIXELS, 16, 14, Suit::HEARTS, false);

    // 6. Label Nama Joker di Bawah Potret (Auto-fit dinamis agar tidak keluar frame kartu)
    float baseScale = (actualH < 100.0f) ? 0.85f : 1.05f;
    float maxNameW = actualW - 10.0f; // Memberikan margin 5px kiri-kanan
    float nameScale = baseScale;
    float actualTextW = getTextWidth(name, nameScale);
    if (actualTextW > maxNameW && actualTextW > 0.0f) {
        nameScale *= (maxNameW / actualTextW);
    }
    float nameY = ry + actualH - ((actualH < 100.0f) ? 12.0f : 16.0f);
    drawCenteredText(name, rx + actualW * 0.5f, nameY, nameScale, Theme::TEXT_DARK);
}

static void drawVoucherEmblem(VoucherType type, int tier, float cx, float cy) {
    (void)tier;
    switch (type) {
        case VoucherType::GRABBER: {
            // Mechanical blue arcade claw / robotic hand gripping a playing card
            float armW = 10.0f, armH = 14.0f;
            Renderer::drawRect(cx - armW * 0.5f, cy + 9.0f, armW, armH, Color(0.20f, 0.35f, 0.55f));
            Renderer::drawRectOutline(cx - armW * 0.5f, cy + 9.0f, armW, armH, 1.0f, Color(0.40f, 0.65f, 0.90f));
            // Claw wrist joint
            Renderer::drawRect(cx - 15.0f, cy + 5.0f, 30.0f, 5.0f, Color(0.30f, 0.45f, 0.65f));
            // Left claw arm
            Renderer::drawRect(cx - 19.0f, cy - 11.0f, 6.0f, 17.0f, Color(0.25f, 0.55f, 0.85f));
            Renderer::drawRect(cx - 15.0f, cy - 13.0f, 8.0f, 5.0f, Color(0.25f, 0.55f, 0.85f));
            // Right claw arm
            Renderer::drawRect(cx + 13.0f, cy - 11.0f, 6.0f, 17.0f, Color(0.25f, 0.55f, 0.85f));
            Renderer::drawRect(cx + 7.0f, cy - 13.0f, 8.0f, 5.0f, Color(0.25f, 0.55f, 0.85f));
            // Center card grabbed in claws
            Renderer::drawRect(cx - 7.0f, cy - 9.0f, 14.0f, 19.0f, Color(0.96f, 0.96f, 0.96f));
            Renderer::drawRectOutline(cx - 7.0f, cy - 9.0f, 14.0f, 19.0f, 1.0f, Theme::CHIPS_BLUE);
            Renderer::drawSuitIcon(Suit::SPADES, cx - 4.0f, cy - 3.5f, 8.0f, Theme::CHIPS_BLUE);
            break;
        }
        case VoucherType::NACHO_TONG: {
            // Golden ornate tongs gripping royal card with sparkles
            Renderer::drawCasinoChip(cx, cy + 16.0f, 6.0f, Theme::GOLD, Color(1, 1, 1), 0.0f);
            // Left tong arm
            Renderer::drawRect(cx - 17.0f, cy - 7.0f, 5.0f, 23.0f, Theme::GOLD);
            Renderer::drawRect(cx - 13.0f, cy - 11.0f, 8.0f, 5.0f, Theme::GOLD);
            // Right tong arm
            Renderer::drawRect(cx + 12.0f, cy - 7.0f, 5.0f, 23.0f, Theme::GOLD);
            Renderer::drawRect(cx + 5.0f, cy - 11.0f, 8.0f, 5.0f, Theme::GOLD);
            // Center glowing gold royal card
            Renderer::drawRect(cx - 8.0f, cy - 8.0f, 16.0f, 22.0f, Color(1.0f, 0.95f, 0.82f));
            Renderer::drawRectOutline(cx - 8.0f, cy - 8.0f, 16.0f, 22.0f, 1.2f, Theme::GOLD);
            Renderer::drawSuitIcon(Suit::HEARTS, cx - 4.5f, cy - 2.5f, 9.0f, Theme::MULT_RED);
            // Golden Sparkles
            Renderer::drawRect(cx - 19.0f, cy - 17.0f, 4.0f, 4.0f, Color(1.0f, 1.0f, 1.0f));
            Renderer::drawRect(cx + 17.0f, cy - 15.0f, 4.0f, 4.0f, Color(1.0f, 1.0f, 1.0f));
            break;
        }
        case VoucherType::WASTEFUL: {
            // Red Disposal Trash Can with discarded cards
            float binW = 26.0f, binH = 26.0f;
            float binY = cy + 2.0f;
            // Trash bin body
            Renderer::drawRect(cx - binW * 0.5f, binY, binW, binH, Theme::MULT_RED);
            Renderer::drawRectOutline(cx - binW * 0.5f, binY, binW, binH, 1.2f, Color(0.55f, 0.12f, 0.12f));
            // Vertical ribs on trash bin
            Renderer::drawRect(cx - 6.0f, binY + 4.0f, 2.0f, binH - 6.0f, Color(0.55f, 0.12f, 0.12f));
            Renderer::drawRect(cx + 4.0f, binY + 4.0f, 2.0f, binH - 6.0f, Color(0.55f, 0.12f, 0.12f));
            // Trash bin collar/rim
            Renderer::drawRect(cx - binW * 0.5f - 3.0f, binY - 3.0f, binW + 6.0f, 5.0f, Color(0.60f, 0.15f, 0.15f));
            // Lid ajar (tilted slightly)
            Renderer::drawRect(cx - binW * 0.5f - 2.0f, binY - 14.0f, binW + 4.0f, 4.0f, Color(0.75f, 0.20f, 0.18f));
            Renderer::drawRect(cx - 3.0f, binY - 18.0f, 6.0f, 4.0f, Color(0.75f, 0.20f, 0.18f));
            // Discarded cards sticking out into bin
            Renderer::drawRect(cx - 9.0f, binY - 11.0f, 10.0f, 12.0f, Color(0.96f, 0.96f, 0.96f));
            Renderer::drawSuitIcon(Suit::HEARTS, cx - 7.0f, binY - 8.0f, 6.0f, Theme::MULT_RED);
            Renderer::drawRect(cx + 1.0f, binY - 9.0f, 10.0f, 11.0f, Color(0.90f, 0.90f, 0.92f));
            Renderer::drawSuitIcon(Suit::SPADES, cx + 3.0f, binY - 7.0f, 6.0f, Theme::TEXT_DARK);
            break;
        }
        case VoucherType::RECYCLOMANCY: {
            // Emerald Magic Recycling Loop with Reborn Card
            Color recCol = Color(0.25f, 0.85f, 0.45f);
            // 3 Curved Arrow segments forming cycle:
            // Top-right arrow
            Renderer::drawRect(cx - 4.0f, cy - 20.0f, 18.0f, 5.0f, recCol);
            Renderer::drawRect(cx + 10.0f, cy - 20.0f, 5.0f, 14.0f, recCol);
            Renderer::drawRect(cx + 11.0f, cy - 23.0f, 5.0f, 5.0f, recCol);
            // Bottom-right to left arrow
            Renderer::drawRect(cx + 6.0f, cy + 6.0f, 5.0f, 14.0f, recCol);
            Renderer::drawRect(cx - 16.0f, cy + 15.0f, 26.0f, 5.0f, recCol);
            Renderer::drawRect(cx - 18.0f, cy + 12.0f, 5.0f, 5.0f, recCol);
            // Left upward arrow
            Renderer::drawRect(cx - 16.0f, cy - 10.0f, 5.0f, 22.0f, recCol);
            Renderer::drawRect(cx - 16.0f, cy - 14.0f, 14.0f, 5.0f, recCol);
            Renderer::drawRect(cx - 6.0f, cy - 18.0f, 5.0f, 5.0f, recCol);
            // Center floating mystical card with glowing aura
            Renderer::drawRect(cx - 7.0f, cy - 9.0f, 14.0f, 18.0f, Color(0.98f, 0.98f, 0.95f));
            Renderer::drawRectOutline(cx - 7.0f, cy - 9.0f, 14.0f, 18.0f, 1.2f, recCol);
            Renderer::drawSuitIcon(Suit::DIAMONDS, cx - 4.0f, cy - 4.0f, 8.0f, Theme::GOLD);
            break;
        }
        case VoucherType::PAINT_BRUSH: {
            // Artist Paintbrush angled diagonally with colorful paint
            Renderer::drawRect(cx - 16.0f, cy + 8.0f, 18.0f, 5.0f, Color(0.65f, 0.45f, 0.25f));
            Renderer::drawRect(cx + 2.0f, cy + 2.0f, 6.0f, 7.0f, Color(0.80f, 0.85f, 0.90f));
            // Bristles loaded with teal paint
            Renderer::drawRect(cx + 8.0f, cy - 6.0f, 8.0f, 9.0f, Color(0.20f, 0.75f, 0.65f));
            Renderer::drawRect(cx + 14.0f, cy - 12.0f, 5.0f, 7.0f, Color(0.15f, 0.85f, 0.75f));
            // Paint splatters
            Renderer::drawRect(cx + 12.0f, cy - 18.0f, 4.0f, 4.0f, Color(0.20f, 0.75f, 0.65f));
            Renderer::drawRect(cx + 18.0f, cy - 8.0f, 3.0f, 3.0f, Theme::MULT_RED);
            Renderer::drawRect(cx + 4.0f, cy - 16.0f, 3.0f, 3.0f, Theme::GOLD);
            break;
        }
        case VoucherType::PALETTE: {
            // Wooden Artist Palette with 4 vibrant paint dollops
            float palW = 34.0f, palH = 26.0f;
            Renderer::drawRect(cx - palW * 0.5f, cy - palH * 0.5f, palW, palH, Color(0.72f, 0.52f, 0.30f));
            Renderer::drawRectOutline(cx - palW * 0.5f, cy - palH * 0.5f, palW, palH, 1.5f, Color(0.50f, 0.35f, 0.18f));
            // Thumb hole
            Renderer::drawRect(cx + 7.0f, cy + 2.0f, 6.0f, 6.0f, Color(0.08f, 0.11f, 0.14f));
            // 4 Paint dollops
            Renderer::drawRect(cx - 12.0f, cy - 9.0f, 6.0f, 6.0f, Theme::MULT_RED);
            Renderer::drawRect(cx - 2.0f, cy - 10.0f, 6.0f, 6.0f, Theme::CHIPS_BLUE);
            Renderer::drawRect(cx + 7.0f, cy - 8.0f, 6.0f, 6.0f, Theme::GOLD);
            Renderer::drawRect(cx - 12.0f, cy + 3.0f, 6.0f, 6.0f, Color(0.20f, 0.80f, 0.40f));
            break;
        }
        case VoucherType::BLANK: {
            // Mysterious silver card with shimmering question mark
            Renderer::drawRect(cx - 13.0f, cy - 17.0f, 26.0f, 34.0f, Color(0.92f, 0.94f, 0.98f));
            Renderer::drawRectOutline(cx - 13.0f, cy - 17.0f, 26.0f, 34.0f, 1.5f, Color(0.60f, 0.65f, 0.75f));
            Renderer::drawCenteredText("?", cx, cy - 11.0f, 2.3f, Color(0.35f, 0.42f, 0.52f));
            break;
        }
        case VoucherType::ANTIMATTER: {
            // Cosmic Black Hole with Electric Rays & Golden Crest
            Renderer::drawCasinoChip(cx, cy, 18.0f, Color(0.10f, 0.04f, 0.18f), Color(0.75f, 0.25f, 0.95f), 0.0f);
            Renderer::drawCasinoChip(cx, cy, 11.0f, Color(0.20f, 0.08f, 0.35f), Color(0.95f, 0.45f, 0.95f), 0.0f);
            Renderer::drawRect(cx - 1.0f, cy - 22.0f, 2.0f, 44.0f, Color(0.85f, 0.45f, 0.95f));
            Renderer::drawRect(cx - 22.0f, cy - 1.0f, 44.0f, 2.0f, Color(0.85f, 0.45f, 0.95f));
            // Jester hat golden silhouette
            Renderer::drawRect(cx - 7.0f, cy - 3.0f, 14.0f, 7.0f, Theme::GOLD);
            Renderer::drawRect(cx - 9.0f, cy - 8.0f, 4.0f, 6.0f, Theme::GOLD);
            Renderer::drawRect(cx + 5.0f, cy - 8.0f, 4.0f, 6.0f, Theme::GOLD);
            Renderer::drawRect(cx - 2.0f, cy - 9.0f, 4.0f, 7.0f, Theme::GOLD);
            break;
        }
        case VoucherType::REROLL_SURPLUS: {
            // Golden casino chip with double refresh arrows
            Color goldCol = Theme::GOLD;
            Renderer::drawCasinoChip(cx, cy, 12.0f, goldCol, Color(1, 1, 1), 0.0f);
            Renderer::drawCenteredText("$", cx, cy - 6.0f, 1.4f, Theme::TEXT_DARK);
            Renderer::drawRect(cx - 16.0f, cy - 16.0f, 24.0f, 4.0f, goldCol);
            Renderer::drawRect(cx + 5.0f, cy - 20.0f, 4.0f, 8.0f, goldCol);
            Renderer::drawRect(cx - 8.0f, cy + 12.0f, 24.0f, 4.0f, goldCol);
            Renderer::drawRect(cx - 9.0f, cy + 12.0f, 4.0f, 8.0f, goldCol);
            break;
        }
        case VoucherType::REROLL_GLUT: {
            // 3D Rolling Casino Dice with motion trails
            Renderer::drawRect(cx - 16.0f, cy - 12.0f, 18.0f, 18.0f, Theme::MULT_RED);
            Renderer::drawRectOutline(cx - 16.0f, cy - 12.0f, 18.0f, 18.0f, 1.2f, Color(0.50f, 0.10f, 0.10f));
            Renderer::drawRect(cx - 13.0f, cy - 9.0f, 3.0f, 3.0f, Theme::TEXT_WHITE);
            Renderer::drawRect(cx - 5.0f, cy - 9.0f, 3.0f, 3.0f, Theme::TEXT_WHITE);
            Renderer::drawRect(cx - 9.0f, cy - 4.0f, 3.0f, 3.0f, Theme::TEXT_WHITE);
            Renderer::drawRect(cx - 13.0f, cy + 1.0f, 3.0f, 3.0f, Theme::TEXT_WHITE);
            Renderer::drawRect(cx - 5.0f, cy + 1.0f, 3.0f, 3.0f, Theme::TEXT_WHITE);
            Renderer::drawRect(cx - 2.0f, cy - 2.0f, 18.0f, 18.0f, Theme::GOLD);
            Renderer::drawRectOutline(cx - 2.0f, cy - 2.0f, 18.0f, 18.0f, 1.2f, Color(0.60f, 0.40f, 0.10f));
            Renderer::drawRect(cx + 2.0f, cy + 2.0f, 3.0f, 3.0f, Theme::TEXT_DARK);
            Renderer::drawRect(cx + 6.0f, cy + 6.0f, 3.0f, 3.0f, Theme::TEXT_DARK);
            Renderer::drawRect(cx + 10.0f, cy + 10.0f, 3.0f, 3.0f, Theme::TEXT_DARK);
            Renderer::drawRect(cx - 18.0f, cy - 18.0f, 36.0f, 3.0f, Theme::GOLD);
            break;
        }
        case VoucherType::CLEARANCE_SALE: {
            // Hanging Price Tag with Big %
            float tagW = 28.0f, tagH = 34.0f;
            Renderer::drawRect(cx - tagW * 0.5f, cy - tagH * 0.5f + 3.0f, tagW, tagH, Color(0.95f, 0.65f, 0.15f));
            Renderer::drawRectOutline(cx - tagW * 0.5f, cy - tagH * 0.5f + 3.0f, tagW, tagH, 1.2f, Color(0.65f, 0.40f, 0.10f));
            Renderer::drawRect(cx - 3.0f, cy - tagH * 0.5f + 5.0f, 6.0f, 6.0f, Color(0.08f, 0.11f, 0.14f));
            Renderer::drawRect(cx - 1.0f, cy - tagH * 0.5f - 4.0f, 2.0f, 9.0f, Color(0.85f, 0.85f, 0.85f));
            Renderer::drawCenteredText("%", cx, cy - 3.0f, 1.8f, Theme::TEXT_DARK);
            break;
        }
        case VoucherType::LIQUIDATION: {
            // Flaming Red Tag with Molten Discount
            float tagW = 28.0f, tagH = 34.0f;
            Renderer::drawRect(cx - 10.0f, cy - 20.0f, 6.0f, 8.0f, Color(1.0f, 0.60f, 0.0f));
            Renderer::drawRect(cx - 2.0f, cy - 22.0f, 6.0f, 10.0f, Color(1.0f, 0.80f, 0.1f));
            Renderer::drawRect(cx + 6.0f, cy - 18.0f, 5.0f, 7.0f, Color(1.0f, 0.45f, 0.0f));
            Renderer::drawRect(cx - tagW * 0.5f, cy - tagH * 0.5f + 3.0f, tagW, tagH, Color(0.92f, 0.22f, 0.15f));
            Renderer::drawRectOutline(cx - tagW * 0.5f, cy - tagH * 0.5f + 3.0f, tagW, tagH, 1.5f, Color(1.0f, 0.70f, 0.10f));
            Renderer::drawCenteredText("%%", cx, cy - 4.0f, 1.4f, Color(1.0f, 0.95f, 0.70f));
            break;
        }
        case VoucherType::OVERSTOCK: {
            // Store showcase shelf with striped canopy and boxes
            Renderer::drawRect(cx - 20.0f, cy - 18.0f, 40.0f, 8.0f, Color(0.20f, 0.65f, 0.35f));
            Renderer::drawRect(cx - 12.0f, cy - 18.0f, 8.0f, 8.0f, Color(0.95f, 0.95f, 0.95f));
            Renderer::drawRect(cx + 4.0f, cy - 18.0f, 8.0f, 8.0f, Color(0.95f, 0.95f, 0.95f));
            Renderer::drawRect(cx - 18.0f, cy + 8.0f, 36.0f, 6.0f, Color(0.55f, 0.35f, 0.20f));
            Renderer::drawRect(cx - 16.0f, cy - 5.0f, 9.0f, 13.0f, Theme::CHIPS_BLUE);
            Renderer::drawRect(cx - 4.0f, cy - 8.0f, 9.0f, 16.0f, Theme::MULT_RED);
            Renderer::drawRect(cx + 8.0f, cy - 4.0f, 9.0f, 12.0f, Theme::GOLD);
            break;
        }
        case VoucherType::OVERSTOCK_PLUS: {
            // Golden royal two-tier luxury showcase
            Renderer::drawRect(cx - 6.0f, cy - 20.0f, 12.0f, 5.0f, Theme::GOLD);
            Renderer::drawRect(cx - 2.0f, cy - 23.0f, 4.0f, 4.0f, Theme::GOLD);
            Renderer::drawRect(cx - 18.0f, cy - 7.0f, 36.0f, 4.0f, Theme::GOLD);
            Renderer::drawRect(cx - 12.0f, cy - 15.0f, 8.0f, 8.0f, Color(0.85f, 0.40f, 0.95f));
            Renderer::drawRect(cx + 4.0f, cy - 15.0f, 8.0f, 8.0f, Color(0.20f, 0.85f, 0.85f));
            Renderer::drawRect(cx - 20.0f, cy + 12.0f, 40.0f, 5.0f, Theme::GOLD);
            Renderer::drawRect(cx - 16.0f, cy + 1.0f, 9.0f, 11.0f, Theme::MULT_RED);
            Renderer::drawRect(cx - 4.0f, cy - 1.0f, 9.0f, 13.0f, Theme::CHIPS_BLUE);
            Renderer::drawRect(cx + 8.0f, cy + 2.0f, 9.0f, 10.0f, Color(0.25f, 0.85f, 0.45f));
            break;
        }
        case VoucherType::DIRECTORS_CUT: {
            // Film Director's Clapperboard
            float clpW = 32.0f, clpH = 24.0f;
            Renderer::drawRect(cx - clpW * 0.5f, cy - 2.0f, clpW, clpH, Color(0.12f, 0.14f, 0.16f));
            Renderer::drawRectOutline(cx - clpW * 0.5f, cy - 2.0f, clpW, clpH, 1.2f, Color(0.40f, 0.45f, 0.50f));
            Renderer::drawRect(cx - clpW * 0.5f + 3.0f, cy + 8.0f, clpW - 6.0f, 1.0f, Color(0.40f, 0.45f, 0.50f));
            Renderer::drawRect(cx - clpW * 0.5f, cy - 14.0f, clpW, 8.0f, Color(0.12f, 0.14f, 0.16f));
            Renderer::drawRect(cx - 12.0f, cy - 14.0f, 4.0f, 8.0f, Theme::TEXT_WHITE);
            Renderer::drawRect(cx - 4.0f, cy - 14.0f, 4.0f, 8.0f, Theme::TEXT_WHITE);
            Renderer::drawRect(cx + 4.0f, cy - 14.0f, 4.0f, 8.0f, Theme::TEXT_WHITE);
            Renderer::drawRect(cx + 12.0f, cy - 14.0f, 3.0f, 8.0f, Theme::TEXT_WHITE);
            break;
        }
        case VoucherType::RETCON: {
            // Rewind Chronometer / Pocket Watch turning back time
            Renderer::drawCasinoChip(cx, cy, 16.0f, Color(0.85f, 0.68f, 0.25f), Color(1, 1, 1), 0.0f);
            Renderer::drawCasinoChip(cx, cy, 12.0f, Color(0.96f, 0.96f, 0.96f), Color(0.60f, 0.45f, 0.15f), 0.0f);
            Renderer::drawRect(cx - 3.0f, cy - 20.0f, 6.0f, 4.0f, Color(0.85f, 0.68f, 0.25f));
            Renderer::drawRectOutline(cx - 5.0f, cy - 24.0f, 10.0f, 5.0f, 1.2f, Color(0.85f, 0.68f, 0.25f));
            Renderer::drawRect(cx - 9.0f, cy - 7.0f, 12.0f, 2.5f, Color(0.10f, 0.45f, 0.75f));
            Renderer::drawRect(cx - 9.0f, cy - 10.0f, 3.0f, 6.0f, Color(0.10f, 0.45f, 0.75f));
            Renderer::drawRect(cx - 1.0f, cy - 7.0f, 2.0f, 8.0f, Theme::TEXT_DARK);
            Renderer::drawRect(cx - 6.0f, cy - 1.0f, 6.0f, 2.0f, Theme::TEXT_DARK);
            break;
        }
        default:
            Renderer::drawCasinoChip(cx, cy, 14.0f, Theme::GOLD, Color(1, 1, 1), 0.0f);
            break;
    }
}

void Renderer::drawVoucherCard(const Voucher& voucher, float x, float y, float w, float h, 
                              bool isHovered, float scale, bool isSoldOut) {
    float actualW = w * scale;
    float actualH = h * scale;
    float rx = x - (actualW - w) * 0.5f;
    float ry = y - (actualH - h) * 0.5f;

    // 1. Drop shadow
    float shadowDist = isHovered ? 6.0f : 3.0f;
    drawRect(rx + shadowDist * 0.6f, ry + shadowDist, actualW, actualH, Color(0.0f, 0.0f, 0.0f, 0.35f));

    // 2. Ticket Body (Ivory Paper)
    Color bodyCol = (voucher.tier == 2) ? Color(0.98f, 0.96f, 0.90f) : Color(0.95f, 0.94f, 0.90f);
    drawRect(rx, ry, actualW, actualH, bodyCol);

    // 3. Border
    Color borderCol(voucher.accentColor.r, voucher.accentColor.g, voucher.accentColor.b);
    if (voucher.tier == 2) {
        drawRectOutline(rx, ry, actualW, actualH, 2.5f, Theme::GOLD);
        drawRectOutline(rx + 2.0f, ry + 2.0f, actualW - 4.0f, actualH - 4.0f, 1.0f, borderCol);
    } else {
        drawRectOutline(rx, ry, actualW, actualH, isHovered ? 2.5f : 1.5f, isHovered ? Theme::GOLD : Color(0.35f, 0.38f, 0.40f));
    }

    // 4. Ticket Cutouts / Notches at Top & Bottom Centers
    float notchW = max(8.0f, actualW * 0.14f);
    float notchH = 5.0f;
    Color notchBg(0.06f, 0.12f, 0.10f, 1.0f);
    drawRect(rx + (actualW - notchW) * 0.5f, ry - 1.0f, notchW, notchH, notchBg);
    drawRect(rx + (actualW - notchW) * 0.5f, ry + actualH - notchH + 1.0f, notchW, notchH, notchBg);

    // 5. Header Banner: "VOUCHER" (Tier I) atau "★ TIER II ★" (Tier II)
    float bannerH = (actualH < 120.0f) ? 15.0f : 18.0f;
    float bannerY = ry + 4.0f;
    float bannerW = actualW - 8.0f;
    float bannerX = rx + 4.0f;
    Color bannerBg = (voucher.tier == 2) ? Color(0.22f, 0.15f, 0.32f) : Color(0.14f, 0.19f, 0.24f);
    drawRect(bannerX, bannerY, bannerW, bannerH, bannerBg);
    drawRectOutline(bannerX, bannerY, bannerW, bannerH, 1.0f, (voucher.tier == 2) ? Theme::GOLD : borderCol);
    
    string bannerText = (voucher.tier == 2) ? "★ TIER II ★" : "VOUCHER";
    Color bannerTextCol = (voucher.tier == 2) ? Theme::GOLD : Theme::TEXT_WHITE;
    float bTextScale = (actualH < 120.0f) ? 0.85f : 0.95f;
    drawCenteredText(bannerText, bannerX + bannerW * 0.5f, bannerY + ((actualH < 120.0f) ? 2.0f : 3.5f), bTextScale, bannerTextCol);

    // 6. Name Plate at Bottom: Lebar, tegas, kontras tinggi & terbaca jelas
    float plateH = (actualH < 120.0f) ? 20.0f : 24.0f;
    float plateW = actualW - 12.0f;
    float plateX = rx + 6.0f;
    float plateY = ry + actualH - plateH - 8.0f; // di atas cutout notch bawah
    Color plateBg = (voucher.tier == 2) ? Color(0.18f, 0.15f, 0.24f) : Color(0.12f, 0.16f, 0.20f);
    Color plateBorder = (voucher.tier == 2) ? Theme::GOLD : Color(0.35f, 0.45f, 0.50f);
    drawRect(plateX, plateY, plateW, plateH, plateBg);
    drawRectOutline(plateX, plateY, plateW, plateH, 1.2f, plateBorder);

    // Teks Nama Voucher di dalam Plate (Skala besar, tebal, tanpa teks dobel yang bertumpuk)
    float baseNameScale = (actualH < 120.0f) ? 0.95f : 1.15f;
    float maxTextW = plateW - 8.0f;
    float nameScale = baseNameScale;
    float textW = getTextWidth(voucher.name, nameScale);
    if (textW > maxTextW && textW > 0.0f) {
        nameScale *= (maxTextW / textW);
    }
    float textY = plateY + (plateH - 12.0f * nameScale) * 0.5f + 1.0f;
    Color nameCol = (voucher.tier == 2) ? Theme::GOLD : Theme::TEXT_WHITE;
    drawCenteredText(voucher.name, plateX + plateW * 0.5f, textY, nameScale, nameCol);

    // 7. Central Artwork Frame (Di antara Banner atas dan Name Plate bawah)
    float boxW = actualW - 12.0f;
    float boxX = rx + 6.0f;
    float boxY = bannerY + bannerH + 4.0f;
    float boxH = plateY - boxY - 4.0f;
    drawRect(boxX, boxY, boxW, boxH, Color(0.08f, 0.11f, 0.15f));
    drawRectOutline(boxX, boxY, boxW, boxH, 1.5f, borderCol);

    // Render Ikon Grafis Pixel Art Masing-Masing Voucher (Tanpa teks clutter seperti +1 DISC)
    float cx = boxX + boxW * 0.5f;
    float cy = boxY + boxH * 0.5f;
    drawVoucherEmblem(voucher.type, voucher.tier, cx, cy);

    // 8. If SOLD OUT: Draw dark overlay and bold red stamp
    if (isSoldOut) {
        drawRect(rx, ry, actualW, actualH, Color(0.0f, 0.0f, 0.0f, 0.65f));
        float stampW = actualW - 10.0f;
        float stampH = 22.0f;
        float stampX = rx + 5.0f;
        float stampY = ry + (actualH - stampH) * 0.5f;
        drawRect(stampX, stampY, stampW, stampH, Color(0.85f, 0.15f, 0.15f, 0.95f));
        drawRectOutline(stampX, stampY, stampW, stampH, 1.5f, Color(1.0f, 1.0f, 1.0f, 0.90f));
        drawCenteredText("SOLD OUT", stampX + stampW * 0.5f, stampY + 5.0f, 1.15f, Theme::TEXT_WHITE);
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

    // Dynamic auto-fit text scale so text NEVER spills outside the button
    float targetScale = min(1.6f, max(0.85f, (h - 6.0f) / 12.0f));
    float maxW = w - 8.0f;
    float textW = getTextWidth(text, targetScale);
    if (textW > maxW && textW > 0.0f) {
        targetScale *= (maxW / textW);
        textW = maxW;
    }
    float textX = x + (w - textW) * 0.5f;
    float textY = y + (h - 12.0f * targetScale) * 0.5f;
    drawText(text, textX, textY, targetScale, finalTxt);
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

void Renderer::drawCasinoChip(float cx, float cy, float radius, Color baseCol, Color stripeCol, float angle) {
    // 1. Drop shadow lembut di bawah koin
    glColor4f(0.0f, 0.0f, 0.0f, 0.35f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx + 3.0f, cy + 4.0f);
    for (int a = 0; a <= 28; ++a) {
        float th = a * (6.2831853f / 28.0f);
        glVertex2f(cx + 3.0f + cos(th) * radius, cy + 4.0f + sin(th) * radius);
    }
    glEnd();

    // 2. Lingkaran luar koin dasar
    glColor4f(baseCol.r, baseCol.g, baseCol.b, baseCol.a);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for (int a = 0; a <= 28; ++a) {
        float th = a * (6.2831853f / 28.0f);
        glVertex2f(cx + cos(th) * radius, cy + sin(th) * radius);
    }
    glEnd();

    // 3. Garis-garis putih tepi koin kasino (6 edge stripes khas chip poker)
    glColor4f(stripeCol.r, stripeCol.g, stripeCol.b, stripeCol.a);
    glBegin(GL_QUADS);
    for (int s = 0; s < 6; ++s) {
        float baseA = angle + s * (6.2831853f / 6.0f);
        float a0 = baseA - 0.16f;
        float a1 = baseA + 0.16f;
        float rIn = radius * 0.68f;
        glVertex2f(cx + cos(a0) * rIn, cy + sin(a0) * rIn);
        glVertex2f(cx + cos(a0) * radius, cy + sin(a0) * radius);
        glVertex2f(cx + cos(a1) * radius, cy + sin(a1) * radius);
        glVertex2f(cx + cos(a1) * rIn, cy + sin(a1) * rIn);
    }
    glEnd();

    // 4. Lingkaran cincin pemisah alur (Groove ring)
    glColor4f(0.0f, 0.0f, 0.0f, 0.35f);
    glBegin(GL_LINE_LOOP);
    for (int a = 0; a <= 28; ++a) {
        float th = a * (6.2831853f / 28.0f);
        glVertex2f(cx + cos(th) * (radius * 0.68f), cy + sin(th) * (radius * 0.68f));
    }
    glEnd();

    // 5. Cincin Dalam (Inner Core disc)
    Color innerCol(baseCol.r * 0.82f, baseCol.g * 0.82f, baseCol.b * 0.82f, baseCol.a);
    glColor4f(innerCol.r, innerCol.g, innerCol.b, innerCol.a);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for (int a = 0; a <= 28; ++a) {
        float th = a * (6.2831853f / 28.0f);
        glVertex2f(cx + cos(th) * (radius * 0.50f), cy + sin(th) * (radius * 0.50f));
    }
    glEnd();

    // 6. Ring aksen emas/putih di lingkar dalam
    glColor4f(1.0f, 1.0f, 1.0f, 0.45f);
    glBegin(GL_LINE_LOOP);
    for (int a = 0; a <= 28; ++a) {
        float th = a * (6.2831853f / 28.0f);
        glVertex2f(cx + cos(th) * (radius * 0.50f), cy + sin(th) * (radius * 0.50f));
    }
    glEnd();

    // 7. Bintang Emas 4-Sisi di Pusat Koin
    drawRect(cx - 2.0f, cy - 2.0f, 4.0f, 4.0f, Theme::GOLD);
    drawRect(cx - 1.0f, cy - 4.5f, 2.0f, 9.0f, Theme::GOLD);
    drawRect(cx - 4.5f, cy - 1.0f, 9.0f, 2.0f, Theme::GOLD);
}

void Renderer::drawCardBack(float x, float y, float w, float h, Color backColor) {
    // 1. Drop shadow kartu
    drawRect(x + 5.0f, y + 6.0f, w, h, Color(0.0f, 0.0f, 0.0f, 0.40f));

    // 2. Kertas kartu tepi putih
    drawRect(x, y, w, h, Theme::CARD_BG);
    drawRectOutline(x, y, w, h, 2.0f, Theme::CARD_BORDER);

    // 3. Motif Belakang Merah Khas Kartu Kasino
    float m = 6.0f;
    drawRect(x + m, y + m, w - 2 * m, h - 2 * m, backColor);

    // 4. Bingkai emas dalam kartu
    drawRectOutline(x + m + 3.0f, y + m + 3.0f, w - 2 * m - 6.0f, h - 2 * m - 6.0f, 1.5f, Color(0.95f, 0.85f, 0.35f, 0.85f));

    // 5. Belah ketupat aksen di tengah
    float cx = x + w * 0.5f;
    float cy = y + h * 0.5f;
    float dSize = 18.0f;
    glColor4f(1.0f, 1.0f, 1.0f, 0.35f);
    glBegin(GL_LINE_LOOP);
        glVertex2f(cx, cy - dSize);
        glVertex2f(cx + dSize, cy);
        glVertex2f(cx, cy + dSize);
        glVertex2f(cx - dSize, cy);
    glEnd();

    // Pip emas tengah
    drawRect(cx - 3.0f, cy - 3.0f, 6.0f, 6.0f, Theme::GOLD);
}

void Renderer::drawGlowingAura(float x, float y, float w, float h, Color glowColor, float intensity) {
    for (int step = 1; step <= 4; ++step) {
        float pad = step * 3.5f;
        float alpha = (intensity * 0.15f) / static_cast<float>(step);
        drawRectOutline(x - pad, y - pad, w + 2 * pad, h + 2 * pad, 3.0f, Color(glowColor.r, glowColor.g, glowColor.b, alpha));
    }
}

void Renderer::drawTagIcon(TagType type, float cx, float cy, float size) {
    float half = size * 0.5f;

    switch (type) {
        case TagType::D6_REROLL_TAG: {
            // Ikon Dadu (D6): Kubus putih/merah dengan titik dadu (pips)
            float diceW = size * 0.85f;
            float diceH = size * 0.85f;
            float dx = cx - diceW * 0.5f;
            float dy = cy - diceH * 0.5f;
            drawRect(dx, dy, diceW, diceH, Color(0.92f, 0.25f, 0.22f));
            drawRectOutline(dx, dy, diceW, diceH, 1.5f, Color(0.6f, 0.1f, 0.1f));
            // 5 Pips dadu (titik putih)
            float pipR = 2.0f;
            Color pipCol = Theme::TEXT_WHITE;
            drawRect(cx - pipR, cy - pipR, pipR * 2, pipR * 2, pipCol); // Tengah
            drawRect(dx + 4.0f, dy + 4.0f, pipR * 2, pipR * 2, pipCol); // Kiri atas
            drawRect(dx + diceW - 4.0f - pipR * 2, dy + 4.0f, pipR * 2, pipR * 2, pipCol); // Kanan atas
            drawRect(dx + 4.0f, dy + diceH - 4.0f - pipR * 2, pipR * 2, pipR * 2, pipCol); // Kiri bawah
            drawRect(dx + diceW - 4.0f - pipR * 2, dy + diceH - 4.0f - pipR * 2, pipR * 2, pipR * 2, pipCol); // Kanan bawah
            break;
        }
        case TagType::SPEED_TAG: {
            // Ikon Petir / Speed ($$) warna Emas
            drawCenteredText("$$", cx, cy - 6.0f, 1.4f, Theme::GOLD);
            break;
        }
        case TagType::HANDY_TAG: {
            // Ikon Telapak Tangan (Hand)
            float hx = cx - 7.0f;
            float hy = cy - 8.0f;
            // Telapak
            drawRect(hx, hy + 5.0f, 14.0f, 10.0f, Color(0.20f, 0.55f, 0.90f));
            // 4 Jari ke atas
            drawRect(hx + 1.0f, hy, 2.5f, 6.0f, Color(0.20f, 0.55f, 0.90f));
            drawRect(hx + 4.5f, hy - 2.0f, 2.5f, 8.0f, Color(0.20f, 0.55f, 0.90f));
            drawRect(hx + 8.0f, hy - 1.0f, 2.5f, 7.0f, Color(0.20f, 0.55f, 0.90f));
            drawRect(hx + 11.5f, hy + 2.0f, 2.5f, 5.0f, Color(0.20f, 0.55f, 0.90f));
            // Jempol ke kiri
            drawRect(hx - 3.0f, hy + 7.0f, 4.0f, 3.0f, Color(0.20f, 0.55f, 0.90f));
            break;
        }
        case TagType::GARBAGE_TAG: {
            // Ikon Tempat Sampah (Bin)
            float bx = cx - 6.0f;
            float by = cy - 7.0f;
            // Tutup tempat sampah
            drawRect(bx - 2.0f, by, 16.0f, 2.5f, Color(0.85f, 0.35f, 0.30f));
            drawRect(cx - 2.0f, by - 2.0f, 4.0f, 2.0f, Color(0.85f, 0.35f, 0.30f));
            // Badan kaleng sampah
            drawRect(bx, by + 3.5f, 12.0f, 12.0f, Color(0.85f, 0.35f, 0.30f));
            // Garis alur kaleng
            drawRect(bx + 3.0f, by + 5.5f, 1.5f, 8.0f, Theme::TEXT_WHITE);
            drawRect(bx + 7.5f, by + 5.5f, 1.5f, 8.0f, Theme::TEXT_WHITE);
            break;
        }
        case TagType::ECONOMY_TAG: {
            // Ikon Pengganda Uang "X2" warna Hijau/Emas
            drawRect(cx - half + 2.0f, cy - half + 2.0f, size - 4.0f, size - 4.0f, Color(0.18f, 0.60f, 0.32f));
            drawRectOutline(cx - half + 2.0f, cy - half + 2.0f, size - 4.0f, size - 4.0f, 1.0f, Color(0.10f, 0.40f, 0.20f));
            drawCenteredText("X2", cx, cy - 5.0f, 1.3f, Theme::TEXT_WHITE);
            break;
        }
        case TagType::COUPON_TAG: {
            // Ikon Kupon Diskon Belanja (Tag Kertas)
            float tx = cx - 7.0f;
            float ty = cy - 8.0f;
            drawRect(tx, ty, 14.0f, 16.0f, Color(0.95f, 0.65f, 0.15f));
            drawRectOutline(tx, ty, 14.0f, 16.0f, 1.0f, Color(0.70f, 0.45f, 0.05f));
            // Lubang gantung kupon
            drawRect(cx - 2.0f, ty + 2.0f, 4.0f, 4.0f, Color(0.15f, 0.20f, 0.25f));
            drawCenteredText("%", cx, cy, 1.0f, Theme::TEXT_WHITE);
            break;
        }
    }
}

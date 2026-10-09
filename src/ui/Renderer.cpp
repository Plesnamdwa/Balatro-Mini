#include <GLFW/glfw3.h>
#include "../../include/ui/Renderer.h"
#include "../../include/ui/PixelFont.h"
#include <algorithm>

using namespace std;

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

void Renderer::drawCard(const Card& card, float x, float y, float w, float h, bool isHovered) {
    float renderY = card.isSelected() ? y - 28.0f : y;

    // 1. Bayangan kartu
    drawRect(x + 4.0f, renderY + 4.0f, w, h, Color(0.0f, 0.0f, 0.0f, 0.35f));

    // 2. Kertas kartu
    Color bodyColor = card.isDebuffed() ? Color(0.85f, 0.85f, 0.85f) : Theme::CARD_BG;
    drawRect(x, renderY, w, h, bodyColor);

    // 3. Border
    if (card.isSelected()) {
        drawRectOutline(x, renderY, w, h, 4.0f, Theme::CARD_SELECTED);
    } else if (isHovered) {
        drawRectOutline(x, renderY, w, h, 3.0f, Color(1.0f, 1.0f, 1.0f, 0.8f));
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

    // 6. Simbol Tengah
    drawText(suitStr, x + (w / 2.0f) - 14.0f, renderY + (h / 2.0f) - 8.0f, 2.0f, suitColor);

    // 7. Base Chips Pojok Kanan Bawah
    string chipStr = "+" + to_string(card.getBaseChips());
    drawText(chipStr, x + w - 38.0f, renderY + h - 22.0f, 1.5f, Theme::CHIPS_BLUE);

    // 8. Label DEBUFF jika aktif
    if (card.isDebuffed()) {
        drawRect(x, renderY + h - 34.0f, w, 14.0f, Color(0.2f, 0.2f, 0.2f, 0.8f));
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

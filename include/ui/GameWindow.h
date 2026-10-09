#ifndef GAME_WINDOW_H
#define GAME_WINDOW_H

#include <GLFW/glfw3.h>
#include "../core/GameEngine.h"
#include "Renderer.h"
#include <string>

#include <vector>

using namespace std;

struct CardAnim {
    float x = 0.0f;
    float y = 530.0f;
    float elevation = 0.0f;
    bool initialized = false;
};

class GameWindow {
private:
    GLFWwindow* window;
    int windowWidth;
    int windowHeight;
    string windowTitle;
    GameEngine engine;

    // State Animasi Kartu yang Mulus (Lerp & Hover Physics)
    vector<CardAnim> cardAnims;

    // State Toast Notification (Error / Warning feedback)
    string toastMessage;
    float toastTimer;

    // Tracking Mouse
    double mouseX;
    double mouseY;

    void processInput();
    void updateCardAnimations(float dt);
    void renderUI(float dt);

    // Layar Menu Utama (Title Screen)
    void renderMainMenu();

    // Layar Pemilihan Blind (Small, Big, Boss Blind)
    void renderBlindSelect();

    // Layar Gameplay Ronde
    void renderRoundGameplay();
    void renderHUD();
    void renderCards();
    void renderControls();
    void renderScoringPreview();

    bool isPointInRect(float px, float py, float rx, float ry, float rw, float rh) const;

public:
    GameWindow(int width = 1024, int height = 720, const string& title = "Balatro Mini");
    ~GameWindow();

    bool init();
    void run();
    void showToast(const string& message);

    // Callback mouse click & keyboard
    void handleMouseClick(int button, int action, int mods);
    void handleKey(int key, int scancode, int action, int mods);
};

#endif // GAME_WINDOW_H

# Balatro Mini — Low-Fidelity Poker Roguelike

An interactive, low-fidelity poker roguelike inspired by **Balatro**, built in modern **C++17** with **OpenGL** and **GLFW**. The game faithfully implements the iconic **Chips × Mult** scoring formula, 9 official poker hand classifications, blind escalation across Antes, and unique Boss Blind debuffs. The presentation features a custom 60 FPS retro casino aesthetic powered by an in-memory monochrome 8x8 bitmap font, requiring **zero external texture or font files**.

Under the hood, the project is architected with clean **Object-Oriented Design Patterns** (Factory Method, Strategy, and State Machine) coupled with rigorous defensive error handling to prevent illegal moves and crashes.

> **Platform:** Windows (x64) — compiled with MinGW-w64 GCC.

---

## Features

### Core Game Mechanics
- **Authentic Chips × Mult Scoring** — matches the mathematical calculation from Balatro:
  $$\text{Total Score} = (\text{Base Chips} + \sum \text{Card Chips}) \times (\text{Base Mult} + \text{Bonus Mult})$$
- **9 Poker Hand Combinations** — accurately detects and prioritizes played hands from 1 to 5 cards:
  - *High Card* (5 Chips × 1 Mult)
  - *Pair* (10 Chips × 2 Mult)
  - *Two Pair* (20 Chips × 2 Mult)
  - *Three of a Kind* (30 Chips × 3 Mult)
  - *Straight* (30 Chips × 4 Mult, supporting standard straights and Ace-low *5-4-3-2-A*)
  - *Flush* (35 Chips × 4 Mult, 5 cards of identical suit)
  - *Full House* (40 Chips × 4 Mult)
  - *Four of a Kind* (60 Chips × 7 Mult)
  - *Straight Flush* (100 Chips × 8 Mult)
- **Scoring Card Discrimination** — only cards actively contributing to the formed combination score chips (e.g., in a Pair of Kings with a kicker 4, only the Kings add card chips).
- **Ante & Blind Escalation**:
  - *Small Blind*: Standard score target ($1.0\times$), $\$3$ reward.
  - *Big Blind*: Higher score target ($1.5\times$), $\$4$ reward.
  - *Boss Blind*: Demanding score target ($2.0\times$), $\$5$ reward, with active **Boss Debuffs**.
  - *Ante Scaling*: Conquering the Boss Blind advances to the next Ante ($1 \rightarrow 8$) with exponentially scaling target scores.
- **Boss Blind Debuffs**:
  - *The Heart*: All Heart cards are debuffed (provide $0$ Chips).
  - *The Spade*: All Spade cards are debuffed (provide $0$ Chips).
  - *Hand Size Reduction*: Hand capacity permanently reduced from 8 to 7 cards for the round.
- **Complete Run Lifecycle**:
  - *Title Screen* $\rightarrow$ *Blind Selection* $\rightarrow$ *In-Round Play* $\rightarrow$ *Round Cleared (Cash Out)* $\rightarrow$ *Ante Escalation* or *Game Over*.

### Low-Fidelity Retro Presentation & UI
- **2D Immediate-Mode Pipeline** — rendered using OpenGL orthographic projection (`glOrtho`) with hardware alpha blending at 60 FPS V-Sync.
- **Felt-Green Table Aesthetic** — classic dark casino tabletop with rounded low-fi card geometry and high-contrast suit colors (Crimson Red for Hearts/Diamonds, Dark Charcoal for Spades/Clubs).
- **Interactive Card Elevation** — clicking cards lifts them smoothly by 28 pixels with a glowing golden border and hover state highlights.
- **Real-Time Live Hand Preview** — dynamically evaluates selected cards on the fly, rendering live poker hand detection and estimated Chips $\times$ Mult score boxes before pressing Play.
- **Built-in 8x8 Bitmap Pixel Font** — an embedded, header-only ASCII raster font that draws crisp retro pixel letters directly to quads without FreeType or image assets.

### Software Architecture & Design Patterns
- **Factory Method Pattern (`IDeckFactory`, `StandardDeckFactory`)** — decouples standard 52-card poker deck instantiation and Fisher-Yates shuffle algorithms (`std::mt19937`) from deck storage.
- **Strategy Pattern (`IPokerEvaluatorStrategy`, `StandardPokerEvaluator`)** — isolates poker hand classification and ranking into interchangeable runtime evaluation strategies utilized by `ScoringEngine`.
- **State Pattern (`IGameState`)** — robust finite state machine encapsulating distinct state behaviors:
  - `MainMenuState`: Handles game launch and new run initialization.
  - `BlindSelectState`: Displays the 3 blind cards (upcoming, active, or defeated).
  - `SelectingCardState`: Manages player input, card selection, discards, and playing hands.
  - `RoundWonState`: Handles cash collection and transitions back to Blind selection.
  - `GameOverState`: Freezes round inputs and provides quick restart options.
- **Robust Defensive Error Handling** — structured exception hierarchy (`GameException`, `InvalidCardCountException`, `InvalidActionException`, `DeckEmptyException`) with user-friendly retro toast alert banners. The game never crashes or freezes on illegal inputs (e.g. selecting $>5$ cards, empty plays, or zero discards).

---

## Dependencies

| Dependency | Purpose |
|---|---|
| **C++17 Compiler** | Standard library features (`std::unique_ptr`, `std::vector`, `std::map`, `<random>`, chrono) |
| **[GLFW 3](https://www.glfw.org/)** (v3.5.1 included) | Cross-platform window creation, OpenGL context, mouse & keyboard events |
| **OpenGL** (`opengl32`) | Hardware-accelerated 2D rendering (ships with Windows) |
| **GDI32** (`gdi32`) | Win32 graphics device interface (ships with Windows) |

GLAD, GLEW, SDL, and external font/image libraries are **not** required. The GLFW static library and headers are pre-bundled in `lib/` and `include/` for instant zero-configuration builds.

Recommended toolchains: **MinGW-w64 GCC** (GCC 10+) or MSYS2.

---

## How to Build

### Option A — Instant Batch Build (Recommended)

1. Open a terminal or Command Prompt inside the `balatro-mini` folder.
2. Run the build script:
   ```cmd
   build_and_run.bat
   ```
3. The script compiles all sources, links GLFW/OpenGL, copies binaries, and launches `balatro_mini.exe`.

### Option B — Manual Command-Line Compilation (MinGW-w64)

Compile directly with `g++`:
```bash
g++ -std=c++17 -Wall -Wextra -Iinclude src/main.cpp -Llib -lglfw3 -lopengl32 -lgdi32 -o balatro_mini.exe
```
Then execute:
```bash
./balatro_mini.exe
```

### Option C — VS Code Integration

1. Open the `balatro-mini` folder in **Visual Studio Code**.
2. Open `src/main.cpp`.
3. Press **F5** (or run the default build task `Ctrl + Shift + B`). Pre-configured `.vscode/tasks.json` and `launch.json` will build and start debugging automatically.

### Option D — Standalone Portable Release (No Compiler Needed)

1. Download `Balatro-Mini-v1.0.0-Windows.zip` from the **Releases** page.
2. Extract the archive.
3. Double-click `run_game.bat` or `balatro_mini.exe` to play immediately.

---

## Controls

### Mouse

| Action | Effect |
|---|---|
| **Left Click on Card** | Select / deselect a card (maximum 5 cards, lifts card upward) |
| **Click `PLAY HAND`** | Score the selected poker hand and consume 1 Hand quota |
| **Click `DISCARD`** | Discard selected cards and draw replacements from the deck (consumes 1 Discard) |
| **Click `SELECT BLIND`** | Enter the active blind round from the Blind Selection screen |
| **Click `CASH OUT`** | Collect blind monetary reward and return to Choose Blind screen |
| **Click `START NEW RUN`** | Begin a fresh run at Ante 1 Small Blind |
| **Click `MAIN MENU` / `QUIT`** | Return to Title Screen or close the application |

### Keyboard

| Key | Action |
|---|---|
| `1` – `8` | Toggle card selection for cards 1 through 8 in your hand |
| `Space` / `Enter` | Trigger primary action: Play Hand, Select Blind, Cash Out, or Start Run |
| `D` | Discard selected cards |
| `R` | Restart run immediately |
| `Esc` | Return to Main Menu or exit game |

---

## Notes

- **Hand Quotas**: Each round starts with **4 Hands** and **3 Discards**. Both replenish automatically upon starting a new blind.
- **Card Selection Cap**: In accordance with authentic Balatro rules, a maximum of 5 cards can be played or discarded in a single action. Attempting to select a 6th card triggers an on-screen toast warning.
- **Self-Contained Executable**: All visual assets and fonts are mathematically rasterized at runtime via OpenGL primitives, making the executable completely portable.
- **Unit Testing**: Modular unit test suites for individual development milestones can be built and run using `build_segmen1.bat`, `build_segmen2.bat`, and `build_segmen3.bat`.

---

## Project Structure

```
balatro-mini/
├── include/
│   ├── core/
│   │   ├── Card.h              # Card data model (Rank, Suit, Base Chips)
│   │   ├── Deck.h              # Deck draw, discard, and recycle pile
│   │   ├── DeckFactory.h       # Factory Method interface & standard deck builder
│   │   ├── PokerHand.h         # Hand type enums and Balatro base chip/mult stats
│   │   ├── PokerEvaluator.h    # Strategy Pattern interface for poker classification
│   │   ├── ScoringEngine.h     # Chips x Mult calculation context
│   │   ├── Blind.h             # Small, Big, and Boss Blind management & Ante scaling
│   │   ├── GameState.h         # State Pattern interface & concrete states
│   │   ├── GameEngine.h        # Central game engine context & state delegates
│   │   └── Exceptions.h        # Custom game exception hierarchy
│   ├── ui/
│   │   ├── PixelFont.h         # Monochrome 8x8 bitmap font rasterizer
│   │   ├── Renderer.h          # 2D OpenGL immediate-mode rendering helpers
│   │   └── GameWindow.h        # GLFW window lifecycle, event loop, and UI views
│   └── GLFW/                   # Bundled GLFW 3.5.1 C++ headers
├── src/
│   ├── core/                   # Business logic implementations
│   ├── ui/                     # UI, font rasterizer, and window implementations
│   └── main.cpp                # Application entry point
├── lib/
│   └── libglfw3.a              # Static MinGW-w64 GLFW library
├── tests/
│   ├── test_segment1.cpp       # Deck, Card & Factory Pattern unit tests
│   ├── test_segment2.cpp       # Poker evaluator & scoring unit tests
│   └── test_segment3.cpp       # State machine, blind progression & quota unit tests
├── run_game.bat                # Portable launcher script
├── build_and_run.bat           # One-click build and launch script
└── README.md
```

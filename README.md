# Interactive C++ Chess Application

A high-performance, command-line chess engine and interactive application written in C++20. Designed around **Data-Oriented Design (DOD)** principles, bitboard representations, and branchless execution paths to maximize movement validation and check simulation performance.

---

## Key Architectural & Design Highlights

* **Bitboard Representation:** Uses 64-bit unsigned integers (`uint64_t`) to represent piece placements and board occupancy, allowing ultra-fast bitwise move calculations.
* **Data-Oriented Structure (SoA / AoA):** Separates core board state, movement components, and evaluation frames into contiguous, cache-friendly data layouts.
* **Branchless & Mask-Based Logic:** Utilizes truth masks, bitwise arithmetic, and bit manipulation primitives (`std::bit_floor`, `std::countr_zero`) to minimize branching overhead during ray casting and move validation.
* **Modular System Architecture:** Separate decoupled systems for player input, move generation/validation, board state updates, check/mate evaluation, and console visualization.

---

## Application Features

- **Legal Move Validation:** Handles piece-specific movement vectors (pawns, knights, rooks, bishops, queens, and kings).
- **Special Moves Support:** Integrated support for initial double pawn steps, capture/attack masks, and en-passant tracking.
- **Check & Checkmate Simulation Frame:** Clones and simulates future states using lightweight frame components (`CheckSimFrameComponent`) to evaluate discovered checks, imposed checks, and checkmate conditions.
- **Material Evaluation Interface:** Tracks captures and positional values for upcoming engine/AI integrations.
- **Console Board Visualizer:** Renders an ASCII-based chess board directly in the terminal interface.

---

## Directory & File Structure

```text
Interactive Chess Application/
├── App.cpp                            # Application entry point (Main execution loop)
├── init_game_state.h / .cpp           # Board initialization and initial piece layout (SoA)
├── movement_data_component.h          # Data component for current turn/piece movement
├── movement_data_system.h / .cpp      # Calculates active move indices, colors, and piece masks
├── move_validation_system.h / .cpp    # Core bitwise raycasting & legal move generator
├── board_updating_system.h / .cpp     # Enacts board state mutation, captures, & en-passant logic
├── eval_check_and_mate_system.h / .cpp# Handles check, discovered check, & checkmate evaluation
├── display.h / .cpp                   # ASCII board layout renderer
├── player_input.h / .cpp              # CLI user input parser and validator
│
├── Components & Helpers /
│   ├── check_sim_frame_component.h    # Lightweight frame buffer for check simulations
│   ├── positional_eval_component.h    # Positional evaluation component
│   ├── positional_eval_system.h / .cpp# Material & positional scoring tracking
│   ├── stalemate_data_component.h     # Stalemate tracking component
│   └── trace_path_component.h         # Ray tracing path buffer for checking vectors
│
└── Namespaces & Enumerations /
    ├── piece_info.h                   # Piece indices and types
    ├── occupancy_info.h               # Color & board occupancy indices
    ├── move_info.h                    # Move phase identifiers
    ├── extract_ray_type_info.h        # Attack rays vs. check rays enum
    ├── king_move_indicies.h           # Directional indexing for king movements
    ├── king_subopt_info.h             # Optimization flags for king evaluations
    ├── ray_direction_info.h           # Ray direction constants
    └── ray_transposition_info.h       # Diagonal/Non-diagonal shift info
```
# Requirements & Environment

Language Standard: C++20 (stdcpp20 minimum required for std bit manipulation functions).
Compiler: MSVC (Visual Studio 2022 / v143 toolset recommended), GCC 10+, or Clang 11+.
Platform: Windows (x64 or Win32) / Linux / macOS.

# Build & Run Instructions
## Option 1: Visual Studio (Windows)

Open Interactive Chess Application.vcxproj or the solution file in Visual Studio 2022.
Select Release or Debug configuration (x64 recommended).
Ensure C++ Standard is set to ISO C++20 Standard (/std:c++20).
Build (Ctrl+Shift+B) and run (Ctrl+F5).

## Option 2: Command Line (GCC / Clang)
Run the following command in the project root directory:

g++ -std=c++20 -O3 *.cpp -o InteractiveChessApp && ./InteractiveChessApp

# How to Play
Upon execution, the initial board state will be drawn to the console.
Enter moves when prompted using board square indices by typing the origin square and the target square with a dash in between (e.g a1-a2).
The board will automatically validate your move against legal bitmasks, apply state transformations, check for checks/mates, and toggle turn state.

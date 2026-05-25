#include "bitwise_move_validation.h"
#include "display.h"
#include "init_game_state.h"
#include "player_input.h"
#include "positional_evaluation.h"
#include <cstdint>
#include <iostream>

int main() {
  // Initialize game state
  InitGameState initBoard;
  InitGameState::Board board = initBoard.getInitBoardState();

  // Initialize validation for a given board (statics matter)
  BitwiseMoveValidation validateThisBoard(board);

  while (true) { // Main loop
    // Draw board
    Display::displayBoard(&board);
    // Input validation
    auto [picked_square_idx, placement_square_idx] =
        PlayerInput::moveHandling(); // obtain square indices
    // Move validation
    validateThisBoard.setUpdatedState(picked_square_idx, placement_square_idx);
    if (!validateThisBoard.callPieceTypesValidator()) {
      continue;
    }
  }
  // determine outcome
  // clear board

  return 0;
}

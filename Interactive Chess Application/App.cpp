#include <iostream>
#include <cstdint>
#include "init_game_state.h"
#include "player_input.h"
#include "display.h"
#include "bitwise_move_validation.h"

int main() {
	//Initialize game state
	InitGameState initBoard;
	InitGameState::Board board = initBoard.getInitBoardState();

	while (true) { //Main loop
	  //Draw board
	  Display::displayBoard(board);
	  //Input validation
	  auto [picked_square_idx, placement_square_idx] = PlayerInput::moveHandling(); //obtain square indicies
	  //Move validation
	  BitwiseMoveValidation validateThisBoard(board, picked_square_idx, placement_square_idx);
	  validateThisBoard.callPieceTypesValidator();
	}
	

	//determine outcome 
	//clear board

	return 0;
} 
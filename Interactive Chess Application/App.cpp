#include <iostream>
#include <cstdint>
#include "init_game_state.h"
#include "player_input.h"
#include "display.h"
#include "bitwise_move_validation.h"
#include "positional_evaluation.h"
#include "movement_data_component.h"
#include "movement_data_system.h"
#include "move_info.h"
#include "move_validation_system.h"
#include "board_updating_system.h"
#include "positional_eval_component.h"
#include "positional_eval_system.h"

int main() {
	//Initialize game state
	InitGameState init_board;
	InitGameState::Board board = init_board.getInitBoardState();

	//Initialize movement state,
	MovementData movement_data;
	
	while (true) {

		//---------------//---------------//--------------- PREPERATION ---------------//---------------//---------------//---------------
		//Create state for an AI instance
		PositionalEvalComponent pos_eval;

		//Draw board
		Display::displayBoard(&board);

		//Input validation
		auto [picked_square_idx, placement_square_idx] = PlayerInput::moveHandling(); //obtain square indices
		
		//---------------//---------------//--------------- MOVEMENT VALIDATION ---------------//---------------//---------------//---------------
		//Obtain movement data for the picked piece
		GetMovementInfoSystem::executeMovementInfo(board, movement_data, moveInfo::picked, picked_square_idx, placement_square_idx);

		//Validate legal moves for the picked piece
		uint64_t valid_moves = MoveValidationSystem::validator(board, movement_data);

		//Obtain movement data for the placed piece
		GetMovementInfoSystem::executeMovementInfo(board, movement_data, moveInfo::placed, picked_square_idx, placement_square_idx);
		
		//Look if there are any legal moves, to enforce correctness
		uint64_t piece_placement = BoardUpdatingSystem::movementValidation(movement_data, valid_moves);
		if (piece_placement == 0ULL) {
			PlayerInput::outOfScope();
			continue;
		}
		//Update the board with new values
		
		BoardUpdatingSystem::updateBoards(board, movement_data, pos_eval, piece_placement);
		//---------------//---------------//---------------//---------------//---------------//---------------//---------------//---------------

		//Move validation (commented out for now while refactoring)
		/*validateThisBoard.setUpdatedState(picked_square_idx, placement_square_idx);
		if (!validateThisBoard.callPieceTypesValidator()) {
			continue;
		}*/
	}
	//cleanup the code
	//uint tests

	return 0;
}
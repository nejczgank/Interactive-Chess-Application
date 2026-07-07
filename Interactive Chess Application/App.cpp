#include <iostream>
#include <cstdint>
#include "init_game_state.h"
#include "player_input.h"
#include "display.h"
#include "bitwise_move_validation.h"
#include "positional_evaluation.h"
#include "movement_data_component.h"
#include "movement_data_system.h"
#include "move_validation_system.h"
#include "board_updating_system.h"
#include "positional_eval_component.h"
#include "positional_eval_system.h"
#include "special_move_validation_component.h"
#include "special_move_validation_system.h"

int main() {

	//scoped enum declaration
	using enum moveInfo::move;

	//Initialize game state
	InitGameState init_board;
	InitGameState::Board board = init_board.getInitBoardState();

	//Initialize movement state,
	MovementData movement_data;

	//Create state for an AI instance
	PositionalEvalComponent pos_eval;

	//bool is_king_check = SpecialMoveValidationSystem::isKingCheck(board, 0);

	while (true) {

		//---------------//---------------//--------------- PREPERATION ---------------//---------------//---------------//---------------
		//Draw board
		Display::displayBoard(&board);

		//Input validation
		//!THIS WILL HAVE TO PREVENT PICKING THE OPPONENTS PIECES
		auto [picked_square_idx, placement_square_idx] = PlayerInput::moveHandling(); //obtain square indices
		
		//---------------//---------------//--------------- MOVEMENT VALIDATION ---------------//---------------//---------------//---------------
		//Obtain movement data for the picked piece
		GetMovementInfoSystem::pickingInfo(board, movement_data, picked_square_idx, placement_square_idx);

		//Validate legal moves for the picked piece
		const uint64_t VALID_MOVES = MoveValidationSystem::validator(board, movement_data);

		//Look if there are any legal moves, to enforce correctness
		const uint64_t VALID_PIECE_PLACEMENT = BoardUpdatingSystem::movementValidation(movement_data, VALID_MOVES);
		if (VALID_PIECE_PLACEMENT == 0ULL) {
			PlayerInput::outOfScope();
			continue;
		}

		//Obtain movement data for the placed piece
		GetMovementInfoSystem::placementInfo(board, movement_data, placement_square_idx);
		
		//Update the board with new values
		BoardUpdatingSystem::updateBoards(board, movement_data, pos_eval, VALID_PIECE_PLACEMENT);
		//---------------//---------------//---------------//---------------//---------------//---------------//---------------//---------------
	}

	std::cin.ignore();
	return 0;
}
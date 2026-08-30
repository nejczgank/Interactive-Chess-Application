#include <iostream>
#include <cstdint>
#include "init_game_state.h"
#include "player_input.h"
#include "display.h"
#include "movement_data_component.h"
#include "movement_data_system.h"
#include "move_validation_system.h"
#include "board_updating_system.h"
#include "positional_eval_component.h"
#include "positional_eval_system.h"
#include "eval_check_and_mate_system.h"
#include "stalemate_data_component.h"
#include "trace_path_component.h"
#include "check_sim_frame_component.h"
#include "king_subopt_info.h"
#include "extract_ray_type_info.h"

int main() {

	//scoped enum declaration
	using enum moveInfo::move;
	using enum ExtractRayTypeInfo::ray_type;

	//Initialize game state
	InitGameState init_board;
	InitGameState::Board board = init_board.getInitBoardState();

	//Initialize movement state,
	MovementData movement_data;

	//Create state for an AI instance
	PositionalEvalComponent pos_eval;

	//Initialize stalemate data
	StalemateDataComponent stalemate_data{};

	//Initialize player turn state
	bool white_turn = 1;
	bool black_turn = 0;

	while (true) {

		//---------------//---------------//--------------- PREPERATION ---------------//---------------//---------------//---------------
		//Draw board
		Display::displayBoard(&board);

		//Input validation
		auto [picked_square_idx, placement_square_idx] = PlayerInput::moveHandling(); //obtain square indices
		
		//Determine color of the picked and placed piece
		GetMovementInfoSystem::basicInfo(board, movement_data, picked_square_idx, placement_square_idx);

		//enforce picked piece color correctness, and disallow an empty square to represent the picked piece
		const int TURN = PlayerInput::turnValidation(board, movement_data, white_turn, black_turn);

		constexpr int NO_VALID_TURN = 0;
		if (TURN == NO_VALID_TURN)
		{
			PlayerInput::invalidPick();
			continue;
		}

		//---------------//---------------//--------------- MOVEMENT VALIDATION ---------------//---------------//---------------//---------------
		//Obtain movement data for the picked piece
		GetMovementInfoSystem::pickingInfo(board, movement_data);

		//Validate legal moves for the picked piece
		TracePathComponent probe_path_data;
		const uint64_t VALID_MOVES = MoveValidationSystem::validator(board, movement_data, probe_path_data, attack_ray);

		//Look if there are any legal moves, to enforce correctness
		const uint64_t VALID_PIECE_PLACEMENT = BoardUpdatingSystem::movementValidation(movement_data, VALID_MOVES);

		constexpr uint64_t NO_VALID_PLACEMENT = 0ULL;
		if (VALID_PIECE_PLACEMENT == NO_VALID_PLACEMENT)
		{
			PlayerInput::outOfScope();
			continue;
		}

		//Obtain movement data for the placed piece
		GetMovementInfoSystem::placementInfo(board, movement_data);

		//---------------//---------------//--------------- CHECK VALIDATION / GAME ENDING CONDITION ---------------//---------------//---------------//---------------

		//obtain an evaluation regarding checks or checkmates
		const int check = EvalCheckAndMateSystem::checkmateHandler(board, movement_data, pos_eval, VALID_PIECE_PLACEMENT, stalemate_data);

		//---------------//---------------//--------------- UPDATE STATE ---------------//---------------//---------------//---------------

		//Update the board with new values
		BoardUpdatingSystem::updateBoards(board, movement_data, pos_eval, VALID_PIECE_PLACEMENT);

		//Update turn state
		white_turn ^= 1;
		black_turn ^= 1;
		//---------------//---------------//---------------//---------------//---------------//---------------//---------------//---------------
	}

	std::cin.ignore();
	return 0;
}
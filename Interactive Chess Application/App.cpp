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
#include "king_subopt_info.h"
#include "extract_ray_type_info.h"
#include "en_passant_state_component.h"
#include "game_state_component.h"
#include "extract_path_component.h"
#include "check_type_eval_flags_info.h"
#include "eval_board_state_edit_info.h"
#include "check_result_flags_info.h"

int main() {

	//scoped enum declaration
	using enum moveInfo::move;
	using enum ExtractRayTypeInfo::ray_type;
	using enum CheckResultFlagsInfo::eval_flags;

	//Initialize game state
	InitGameState init_board{};
	InitGameState::Board board = init_board.getInitBoardState();

	//Initialize movement state,
	MovementData movement_data{};

	//En passant state component
	EnPassantStateComponent en_passant_data{}; //used for move reversa l

	//Create state for an AI instance
	PositionalEvalComponent pos_eval_data{};

	//Initialize stalemate data
	StalemateDataComponent stalemate_data{};

	//Initialize validated placement bit information
	uint64_t VALID_PIECE_PLACEMENT{};

	//Introduce Context Parameter Object
	GameStateContextComponent game_state_ctx{
		board,
		movement_data,
		pos_eval_data,
		//VALID_PIECE_PLACEMENT,
		en_passant_data
	};

	//Initialize player turn state
	bool white_turn = 1;
	bool black_turn = 0;

	//Initialize checkmate data
	std::string winner{};
	int eval_check{};
	int prev_check{};

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
		//TracePathComponent probe_path_data;
		ExtractPathComponent probe_path_data{ {}, attack_ray };
		const uint64_t VALID_MOVES = MoveValidationSystem::validator(board, movement_data, probe_path_data);

		//Look if there are any legal moves, to enforce correctness
		VALID_PIECE_PLACEMENT = BoardUpdatingSystem::movementValidation(movement_data, VALID_MOVES);

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
		eval_check = EvalCheckAndMateSystem::checkmateHandler(game_state_ctx, VALID_PIECE_PLACEMENT);

		//determine check and update state of the game
		switch (eval_check)
		{
		case discovered:
			std::cout << "Illegal move. King exposed to an incoming attack\n";
			continue;
		case check:
			if (prev_check == check)
			{
				std::cout << "Illegal move. Check not resolved\n";
				continue;
			}
			std::cout << "Check\n";
			break;
		case checkmate:
			BoardUpdatingSystem::updateBoards(game_state_ctx, VALID_PIECE_PLACEMENT);
			Display::displayBoard(&board);
			winner = movement_data.attacker_color == 0 ? "White" : "Black";
			std::cout << "Checkmate " << winner << " wins!\n\n\n";
			return 0;
		};
		std::cout << eval_check << '\n';

		//---------------//---------------//--------------- UPDATE STATE ---------------//---------------//---------------//---------------

		//Update the board with new values
		BoardUpdatingSystem::updateBoards(game_state_ctx, VALID_PIECE_PLACEMENT);

		//Update turn state
		white_turn ^= 1;
		black_turn ^= 1;

		//previous check state
		prev_check = eval_check;
		//---------------//---------------//---------------//---------------//---------------//---------------//---------------//---------------
	}

	std::cin.ignore();
	return 0;
}
#include "board_updating_system.h"

uint64_t BoardUpdatingSystem::movementValidation(MovementData& movement_data, uint64_t found_moves)
{
	/*
	* If where the player wants to place the square is among valid moves
	* for the selected piece, then those valid moves are returned.
	* Otherwise no valid moves are returned
	*/

	return found_moves & (1ULL << movement_data.placement_square_idx);
}

void BoardUpdatingSystem::updateBoards(InitGameState::Board& board, MovementData& movement_data,  PositionalEvalComponent& pos_eval_data, const uint64_t VALID_PIECE_PLACEMENT)
{
	/*
	* Handles piece placement or replacement once a move is enacted and validated
	* This is done by first clearing square where the picked piece resides, then the piece at the occupied square (if it exists!).
	* and lastly placing the picked piece at the desired location
	*/

	using enum occupancyInfo::occupancy;

	constexpr int FIRST_BLACK_PIECE = 6;
	const int ATTACKER_COLOR = (movement_data.picked_piece_type >= FIRST_BLACK_PIECE); // 0 - white, 1 - black
	const int DEFENDER_COLOR = 1 - ATTACKER_COLOR;

	clearPickedPieceHelper(board, movement_data, ATTACKER_COLOR);

	//a piece was overtaken
	constexpr int NO_PIECE_FLAG = -1;
	if (movement_data.placed_piece_type != NO_PIECE_FLAG) 
	{
		clearOvertakenSquareHelper(board, movement_data, pos_eval_data, VALID_PIECE_PLACEMENT, ATTACKER_COLOR, DEFENDER_COLOR);
	}

	placeNewPieceHelper(board, movement_data, VALID_PIECE_PLACEMENT, ATTACKER_COLOR, DEFENDER_COLOR);

	//adjust all occupancy
	board.occupancy[all] = board.occupancy[white] | board.occupancy[black];
}

void BoardUpdatingSystem::clearPickedPieceHelper(InitGameState::Board& board, MovementData& movement_data, int ATTACKER_COLOR)
{
	//clearing the boards for the picked piece
	board.occupancy[ATTACKER_COLOR] &= ~(1ULL << movement_data.picked_square_idx);

	//clearing the piece
	board.pieces[movement_data.picked_piece_type] &= ~(1ULL << movement_data.picked_square_idx);
}

void BoardUpdatingSystem::clearOvertakenSquareHelper(InitGameState::Board& board, MovementData& movement_data, PositionalEvalComponent& pos_eval_data, const uint64_t VALID_PIECE_PLACEMENT, const int ATTACKER_COLOR, const int DEFENDER_COLOR)
{
	//passes the value of the overtaken piece to the AI system
	int const MATERIAL_COUNTS[] = { 1, 3, 3, 5, 9, 0 };
	int constexpr COLOR_CORRECTION = 6;

	//calculates the value of the capture
	int const own_material_count = ATTACKER_COLOR * MATERIAL_COUNTS[movement_data.placed_piece_type % COLOR_CORRECTION];
	int const opposite_material_count = DEFENDER_COLOR * MATERIAL_COUNTS[movement_data.placed_piece_type % COLOR_CORRECTION];

	//it's important to do so since the AI will simulate board movement and captures, recursively accessing newer states
	PositionalEvalSystem::materialCount(pos_eval_data, own_material_count, opposite_material_count);

	//prepares the spot for the new piece to take hold on the board
	board.occupancy[DEFENDER_COLOR] &= ~VALID_PIECE_PLACEMENT;
	board.pieces[movement_data.placed_piece_type] &= ~VALID_PIECE_PLACEMENT;
}

void BoardUpdatingSystem::placeNewPieceHelper(InitGameState::Board& board, MovementData& movement_data, const uint64_t VALID_PIECE_PLACEMENT, const int ATTACKER_COLOR, const int DEFENDER_COLOR)
{
	//adjusting new occupancy
	board.occupancy[ATTACKER_COLOR] |= VALID_PIECE_PLACEMENT;

	//adjusting piece type, whether pawn promotion occured
	const int SEL_PIECE_IDX = (movement_data.promoted_piece_type > 0);
	uint64_t* regular_placement = &board.pieces[movement_data.picked_piece_type];
	uint64_t* pawn_promotion_placement = &board.pieces[movement_data.promoted_piece_type];
	uint64_t* board_selection[] = {regular_placement, pawn_promotion_placement};
	
	//placing the new piece
	*board_selection[SEL_PIECE_IDX] |= VALID_PIECE_PLACEMENT;
}
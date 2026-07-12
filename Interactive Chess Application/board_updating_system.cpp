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

	//handle en-passant state tracking, to determine whether en-passant is disallowed
	//check if a pawn just moved from it's initial position (a precondition to en-passant)

	uint64_t IF_PAWN_DOUBLE_MOVED = enPassantPrecondition(movement_data);

	if (IF_PAWN_DOUBLE_MOVED > 0) {
		enPassantHelper(board, movement_data);
	}
	

	clearPickedPieceHelper(board, movement_data, ATTACKER_COLOR);

	//a piece was overtaken
	constexpr int NO_PIECE_FLAG = -1;
	if (movement_data.placed_piece_type != NO_PIECE_FLAG) //this may cause serious branch misprediction. possible rework after profiling tests
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

	//adjusting piece type, whether pawn promotion occurred
	const int SEL_PIECE_IDX = (movement_data.promoted_piece_type > 0); //0 - regular move, 1 - pawn promotion 
	uint64_t* regular_placement = &board.pieces[movement_data.picked_piece_type];
	uint64_t* pawn_promotion_placement = &board.pieces[movement_data.promoted_piece_type];
	uint64_t* board_selection[] = {regular_placement, pawn_promotion_placement};
	
	//placing the new piece
	*board_selection[SEL_PIECE_IDX] |= VALID_PIECE_PLACEMENT;
}

uint64_t BoardUpdatingSystem::enPassantPrecondition(MovementData& movement_data)
{
	/*
	* check whether any pawn used the double initial move, which could potentially validate an en-passant
	*/

	using enum pieceInfo::piece;
	
	//position eval
	constexpr uint64_t SECOND_FORWARD_SQUARE_ADJUST = 16;
	const uint64_t PICKED_BIT_MASK = (1ULL << movement_data.picked_square_idx);

	const uint64_t WHITE_DOUBLE_MOVE_MASK = (PICKED_BIT_MASK << SECOND_FORWARD_SQUARE_ADJUST);
	const uint64_t BLACK_DOUBLE_MOVE_MASK = (PICKED_BIT_MASK >> SECOND_FORWARD_SQUARE_ADJUST);

	const uint64_t PLACED_BIT_MASK = (1ULL << movement_data.placement_square_idx);

	const uint64_t IF_WHITE_DOUBLE_MOVED = -( (PLACED_BIT_MASK & WHITE_DOUBLE_MOVE_MASK) > 0);
	const uint64_t IF_BLACK_DOUBLE_MOVED = -( (PLACED_BIT_MASK & BLACK_DOUBLE_MOVE_MASK) > 0);

	const uint64_t IF_DOUBLE_MOVED = IF_WHITE_DOUBLE_MOVED | IF_BLACK_DOUBLE_MOVED;

	//piece eval
	const uint64_t IF_PIECE_IS_PAWN = -(movement_data.picked_piece_type == white_pawn |
										movement_data.picked_piece_type == black_pawn )
	;

	const uint64_t IF_PAWN_DOUBLE_MOVED = IF_PIECE_IS_PAWN & IF_DOUBLE_MOVED;

	return IF_PAWN_DOUBLE_MOVED;
}

void BoardUpdatingSystem::enPassantHelper(InitGameState::Board& board, MovementData& movement_data)
{
	using enum pieceInfo::piece;

	//if the position for the passant is available, mark that field
	constexpr int UP = 8;
	constexpr int DOWN = 8;
	const uint64_t PLACED_BIT_MASK = 1ULL << movement_data.placement_square_idx;

	const uint64_t ADJUST_TO_WHITE_PASSANT_MASK = PLACED_BIT_MASK << UP;
	const uint64_t ADJUST_TO_BLACK_PASSANT_MASK = PLACED_BIT_MASK >> DOWN;
	
	//check the perspective of the piece that just moved if the next move has the possibility of an en-passant
	constexpr int SIDE = 1;

	const uint64_t IF_WHITE_PAWN_LEFT  = -(int64_t)( ( (PLACED_BIT_MASK >> SIDE) & board.pieces[white_pawn]) > 0);
	const uint64_t IF_WHITE_PAWN_RIGHT = -(int64_t)( ( (PLACED_BIT_MASK << SIDE) & board.pieces[white_pawn]) > 0);
	const uint64_t IF_BLACK_PAWN_LEFT  = -(int64_t)( ( (PLACED_BIT_MASK >> SIDE) & board.pieces[black_pawn]) > 0);
	const uint64_t IF_BLACK_PAWN_RIGHT = -(int64_t)( ( (PLACED_BIT_MASK << SIDE) & board.pieces[black_pawn]) > 0);

	const uint64_t IF_NEXT_TO_WHITE_PAWN = IF_WHITE_PAWN_LEFT | IF_WHITE_PAWN_RIGHT;
	const uint64_t IF_NEXT_TO_BLACK_PAWN = IF_BLACK_PAWN_LEFT | IF_BLACK_PAWN_RIGHT;

	//account for possible edge-case wrap-up bug

	/*
	 * when the opportunity arises, the state tracker enables en-passant at a specified position
	 * but first the previous state gets cleared
	 * this matters because en-passant is available only on it's first occurrence at that specified spot
	*/

	movement_data.passant_mask = 0;

	const uint64_t WHITE_PASSANT_MASK = IF_NEXT_TO_WHITE_PAWN & ADJUST_TO_WHITE_PASSANT_MASK;
	const uint64_t BLACK_PASSANT_MASK = IF_NEXT_TO_BLACK_PAWN & ADJUST_TO_BLACK_PASSANT_MASK;

	movement_data.passant_mask = WHITE_PASSANT_MASK | BLACK_PASSANT_MASK;
}
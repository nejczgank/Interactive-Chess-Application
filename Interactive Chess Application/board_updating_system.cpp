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

void BoardUpdatingSystem::updateBoards(InitGameState::Board& board, MovementData& movement_data, PositionalEvalComponent& pos_eval_data, const uint64_t VALID_PIECE_PLACEMENT)
{
	/*
	* Handles piece placement or replacement once a move is enacted and validated
	* This is done by first clearing square where the picked piece resides, then the piece at the occupied square (if it exists!).
	* and lastly placing the picked piece at the desired location
	* **is also coupled with en-passant detection, and dynamically adjusts the clearing method based on whether or not
	*   en-passant occurred
	*/

	using enum occupancyInfo::occupancy;
	using enum pieceInfo::piece;

	//check if a pawn just moved from it's initial position (a precondition to en-passant)
	int const PAWN_DOUBLE_MOVED = (enPassantPrecondition(movement_data) > 0);

	switch (PAWN_DOUBLE_MOVED)
	{
	case false:
	  case true:
		  //find the mask of where a pawn can en-passant to
		  enPassantDetectionHelper(board, movement_data);
		  break;
	}

	clearPickedPieceHelper(board, movement_data);

	//create a jump table for determining the cleaning methods for the overtaken piece
	//first one is generic, second is en-passant specific
	using board_clearing_type = void (*)(InitGameState::Board&, MovementData&, PositionalEvalComponent&, const uint64_t);

	static board_clearing_type jump_table[] = {
		&BoardUpdatingSystem::clearOvertakenSquareHelper,
		&BoardUpdatingSystem::clearPassantedPieceHelper
	};

	constexpr int NO_PIECE_FLAG = -1;
	//enPassantOccurrenceHelper(movement_data);

	//**mathematical expression for adjusting the placed piece
	const int PASSANT_USED = movement_data.passant_used;
	const int PLACED_TYPE = movement_data.placed_piece_type;

	//if no piece gets overtaken the -1 flag persists, otherwise it gets overwritten
	//prompting default (overtaking) behaviour
	movement_data.placed_piece_type = (1 - PASSANT_USED) * PLACED_TYPE;

	switch (movement_data.placed_piece_type)
	{	
		//no piece was blocking movement
		case NO_PIECE_FLAG:
			break;

		// 0 - regular clearing
		// 1 - en-passant clearing
		default:
			jump_table[movement_data.passant_used](board, movement_data, pos_eval_data, VALID_PIECE_PLACEMENT);
			break;
	}
	//reset passant mask
	movement_data.passant_used = 0;

	placeNewPieceHelper(board, movement_data, VALID_PIECE_PLACEMENT);

	//adjust all occupancy
	board.occupancy[all] = board.occupancy[white] | board.occupancy[black];
}

void BoardUpdatingSystem::clearOvertakenSquareHelper(InitGameState::Board& board, MovementData& movement_data, PositionalEvalComponent& pos_eval_data, const uint64_t VALID_PIECE_PLACEMENT)
{
	/*
	* clears the overtaken square and calculates material count
	*/

	//passes the value of the overtaken piece to the AI system
	int const MATERIAL_COUNTS[] = { 1, 3, 3, 5, 9, 0 };
	int constexpr COLOR_CORRECTION = 6;

	//calculates the value of the capture
	int const own_material_count = movement_data.attacker_color * MATERIAL_COUNTS[movement_data.placed_piece_type % COLOR_CORRECTION];
	int const opposite_material_count = movement_data.defender_color * MATERIAL_COUNTS[movement_data.placed_piece_type % COLOR_CORRECTION];

	//it's important to do so since the AI will simulate board movement and captures, recursively accessing newer states
	PositionalEvalSystem::materialCount(pos_eval_data, own_material_count, opposite_material_count);

	//prepares the spot for the new piece to take hold on the board
	//**both occupancy and piece data
	board.occupancy[movement_data.defender_color] &= ~VALID_PIECE_PLACEMENT;
	board.pieces[movement_data.placed_piece_type] &= ~VALID_PIECE_PLACEMENT;
}

void BoardUpdatingSystem::clearPassantedPieceHelper(InitGameState::Board& board, MovementData& movement_data, PositionalEvalComponent& pos_eval_data, const uint64_t VALID_PIECE_PLACEMENT)
{
	/*
	* adjusts overtaking the square when en-passant is enacted, because
	* where the picked piece is placed doesn't correspond to the area needed to be cleared.
	* mask clears the bottom black piece when white is attacking, and vice versa
	*/

	using enum occupancyInfo::occupancy;
	using enum pieceInfo::piece;

	constexpr int HORIZONTAL_MOVE = 8;

	const int IS_WHITE_ATTACKER = (movement_data.attacker_color == white);
	const int IS_BLACK_ATTACKER = (movement_data.attacker_color == black);

	const int DOWN = HORIZONTAL_MOVE * IS_WHITE_ATTACKER;
	const int UP   = HORIZONTAL_MOVE * IS_BLACK_ATTACKER;
	
	const uint64_t IF_DOWN = -(DOWN > 0);
	const uint64_t IF_UP   = -(UP > 0);

	const uint64_t SHIFTED_DOWN_MASK = VALID_PIECE_PLACEMENT >> DOWN;
	const uint64_t SHIFTED_UP_MASK   = VALID_PIECE_PLACEMENT << UP;

	const uint64_t CLEAR_PAWN_MASK = (IF_DOWN & SHIFTED_DOWN_MASK) | (IF_UP & SHIFTED_UP_MASK);

	//clear the spot where a pawn was en-passanted
	//**both occupancy and piece data
	board.occupancy[movement_data.defender_color] &= ~CLEAR_PAWN_MASK;

	//reach out for the exact piece type, that can be either a black or white pawn
	//**given that en-passant only functions between pawns
	const int PAWN_COLOR_IDX = (IS_WHITE_ATTACKER * black_pawn) + (IS_BLACK_ATTACKER * white_pawn);
	//board.pieces[movement_data.placed_piece_type] &= ~CLEAR_PAWN_MASK;
	board.pieces[PAWN_COLOR_IDX] &= ~CLEAR_PAWN_MASK;
}

void BoardUpdatingSystem::clearPickedPieceHelper(InitGameState::Board& board, MovementData& movement_data)
{
	/*
	* clears the spot where the defender piece lies so that the attacking piece can overwrite it
	*/

	//clearing the boards for the picked piece
	board.occupancy[movement_data.attacker_color] &= ~(1ULL << movement_data.picked_square_idx);

	//clearing the piece
	board.pieces[movement_data.picked_piece_type] &= ~(1ULL << movement_data.picked_square_idx);
}

void BoardUpdatingSystem::placeNewPieceHelper(InitGameState::Board& board, MovementData& movement_data, const uint64_t VALID_PIECE_PLACEMENT)
{
	/*
	* places the picked piece down to a spot that is empty, but it can also
	* transform a pawn piece that got promoted
	*/

	//adjusting new occupancy
	board.occupancy[movement_data.attacker_color] |= VALID_PIECE_PLACEMENT;

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

	//clean the passant mask state, to enforce one move availability
	movement_data.passant_mask = 0;

	return IF_PAWN_DOUBLE_MOVED;
}

void BoardUpdatingSystem::enPassantDetectionHelper(const InitGameState::Board& board, MovementData& movement_data)
{
	/*
	* checks whether the remaining conditions for en-passant can occur, and
	* if they can, the exact square gets saved as mask. This state persists.
	* Picked up by the movement validation, which determines the precise movement of the pawn
	*/

	using enum pieceInfo::piece;

	//if the position for the passant is available, mark that field
	constexpr int UP = 8;
	constexpr int DOWN = 8;
	const uint64_t PLACED_BIT_MASK = 1ULL << movement_data.placement_square_idx;

	//adjusted mask where the attacking pawn may move
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

	const uint64_t WHITE_PASSANT_MASK = IF_NEXT_TO_WHITE_PAWN & ADJUST_TO_WHITE_PASSANT_MASK;
	const uint64_t BLACK_PASSANT_MASK = IF_NEXT_TO_BLACK_PAWN & ADJUST_TO_BLACK_PASSANT_MASK;

	movement_data.passant_mask = WHITE_PASSANT_MASK | BLACK_PASSANT_MASK;
}
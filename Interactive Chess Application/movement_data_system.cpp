#include "movement_data_system.h"

void GetMovementInfoSystem::basicInfo(const InitGameState::Board& board, MovementData& movement_data, int picked_square_idx, int placement_square_idx)
{
	/*
	* BasicInfo obtains essential data once players input is confirmed valid
	* The function obtains data of the indices of the picked and placed piece
	* as well as determining the color of said pieces
	*/

	using enum occupancyInfo::occupancy;

	movement_data.picked_square_idx = picked_square_idx;
	movement_data.placement_square_idx = placement_square_idx;

	const int64_t CHECKING_IF_BLACK = board.occupancy[black] & (1ULL << picked_square_idx);
	const int PIECE_COLOR = CHECKING_IF_BLACK > 0ULL ? black : white;

	movement_data.attacker_color = PIECE_COLOR;		  //0 - white, 1 - black
	movement_data.defender_color = (1 - PIECE_COLOR); //0 - white, 1 - black

}

void GetMovementInfoSystem::pickingInfo(const InitGameState::Board& board, MovementData& movement_data)
{
	/*
	* This function is responsible for finding movement data when a piece is picked.
	* That entails recalculating allies and enemies for upcoming piece movement validation to be accurate,
	* given that they were most likely moved when the board updated
	* Aside from that the index value of the picked piece gets calculated to be used by the jump table,
	* contained within the validator function. This is integral to determining where the selected piece
	* can legally move.
	* **also clears the piece index for pawn promotion, as it interferes with subsequent placements
	*/

	using enum pieceInfo::piece;
	using enum occupancyInfo::occupancy;
	using enum moveInfo::move;

	//clear pawn promotion
	movement_data.promoted_piece_type = 0;

	//determine which board pieces are allies or enemies based on color of the attacker found in basicInfo
	const uint64_t IF_WHITE = -((int)movement_data.attacker_color < 1);
	const uint64_t IF_BLACK = -((int)movement_data.attacker_color > 0);

	movement_data.allies  = (IF_WHITE & board.occupancy[white]) | (IF_BLACK & board.occupancy[black]);
	movement_data.enemies = (IF_WHITE & board.occupancy[black]) | (IF_BLACK & board.occupancy[white]);

	const uint64_t PIECE_IDX_MASK = ComputePieceIdxMaskHelper(board, movement_data.picked_square_idx);

	movement_data.picked_piece_type = (int)std::countr_zero(PIECE_IDX_MASK);
}

void GetMovementInfoSystem::placementInfo(const InitGameState::Board& board, MovementData& movement_data)
{
	/*
	* Handles determining occupancy where the piece is supposed to be placed.
	* If a piece exists there it's index number calculated, but if it doesn't
	* a branchless condition sets it to a flag of -1, which is used for properly adjusting movement
	* in board_updating_system.cpp
	*/

	using enum occupancyInfo::occupancy;

	const uint64_t PIECE_IDX_MASK = ComputePieceIdxMaskHelper(board, movement_data.placement_square_idx);

	const uint64_t IF_NO_PIECES = -(PIECE_IDX_MASK == 0);

	constexpr int OVERFLOW_TO_ZERO = 1;
	const int NO_PIECES_CONST = (int)(IF_NO_PIECES + OVERFLOW_TO_ZERO == 0ULL);
	const uint64_t NO_PIECE_FLAG = (uint64_t)(NO_PIECES_CONST * -1);

	const int FOUND_PIECE = std::countr_zero(PIECE_IDX_MASK);
	
	movement_data.placed_piece_type = (int)(IF_NO_PIECES & NO_PIECE_FLAG) | (~IF_NO_PIECES & FOUND_PIECE);
}

uint64_t GetMovementInfoSystem::ComputePieceIdxMaskHelper(const InitGameState::Board& board, const int square_idx)
{
	/*
	* Used for calculating the exact piece number of an encountered piece
	* the shifted bit and individual piece type masks are intersected to check for occupancy
	* if the occupancy exists, they get converted to 1ULL that gets shifted, by the index amount
	* of the corresponding type.
	* This allows for converting the mask to an integer via a countr_zero function call
	*/

	using enum pieceInfo::piece;

	const uint64_t BIT_MASK = (1ULL << square_idx);

	const uint64_t PIECE_IDX_MASK =
		(1ULL * ( (board.pieces[white_pawn]   & BIT_MASK) > 0) << white_pawn)   |
		(1ULL * ( (board.pieces[white_knight] & BIT_MASK) > 0) << white_knight) |
		(1ULL * ( (board.pieces[white_rook]   & BIT_MASK) > 0) << white_rook)   |
		(1ULL * ( (board.pieces[white_bishop] & BIT_MASK) > 0) << white_bishop) |
		(1ULL * ( (board.pieces[white_queen]  & BIT_MASK) > 0) << white_queen)  |
		(1ULL * ( (board.pieces[white_king]   & BIT_MASK) > 0) << white_king)   |
		(1ULL * ( (board.pieces[black_pawn]   & BIT_MASK) > 0) << black_pawn)   |
		(1ULL * ( (board.pieces[black_knight] & BIT_MASK) > 0) << black_knight) |
		(1ULL * ( (board.pieces[black_rook]   & BIT_MASK) > 0) << black_rook)   |
		(1ULL * ( (board.pieces[black_bishop] & BIT_MASK) > 0) << black_bishop) |
		(1ULL * ( (board.pieces[black_queen]  & BIT_MASK) > 0) << black_queen)  |
		(1ULL * ( (board.pieces[black_king]   & BIT_MASK) > 0) << black_king)
	;

	return PIECE_IDX_MASK;
}
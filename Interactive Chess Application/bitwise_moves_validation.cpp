#include "bitwise_move_validation.h"

BitwiseMoveValidation::BitwiseMoveValidation(InitGameState::Board& all_boards) {
	white_pawns_ = all_boards.white_pawns;
	white_knights_ = all_boards.white_knights;
	white_rooks_ = all_boards.white_rooks;
	white_bishops_ = all_boards.white_bishops;
	white_queens_ = all_boards.white_queens;
	white_king_ = all_boards.white_king;

	black_pawns_ = all_boards.black_pawns;
	black_knights_ = all_boards.black_knights;
	black_rooks_ = all_boards.black_rooks;
	black_bishops_ = all_boards.black_bishops;
	black_queens_ = all_boards.black_queens;
	black_king_ = all_boards.black_king;

	white_occupancy_ = all_boards.white_occupancy;
	black_occupancy_ = all_boards.black_occupancy;
	all_occupancy_ = all_boards.all_occupancy;
}

//pawns (determining direction, decoupling captures and moves, extra initial move, en-passant)
//knights (no-blocking mechanic)
//rooks
//bishops
//queens
//kings

//check
//check-mate
//en-passant
//castling
//ties

//using bitmasks, avoiding if statements and loops
//final rendition a universal function with delta direction for raycasts

BitwiseMoveValidation::~BitwiseMoveValidation() = default;
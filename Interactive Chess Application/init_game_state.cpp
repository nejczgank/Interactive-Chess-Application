#include "init_game_state.h"

InitGameState::InitGameState() {

	white_pawns_     = 0x000000000000FF00;
	white_knights_   = 0x0000000000000042;
	white_rooks_     = 0x0000000000000081;
	white_bishops_   = 0x0000000000000024;
	//white_queens_    = 0x0000000000000008;
	white_queens_    = 0x0000002000000008;
	white_king_      = 0x0000000000000010;

	black_pawns_     = 0x00FF000000000000;
	black_knights_   = 0x4200000000000000;
	black_rooks_     = 0x8100000000000000;
	black_bishops_   = 0x2400000000000000;
	black_queens_    = 0x0800000000000000;
	black_king_      = 0x1000000000000000;

	//white_occupancy_ = 0x000000000000FFFF;
	white_occupancy_ = 0x000000200000ffff;
	black_occupancy_ = 0xFFFF000000000000;
	all_occupancy_   = white_occupancy_ | black_occupancy_;
}

InitGameState::Board InitGameState::getInitBoardState() const {

	Board board;

	board.white_pawns = white_pawns_;
	board.white_knights = white_knights_;
	board.white_rooks = white_rooks_;
	board.white_bishops = white_bishops_;
	board.white_queens = white_queens_;
	board.white_king = white_king_;

	board.black_pawns = black_pawns_;
	board.black_knights = black_knights_;
	board.black_rooks = black_rooks_;
	board.black_bishops = black_bishops_;
	board.black_queens = black_queens_;
	board.black_king = black_king_;

	board.white_occupancy = white_occupancy_;
	board.black_occupancy = black_occupancy_;
	board.all_occupancy = white_occupancy_ | black_occupancy_;

	return board;
}

InitGameState::~InitGameState() = default;
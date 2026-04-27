#include "init_game_state.h"

InitGameState::InitGameState() {

	//white_pawns_     = 0x000000000000FF00;
	white_pawns_ = 0x1ff00;
	white_knights_   = 0x0000000000000042;
	white_rooks_     = 0x0000000000000081;
	white_bishops_   = 0x0000000000000024;
	white_queens_    = 0x0000000000000008;
	white_king_      = 0x0000000000000010;

	//black_pawns_     = 0x00FF000000000000;
	black_pawns_ = 0xff000002800000;
	black_knights_   = 0x4200000000000000;
	black_rooks_     = 0x8100000000000000;
	black_bishops_   = 0x2400000000000000;
	black_queens_    = 0x0800000000000000;
	black_king_      = 0x1000000000000000;

	white_occupancy_ = white_pawns_ | white_knights_ | white_rooks_ | white_bishops_ | white_queens_ | white_king_;
	black_occupancy_ = black_pawns_ | black_knights_ | black_rooks_ | black_bishops_ | black_queens_ | black_king_;
	all_occupancy_   = white_occupancy_ | black_occupancy_;
}

InitGameState::Board InitGameState::getInitBoardState() const {

	Board board;

	board.pieces[0] = white_pawns_;
	board.pieces[1] = white_knights_;
	board.pieces[2] = white_rooks_;
	board.pieces[3] = white_bishops_;
	board.pieces[4] = white_queens_;
	board.pieces[5] = white_king_;
	board.pieces[6] = black_pawns_;
	board.pieces[7] = black_knights_;
	board.pieces[8] = black_rooks_;
	board.pieces[9] = black_bishops_;
	board.pieces[10] = black_queens_;
	board.pieces[11] = black_king_;

	board.occupancy[0] = white_occupancy_;
	board.occupancy[1] = black_occupancy_;
	board.occupancy[2] = white_occupancy_ | black_occupancy_;

	return board;
}

InitGameState::~InitGameState() = default;
#pragma once
#include <iostream>
#include "init_game_state.h"

class BitwiseMoveValidation {
public:
	BitwiseMoveValidation(InitGameState::Board&);
	~BitwiseMoveValidation();
private:
	uint64_t white_pawns_;
	uint64_t white_knights_;
	uint64_t white_rooks_;
	uint64_t white_bishops_;
	uint64_t white_queens_;
	uint64_t white_king_;

	uint64_t black_pawns_;
	uint64_t black_knights_;
	uint64_t black_rooks_;
	uint64_t black_bishops_;
	uint64_t black_queens_;
	uint64_t black_king_;

	uint64_t white_occupancy_;
	uint64_t black_occupancy_;
	uint64_t all_occupancy_;
};
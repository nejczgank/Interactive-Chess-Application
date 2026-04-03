#pragma once
#include <iostream>
#include "init_game_state.h"
#include <cstdint>
#include <bit>

#include "display.h"

class BitwiseMoveValidation {
public:
	BitwiseMoveValidation(InitGameState::Board&, int, int);
	~BitwiseMoveValidation();
	void callPieceTypesValidator();
private:
	void determinePickedPiece();
	uint64_t universalRay(uint64_t, uint64_t);
	uint64_t rayHalvingHelper(uint64_t*, uint64_t*, uint64_t*);
	uint64_t diagonalTransformation(uint64_t*);
	uint64_t nonDiagonalTransformation(uint64_t*, uint64_t*);
	uint64_t findEnemyBlockersHelper(uint64_t*, uint64_t*);
	uint64_t findAllyBlockersHelper(uint64_t*, uint64_t*);
	uint64_t pawnValidation();
	uint64_t knightValidation();
	uint64_t rookValidation();
	uint64_t bishopValidation();
	uint64_t queenValidation();
	uint64_t kingValidaiton();
	void moveValidation();
	void updateBoards();
	void checkPositionsBoard();

	InitGameState::Board all_boards_;

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

	int picked_square_idx_;
	int placement_square_idx_;

	uint16_t compressed_piece_type_;

	uint64_t allies_;
	uint64_t enemies_;

	uint64_t universal_ray_;
};
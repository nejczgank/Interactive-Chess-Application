#pragma once
#include <iostream>
#include "init_game_state.h"
#include <cstdint>
#include <bit>
#include "display.h"
#include "player_input.h"
#include "positional_evaluation.h"

class BitwiseMoveValidation {
public:
	BitwiseMoveValidation(InitGameState::Board&);
	~BitwiseMoveValidation();
	void setUpdatedState(int, int);
	bool callPieceTypesValidator();
private:
	void determinePickedPiece();
	void determinePlacedPiece();
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
	uint64_t kingValidation();
	uint64_t movementValidation(uint64_t*);	
	void updateBoards(uint64_t*);
	void clearPieceHelper(uint64_t*, int, int, int);
	void placeNewPieceHelper(uint64_t*, int, int, int);
	void checkPositionsBoard();

	//using an object pointer so the code can persist outside
	InitGameState::Board* all_boards_;

	uint64_t &white_pawns_;
	uint64_t &white_knights_;
	uint64_t &white_rooks_;
	uint64_t &white_bishops_;
	uint64_t &white_queens_;
	uint64_t &white_king_;

	uint64_t &black_pawns_;
	uint64_t &black_knights_;
	uint64_t &black_rooks_;
	uint64_t &black_bishops_;
	uint64_t &black_queens_;
	uint64_t &black_king_;

	uint64_t &white_occupancy_;
	uint64_t &black_occupancy_;
	uint64_t &all_occupancy_;

	int picked_square_idx_;
	int placement_square_idx_;

	static uint64_t InitGameState::Board::* white_selectors_[];
	static uint64_t InitGameState::Board::* black_selectors_[];
	static uint64_t InitGameState::Board::* all_selectors_[];
	static uint64_t InitGameState::Board::* color_selectors_[];

	uint16_t picked_piece_type_;
	uint16_t placed_piece_type_;

	uint64_t allies_;
	uint64_t enemies_;

	PositionalEvaluation evaluateThisBoard;
};
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
	void getMovementInfo(int);
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
	void clearPickedPieceHelper(int);
	void clearNewSpotHelper(uint64_t*, int, int);
	void placeNewPieceHelper(uint64_t*, int, int);

	//using an object pointer so the code can persist outside
	InitGameState::Board* all_boards_;
	uint64_t* p_;
	uint64_t* occ_;

	int picked_square_idx_;
	int placement_square_idx_;

	int picked_piece_type_;
	int placed_piece_type_;

	uint64_t allies_;
	uint64_t enemies_;

	PositionalEvaluation evaluateThisBoard;
};
#pragma once
#include <iostream>
#include <cstdint>
#include <bit>
#include "init_game_state.h"
#include "movement_data_component.h"
#include "occupancy_info.h"
#include "positional_eval_component.h"
#include "positional_eval_system.h"
#include "piece_info.h"

class BoardUpdatingSystem {
public:
	static uint64_t movementValidation(MovementData&, uint64_t);
	static void updateBoards(InitGameState::Board&, MovementData&, PositionalEvalComponent&, uint64_t);
private:
	static void clearPickedPieceHelper(InitGameState::Board&, MovementData&, int);
	static void clearOvertakenSquareHelper(InitGameState::Board&, MovementData&, PositionalEvalComponent&, uint64_t, int, int);
	static void placeNewPieceHelper(InitGameState::Board&, MovementData&, uint64_t, int, int);
	static uint64_t enPassantPrecondition(MovementData&);
	static void enPassantHelper(InitGameState::Board&, MovementData&);
};
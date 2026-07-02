#pragma once
#include <iostream>
#include <cstdint>
#include <bit>
#include "init_game_state.h"
#include "display.h"
#include "player_input.h"
#include "positional_evaluation.h"
#include "movement_data_component.h"
#include "occupancy_info.h"
#include "ray_transposition_info.h"
#include "ray_direction_info.h"

class MoveValidationSystem {
public:
	static uint64_t validator(InitGameState::Board&, MovementData&);
private:
	static uint64_t universalRay(MovementData&, uint64_t, uint64_t);
	static uint64_t rayHalvingHelper(MovementData&, uint64_t, uint64_t, uint64_t);
	static uint64_t diagonalTransformation(MovementData&, uint64_t);
	static uint64_t nonDiagonalTransformation(MovementData&, uint64_t, uint64_t);
	static uint64_t findEnemyBlockersHelper(MovementData&, uint64_t, uint64_t);
	static uint64_t findAllyBlockersHelper(MovementData&, uint64_t, uint64_t);
	static uint64_t pawnValidation(InitGameState::Board&, MovementData&);
	static uint64_t knightValidation(InitGameState::Board&, MovementData&);
	static uint64_t rookValidation(InitGameState::Board&, MovementData&);
	static uint64_t bishopValidation(InitGameState::Board&, MovementData&);
	static uint64_t queenValidation(InitGameState::Board&, MovementData&);
	static uint64_t kingValidation(InitGameState::Board&, MovementData&);
};
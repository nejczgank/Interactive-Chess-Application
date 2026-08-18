#pragma once
#include <iostream>
#include <cstdint>
#include <bit>
#include "init_game_state.h"
#include "display.h"
#include "player_input.h"
#include "movement_data_component.h"
#include "occupancy_info.h"
#include "ray_transposition_info.h"
#include "ray_direction_info.h"
#include "piece_info.h"
#include "which_player_info.h"
#include "king_move_indicies.h"

class MoveValidationSystem {
public:
	static uint64_t validator(const InitGameState::Board&, MovementData&);
	static uint64_t universalRay(const MovementData&, uint64_t, uint64_t); // used in checkmate and castling
private:
	static uint64_t rayHalvingHelper(const MovementData&, uint64_t, uint64_t, uint64_t);
	static uint64_t diagonalTransformation(const MovementData&, uint64_t);
	static uint64_t nonDiagonalTransformation(const MovementData&, uint64_t, uint64_t);
	static uint64_t findBlockersHelper(const MovementData&, uint64_t, uint64_t, whichPlayerInfo::playerInfo);

	static uint64_t pawnValidation(const InitGameState::Board&, MovementData&);
	static uint64_t knightValidation(const InitGameState::Board&, MovementData&);
	static uint64_t rookValidation(const InitGameState::Board&, MovementData&);
	static uint64_t bishopValidation(const InitGameState::Board&, MovementData&);
	static uint64_t queenValidation(const InitGameState::Board&, MovementData&);
	static uint64_t kingValidation(const InitGameState::Board&, MovementData&);
};
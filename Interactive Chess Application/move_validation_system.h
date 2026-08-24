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
#include "trace_path_component.h"

class MoveValidationSystem {
public:
	static uint64_t validator(const InitGameState::Board&, MovementData&, TracePathComponent&);
private:
	static uint64_t universalRay(const MovementData&, uint64_t, uint64_t);
	static uint64_t rayHalvingHelper(const MovementData&, uint64_t, uint64_t, uint64_t);
	static uint64_t diagonalTransformation(const MovementData&, uint64_t);
	static uint64_t nonDiagonalTransformation(const MovementData&, uint64_t, uint64_t);
	static uint64_t findBlockersHelper(const MovementData&, uint64_t, uint64_t, whichPlayerInfo::playerInfo);

	static uint64_t pawnValidation(const InitGameState::Board&, MovementData&, TracePathComponent&);
	static uint64_t knightValidation(const InitGameState::Board&, MovementData&, TracePathComponent&);
	static uint64_t rookValidation(const InitGameState::Board&, MovementData&, TracePathComponent&);
	static uint64_t bishopValidation(const InitGameState::Board&, MovementData&, TracePathComponent&);
	static uint64_t queenValidation(const InitGameState::Board&, MovementData&, TracePathComponent&);
	static uint64_t kingValidation(const InitGameState::Board&, MovementData&, TracePathComponent&);
	
	static inline uint64_t excludeKingOrigin(const InitGameState::Board&, uint64_t);
	static inline uint64_t excludeKingOrigin(const InitGameState::Board&, uint64_t&, TracePathComponent&);
	static uint64_t captureRayHelper(const InitGameState::Board&, MovementData&, const uint64_t, const uint64_t, TracePathComponent&);
};
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
#include "extract_ray_type_info.h"
#include "king_dir_indices_info.h"

class MoveValidationSystem {
public:

	struct KingValidationBundle
	{
		uint64_t KING_DIR_MASKS[8]{};
		uint64_t king_mask;
	};

	static uint64_t validator(const InitGameState::Board&, MovementData&, TracePathComponent&, ExtractRayTypeInfo::ray_type);
	static KingValidationBundle kingMoves(const MovementData&); //determines where the king can move. used for both checkmate validation and king movement validation
	static uint64_t kingValidation(const InitGameState::Board&, MovementData&, TracePathComponent&, ExtractRayTypeInfo::ray_type); //adjusts valid moves based on kings' proximity
private:

	struct MaskComponent
	{
		uint64_t attack_ray{};
		uint64_t defend_squares_mask{};
	};

	struct ValidatedMaskComponent
	{
		uint64_t attack_ray{};
		uint64_t check_ray{};
	};

	static uint64_t universalRay(const MovementData&, uint64_t, uint64_t);
	static uint64_t rayHalvingHelper(const MovementData&, uint64_t, uint64_t, uint64_t);
	static uint64_t diagonalTransformation(const MovementData&, uint64_t);
	static uint64_t nonDiagonalTransformation(const MovementData&, uint64_t, uint64_t);
	static uint64_t findBlockersHelper(const MovementData&, uint64_t, uint64_t, whichPlayerInfo::playerInfo);

	static uint64_t pawnValidation(const InitGameState::Board&, MovementData&, TracePathComponent&, ExtractRayTypeInfo::ray_type);
	static uint64_t knightValidation(const InitGameState::Board&, MovementData&, TracePathComponent&, ExtractRayTypeInfo::ray_type);
	static uint64_t rookValidation(const InitGameState::Board&, MovementData&, TracePathComponent&, ExtractRayTypeInfo::ray_type);
	static uint64_t bishopValidation(const InitGameState::Board&, MovementData&, TracePathComponent&, ExtractRayTypeInfo::ray_type);
	static uint64_t queenValidation(const InitGameState::Board&, MovementData&, TracePathComponent&, ExtractRayTypeInfo::ray_type);
	//static uint64_t kingValidation(const InitGameState::Board&, MovementData&, TracePathComponent&, ExtractRayTypeInfo::ray_type);
	
	static inline uint64_t excludeKingOrigin(const InitGameState::Board&, uint64_t);
	static inline MaskComponent excludeKingOrigin(const InitGameState::Board&, uint64_t, TracePathComponent&);
	static ValidatedMaskComponent captureRayHelper(const InitGameState::Board&, MovementData&, const uint64_t, const uint64_t, TracePathComponent&);
	static inline void extractRays(const InitGameState::Board&, MovementData&, ValidatedMaskComponent&, const uint64_t, const uint64_t, TracePathComponent&);
	static inline uint64_t pickRayType(const ValidatedMaskComponent&, ExtractRayTypeInfo::ray_type);
};
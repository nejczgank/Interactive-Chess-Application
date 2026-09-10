#pragma once
#include <iostream>
#include "init_game_state.h"
#include "movement_data_component.h"
#include "move_info.h"
#include "occupancy_info.h"
#include "piece_info.h"

class GetMovementInfoSystem {
public:
	static void basicInfo(const InitGameState::Board&, MovementData&, int picked_square_idx, int placement_square_idx);
	static void pickingInfo(const InitGameState::Board&, MovementData&);
	static void placementInfo(const InitGameState::Board&, MovementData&);
private:
	static uint64_t ComputePieceIdxMaskHelper(const InitGameState::Board& board, int);
};
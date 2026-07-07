#pragma once
#include <iostream>
#include "init_game_state.h"
#include "movement_data_component.h"
#include "move_info.h"
#include "occupancy_info.h"
#include "piece_info.h"

class GetMovementInfoSystem {
public:
	static void pickingInfo(const InitGameState::Board&, MovementData&, int, int);
	static void placementInfo(const InitGameState::Board&, MovementData&, int);
private:
	static uint64_t ComputePieceIdxMaskHelper(const InitGameState::Board& board, int);
};
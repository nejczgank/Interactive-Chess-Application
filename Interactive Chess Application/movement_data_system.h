#pragma once
#include <iostream>
#include "init_game_state.h"
#include "movement_data_component.h"
#include "occupancy_info.h"

class GetMovementInfoSystem {
public:
	static void executeMovementInfo(const InitGameState::Board&, MovementData&, int flag, int picked_square_idx, int placement_square_idx);
};
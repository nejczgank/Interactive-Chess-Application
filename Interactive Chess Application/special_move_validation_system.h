#pragma once
#include <iostream>
#include <cstdint>
#include <bit>
#include <cmath>
#include "occupancy_info.h"
#include "init_game_state.h"
#include "move_validation_system.h"
#include "piece_info.h"
#include "special_move_validation_component.h"

class SpecialMoveValidationSystem {
public:
	static bool isKingCheck(InitGameState::Board&, int);
	static int isKingCheckmate();
	static bool castling();
	static void stalemate();
};
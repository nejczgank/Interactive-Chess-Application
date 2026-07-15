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
#include "board_updating_system.h"

class SpecialMoveValidationSystem {
public:
	static int checkmateHandler(const InitGameState::Board&, const MovementData&, const uint64_t);
private:
	static int isKingCheck(const InitGameState::Board&, const MovementData&, const uint64_t);
	static void isPieceAtkHelper(const InitGameState::Board&, MovementData&, bool&, const int, const int(&)[6], const int(&)[6]);
	static bool findCheck(const InitGameState::Board& board, MovementData& new_movement_data, const int, const int, const int(&)[6], const int(&)[6]);
	static void isKingCheckmate();
	static bool castling();
	static void stalemate();
};
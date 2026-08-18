#pragma once
#include <iostream>
#include <cstdint>
#include <bit>
#include <cmath>
#include "occupancy_info.h"
#include "init_game_state.h"
#include "move_validation_system.h"
#include "piece_info.h"
#include "eval_check_and_mate_component.h"
#include "board_updating_system.h"
#include "stalemate_data_component.h"

class EvalCheckAndMateSystem {
public:
	static void initState(InitGameState::Board&, MovementData&, const uint64_t, StalemateDataComponent&, const int, EvalCheckAndMateComponent&);
	static int checkmateHandler(EvalCheckAndMateComponent&, StalemateDataComponent&);
private:
	static int isKingCheck(EvalCheckAndMateComponent&);
	static void isPieceAtkHelper(EvalCheckAndMateComponent&, bool&, const int, const int(&)[6], const int(&)[6]);
	static bool findCheck(EvalCheckAndMateComponent&, const int, const int, const int(&)[6], const int(&)[6]);
	static void findTargetIndex(EvalCheckAndMateComponent&, const int(&)[6]);
	static void findOccupation(EvalCheckAndMateComponent&, const int, const int);
	static bool isKingCheckmate(EvalCheckAndMateComponent&, StalemateDataComponent&);

	/*static int isKingCheck(const InitGameState::Board&, const MovementData&, const uint64_t, const int, const int, const bool);
	static void isPieceAtkHelper(const InitGameState::Board&, MovementData&, bool&, const int, const int(&)[6], const int(&)[6]);
	static bool findCheck(const InitGameState::Board&, MovementData& new_movement_data, const int, const int, const int(&)[6], const int(&)[6], const bool);
	static void findTargetIndex(const InitGameState::Board&, MovementData&, const int(&)[6], bool);
	static void findOccupation(const InitGameState::Board&, MovementData&, const int, const int, const bool);
	static bool isKingCheckmate(const InitGameState::Board&, const MovementData&, const uint64_t, StalemateDataComponent&, const int, const int);*/
};
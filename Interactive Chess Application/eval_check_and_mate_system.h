#pragma once
#include <iostream>
#include <cstdint>
#include <bit>
#include <cmath>
#include <algorithm>
#include <iterator>
#include "occupancy_info.h"
#include "init_game_state.h"
#include "move_validation_system.h"
#include "piece_info.h"
#include "board_updating_system.h"
#include "stalemate_data_component.h"
#include "trace_path_component.h"
#include "check_sim_frame_component.h"
#include "king_subopt_info.h"
#include "extract_ray_type_info.h"

class EvalCheckAndMateSystem {
public:
	//static void initState(InitGameState::Board&, MovementData&, const uint64_t, StalemateDataComponent&, const int, EvalCheckAndMateComponent&, TracePathComponent& path_data);
	static int checkmateHandler(InitGameState::Board&, MovementData&, PositionalEvalComponent&, const uint64_t, StalemateDataComponent&);

private:
	//intermediary struct definitions
	struct FoundOccupation
	{
		uint64_t allies{};
		uint64_t enemies{};
	};

	struct CheckTypeColorComponent
	{
		int ATTACKER_COLOR{};
		int DEFENDER_COLOR{};
		int ATK_PIECES[6]{};
		int DEF_PIECES[6]{};
	};

	struct checkTypeFrameComponent
	{
		CheckTypeColorComponent first_check_color_type{};
		CheckTypeColorComponent second_check_color_type{};
		bool IS_KING{};
	};

	//struct setters
	static void setCheckSimFrame(InitGameState::Board&, MovementData&, PositionalEvalComponent&, CheckSimFrameComponent&);
	static void setImpositionCheckColor(const MovementData&, CheckTypeColorComponent&);
	static void setDiscoverCheckColor(const MovementData&, CheckTypeColorComponent&);
	static void setCheckType(checkTypeFrameComponent&, const CheckTypeColorComponent&, const CheckTypeColorComponent&, bool);

	//check
	static int isKingCheck(CheckSimFrameComponent&, checkTypeFrameComponent&, TracePathComponent&, ExtractRayTypeInfo::ray_type);
	//static bool findCheck(const CheckSimFrameComponent&, TracePathComponent&, const int, const int, const int(&)[6], const int(&)[6], bool);
	static bool findCheck(const CheckSimFrameComponent&, CheckTypeColorComponent&, TracePathComponent&, const bool, ExtractRayTypeInfo::ray_type);
	static FoundOccupation findOccupation(const CheckSimFrameComponent&, const int, const int, bool);
	static int findTargetIndex(const CheckSimFrameComponent&, const int(&)[6], bool);
	static void isPieceAtkHelper(MovementData&, const CheckSimFrameComponent&, TracePathComponent&, bool&, const int, const int(&)[6], const int(&)[6], ExtractRayTypeInfo::ray_type);
	
	//checkmate
	/*static bool isKingCheckmate(EvalCheckAndMateComponent&, StalemateDataComponent&);
	static uint64_t findAttackPathHelper(EvalCheckAndMateComponent&);*/

	/*static int isKingCheck(const InitGameState::Board&, const MovementData&, const uint64_t, const int, const int, const bool);
	static void isPieceAtkHelper(const InitGameState::Board&, MovementData&, bool&, const int, const int(&)[6], const int(&)[6]);
	static bool findCheck(const InitGameState::Board&, MovementData& new_movement_data, const int, const int, const int(&)[6], const int(&)[6], const bool);
	static void findTargetIndex(const InitGameState::Board&, MovementData&, const int(&)[6], bool);
	static void findOccupation(const InitGameState::Board&, MovementData&, const int, const int, const bool);
	static bool isKingCheckmate(const InitGameState::Board&, const MovementData&, const uint64_t, StalemateDataComponent&, const int, const int);*/
};


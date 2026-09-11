#pragma once
#include "init_game_state.h"
#include "movement_data_component.h"
#include "positional_eval_component.h"

struct CheckSimFrameComponent
{
	InitGameState::Board new_board{};
	MovementData new_movement_data{};
	PositionalEvalComponent new_pos_eval{};
};

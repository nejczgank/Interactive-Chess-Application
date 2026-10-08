#pragma once
#include "init_game_state.h"
#include "movement_data_component.h"
#include "positional_eval_component.h"
#include "en_passant_state_component.h"

struct GameStateContextComponent
{
	InitGameState::Board& board;
	MovementData& movement_data;
	PositionalEvalComponent& pos_eval_data; // pol ko bo ai
	//uint64_t VALID_PIECE_PLACEMENT;
	EnPassantStateComponent& en_passant_data;
};

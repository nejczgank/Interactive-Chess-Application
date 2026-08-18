#pragma once
#include "piece_info.h"
#include "init_game_state.h"
#include "movement_data_component.h"
#include "stalemate_data_component.h"
#include "positional_eval_component.h"

struct EvalCheckAndMateComponent {

	//obtained parameters
	const InitGameState::Board* BOARD = nullptr;
	MovementData* SEL_MOVEMENT_DATA = nullptr;
	StalemateDataComponent* stalemate_data = nullptr;
	uint64_t VALID_PIECE_PLACEMENT{};
	int TURN{};
	
	//intermediary values
	int IS_KING{};

	int ATTACKER_COLOR_OFFSET{};
	int DEFENDER_COLOR_OFFSET{};

	int ATK_PIECES[6]{};
	int DEF_PIECES[6]{};

	InitGameState::Board new_board;
	MovementData new_movement_data;
	PositionalEvalComponent new_pos_eval;
};
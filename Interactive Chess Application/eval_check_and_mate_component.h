#pragma once
#include "piece_info.h"

struct EvalCheckAndMateComponent {

	//obtained parameters
	InitGameState::Board& BOARD;
	MovementData& SEL_MOVEMENT_DATA;
	StalemateDataComponent& stalemate_data;
	uint64_t VALID_PIECE_PLACEMENT{};
	int TURN{};
	
	//intermediary values
	int IS_KING{};

	int ATTACKER_COLOR_OFFSET{};
	int DEFENDER_COLOR_OFFSET{};

	int ATK_PIECES[6]{};
	int DEF_PIECES[6]{};

	InitGameState::Board new_board{};
	MovementData new_movement_data{};
	PositionalEvalComponent new_pos_eval{};
};
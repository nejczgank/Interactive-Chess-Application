#include "positional_eval_system.h"

void PositionalEvalSystem::materialCount(PositionalEvalComponent& pos_eval, int own_count, int opposite_count) {
	pos_eval.own_material_count += own_count; //how many white pieces have been taken
	pos_eval.opposite_material_count += opposite_count; //how many black pieces have been taken
}
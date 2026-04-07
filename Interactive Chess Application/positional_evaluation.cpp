#include "positional_evaluation.h"

PositionalEvaluation::PositionalEvaluation() {

}

void PositionalEvaluation::materialCount(int white_count, int black_count) {
	white_material_count_ += white_count; //how many white pieces have been taken
	black_material_count_ += black_count; //how many black pieces have been taken
}

PositionalEvaluation::~PositionalEvaluation() = default;
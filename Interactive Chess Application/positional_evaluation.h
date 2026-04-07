#pragma once
class PositionalEvaluation {
public:
	PositionalEvaluation();
	~PositionalEvaluation();
	void materialCount(int, int);
private:
	int white_material_count_ = 0;
	int black_material_count_ = 0;
};
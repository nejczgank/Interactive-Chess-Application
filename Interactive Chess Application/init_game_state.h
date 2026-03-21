#pragma once
#include <iostream>
#include <cstdint>

class InitGameState
{
public:
	struct Board
	{
		uint64_t white_occupancy, black_occupancy, all_occupancy,
			white_pawns, white_knights, white_rooks, white_bishops, white_queens, white_king,
			black_pawns, black_knights, black_rooks, black_bishops, black_queens, black_king;
	};
	InitGameState();
	Board getInitBoardState() const;
	~InitGameState();
private:
	uint64_t white_pawns_;
	uint64_t white_knights_;
	uint64_t white_rooks_;
	uint64_t white_bishops_;
	uint64_t white_queens_;
	uint64_t white_king_;

	uint64_t black_pawns_;
	uint64_t black_knights_;
	uint64_t black_rooks_;
	uint64_t black_bishops_;
	uint64_t black_queens_;
	uint64_t black_king_;

	uint64_t white_occupancy_;
	uint64_t black_occupancy_;
	uint64_t all_occupancy_;
};
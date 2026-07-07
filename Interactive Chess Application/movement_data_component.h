#pragma once
#include <iostream>
#include <bit>

// Array of structures (AoS)
struct MovementData
{
	int picked_square_idx = 0;
	int placement_square_idx = 0;
	int picked_piece_type = 0;
	int placed_piece_type = 0;
	uint64_t allies = 0ULL;
	uint64_t enemies = 0ULL;
	int promoted_piece_type = 0;
};
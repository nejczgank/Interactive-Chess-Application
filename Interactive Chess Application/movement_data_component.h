#pragma once
#include <iostream>
#include <bit>

// Array of structures (AoS)
struct MovementData
{
	//general movement
	int picked_square_idx = 0;
	int placement_square_idx = 0;
	int picked_piece_type = 0;
	int placed_piece_type = 0;
	bool attacker_color = 0;
	bool defender_color = 0;
	uint64_t allies = 0ULL;
	uint64_t enemies = 0ULL;

	//pawn specific
	int promoted_piece_type = 0;
	uint64_t passant_mask = 0;
	bool passant_used = 0;
};
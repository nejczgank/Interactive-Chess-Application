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
	uint64_t allies = 0ULL;
	uint64_t enemies = 0ULL;

	//pawn specific
	int promoted_piece_type = 0;
	uint64_t white_pawn_moved_mask = 0;
	uint64_t black_pawn_moved_mask= 0;

	uint64_t if_white_init_move = 0;
	uint64_t if_black_init_move = 0;

	uint64_t passant_mask = 0;

	uint64_t white_passant_mask = 0;
	uint64_t black_passant_mask = 0;

	uint64_t previous_placed_bit = 0;
	uint64_t if_previous_white_passant = 0;
	uint64_t if_previous_black_passant = 0;
};
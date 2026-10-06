#pragma once
#include <cstdint>

struct EnPassantStateComponent
{
	uint64_t passant_mask{};
	int promoted_piece_type{};
	bool passant_used{};
};
#pragma once
#include <cstdint>

struct TracePathComponent
{
	uint64_t king_and_attacker{};
	uint64_t attacker_origin{};
	uint64_t check_ray{};
};
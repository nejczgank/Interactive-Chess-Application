#pragma once
#include <cstdint>

struct TracePathComponent
{
	uint64_t king_and_attacker{};
	uint64_t attacker_origin{};
	uint64_t attack_ray{};
	//uint64_t IF_PATH_FLAG{};
};
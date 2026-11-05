#pragma once
#include <cstdint>

struct EvalBoardStateEditInfo
{
	enum eval_flags : uint8_t
	{
		discovered_mate_used = 0,
		check_used = 1
	};
};
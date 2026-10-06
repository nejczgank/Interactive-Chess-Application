#pragma once
#include <cstdint>

namespace AttackEvalFlagsInfo 
{
	enum eval_flags : uint8_t
	{
		none	  = 0,
		check	  = 1 << 0,
		checkmate = 1 << 1,
	};
}
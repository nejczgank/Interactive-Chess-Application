#pragma once
#include <cstdint>

namespace rayTranspositionInfo
{
	enum rays : uint64_t
	{
		vertical      = 0x101010101010101,
		horizontal    = 0xff,
		diagonal	  = 0x8040201008040201,
		anti_diagonal = 0x102040810204080
	};
}
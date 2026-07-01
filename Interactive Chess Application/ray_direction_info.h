#pragma once
#include <cstdint>

namespace rayDirectionInfo
{
	enum rays : uint64_t
	{
		north		= 0x0,
		north_east	= 0x0,
		east		= 0x2,
		south_east	= 0x1,
		south		= 0x1,
		south_west	= 0x1,
		west		= 0x3,
		north_west	= 0x0
	};
}
#pragma once
struct CheckOrCheckmateBlock
{
	enum eval_flags : bool
	{
		check = 0,
		checkmate_block = 1
	};
};
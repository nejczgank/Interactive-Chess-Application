#pragma once
#include <cstdint>

/*
* stalemate
* 50 move rule
* insufficient material
* threefold repetition
*/

struct StalemateDataComponent {
	bool stalemate_check{};
	bool draw{};
	bool fifty_move_incr{};
};
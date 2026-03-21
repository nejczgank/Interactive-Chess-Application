#pragma once
#include <iostream>
#include "init_game_state.h"
#include <unordered_map>
#include <algorithm>

class Display {
public:
	static void displayBoard(InitGameState::Board);
private:
	static void addRanksFiles();
	static void constructBoard(InitGameState::Board&);

	static char compressed_board_[64];
};
#pragma once
#include <iostream>
#include <string>
#include <tuple>
#include <unordered_map>
#include "movement_data_component.h"
#include "init_game_state.h"
#include "occupancy_info.h"

class PlayerInput {
public:
	static std::tuple<int, int> moveHandling();
	static void outOfScope();
	static bool turnValidation(InitGameState::Board&, MovementData&, bool, bool);
	static void invalidPick();
private:
	static std::tuple<std::string, std::string> getInput();
	static bool validateInput(std::string&, std::string&, std::string&);
	static int convertToSquareIndex(std::string& square);
	//additional function to convet to algebraic notation
};
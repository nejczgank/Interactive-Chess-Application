#pragma once
#include <iostream>
#include <string>
#include <tuple>
#include <unordered_map>

class PlayerInput {
public:
	static std::tuple<int, int> moveHandling();
	static void outOfScope();
private:
	static std::tuple<std::string, std::string> getInput();
	static bool validateInput(std::string&, std::string&, std::string&);
	static int convertToSquareIndex(std::string& square);
	//additional function to convet to algebraic notation
};
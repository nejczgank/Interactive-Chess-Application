#include "player_input.h"

 std::tuple<int, int> PlayerInput::moveHandling() {

	auto [picked_square, placement_square] = getInput();
	
	return { convertToSquareIndex(picked_square), convertToSquareIndex(placement_square) };
 }

 void PlayerInput::outOfScope() {
	 std::cout << "invalid move. placement outside of scope. Try again\n";
	 return;
 }

std::tuple<std::string, std::string> PlayerInput::getInput() {
	
	std::string format;
	std::string picked_square;
	std::string placement_square;

	while (true) {
		std::cout << "make a move (sq1-sq2): ";

		if (!std::getline(std::cin, format)) {
			continue;
		}

		if (format.length() != 5) {
			std::cout << "invalid format, try again\n";
			continue;
		}

		picked_square = format.substr(0, 2);
		placement_square = format.substr(3, 2);

		if (validateInput(format, picked_square, placement_square)) {
			break;
		}

		std::cout << "invalid format, try again\n";
	}

	return {picked_square, placement_square};
}

bool PlayerInput::validateInput(std::string& format, std::string& picked_square, std::string& placement_square) {

	if (format[2] != '-') {
		return false;
	}

	if (picked_square == placement_square) {
		return false;
	}

	if ((picked_square[0] < 'a' || picked_square[0] > 'h') || (picked_square[1] < '1' || picked_square[1] > '8')) {
		return false;
	}

	return true;
}

int PlayerInput::convertToSquareIndex(std::string& square) { // im getting some weird numbers going on in here

	int file = square[0] - 'a';
	int rank = square[1] - '1';

	/*std::unordered_map<char, int> fileIndicies = {
		{'a', 0},
		{'b', 1},
		{'c', 2},
		{'d', 3},
		{'e', 4},
		{'f', 5},
		{'g', 6},
		{'h', 7},
	};*/

	/*return fileIndicies[file] + 8 * (rank - 1);*/

	return file + (8 * rank);
}
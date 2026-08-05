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

	constexpr int MAX_FORMAT_LENGTH = 5;

	while (true) {
		std::cout << "make a move (sq1-sq2): ";

		if (!std::getline(std::cin, format)) {
			continue;
		}

		if (format.length() != MAX_FORMAT_LENGTH) {
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

	return file + (8 * rank);
}

void PlayerInput::invalidPick()
{
	std::cout << "you picked the wrong piece. Try again\n";
	return;
}

bool PlayerInput::turnValidation(InitGameState::Board& board, MovementData& MOVEMENT_DATA, bool WHITE_COLOR, bool BLACK_COLOR)
{
	using enum occupancyInfo::occupancy;

	const bool PICKED_SQUARE_EMPTY = board.occupancy[white] & (1ULL << MOVEMENT_DATA.picked_square_idx) |
									 board.occupancy[black] & (1ULL << MOVEMENT_DATA.picked_square_idx)
	;

	const bool BOTH_WHITE = (MOVEMENT_DATA.attacker_color == 0 && WHITE_COLOR == 1);
	const bool BOTH_BLACK = (MOVEMENT_DATA.attacker_color == 1 && BLACK_COLOR == 1);

	const bool IS_CORRECT = PICKED_SQUARE_EMPTY && (BOTH_WHITE || BOTH_BLACK);

	return IS_CORRECT;
}
#include <iostream>
#include <cstdint>
#include "init_game_state.h"
#include "player_input.h"
#include "display.h"

int main() {
	//Initialize game state
	InitGameState initBoard;
	
	//Main loop
	//Draw board
	Display::displayBoard(initBoard.getInitBoardState());
	//Input validation
	auto [picked_square_idx, placement_square_idx] = PlayerInput::moveHandling(); //obtain square indicies
	//Move validation


	//determine outcome 
	//clear board

	return 0;
} 
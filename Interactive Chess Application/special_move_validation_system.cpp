#include "special_move_validation_system.h"

int SpecialMoveValidationSystem::checkmateHandler(const InitGameState::Board& BOARD, const MovementData& SEL_MOVEMENT_DATA, const uint64_t VALID_PIECE_PLACEMENT, StalemateDataComponent& stalemate_data, const int TURN)
{
	int constexpr CORRECTION = 6;
	int const ATTACKER_COLOR_OFFSET = SEL_MOVEMENT_DATA.attacker_color * CORRECTION;
	int const DEFENDER_COLOR_OFFSET = SEL_MOVEMENT_DATA.defender_color * CORRECTION;

	const int IS_CHECK = isKingCheck(BOARD, SEL_MOVEMENT_DATA, VALID_PIECE_PLACEMENT, ATTACKER_COLOR_OFFSET, DEFENDER_COLOR_OFFSET);

	constexpr int NO_CHECK = 0;
	constexpr int CHECK_FOUND = 2;
	constexpr int CHECKMATE_CONST = 3;

	//results table
	/*
	* 0 - no check
	* 1 - discovered check
	* 2 - imposed check
	* 3 - checkmate
	*/

	switch (IS_CHECK)
	{
	case NO_CHECK:
		stalemate_data.stalemate_check = isKingCheckmate(BOARD, SEL_MOVEMENT_DATA, VALID_PIECE_PLACEMENT, stalemate_data, ATTACKER_COLOR_OFFSET, DEFENDER_COLOR_OFFSET);
		return NO_CHECK;
	case CHECK_FOUND:
		return CHECKMATE_CONST * isKingCheckmate(BOARD, SEL_MOVEMENT_DATA, VALID_PIECE_PLACEMENT, stalemate_data, ATTACKER_COLOR_OFFSET, DEFENDER_COLOR_OFFSET);
	}

	return IS_CHECK;
}

void SpecialMoveValidationSystem::isPieceAtkHelper(const InitGameState::Board& new_board, MovementData& new_movement_data, bool& ATTACK_FOUND, const int PIECE, const int(&PIECES)[6], const int(&OPP_PIECES)[6])
{
	new_movement_data.picked_piece_type = PIECES[PIECE];
	uint64_t valid_moves = MoveValidationSystem::validator(new_board, new_movement_data);
	//uint64_t temp1 = new_board.pieces[OPP_PIECES[PIECE]];
	ATTACK_FOUND |= ( (new_board.pieces[OPP_PIECES[PIECE]] & valid_moves) > 0);
}

bool SpecialMoveValidationSystem::findCheck(const InitGameState::Board& new_board, MovementData& new_movement_data, const int COLOR, const int OPP_COLOR, const int(&PIECES)[6], const int(&OPP_PIECES)[6])
{
	using enum pieceInfo::piece;

	new_movement_data.allies = new_board.occupancy[COLOR];
	new_movement_data.enemies = new_board.occupancy[OPP_COLOR];
	int64_t const KING_BOARD = (int64_t)new_board.pieces[PIECES[king]];
	new_movement_data.picked_square_idx = std::countr_zero(static_cast<uint64_t>(KING_BOARD));

	bool attack_found = false;

	isPieceAtkHelper(new_board, new_movement_data, attack_found, pawn, PIECES, OPP_PIECES);
	isPieceAtkHelper(new_board, new_movement_data, attack_found, knight, PIECES, OPP_PIECES);
	isPieceAtkHelper(new_board, new_movement_data, attack_found, rook, PIECES, OPP_PIECES);
	isPieceAtkHelper(new_board, new_movement_data, attack_found, bishop, PIECES, OPP_PIECES);
	isPieceAtkHelper(new_board, new_movement_data, attack_found, queen, PIECES, OPP_PIECES);

	return attack_found;
}

int SpecialMoveValidationSystem::isKingCheck(const InitGameState::Board& BOARD, const MovementData& SEL_MOVEMENT_DATA, const uint64_t VALID_PIECE_PLACEMENT, const int ATTACKER_COLOR_OFFSET, const int DEFENDER_COLOR_OFFSET)
{
	//this function runs whenever a piece on the board is moved
	//it determines whether an opposing piece is forcing a check
	//the way it works is by casting rays from the position of the king piece,
	//and checking if the encountered pieces matches a corresponding enemy piece.
	//It calculates both discovered checks and imposed checks

	using enum occupancyInfo::occupancy;
	using enum pieceInfo::piece;

	/*int constexpr CORRECTION = 6;
	int const ATTACKER_COLOR_OFFSET = SEL_MOVEMENT_DATA.attacker_color * CORRECTION;*/
	//int const DEFENDER_COLOR_OFFSET = SEL_MOVEMENT_DATA.defender_color * CORRECTION;

	int	const ATK_PIECES[6] = {
		pawn + ATTACKER_COLOR_OFFSET,
		knight + ATTACKER_COLOR_OFFSET,
		rook + ATTACKER_COLOR_OFFSET,
		bishop + ATTACKER_COLOR_OFFSET,
		queen + ATTACKER_COLOR_OFFSET,
		king + ATTACKER_COLOR_OFFSET
	};

	int const DEF_PIECES[6] = {
		pawn + DEFENDER_COLOR_OFFSET,
		knight + DEFENDER_COLOR_OFFSET,
		rook + DEFENDER_COLOR_OFFSET,
		bishop + DEFENDER_COLOR_OFFSET,
		queen + DEFENDER_COLOR_OFFSET,
		king + DEFENDER_COLOR_OFFSET
	};
	 
	//create temporary move_data
	MovementData new_movement_data = SEL_MOVEMENT_DATA;

	//create temporary board, based on the current layout
	//this will be used to simulate the movement of the current piece
	//it has to be simulated because if the movement turns out to be a discovered check, it's invalid
	InitGameState::Board new_board = BOARD;
	PositionalEvalComponent new_pos_eval;

	//enact piece movement
	BoardUpdatingSystem::updateBoards(new_board, new_movement_data, new_pos_eval, VALID_PIECE_PLACEMENT);

	//FIND POSSIBLE DISCOVERED CHECK
	const uint64_t DISCOVERED_CHECK = -(findCheck(new_board, new_movement_data, SEL_MOVEMENT_DATA.attacker_color, SEL_MOVEMENT_DATA.defender_color, ATK_PIECES, DEF_PIECES) );

	//FIND POSSIBLE CHECK IMPOSED BY THE ATTACKER
	const uint64_t IMPOSED_CHECK	= -(findCheck(new_board, new_movement_data, SEL_MOVEMENT_DATA.defender_color, SEL_MOVEMENT_DATA.attacker_color, DEF_PIECES, ATK_PIECES) );

	//calculate result
	/*
	* 0 - no check
	* 1 - discovered check
	* 2 - imposed check
	*/

	const     uint64_t ANY_CHECK			 = DISCOVERED_CHECK | IMPOSED_CHECK;
	constexpr uint64_t DISCOVERED_CHECK_MASK = 1;
	constexpr uint64_t IMPOSED_CHECK_MASK	 = 2;

	const uint64_t FINAL_RESULT =
		ANY_CHECK &  DISCOVERED_CHECK & ~IMPOSED_CHECK & DISCOVERED_CHECK_MASK |
		ANY_CHECK & ~DISCOVERED_CHECK &  IMPOSED_CHECK & IMPOSED_CHECK_MASK	   |
		ANY_CHECK &  DISCOVERED_CHECK &  IMPOSED_CHECK & DISCOVERED_CHECK_MASK
	;

	return (int)(FINAL_RESULT);
}

bool SpecialMoveValidationSystem::isKingCheckmate(const InitGameState::Board& BOARD, const MovementData& SEL_MOVEMENT_DATA, const uint64_t VALID_PIECE_PLACEMENT, StalemateDataComponent& stalemate_data, const int ATTACKER_COLOR_OFFSET, const int DEFENDER_COLOR_OFFSET)
{
	/*
	* checkmate is determined by moving the king to all available squares and reducing them retroactively
	* by invoking the isKingCheck function for all of them if the king there is under attack
	*/


	using enum pieceInfo::piece;
	using enum occupancyInfo::occupancy;

	//Create a copy of the new board instance, with all passed data
	MovementData new_movement_data = SEL_MOVEMENT_DATA;
	InitGameState::Board new_board = BOARD;
	PositionalEvalComponent new_pos_eval;
	//update for the piece that just moved
	BoardUpdatingSystem::updateBoards(new_board, new_movement_data, new_pos_eval, VALID_PIECE_PLACEMENT);

	//first check whether the piece imposing the check can be overtaken
	constexpr int PIECE_EXPOSED = 2;
	constexpr int NO_CHECKMATE = 0;
	constexpr int NO_MOVES = 0; //the board has already been updated, anddoesn't need to be updated again in isKingCheck

	//subopts the isKingCheck to see whether the attacking piece is threatened, thus preventing a checkmate

	//problem will arise in that picked_square_idx is hardcoded for the king in findcheckhelper
	switch (isKingCheck(new_board, new_movement_data, NO_MOVES, ATTACKER_COLOR_OFFSET, DEFENDER_COLOR_OFFSET)) {
	case PIECE_EXPOSED:
		//also needs to account for a possible imposed check
		return NO_CHECKMATE;
	}

	//set the picked piece to the enemy king, to find the moves available to the king
	//and other relevant data
	new_movement_data.picked_piece_type = king + DEFENDER_COLOR_OFFSET;
	new_movement_data.allies = new_board.occupancy[new_movement_data.defender_color];
	new_movement_data.enemies = new_board.occupancy[new_movement_data.attacker_color];
	int64_t const KING_BOARD = (int64_t)new_board.pieces[king + DEFENDER_COLOR_OFFSET];
	new_movement_data.picked_square_idx = std::countr_zero(static_cast<uint64_t>(KING_BOARD));

	uint64_t valid_moves = MoveValidationSystem::validator(new_board, new_movement_data);

	uint64_t checkmate_found = 0;
	constexpr int CHECK = 2;
	
	while(valid_moves > 0)
	{
		//Isolate and remove the least significant chess position
		uint64_t validate_check_pos = valid_moves & (0ULL - valid_moves);

		//infinite loop now somehow
		uint64_t push_once = validate_check_pos << 1;
		uint64_t tail = push_once - 1;
		uint64_t invert = ~tail;
		valid_moves &= invert;

		//valid_check_pos or-ed with allies, for some reason
		int check_case = isKingCheck(new_board, new_movement_data, validate_check_pos, ATTACKER_COLOR_OFFSET, DEFENDER_COLOR_OFFSET);
		bool not_check = (check_case != CHECK);

		checkmate_found |= -(not_check);
	}
	
	return (checkmate_found == 0);
}

//TODO
//checkmate works, but it still doesn't account for whether overtaking the attacker neutralizes the checkmate.
//
//also account for en passant check!

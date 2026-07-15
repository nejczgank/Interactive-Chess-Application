#include "special_move_validation_system.h"

int SpecialMoveValidationSystem::checkmateHandler(const InitGameState::Board& board, const MovementData& SEL_MOVEMENT_DATA, const uint64_t VALID_PIECE_PLACEMENT)
{
	const int IS_CHECK = isKingCheck(board, SEL_MOVEMENT_DATA, VALID_PIECE_PLACEMENT);

	return IS_CHECK;
}

void SpecialMoveValidationSystem::isPieceAtkHelper(const InitGameState::Board& new_board, MovementData& new_movement_data, bool& ATTACK_FOUND, const int PIECE, const int(&PIECES)[6], const int(&OPP_PIECES)[6])
{
	new_movement_data.picked_piece_type = PIECES[PIECE];
	uint64_t valid_moves = MoveValidationSystem::validator(new_board, new_movement_data);
	uint64_t temp1 = new_board.pieces[OPP_PIECES[PIECE]];
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

int SpecialMoveValidationSystem::isKingCheck(const InitGameState::Board& board, const MovementData& SEL_MOVEMENT_DATA, const uint64_t VALID_PIECE_PLACEMENT)
{
	//this function runs whenever a piece on the board is moved
	//it determines whether an opposing piece is forcing a check
	//the way it works is by casting rays from the position of the king piece,
	//and checking if the encountered pieces matches a corresponding enemy piece.
	//It calculates both discovered checks and imposed checks

	using enum occupancyInfo::occupancy;
	using enum pieceInfo::piece;

	int constexpr CORRECTION = 6;
	int const ATTACKER_COLOR_OFFSET = SEL_MOVEMENT_DATA.attacker_color * CORRECTION;
	int const DEFENDER_COLOR_OFFSET = SEL_MOVEMENT_DATA.defender_color * CORRECTION;

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
	//THIS WILL HAVE TO BE REWORKED, SINCE DEEP COPYING A BOARD IS GOING TO TANK PERFORMANCE
	//ILL NEED TO DO A MAKE/UNMAKE MOVE SYSTEM
	InitGameState::Board new_board = board;
	PositionalEvalComponent new_pos_eval;

	//enact piece movement
	BoardUpdatingSystem::updateBoards(new_board, new_movement_data, new_pos_eval, VALID_PIECE_PLACEMENT);

	//FIND POSSIBLE DISCOVERED CHECK
	const uint64_t DISCOVERED_CHECK = -(findCheck(new_board, new_movement_data, SEL_MOVEMENT_DATA.attacker_color, SEL_MOVEMENT_DATA.defender_color, ATK_PIECES, DEF_PIECES) );

	//FIND POSSIBLE CHECK IMPOSED BY THE ATTACKER
	const uint64_t IMPOSED_CHECK	= -(findCheck(new_board, new_movement_data, SEL_MOVEMENT_DATA.defender_color, SEL_MOVEMENT_DATA.attacker_color, DEF_PIECES, ATK_PIECES) );

	//calculate resul
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



void SpecialMoveValidationSystem::isKingCheckmate() 
{
	//checkmate is determined by moving the king to all available squares and reducing them retroactively
	//by invoking the isKingCheck function for all of them if the king there is under attack
	//**the function also invokes stalemate if only the square the king is currently occupying is safe (and no other pieces are available to be moved) actually no
	// stalemates will calculate be handled differently somehow. some kind of event listener kind of ordeal
	//**clockwise direction of checks

}

//TODO
//make a class that will obtain player turn state and will appropriatelly calculate
//regular checks or discovered checks
//this branching seems find but I think it might incur some serious branch missprediction
//therefore doing it branchless seems so much better imo. afterall you only need to return true or false
//also account for en passant check!

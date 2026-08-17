#include "special_move_validation_system.h"

int SpecialMoveValidationSystem::checkmateHandler(const InitGameState::Board& BOARD, const MovementData& SEL_MOVEMENT_DATA, const uint64_t VALID_PIECE_PLACEMENT, StalemateDataComponent& stalemate_data, const int TURN)
{
	int constexpr CORRECTION = 6;
	int const ATTACKER_COLOR_OFFSET = SEL_MOVEMENT_DATA.attacker_color * CORRECTION;
	int const DEFENDER_COLOR_OFFSET = SEL_MOVEMENT_DATA.defender_color * CORRECTION;

	constexpr bool IS_KING = true;

	const int IS_CHECK = isKingCheck(BOARD, SEL_MOVEMENT_DATA, VALID_PIECE_PLACEMENT, ATTACKER_COLOR_OFFSET, DEFENDER_COLOR_OFFSET, IS_KING);

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
		const int CHECKMATE_FOUND = CHECKMATE_CONST * isKingCheckmate(BOARD, SEL_MOVEMENT_DATA, VALID_PIECE_PLACEMENT, stalemate_data, ATTACKER_COLOR_OFFSET, DEFENDER_COLOR_OFFSET);
		return (CHECKMATE_FOUND > 0) ? CHECKMATE_FOUND : CHECK_FOUND;		
	}

	return IS_CHECK;
}

void SpecialMoveValidationSystem::isPieceAtkHelper(const InitGameState::Board& new_board, MovementData& new_movement_data, bool& ATTACK_FOUND, const int PIECE, const int(&PIECES)[6], const int(&OPP_PIECES)[6])
{
	new_movement_data.picked_piece_type = PIECES[PIECE];
	uint64_t valid_moves = MoveValidationSystem::validator(new_board, new_movement_data);
	ATTACK_FOUND |= ( (new_board.pieces[OPP_PIECES[PIECE]] & valid_moves) > 0);
}

bool SpecialMoveValidationSystem::findCheck(const InitGameState::Board& new_board, MovementData& new_movement_data, const int COLOR, const int OPP_COLOR, const int(&PIECES)[6], const int(&OPP_PIECES)[6], const bool IF_KING)
{
	using enum pieceInfo::piece;

	//this is problematic, because it also has to adjust dynamically if we're using this for the king eval or atk_piece eval
	/*new_movement_data.allies = new_board.occupancy[COLOR];
	new_movement_data.enemies = new_board.occupancy[OPP_COLOR];*/

	findOccupation(new_board, new_movement_data, COLOR, OPP_COLOR, IF_KING);
	//dynamically determines the target index, whether it's a king or an attacking piece
	findTargetIndex(new_board, new_movement_data, PIECES, IF_KING);
	//does the same but for allies and enemies
	

	/*int64_t const KING_BOARD = (int64_t)new_board.pieces[PIECES[king]];
	new_movement_data.picked_square_idx = std::countr_zero(static_cast<uint64_t>(KING_BOARD));*/

	bool attack_found = false;

	isPieceAtkHelper(new_board, new_movement_data, attack_found, pawn, PIECES, OPP_PIECES);
	isPieceAtkHelper(new_board, new_movement_data, attack_found, knight, PIECES, OPP_PIECES);
	isPieceAtkHelper(new_board, new_movement_data, attack_found, rook, PIECES, OPP_PIECES);
	isPieceAtkHelper(new_board, new_movement_data, attack_found, bishop, PIECES, OPP_PIECES);
	isPieceAtkHelper(new_board, new_movement_data, attack_found, queen, PIECES, OPP_PIECES);

	return attack_found;
}

void SpecialMoveValidationSystem::findTargetIndex(const InitGameState::Board& new_board, MovementData& new_movement_data, const int(&PIECES)[6], const bool IF_KING)
{
	/*
	* this function is responsible for ensuring that the picked_square_idx gets adjusted, given that
	* findCheck function is sub-opted from it's original purpose of finding a check for also determining
	* whether the attacking piece can be overtaken inside the checkmate function. The logic for finding any of all possible
	* types of attacking pieces is attacking the square in question is identical.
	* Making this adjustment is required for the validator to find the correct moves in isPieceAtkHelper
	*/

	using enum pieceInfo::piece;

	const uint64_t IF_KING_MASK = -(IF_KING);

	int64_t const KING_BOARD = (int64_t)new_board.pieces[PIECES[king]];
	uint64_t const KING_IDX  = std::countr_zero(static_cast<uint64_t>(KING_BOARD));

	uint64_t const ATTACKING_PIECE_IDX = new_movement_data.placement_square_idx;

	new_movement_data.picked_square_idx = (IF_KING_MASK & KING_IDX) | (~IF_KING_MASK & ATTACKING_PIECE_IDX);
}

void SpecialMoveValidationSystem::findOccupation(const InitGameState::Board& new_board, MovementData& new_movement_data, const int COLOR, const int OPP_COLOR, const bool IF_KING)
{
	const uint64_t IF_KING_MASK = -(IF_KING);

	const uint64_t ALLIES_OCCUPANCY  = (IF_KING_MASK & new_board.occupancy[COLOR])     | (~IF_KING_MASK & new_board.occupancy[OPP_COLOR]);
	const uint64_t ENEMIES_OCCUPANCY = (IF_KING_MASK & new_board.occupancy[OPP_COLOR]) | (~IF_KING_MASK & new_board.occupancy[COLOR]);

	new_movement_data.allies  = ALLIES_OCCUPANCY;
	new_movement_data.enemies = ENEMIES_OCCUPANCY;
}

int SpecialMoveValidationSystem::isKingCheck(const InitGameState::Board& BOARD, const MovementData& SEL_MOVEMENT_DATA, const uint64_t VALID_PIECE_PLACEMENT, const int ATTACKER_COLOR_OFFSET, const int DEFENDER_COLOR_OFFSET, const bool IF_KING)
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
	const uint64_t DISCOVERED_CHECK = -(findCheck(new_board, new_movement_data, SEL_MOVEMENT_DATA.attacker_color, SEL_MOVEMENT_DATA.defender_color, ATK_PIECES, DEF_PIECES, IF_KING) );

	//FIND POSSIBLE CHECK IMPOSED BY THE ATTACKER
	const uint64_t IMPOSED_CHECK	= -(findCheck(new_board, new_movement_data, SEL_MOVEMENT_DATA.defender_color, SEL_MOVEMENT_DATA.attacker_color, DEF_PIECES, ATK_PIECES, IF_KING) );

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
	constexpr int NO_MOVES = 0; //the board has already been updated, and doesn't need to be updated again in isKingCheck
	constexpr bool NOT_KING = 0;

	//sub-opts the isKingCheck to see whether the attacking piece is threatened, thus preventing a checkmate
	//defender and attacker switch places since the piece that just imposed a check has it's safety now being inspected

	/*OKAY SO HERE FIND THE LINE BETWEEN THE ATTACKER AND THE DEFENDING KING
	*check if any piece can move where this line is without incurring imposition checks
	*do a loop for every square here in the same fashion as the lower while loop
	*once the first valid blockade is found return 0 to denote that the checkmate is insolvent
	* **imposed checks from the defender get reverted by ischeck imposition criteria. here all that matters is finding IF any blockade exists
	* **I think I'll have to make additional adjustments for changing the attacking squares and making sure they're somehow occupied. what a pain
	*/

	uint64_t blocker_exists = 0;
	//here you invoke the universal ray function from move validation
	//so to check which array exactly it is I can simply loop through all universal ray directions
	//and to find which one matches exactly I can use an rectangular border as a mask
	//if this mask doesn't intersect with the universal ray, then this is the correct ray
	//**use some bitwise tricks to prevent branching inside the loop once the ray is found

	//or wait no the up above solution has an issue
	//how does universal ray handle king inclusion? does it register it's position as valid placement?
	//or does it negate it
	//if it doesn't negate the position, then I'll simply revert to checking if the king and the universal ray intersect
	//that simple
	//I have to trace back to see exactly how this is done
	//im curious to know, because this is an actual issue if it allows the king to be overtaken like that
	//conversely I'm a bit concerned (though not as much) whether not including the king is an issue (I see how this problem doesn't have to be a problem)

	const uint64_t attack_ray = 0; 
	while (attack_ray > 0)
	{
		isKingCheck(new_board, new_movement_data, NO_MOVES, DEFENDER_COLOR_OFFSET, ATTACKER_COLOR_OFFSET, NOT_KING);
		//retract pieces and update blocker exists
	}

	switch (isKingCheck(new_board, new_movement_data, NO_MOVES, DEFENDER_COLOR_OFFSET, ATTACKER_COLOR_OFFSET, NOT_KING) ) {
	case PIECE_EXPOSED:
		
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
	constexpr bool IS_KING = true;
	
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
		int check_case = isKingCheck(new_board, new_movement_data, validate_check_pos, ATTACKER_COLOR_OFFSET, DEFENDER_COLOR_OFFSET, IS_KING);
		bool not_check = (check_case != CHECK);

		checkmate_found |= -(not_check);
	}
	
	return (checkmate_found == 0);
}

//TODO
//checkmate works, but it still doesn't account for whether overtaking the attacker neutralizes the checkmate.
//
//also account for en passant check!

#include "eval_check_and_mate_system.h"

int EvalCheckAndMateSystem::checkmateHandler(InitGameState::Board& board, MovementData& movement_data, PositionalEvalComponent& pos_eval, const uint64_t VALID_PIECE_PLACEMENT, StalemateDataComponent& stalemate_data)
{
	using enum kingSuboptInfo::subopt;
	using enum ExtractRayTypeInfo::ray_type;

	//create a frame of current game state which is used to
	//simulate board movements for the purpose of determining check or checkmate
	//persistent throughout check and checkmate functions
	CheckSimFrameComponent check_sim_frame;
	setCheckSimFrame(board, movement_data, pos_eval, check_sim_frame);

	//enact movement on this simulated state so it can be utilized for further processing
	//by both check and checkmate
	BoardUpdatingSystem::updateBoards(
		check_sim_frame.new_board,
		check_sim_frame.new_movement_data,
		check_sim_frame.new_pos_eval,
		VALID_PIECE_PLACEMENT
	);

	TracePathComponent path_data;

	CheckTypeColorComponent discovered_check_type;
	setDiscoverCheckColor(movement_data, discovered_check_type);

	CheckTypeColorComponent imposition_check_type;
	setImpositionCheckColor(movement_data, imposition_check_type);

	//set check type for non-subopted king check
	checkTypeFrameComponent check_type;
	setCheckType(check_type, discovered_check_type, imposition_check_type, IS_KING);

	const int IS_CHECK = isKingCheck(check_sim_frame, check_type, path_data, attack_ray);

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

	/*switch (IS_CHECK)
	{
	case NO_CHECK:
		stalemate_data.stalemate_check = isKingCheckmate(check_data, stalemate_data);
		return NO_CHECK;
	case CHECK_FOUND:
		const int CHECKMATE_FOUND = CHECKMATE_CONST * isKingCheckmate(check_data, stalemate_data);
		return (CHECKMATE_FOUND > 0) ? CHECKMATE_FOUND : CHECK_FOUND;		
	}*/

	return IS_CHECK;
}

void EvalCheckAndMateSystem::setCheckSimFrame(InitGameState::Board& board, MovementData& movement_data, PositionalEvalComponent& pos_eval, CheckSimFrameComponent& check_sim_frame)
{
	check_sim_frame.new_board = board;
	check_sim_frame.new_movement_data = movement_data;
	check_sim_frame.new_pos_eval = pos_eval;
}

void EvalCheckAndMateSystem::setDiscoverCheckColor(const MovementData& MOVEMENT_DATA, CheckTypeColorComponent& check_type)
{
	using enum pieceInfo::piece;

	int constexpr CORRECTION = 6;

	check_type.ATTACKER_COLOR = MOVEMENT_DATA.attacker_color;
	check_type.DEFENDER_COLOR = MOVEMENT_DATA.defender_color;

	const int ATTACKER_COLOR_OFFSET = MOVEMENT_DATA.attacker_color * CORRECTION;
	const int DEFENDER_COLOR_OFFSET = MOVEMENT_DATA.defender_color * CORRECTION;

	check_type.ATK_PIECES[0] = pawn + ATTACKER_COLOR_OFFSET;
	check_type.ATK_PIECES[1] = knight + ATTACKER_COLOR_OFFSET;
	check_type.ATK_PIECES[2] = rook + ATTACKER_COLOR_OFFSET;
	check_type.ATK_PIECES[3] = bishop + ATTACKER_COLOR_OFFSET;
	check_type.ATK_PIECES[4] = queen + ATTACKER_COLOR_OFFSET;
	check_type.ATK_PIECES[5] = king + ATTACKER_COLOR_OFFSET;

	check_type.DEF_PIECES[0] = pawn + DEFENDER_COLOR_OFFSET;
	check_type.DEF_PIECES[1] = knight + DEFENDER_COLOR_OFFSET;
	check_type.DEF_PIECES[2] = rook + DEFENDER_COLOR_OFFSET;
	check_type.DEF_PIECES[3] = bishop + DEFENDER_COLOR_OFFSET;
	check_type.DEF_PIECES[4] = queen + DEFENDER_COLOR_OFFSET;
	check_type.DEF_PIECES[5] = king + DEFENDER_COLOR_OFFSET;
}

void EvalCheckAndMateSystem::setImpositionCheckColor(const MovementData& MOVEMENT_DATA, CheckTypeColorComponent& check_type)
{
	using enum pieceInfo::piece;

	int constexpr CORRECTION = 6;

	check_type.ATTACKER_COLOR = MOVEMENT_DATA.defender_color;
	check_type.DEFENDER_COLOR = MOVEMENT_DATA.attacker_color;

	const int ATTACKER_COLOR_OFFSET = MOVEMENT_DATA.defender_color * CORRECTION;
	const int DEFENDER_COLOR_OFFSET = MOVEMENT_DATA.attacker_color * CORRECTION;

	check_type.ATK_PIECES[0] = pawn + ATTACKER_COLOR_OFFSET;
	check_type.ATK_PIECES[1] = knight + ATTACKER_COLOR_OFFSET;
	check_type.ATK_PIECES[2] = rook + ATTACKER_COLOR_OFFSET;
	check_type.ATK_PIECES[3] = bishop + ATTACKER_COLOR_OFFSET;
	check_type.ATK_PIECES[4] = queen + ATTACKER_COLOR_OFFSET;
	check_type.ATK_PIECES[5] = king + ATTACKER_COLOR_OFFSET;

	check_type.DEF_PIECES[0] = pawn + DEFENDER_COLOR_OFFSET;
	check_type.DEF_PIECES[1] = knight + DEFENDER_COLOR_OFFSET;
	check_type.DEF_PIECES[2] = rook + DEFENDER_COLOR_OFFSET;
	check_type.DEF_PIECES[3] = bishop + DEFENDER_COLOR_OFFSET;
	check_type.DEF_PIECES[4] = queen + DEFENDER_COLOR_OFFSET;
	check_type.DEF_PIECES[5] = king + DEFENDER_COLOR_OFFSET;
}

void EvalCheckAndMateSystem::setCheckType(checkTypeFrameComponent& check_type, const CheckTypeColorComponent& FIRST_CHECK_COLOR_TYPE, const CheckTypeColorComponent& SECOND_CHECK_COLOR_TYPE, bool IS_KING)
{
	check_type.first_check_color_type = FIRST_CHECK_COLOR_TYPE;
	check_type.second_check_color_type = SECOND_CHECK_COLOR_TYPE;
	check_type.IS_KING = IS_KING;
}

int EvalCheckAndMateSystem::isKingCheck(CheckSimFrameComponent& check_sim_frame, checkTypeFrameComponent& check_type, TracePathComponent& path_data, ExtractRayTypeInfo::ray_type ray_type)
{
	//this function runs whenever a piece on the board is moved
	//it determines whether an opposing piece is forcing a check
	//the way it works is by casting rays from the position of the king piece,
	//and checking if the encountered pieces matches a corresponding enemy piece.
	//It calculates both discovered checks and imposed checks

	using enum occupancyInfo::occupancy;
	using enum pieceInfo::piece;

	auto& DISCOVERED = check_type.first_check_color_type;
	auto& IMPOSED = check_type.second_check_color_type;
	auto& IS_KING = check_type.IS_KING;

	//FIND POSSIBLE DISCOVERED CHECK
	const uint64_t DISCOVERED_CHECK = -(
		findCheck(
			check_sim_frame,
			DISCOVERED,
			path_data,
			IS_KING,
			ray_type
		)
	);

	//FIND POSSIBLE CHECK IMPOSED BY THE ATTACKER
	const uint64_t IMPOSED_CHECK = -(
		findCheck(
			check_sim_frame,
			IMPOSED,
			path_data,
			IS_KING,
			ray_type
		)
	);

	//calculate result
	/*
	* 0 - no check
	* 1 - discovered check
	* 2 - imposed check
	*/

	const     uint64_t ANY_CHECK = DISCOVERED_CHECK | IMPOSED_CHECK;
	constexpr uint64_t DISCOVERED_CHECK_MASK = 1;
	constexpr uint64_t IMPOSED_CHECK_MASK = 2;

	const uint64_t FINAL_RESULT =
		ANY_CHECK & DISCOVERED_CHECK & ~IMPOSED_CHECK & DISCOVERED_CHECK_MASK |
		ANY_CHECK & ~DISCOVERED_CHECK & IMPOSED_CHECK & IMPOSED_CHECK_MASK |
		ANY_CHECK & DISCOVERED_CHECK & IMPOSED_CHECK & DISCOVERED_CHECK_MASK
		;

	return (int)(FINAL_RESULT);
}

//bool EvalCheckAndMateSystem::findCheck(const CheckSimFrameComponent& check_sim_frame, TracePathComponent& path_data, const int COLOR, const int OPP_COLOR, const int(&ALLY_PIECES)[6], const int(&OPP_PIECES)[6], bool IS_KING)
bool EvalCheckAndMateSystem::findCheck(const CheckSimFrameComponent& check_sim_frame, CheckTypeColorComponent& check_type, TracePathComponent& path_data, const bool IS_KING, ExtractRayTypeInfo::ray_type ray_type)
{
	using enum pieceInfo::piece;

	auto& COLOR = check_type.ATTACKER_COLOR;
	auto& OPP_COLOR = check_type.DEFENDER_COLOR;
	auto& ALLY_PIECES = check_type.ATK_PIECES;
	auto& OPP_PIECES = check_type.DEF_PIECES;

	FoundOccupation found_occupation;

	//determines the target index, whether it's a king or an attacking piece
	found_occupation = findOccupation(
		check_sim_frame,
		COLOR, 
		OPP_COLOR,
		IS_KING
	);
	
	//does the same but for allies and enemies
	const int PICKED_SQUARE_IDX = findTargetIndex(
		check_sim_frame,
		ALLY_PIECES,
		IS_KING
	);

	//here is the transient state. only reads the simulated data for movement and adjusts it in this local struct
	//now I just gotta fix isPieceAtkHelper so that it passes this locally created struct to the validation function it contains
	MovementData probe_sim_movement = check_sim_frame.new_movement_data;
	probe_sim_movement.allies = found_occupation.allies;
	probe_sim_movement.enemies = found_occupation.enemies;
	probe_sim_movement.picked_square_idx = PICKED_SQUARE_IDX;

	bool attack_found = false;

	isPieceAtkHelper(probe_sim_movement, check_sim_frame, path_data, attack_found, pawn,   ALLY_PIECES, OPP_PIECES, ray_type);
	isPieceAtkHelper(probe_sim_movement, check_sim_frame, path_data, attack_found, knight, ALLY_PIECES, OPP_PIECES, ray_type);
	isPieceAtkHelper(probe_sim_movement, check_sim_frame, path_data, attack_found, rook,   ALLY_PIECES, OPP_PIECES, ray_type);
	isPieceAtkHelper(probe_sim_movement, check_sim_frame, path_data, attack_found, bishop, ALLY_PIECES, OPP_PIECES, ray_type);
	isPieceAtkHelper(probe_sim_movement, check_sim_frame, path_data, attack_found, queen,  ALLY_PIECES, OPP_PIECES, ray_type);

	return attack_found;
}

EvalCheckAndMateSystem::FoundOccupation EvalCheckAndMateSystem::findOccupation(const CheckSimFrameComponent& check_sim_frame, const int COLOR, const int OPP_COLOR, bool IS_KING)
{
	auto& NEW_BOARD = check_sim_frame.new_board;
	auto& NEW_MOVEMENT_DATA = check_sim_frame.new_movement_data;

	const uint64_t IF_KING_MASK = -(IS_KING);

	const uint64_t ALLIES_OCCUPANCY  = (IF_KING_MASK & NEW_BOARD.occupancy[COLOR])	   | (~IF_KING_MASK & NEW_BOARD.occupancy[OPP_COLOR]);
	const uint64_t ENEMIES_OCCUPANCY = (IF_KING_MASK & NEW_BOARD.occupancy[OPP_COLOR]) | (~IF_KING_MASK & NEW_BOARD.occupancy[COLOR]);

	return { ALLIES_OCCUPANCY, ENEMIES_OCCUPANCY };
}

int EvalCheckAndMateSystem::findTargetIndex(const CheckSimFrameComponent& check_sim_frame, const int(&PIECES)[6], bool IS_KING)
{
	/*
	* this function is responsible for ensuring that the picked_square_idx gets adjusted, given that
	* findCheck function is sub-opted from it's original purpose of finding a check for also determining
	* whether the attacking piece can be overtaken inside the checkmate function. The logic for finding any of all possible
	* types of attacking pieces is attacking the square in question is identical.
	* Making this adjustment is required for the validator to find the correct moves in isPieceAtkHelper
	*/

	using enum pieceInfo::piece;

	auto& NEW_BOARD = check_sim_frame.new_board;
	auto& new_movement_data = check_sim_frame.new_movement_data;

	const uint64_t IF_KING_MASK = -(IS_KING);

	int64_t  const KING_BOARD = (int64_t)NEW_BOARD.pieces[ PIECES[king] ];
	uint64_t const KING_IDX   = std::countr_zero(static_cast<uint64_t>(KING_BOARD) );

	uint64_t const ATTACKING_PIECE_IDX = new_movement_data.placement_square_idx;

	const int PICKED_SQUARE_IDX = (IF_KING_MASK & KING_IDX) | (~IF_KING_MASK & ATTACKING_PIECE_IDX);

	return PICKED_SQUARE_IDX;
}

void EvalCheckAndMateSystem::isPieceAtkHelper(MovementData& probe_sim_movement, const CheckSimFrameComponent& check_sim_frame, TracePathComponent& path_data, bool& attack_found, const int PIECE, const int(&PIECES)[6], const int(&OPP_PIECES)[6], ExtractRayTypeInfo::ray_type ray_type)
{
	auto& new_board = check_sim_frame.new_board;

	probe_sim_movement.picked_piece_type = PIECES[PIECE];														//set the exact piece type of the attacker
	uint64_t valid_moves = MoveValidationSystem::validator(new_board, probe_sim_movement, path_data, ray_type); //find where the attacker can move
	attack_found |= ( (new_board.pieces[ OPP_PIECES[PIECE] ] & valid_moves) > 0);								//find if it's movement profile intersects with the piece we're inspecting
}

//bool EvalCheckAndMateSystem::isKingCheckmate(EvalCheckAndMateComponent& check_data, StalemateDataComponent& stalemate_data)
//{
//	/*
//	* checkmate is determined by moving the king to all available squares and reducing them retroactively
//	* by invoking the isKingCheck function for all of them if the king there is under attack
//	*/
//
//	using enum pieceInfo::piece;
//	using enum occupancyInfo::occupancy;
//
//	/*auto& new_board = check_data.new_board;
//	auto& new_movement_data = check_data.new_movement_data;
//	auto& new_pos_eval = check_data.new_pos_eval;
//	auto& VALID_PIECE_PLACEMENT = check_data.VALID_PIECE_PLACEMENT;
//	auto& ATTACKER_COLOR_OFFSET = check_data.ATTACKER_COLOR_OFFSET;
//	auto& DEFENDER_COLOR_OFFSET = check_data.DEFENDER_COLOR_OFFSET;
//	auto& path_data = *check_data.path_data;*/
//
//	//update for the piece that just moved
//	//BoardUpdatingSystem::updateBoards(new_board, new_movement_data, new_pos_eval, VALID_PIECE_PLACEMENT);
//
//	//first check whether the piece imposing the check can be overtaken
//	constexpr int PIECE_EXPOSED = 2;
//	constexpr int NO_CHECKMATE = 0;
//	constexpr int NO_MOVES = 0; //the board has already been updated, and doesn't need to be updated again in isKingCheck
//	//constexpr int IS_KING = false;
//
//	//OBTAIN THE RAY IMPOSED BY ATTACKING PIECES
//	uint64_t isolated_atk_ray = findAttackPathHelper(check_data);
//	
//	//FILL ATTACK RAY SQUARES WITH ARTIFICIAL PIECES
//
//
//	constexpr bool NO_KING = false;
//	constexpr int CHECK = 2;
//	int block_available = 0;
//
//	//while (isolated_atk_ray > 0)
//	//{
//	//	//Isolate the least significant square
//	//	uint64_t validate_blocked_square = isolated_atk_ray & (0ULL - isolated_atk_ray);
//	//	
//	//	//Remove the least significant square for subsequent iterations
//	//	isolated_atk_ray &= ~validate_blocked_square;
//
//	//	//update state
//	//	new_movement_data.picked_square_idx = std::countr_zero(validate_blocked_square);
//
//	//	int check_case = isKingCheck(check_data, NO_KING);
//	//	bool not_check = (check_case != CHECK); //check how this functions for when squares are empty
//
//	//	block_available|= -(not_check);
//	//	/*isKingCheck(new_board, new_movement_data, NO_MOVES, DEFENDER_COLOR_OFFSET, ATTACKER_COLOR_OFFSET, NOT_KING);
//	//	retract pieces and update blocker exists*/
//	//}
//
//	//THIS IS STILL USEFUL I THINK, FOR 
//	/*switch (isKingCheck(new_board, new_movement_data, NO_MOVES, DEFENDER_COLOR_OFFSET, ATTACKER_COLOR_OFFSET, NOT_KING) ) {
//	case PIECE_EXPOSED:
//		return NO_CHECKMATE;
//	}*/
//
//	//set the picked piece to the enemy king, to find the moves available to the king
//	//and other relevant data
//	//new_movement_data.picked_piece_type = king + DEFENDER_COLOR_OFFSET;
//	//new_movement_data.allies = new_board.occupancy[new_movement_data.defender_color];
//	//new_movement_data.enemies = new_board.occupancy[new_movement_data.attacker_color];
//	//	
//	//int64_t const KING_BOARD = (int64_t)new_board.pieces[king + DEFENDER_COLOR_OFFSET];
//	//new_movement_data.picked_square_idx = std::countr_zero(static_cast<uint64_t>(KING_BOARD));
//	//
//	////find potential valid king moves
//	//uint64_t valid_moves = MoveValidationSystem::validator(new_board, new_movement_data, path_data);
//
//	/*
//	 * perform check calculations for each potential king movement,
//	 * to extrude viable legal moves, if any exist
//	 * otherwise pass a checkmate found flag
//	*/
//
//	uint64_t checkmate_found = 0;
//	//constexpr int CHECK = 2;
//	constexpr int IS_KING = true;
//	
//	//while(valid_moves > 0)
//	//{
//	//	//Isolate and remove the least significant king position
//	//	uint64_t validate_check_pos = valid_moves & (0ULL - valid_moves);
//	//
//	//	uint64_t push_once = validate_check_pos << 1;
//	//	uint64_t tail = push_once - 1;
//	//	uint64_t invert = ~tail;
//	//	valid_moves &= invert;
//
//	//	//valid_check_pos or-ed with allies, for some reason
//	//	//int check_case = isKingCheck(new_board, new_movement_data, validate_check_pos, ATTACKER_COLOR_OFFSET, DEFENDER_COLOR_OFFSET, IS_KING);
//	//	int check_case = isKingCheck(check_data, IS_KING);
//	//	bool not_check = (check_case != CHECK);
//
//	//	checkmate_found |= -(not_check);
//	//}
//	
//	return (checkmate_found == 0);
//	//combine both blocks and checkmate found, or just preemptively exit at blockers. whatever is more efficient
//}

//uint64_t EvalCheckAndMateSystem::findAttackPathHelper(EvalCheckAndMateComponent& check_data)
//{
//	/*
//	* This function traces the exact ray with which a sliding piece is threatening a king.
//	* The ray is then used for determining exact squares the defender can block (or overtake the attacker)
//	* to prevent a checkmate.
//	*/
//
//	//using enum pieceInfo::piece;
//	//using enum occupancyInfo::occupancy;
//
//	//auto& new_board = check_data.new_board;
//	//auto& new_movement_data = check_data.new_movement_data;
//	//auto& DEFENDER_COLOR_OFFSET = check_data.DEFENDER_COLOR_OFFSET;
//	//auto& path_data = *check_data.path_data;
//
//	////new board instance that will populates only the king and attacker
//	//InitGameState::Board simulated_board = new_board;
//
//	//const uint64_t ISOLATE_ATK_PIECE = (1ULL << new_movement_data.placement_square_idx);
//	//const uint64_t ISOLATE_DEF_KING = simulated_board.pieces[DEFENDER_COLOR_OFFSET + king];
//
//	////reassign board occupancy to those pieces
//	//simulated_board.occupancy[new_movement_data.attacker_color] = ISOLATE_ATK_PIECE;
//	//simulated_board.occupancy[new_movement_data.defender_color] = ISOLATE_DEF_KING;
//	//simulated_board.occupancy[all] = ISOLATE_ATK_PIECE | ISOLATE_DEF_KING;
//
//	////reassign board piece occupancy to those two pieces
//	//std::fill(std::begin(simulated_board.pieces), std::end(simulated_board.pieces), 0ULL);
//	//simulated_board.pieces[new_movement_data.picked_piece_type] = ISOLATE_ATK_PIECE;
//	//simulated_board.pieces[king + DEFENDER_COLOR_OFFSET] = ISOLATE_DEF_KING;
//
//	////update movement state, for the validator function to execute properly
//	//new_movement_data.placed_piece_type = king + DEFENDER_COLOR_OFFSET;
//	//new_movement_data.allies = ISOLATE_ATK_PIECE;
//	//new_movement_data.enemies = ISOLATE_DEF_KING;
//	////given that a movement occurred, we simply update the movement data
//	//new_movement_data.picked_square_idx = new_movement_data.placement_square_idx;
//	//new_movement_data.placement_square_idx = std::countr_zero(simulated_board.occupancy[new_movement_data.defender_color]); //king attack assumed implicitly
//
//	////update specific path data state, for proper universalRay interaction
//	//path_data.king_and_attacker = simulated_board.occupancy[all];
//	//path_data.attacker_origin = ISOLATE_ATK_PIECE;
//	//path_data.IF_PATH_FLAG = -(true);
//
//	////execute and only obtain the exact ray, stored in path_data state
//	//MoveValidationSystem::validator(simulated_board, new_movement_data, path_data);
//	//const uint64_t ISOLATED_ATK_RAY = path_data.attack_ray;
//
//	////reset state for the next iteration
//	//path_data.IF_PATH_FLAG = 0;
//	//path_data.attacker_origin = 0;
//	//path_data.attack_ray = 0;
//	//path_data.king_and_attacker = 0;
//
//	//new_movement_data.placed_piece_type = 0;
//	//new_movement_data.picked_square_idx = 0;
//	//new_movement_data.placement_square_idx = 0;
//	//new_movement_data.allies = 0;
//	//new_movement_data.enemies = 0;
//
//	//return ISOLATED_ATK_RAY;
//}

//TODO
//commit these changes
//clear up all this state mutation inside functions inside functions that should only transform data
//create locally scoped structs to prevent state persistance accross functions (no cleanup required)
//fix the function in move validation to also not mutate state internally
//once these changes are instituted, complete isKingCheckmate function, and finally finish this segment for good
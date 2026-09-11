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

	const uint64_t NO_VALID_KING_MOVES = 0ULL;

	CheckTypeColorComponent discovered_check_type;
	setDiscoverCheckColor(movement_data, discovered_check_type, NO_VALID_KING_MOVES);

	CheckTypeColorComponent imposition_check_type;
	setImpositionCheckColor(movement_data, imposition_check_type, NO_VALID_KING_MOVES);

	//set check type for non-subopted king check
	CheckTypeFrameComponent check_type;
	setCheckType(check_type, discovered_check_type, imposition_check_type, IS_KING);

	const int IS_CHECK = isKingCheck(check_sim_frame, check_type, attack_ray);

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
		stalemate_data.stalemate_check = isKingCheckmate(check_sim_frame, check_type, stalemate_data);
		return NO_CHECK;
	case CHECK_FOUND:
		const int CHECKMATE_FOUND = CHECKMATE_CONST * isKingCheckmate(check_sim_frame, check_type, stalemate_data);
		return (CHECKMATE_FOUND > 0) ? CHECKMATE_FOUND : CHECK_FOUND;		
	}

	return IS_CHECK;
}

void EvalCheckAndMateSystem::setCheckSimFrame(InitGameState::Board& board, MovementData& movement_data, PositionalEvalComponent& pos_eval, CheckSimFrameComponent& check_sim_frame)
{
	check_sim_frame.new_board = board;
	check_sim_frame.new_movement_data = movement_data;
	check_sim_frame.new_pos_eval = pos_eval;
}

void EvalCheckAndMateSystem::setDiscoverCheckColor(const MovementData& MOVEMENT_DATA, CheckTypeColorComponent& check_type, const uint64_t VALID_KING_MOVES)
{
	using enum pieceInfo::piece;

	int constexpr CORRECTION = 6;

	check_type.ATTACKER_COLOR = MOVEMENT_DATA.attacker_color;
	check_type.DEFENDER_COLOR = MOVEMENT_DATA.defender_color;

	check_type.ATTACKER_COLOR_OFFSET = MOVEMENT_DATA.attacker_color * CORRECTION;
	check_type.DEFENDER_COLOR_OFFSET = MOVEMENT_DATA.defender_color * CORRECTION;

	check_type.ATK_PIECES[0] = pawn + check_type.ATTACKER_COLOR_OFFSET;
	check_type.ATK_PIECES[1] = knight + check_type.ATTACKER_COLOR_OFFSET;
	check_type.ATK_PIECES[2] = rook + check_type.ATTACKER_COLOR_OFFSET;
	check_type.ATK_PIECES[3] = bishop + check_type.ATTACKER_COLOR_OFFSET;
	check_type.ATK_PIECES[4] = queen + check_type.ATTACKER_COLOR_OFFSET;
	check_type.ATK_PIECES[5] = king + check_type.ATTACKER_COLOR_OFFSET;

	check_type.DEF_PIECES[0] = pawn + check_type.DEFENDER_COLOR_OFFSET;
	check_type.DEF_PIECES[1] = knight + check_type.DEFENDER_COLOR_OFFSET;
	check_type.DEF_PIECES[2] = rook + check_type.DEFENDER_COLOR_OFFSET;
	check_type.DEF_PIECES[3] = bishop + check_type.DEFENDER_COLOR_OFFSET;
	check_type.DEF_PIECES[4] = queen + check_type.DEFENDER_COLOR_OFFSET;
	check_type.DEF_PIECES[5] = king + check_type.DEFENDER_COLOR_OFFSET;

	check_type.VALID_KING_MOVES = VALID_KING_MOVES;
}

void EvalCheckAndMateSystem::setImpositionCheckColor(const MovementData& MOVEMENT_DATA, CheckTypeColorComponent& check_type, const uint64_t VALID_KING_MOVES)
{
	using enum pieceInfo::piece;

	int constexpr CORRECTION = 6;

	check_type.ATTACKER_COLOR = MOVEMENT_DATA.defender_color;
	check_type.DEFENDER_COLOR = MOVEMENT_DATA.attacker_color;

	check_type.ATTACKER_COLOR_OFFSET = MOVEMENT_DATA.defender_color * CORRECTION;
	check_type.DEFENDER_COLOR_OFFSET = MOVEMENT_DATA.attacker_color * CORRECTION;

	check_type.ATK_PIECES[0] = pawn + check_type.ATTACKER_COLOR_OFFSET;
	check_type.ATK_PIECES[1] = knight + check_type.ATTACKER_COLOR_OFFSET;
	check_type.ATK_PIECES[2] = rook + check_type.ATTACKER_COLOR_OFFSET;
	check_type.ATK_PIECES[3] = bishop + check_type.ATTACKER_COLOR_OFFSET;
	check_type.ATK_PIECES[4] = queen + check_type.ATTACKER_COLOR_OFFSET;
	check_type.ATK_PIECES[5] = king + check_type.ATTACKER_COLOR_OFFSET;

	check_type.DEF_PIECES[0] = pawn + check_type.DEFENDER_COLOR_OFFSET;
	check_type.DEF_PIECES[1] = knight + check_type.DEFENDER_COLOR_OFFSET;
	check_type.DEF_PIECES[2] = rook + check_type.DEFENDER_COLOR_OFFSET;
	check_type.DEF_PIECES[3] = bishop + check_type.DEFENDER_COLOR_OFFSET;
	check_type.DEF_PIECES[4] = queen + check_type.DEFENDER_COLOR_OFFSET;
	check_type.DEF_PIECES[5] = king + check_type.DEFENDER_COLOR_OFFSET;

	check_type.VALID_KING_MOVES = VALID_KING_MOVES;
}

void EvalCheckAndMateSystem::setCheckType(CheckTypeFrameComponent& check_type, const CheckTypeColorComponent& FIRST_CHECK_COLOR_TYPE, const CheckTypeColorComponent& SECOND_CHECK_COLOR_TYPE, bool IF_KING)
{
	check_type.first_check_color_type = FIRST_CHECK_COLOR_TYPE;
	check_type.second_check_color_type = SECOND_CHECK_COLOR_TYPE;
	check_type.IF_KING = IF_KING;
}

int EvalCheckAndMateSystem::isKingCheck(CheckSimFrameComponent& check_sim_frame, CheckTypeFrameComponent& check_type, ExtractRayTypeInfo::ray_type ray_type)
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
	auto& IS_KING = check_type.IF_KING;

	//FIND POSSIBLE DISCOVERED CHECK
	const uint64_t DISCOVERED_CHECK = -(
		findCheck(
			check_sim_frame,
			DISCOVERED,
			IS_KING,
			ray_type
		)
	);

	//FIND POSSIBLE CHECK IMPOSED BY THE ATTACKER
	const uint64_t IMPOSED_CHECK = -(
		findCheck(
			check_sim_frame,
			IMPOSED,
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

bool EvalCheckAndMateSystem::findCheck(const CheckSimFrameComponent& check_sim_frame, CheckTypeColorComponent& check_type, const bool IF_KING, ExtractRayTypeInfo::ray_type ray_type)
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
		IF_KING
	);
	
	//does the same but for allies and enemies
	const int PICKED_SQUARE_IDX = findTargetIndex(
		check_sim_frame,
		ALLY_PIECES,
		IF_KING
	);

	//here is the transient state. only reads the simulated data for movement and adjusts it in this local struct
	//now I just gotta fix isPieceAtkHelper so that it passes this locally created struct to the validation function it contains
	MovementData probe_sim_movement = check_sim_frame.new_movement_data;
	probe_sim_movement.allies = found_occupation.allies;
	probe_sim_movement.enemies = found_occupation.enemies;
	probe_sim_movement.picked_square_idx = PICKED_SQUARE_IDX;

	bool attack_found = false;

	TracePathComponent empty_path_data;

	isPieceAtkHelper(probe_sim_movement, check_sim_frame, empty_path_data, attack_found, pawn,   ALLY_PIECES, OPP_PIECES, ray_type);
	isPieceAtkHelper(probe_sim_movement, check_sim_frame, empty_path_data, attack_found, knight, ALLY_PIECES, OPP_PIECES, ray_type);
	isPieceAtkHelper(probe_sim_movement, check_sim_frame, empty_path_data, attack_found, rook,   ALLY_PIECES, OPP_PIECES, ray_type);
	isPieceAtkHelper(probe_sim_movement, check_sim_frame, empty_path_data, attack_found, bishop, ALLY_PIECES, OPP_PIECES, ray_type);
	isPieceAtkHelper(probe_sim_movement, check_sim_frame, empty_path_data, attack_found, queen,  ALLY_PIECES, OPP_PIECES, ray_type);
	isPieceKingAtkHelper(probe_sim_movement, check_sim_frame, check_type, empty_path_data, attack_found, king, ALLY_PIECES, OPP_PIECES, ray_type, IF_KING);

	return attack_found;
}

EvalCheckAndMateSystem::FoundOccupation EvalCheckAndMateSystem::findOccupation(const CheckSimFrameComponent& check_sim_frame, const int COLOR, const int OPP_COLOR, bool IF_KING)
{
	//find the correct occupancy. varies based on whether the function gets subopted

	auto& NEW_BOARD = check_sim_frame.new_board;
	auto& NEW_MOVEMENT_DATA = check_sim_frame.new_movement_data;

	const uint64_t IF_KING_MASK = -(IF_KING);

	const uint64_t ALLIES_OCCUPANCY  = (IF_KING_MASK & NEW_BOARD.occupancy[COLOR])	   | (~IF_KING_MASK & NEW_BOARD.occupancy[OPP_COLOR]);
	const uint64_t ENEMIES_OCCUPANCY = (IF_KING_MASK & NEW_BOARD.occupancy[OPP_COLOR]) | (~IF_KING_MASK & NEW_BOARD.occupancy[COLOR]);

	return { ALLIES_OCCUPANCY, ENEMIES_OCCUPANCY };
}

int EvalCheckAndMateSystem::findTargetIndex(const CheckSimFrameComponent& check_sim_frame, const int(&PIECES)[6], bool IF_KING)
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

	const uint64_t IF_KING_MASK = -(IF_KING);

	int64_t  const KING_BOARD = (int64_t)NEW_BOARD.pieces[ PIECES[king] ];
	uint64_t const KING_IDX   = std::countr_zero(static_cast<uint64_t>(KING_BOARD) );

	uint64_t const ATTACKING_PIECE_IDX = new_movement_data.picked_square_idx; //no clue why but right now this is placement. I'll change it to picked

	const int PICKED_SQUARE_IDX = (IF_KING_MASK & KING_IDX) | (~IF_KING_MASK & ATTACKING_PIECE_IDX);

	return PICKED_SQUARE_IDX;
}

void EvalCheckAndMateSystem::isPieceAtkHelper(MovementData& probe_sim_movement, const CheckSimFrameComponent& check_sim_frame, TracePathComponent& path_data, bool& attack_found, const int PIECE, const int(&PIECES)[6], const int(&OPP_PIECES)[6], ExtractRayTypeInfo::ray_type ray_type)
{
	using enum pieceInfo::piece;

	auto& new_board = check_sim_frame.new_board;
	probe_sim_movement.picked_piece_type = PIECES[PIECE];														//set the exact piece type of the attacker
	uint64_t valid_moves = MoveValidationSystem::validator(new_board, probe_sim_movement, path_data, ray_type); //find where the attacker can move
	attack_found |= ( (new_board.pieces[ OPP_PIECES[PIECE] ] & valid_moves) > 0);								//find if it's movement profile intersects with the piece we're inspecting
}

void EvalCheckAndMateSystem::isPieceKingAtkHelper(MovementData& probe_sim_movement, const CheckSimFrameComponent& check_sim_frame, CheckTypeColorComponent& check_type, TracePathComponent& path_data, bool& attack_found, const int PIECE, const int(&PIECES)[6], const int(&OPP_PIECES)[6], ExtractRayTypeInfo::ray_type ray_type, const bool IF_KING)
{
	/*
	* This function exists to determine whether a king is capable of blocking an imposed check
	* First it orients itself to the attacking piece, and it applies validated king moves for that piece
	* Secondly, if a king is found within proximity, then it also checks whether overtaking the attacker
	* would put the king within the radius of an opponents king, rendering the overtake unfeasible.
	*/

	using enum pieceInfo::piece;
	using enum kingSuboptInfo::subopt;

	//guard clause to prevent king imposing a check on another king
	if (IF_KING == IS_KING)
	{
		return;
	}

	auto& new_board = check_sim_frame.new_board;

	probe_sim_movement.picked_piece_type = PIECES[PIECE]; //set the exact piece type of the attacker	

	uint64_t valid_moves{};
	//this part is required when the checkmate function subopts the check function to determine possible blocks. king can overtake a piece imposing check
	MoveValidationSystem::KingValidationBundle king_validation_bundle;
	auto& kVB = king_validation_bundle;
	kVB = MoveValidationSystem::kingMoves(probe_sim_movement); //king requires specialized function due to adjacency
	valid_moves = kVB.king_mask;

	//setting movement data 
	MovementData probe_king_sim_movement;
	probe_king_sim_movement = probe_sim_movement;
	probe_king_sim_movement.picked_piece_type = PIECES[king];
	probe_king_sim_movement.picked_square_idx = std::countr_zero(new_board.pieces[PIECES[PIECE]]);
	probe_king_sim_movement.placement_square_idx = probe_sim_movement.picked_square_idx;

	const int REASSIGN_ONE = probe_king_sim_movement.attacker_color;
	probe_king_sim_movement.attacker_color = probe_king_sim_movement.defender_color;
	probe_king_sim_movement.defender_color = REASSIGN_ONE;

	const uint64_t REASSIGN_TWO = probe_king_sim_movement.allies; //reassign enemies and allies
	probe_king_sim_movement.allies = probe_king_sim_movement.enemies;
	probe_king_sim_movement.enemies = REASSIGN_TWO;

	const uint64_t VALID_KING_MOVES = MoveValidationSystem::validator(new_board, probe_king_sim_movement, path_data, ray_type);
	check_type.VALID_KING_MOVES = VALID_KING_MOVES;
	
	//attack_found |= ((VALID_KING_MOVES & valid_moves) > 0);
}

bool EvalCheckAndMateSystem::isKingCheckmate(const CheckSimFrameComponent& check_sim_frame, CheckTypeFrameComponent& check_type_frame, StalemateDataComponent& stalemate_data)
{
	/*
	* checkmate is determined by moving the king to all available squares and reducing them retroactively
	* by invoking the isKingCheck function for all of them if the king there is under attack
	*/

	using enum pieceInfo::piece;
	using enum occupancyInfo::occupancy;
	using enum kingSuboptInfo::subopt;
	using enum ExtractRayTypeInfo::ray_type;
	
	auto& CTF = check_type_frame;
	const auto& DISCOVERED_CHECK_TYPE = check_type_frame.first_check_color_type;
	const auto& IMPOSITION_CHECK_TYPE = check_type_frame.second_check_color_type;

	CTF.IF_KING = NO_KING;

	CheckSimFrameComponent probe_check_sim_frame = check_sim_frame;
	auto& pCSM = probe_check_sim_frame;

	TracePathComponent path_data = findAttackPathHelper(check_sim_frame, DISCOVERED_CHECK_TYPE, stalemate_data);
	uint64_t check_ray_path = path_data.check_ray;

	
	constexpr int CHECK = 2;

	while (check_ray_path > 0)
	{
		//Isolate the least significant square
		uint64_t processed_square = check_ray_path & (0ULL - check_ray_path);
		
		//Remove the least significant square for subsequent iterations
		check_ray_path &= ~processed_square;

		//update state. moves the piece to that position for evaluation
		//pCSM = check_sim_frame;
		//setCheckType(check_type, DISCOVERED_CHECK_TYPE, IMPOSITION_CHECK_TYPE, NO_KING);
		/*pCSM.new_movement_data.picked_square_idx = pCSM.new_movement_data.placement_square_idx;
		pCSM.new_movement_data.placement_square_idx = std::countr_zero(processed_square);

		BoardUpdatingSystem::updateBoards(
			pCSM.new_board,
			pCSM.new_movement_data,
			pCSM.new_pos_eval,
			processed_square
		);*/

		pCSM.new_movement_data.picked_square_idx = pCSM.new_movement_data.placement_square_idx;
		int check_case = isKingCheck(pCSM, CTF, attack_ray);

		if (check_case == CHECK)
		{
			return false; //block available
		}		
	}

	//pCSM = check_sim_frame;
	uint64_t const ATTACKED_KING_MASK = pCSM.new_board.pieces[CTF.first_check_color_type.DEF_PIECES[king]];
	uint64_t process_king_moves = CTF.second_check_color_type.VALID_KING_MOVES;
	uint64_t valid_processed_moves = process_king_moves;

	//simulates the king moving, and prunes moves where the king would be exposed were he to overtake
	if(CTF.second_check_color_type.VALID_KING_MOVES > 0ULL)
	{	
		CTF.IF_KING = IS_KING;

		while (process_king_moves > 0)
		{
			//Isolate the least significant square
			uint64_t processed_square = process_king_moves & (0ULL - process_king_moves);

			//Remove the least significant square for subsequent iterations
			process_king_moves &= ~processed_square;

			//set check state and 
			pCSM.new_movement_data.picked_square_idx = std::countr_zero(ATTACKED_KING_MASK);
			pCSM.new_movement_data.placement_square_idx = std::countr_zero(processed_square);

			int check_case = isKingCheck(pCSM, CTF, attack_ray);


			valid_processed_moves &= check_case * processed_square;
		}
	}

	if (valid_processed_moves > 0)
	{
		return true; //block via king available
	}

	//set the picked piece to the enemy king, to find the moves available to the king and other relevant data
	pCSM = check_sim_frame;
	pCSM.new_movement_data.picked_piece_type = CTF.first_check_color_type.DEF_PIECES[king]; //king + DEFENDER_COLOR_OFFSET;
	pCSM.new_movement_data.allies  = pCSM.new_board.occupancy[pCSM.new_movement_data.defender_color];
	pCSM.new_movement_data.enemies = pCSM.new_board.occupancy[pCSM.new_movement_data.attacker_color];
		
	//uint64_t const ATTACKED_KING_MASK = pCSM.new_board.pieces[CTF.first_check_color_type.DEF_PIECES[king]];
	pCSM.new_movement_data.picked_square_idx = std::countr_zero(ATTACKED_KING_MASK);
	
	//find potential valid king moves
	uint64_t valid_king_moves = MoveValidationSystem::validator(pCSM.new_board, pCSM.new_movement_data, path_data, attack_ray);

	/*
	 * perform check calculations for each potential king movement,
	 * to extrude viable legal moves, if any exist
	 * otherwise pass a checkmate found flag
	*/
	
	while(valid_king_moves > 0)
	{
		//Isolate and remove the least significant king position
		uint64_t processed_square = valid_king_moves & (0ULL - valid_king_moves);
		
		//Remove the least significant square for subsequent iterations
		valid_king_moves &= ~processed_square;
		//static int isKingCheck(CheckSimFrameComponent&, checkTypeFrameComponent&, ExtractRayTypeInfo::ray_type);
		int check_case = isKingCheck(pCSM, CTF, attack_ray);

		if (check_case != CHECK)
		{
			return false; //king moves found
		}
	}

	return true; //checkmate found
}

TracePathComponent EvalCheckAndMateSystem::findAttackPathHelper(const CheckSimFrameComponent& check_sim_frame, const CheckTypeColorComponent& DISCOVERED_CHECK_TYPE, StalemateDataComponent& stalemate_data)
{
	/*
	* This function traces the exact ray with which a sliding piece is threatening a king.
	* The ray is then used for determining exact squares the defender can block (or overtake the attacker)
	* to prevent a checkmate.
	*/

	using enum pieceInfo::piece;
	using enum occupancyInfo::occupancy;
	using enum ExtractRayTypeInfo::ray_type;

	const auto& BOARD = check_sim_frame.new_board;
	const auto& MOVEMENT_DATA = check_sim_frame.new_movement_data;

	//new board instance that will populates only the king and attacker
	InitGameState::Board probe_board = BOARD;
	const uint64_t ISOLATE_ATK_PIECE = (1ULL << MOVEMENT_DATA.placement_square_idx);
	const uint64_t ISOLATE_DEF_KING = probe_board.pieces[DISCOVERED_CHECK_TYPE.DEF_PIECES[king]]; //probe_board.pieces[DISCOVERED_CHECK_TYPE.DEFENDER_COLOR_OFFSET + king];

	//reassign board occupancy to those pieces
	probe_board.occupancy[DISCOVERED_CHECK_TYPE.ATTACKER_COLOR] = ISOLATE_ATK_PIECE;
	probe_board.occupancy[DISCOVERED_CHECK_TYPE.DEFENDER_COLOR] = ISOLATE_DEF_KING;
	probe_board.occupancy[all] = ISOLATE_ATK_PIECE | ISOLATE_DEF_KING;

	//reassign board piece occupancy to those two pieces
	std::fill(std::begin(probe_board.pieces), std::end(probe_board.pieces), 0ULL);
	probe_board.pieces[MOVEMENT_DATA.picked_piece_type] = ISOLATE_ATK_PIECE;
	probe_board.pieces[DISCOVERED_CHECK_TYPE.DEF_PIECES[king]] = ISOLATE_DEF_KING;

	//create a new movement data probe, for the validator function to execute properly
	MovementData probe_movement_data;
	probe_movement_data.placed_piece_type = DISCOVERED_CHECK_TYPE.DEF_PIECES[king];
	probe_movement_data.picked_piece_type = MOVEMENT_DATA.picked_piece_type;
	probe_movement_data.allies = ISOLATE_ATK_PIECE;
	probe_movement_data.enemies = ISOLATE_DEF_KING;
	//given that a movement occurred, we simply update the movement data
	probe_movement_data.picked_square_idx = MOVEMENT_DATA.placement_square_idx;
	probe_movement_data.placement_square_idx = std::countr_zero(probe_board.occupancy[MOVEMENT_DATA.defender_color]); //king attack assumed implicitly

	//update specific path data state, for proper universalRay interaction
	TracePathComponent sim_path_data{};
	sim_path_data.king_and_attacker = probe_board.occupancy[all];
	sim_path_data.attacker_origin = ISOLATE_ATK_PIECE;

	//execute and only obtain the exact ray, stored in path_data state
	MoveValidationSystem::validator(probe_board, probe_movement_data, sim_path_data, check_ray);

	return sim_path_data;
}

//ISSUES
//1. en passant not accounted for in finding blockers
//2. overtaking with a king when in checkmate must also ensure the piece isn't pinned

//SOLUTION
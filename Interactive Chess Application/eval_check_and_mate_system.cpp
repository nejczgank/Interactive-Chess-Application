#include "eval_check_and_mate_system.h"

int EvalCheckAndMateSystem::checkmateHandler(GameStateContextComponent& game_state_ctx, uint64_t VALID_PIECE_PLACEMENT)
{
	/*using enum kingSuboptInfo::subopt;*/
	using enum ExtractRayTypeInfo::ray_type;
	using enum AttackEvalFlagsInfo::eval_flags;
	using enum ProcessedSquareInfo::squareInfo;

	//create a frame of current game state which is used to
	//simulate board movements for the purpose of determining check or checkmate
	//persistent throughout check and checkmate functions
	/*CheckSimFrameComponent check_sim_frame;
	setCheckSimFrame(board, movement_data, pos_eval, check_sim_frame);*/

	//enact movement on this simulated state so it can be utilized for further processing
	//by both check and checkmate
	/*BoardUpdatingSystem::updateBoards(
		check_sim_frame.new_board,
		check_sim_frame.new_movement_data,
		check_sim_frame.new_pos_eval,
		VALID_PIECE_PLACEMENT
	);*/

	//const uint64_t NO_VALID_KING_MOVES = 0ULL;

	//CheckTypeColorComponent discovered_check_type;
	//setDiscoverCheckColor(movement_data, discovered_check_type, NO_VALID_KING_MOVES);

	//CheckTypeColorComponent imposition_check_type;
	//setImpositionCheckColor(movement_data, imposition_check_type, NO_VALID_KING_MOVES);

	////set check type for non-subopted king check
	//CheckTypeFrameComponent check_type;
	//setCheckType(check_type, discovered_check_type, imposition_check_type, IS_KING);

	//const int IS_CHECK = isKingCheck(check_sim_frame, check_type, attack_ray);

	constexpr int NO_CHECK = 0;
	constexpr int CHECK_FOUND = 2;
	constexpr int CHECKMATE_FOUND = 3;

	//results table
	/*
	* 0 - no check
	* 1 - discovered check
	* 2 - imposed check
	* 3 - checkmate
	*/

	ExtractPathComponent path_data{ {}, attack_ray };
	const int IS_CHECK = isSquareAttackedSimulated(game_state_ctx, VALID_PIECE_PLACEMENT, check, NO_PROCESSED_SQ, path_data);

	//exit as imposed or discovered
	if (IS_CHECK != CHECK_FOUND)
	{
		return IS_CHECK;
	}

	const int IS_MATE = isKingCheckmate(game_state_ctx, VALID_PIECE_PLACEMENT);

	return (IS_MATE) ? CHECKMATE_FOUND : CHECK_FOUND;
}

int EvalCheckAndMateSystem::isSquareAttackedSimulated(GameStateContextComponent& game_state_ctx, uint64_t VALID_PIECE_PLACEMENT, const AttackEvalFlagsInfo::eval_flags eval_flags, const uint64_t PROCESSED_SQ, ExtractPathComponent& path_data)
{
	using enum AttackEvalFlagsInfo::eval_flags;
	using enum CheckTypeFlagsInfo::eval_flags;
	using enum EvalBoardStateEditInfo::eval_flags;

	//add that processed square is null
	if ( (AttackEvalFlagsInfo::check & eval_flags) != none)
	{
		const auto [IMPOSED_CHECK_SQ_IDX, DISCOVERED_CHECK_SQ_IDX, _1] = findPickedPieces(game_state_ctx, eval_flags, PROCESSED_SQ);

		int const IMPOSING = evalSquareAttack(game_state_ctx, VALID_PIECE_PLACEMENT, 0ULL, imposing, check_used, IMPOSED_CHECK_SQ_IDX, path_data);
		int const DISCOVERED = evalSquareAttack(game_state_ctx, VALID_PIECE_PLACEMENT, 0ULL, discovered, check_used, DISCOVERED_CHECK_SQ_IDX, path_data);

		//result table
		/*
		 * 0 - no check
		 * 1 - discovered check
		 * 2 - imposed check
		 * 1, 2 -> 1 - discovered check
		 */

		//multiplexor where if discovered is enabled, it overrides imposing
		return IMPOSING * (1 - DISCOVERED) + DISCOVERED;
	}

	//if ( (AttackEvalFlagsInfo::checkmate & eval_flags) != none) //and not no_processed_square
	//{
	//	//wait... why did I even have this code in the first place? I think it might have been needed for the exact thing I wanted to do with processed squares...
	//	const auto [_1, _2, PROCESSED_SQ_IDX] = findPickedPieces(game_state_ctx, eval_flags, PROCESSED_SQ);

	//	int const IMPOSING = isSquareAttacked(game_state_ctx, VALID_PIECE_PLACEMENT, PROCESSED_SQ_IDX, path_data);
	//
	//	int constexpr CHECKMATE_FOUND = 2;
	//	int constexpr DEFAULT_TO_IMPOSITION = 2;
	//	int constexpr CHECKMATE_RETURN_CODE = 3;

	//	//uhhh I guess return code 4 - possible to block

	//	return (IMPOSING == CHECKMATE_FOUND) ? CHECKMATE_RETURN_CODE : DEFAULT_TO_IMPOSITION;
	//}

	if ( (AttackEvalFlagsInfo::checkmate & eval_flags) != none)
	{
		const auto [_1, _2, PROCESSED_SQ_IDX] = findPickedPieces(game_state_ctx, eval_flags, PROCESSED_SQ);

		int const IMPOSING = evalSquareAttack(game_state_ctx, VALID_PIECE_PLACEMENT, 0ULL, imposing, check_used, PROCESSED_SQ_IDX, path_data); // finds if any piece overlaps with the attackers pos

		//you forgot to account for the fact that squares can be empty... not good
		//also you have left over squares where the processed square is, so you gotta fix that

		int constexpr BLOCKER_FOUND = 2;

		if (IMPOSING == BLOCKER_FOUND)
		{
			bool eval_eligibility = evalBlockerEligibility(game_state_ctx, VALID_PIECE_PLACEMENT, PROCESSED_SQ_IDX, PROCESSED_SQ);
			
			int constexpr ELIGIBLE_BLOCKERS_VALUE = 4;

			return ELIGIBLE_BLOCKERS_VALUE * eval_eligibility;
		}

		// both processed square types have idx=processed and allies=enemies
		// here begins custom code I think similar to previous layers.

		return BLOCKER_FOUND;
	}

	//if checkmate and processed_square
	//bro this shit is genuinely COOKED
	//i'll need a totally adjusted function seperate from isAnyPieceAttacking and is isSquareAttacked
	//you perform the same king of intersect which tells you whether a value is true or not but this is used to identify the index of where the possible blockers exists
	//after then you use it as a starting index position and the processed square as a valid_placemenet to update the board
	//once you update the board you have to run a discovered check and if this discovered check exists you return the status code 4
	//crazy shit


	return 0;
}

int inline EvalCheckAndMateSystem::calcPickedPieceType(const GameStateContextComponent& game_state_ctx, const uint64_t processed_square)
{
	using enum pieceInfo::piece;

	auto& BOARD = game_state_ctx.board;

	constexpr int WHITE = 0;
	constexpr int BLACK = 1;

	const uint64_t FIND_TYPE = 
		white_pawn   & -((BOARD.pieces[white_pawn]   & processed_square) > 0) |
		white_knight & -((BOARD.pieces[white_knight] & processed_square) > 0) |
		white_rook   & -((BOARD.pieces[white_rook]   & processed_square) > 0) |
		white_bishop & -((BOARD.pieces[white_bishop] & processed_square) > 0) |
		white_queen  & -((BOARD.pieces[white_queen]  & processed_square) > 0) |
		white_king   & -((BOARD.pieces[white_king]   & processed_square) > 0) |
		black_pawn   & -((BOARD.pieces[black_pawn]   & processed_square) > 0) |
		black_knight & -((BOARD.pieces[black_knight] & processed_square) > 0) |
		black_rook   & -((BOARD.pieces[black_rook]   & processed_square) > 0) |
		black_bishop & -((BOARD.pieces[black_bishop] & processed_square) > 0) |
		black_queen  & -((BOARD.pieces[black_queen]  & processed_square) > 0) |
		black_king   & -((BOARD.pieces[black_king]   & processed_square) > 0);

	return (int)FIND_TYPE;
}

uint64_t inline EvalCheckAndMateSystem::calcAllyKingMask(const GameStateContextComponent& GAME_STATE_CTX)
{
	using enum pieceInfo::piece;

	auto& [BOARD, MOVEMENT_DATA, _1, _2] = GAME_STATE_CTX;

	const uint64_t IF_WHITE = -(MOVEMENT_DATA.attacker_color == 0);
	const uint64_t IF_BLACK = -(MOVEMENT_DATA.attacker_color == 1);

	const uint64_t ALLY_KING_MASK = (IF_WHITE & BOARD.pieces[white_king]) | (IF_BLACK & BOARD.pieces[black_king]);

	return ALLY_KING_MASK;
}

bool EvalCheckAndMateSystem::evalBlockerEligibility(GameStateContextComponent& game_state_ctx, uint64_t VALID_PIECE_PLACEMENT, const int PROCESSED_SQ_IDX, const uint64_t PROCESSED_SQ)
{
	using enum pieceInfo::piece;
	using enum AttackEvalFlagsInfo::eval_flags;
	using enum CheckTypeFlagsInfo::eval_flags;
	using enum ExtractRayTypeInfo::ray_type;
	using enum EvalBoardStateEditInfo::eval_flags;

	auto& [board, movement_data, _1, _2] = game_state_ctx;

	//put the attacker in position to attack the king
	BoardUpdatingSystem::updateBoards(game_state_ctx, VALID_PIECE_PLACEMENT);

	//hold previous state
	int const HOLD_PICKED_IDX = movement_data.picked_square_idx;
	int const HOLD_PICKED_PIECE_TYPE = movement_data.picked_piece_type;
	int const HOLD_PLACEMENT_SQUARE_IDX = movement_data.placement_square_idx;

	//adjust to new state/idx
	movement_data.picked_square_idx = PROCESSED_SQ_IDX;

	//find the intersects to dissect each one systematically. while loop their uint board mask
	//find all blockers
	uint64_t unfiltered_blockers_mask = 0ULL;

	//find pawn blockers
	const uint64_t VALID_PAWN_MOVES = MoveValidationSystem::findPawnAttacks(board, movement_data); // all valid attacks for enemy pawns
	const uint64_t ENEMY_PAWN_LOCATION = board.pieces[getPieceIndex(pawn, movement_data.defender_color)]; // all enemy pawn positions
	unfiltered_blockers_mask |= (VALID_PAWN_MOVES & ENEMY_PAWN_LOCATION); // intersect for valid attacking pawns

	//find sliding piece blockers
	constexpr pieceInfo::piece ATTACKERS[] = {knight, rook, bishop, queen};
	
	ExtractPathComponent sim_path_data{ {}, attack_ray };
	for (const auto piece : ATTACKERS)
	{
		//set this value to access the exact validation type inside isAnyPieceAttacking
		movement_data.picked_piece_type = getPieceIndex(piece, movement_data.attacker_color);

		//get the exact moves where an enemy blocker of a given type can be
		uint64_t valid_moves = MoveValidationSystem::validator(board, movement_data, sim_path_data); 

		//figure out the blockers type
		const int OPPOSITE_PIECE = getPieceIndex(piece, movement_data.defender_color);

		//intersect positions of appropriate piece types with valid moves wherefrom the attack can occur.
		//append it the value to unfiltered blockers
		unfiltered_blockers_mask |= board.pieces[OPPOSITE_PIECE] & valid_moves;
	}

	//find king piece blockers
	auto [_3, VALID_KING_MOVES] = MoveValidationSystem::kingMoves(movement_data);
	const uint64_t ENEMY_KING_LOCATIONS = board.pieces[getPieceIndex(king, movement_data.defender_color)];
	unfiltered_blockers_mask |= (VALID_KING_MOVES & ENEMY_KING_LOCATIONS);

	//for each blocker previously added to the list, find whether they aren't pinned down
	//I THINK THE ABOVE COMMENT MAY NOT BE CORRECT

	//switch teams to prepare state
	constexpr uint64_t EXECUTE = ~0ULL;

	SwitchTeamsComponent switch_teams_sim = getSwitchedTeamsData(game_state_ctx);
	switchTeams(game_state_ctx, switch_teams_sim, EXECUTE);

	const uint64_t ALLY_KING_MASK = calcAllyKingMask(game_state_ctx);

	while (unfiltered_blockers_mask > 0ULL)
	{
		//Isolate the least significant square
		uint64_t processed_square = unfiltered_blockers_mask & (0ULL - unfiltered_blockers_mask);

		//Remove the least significant square for subsequent iterations
		unfiltered_blockers_mask &= ~processed_square;

		//prepare state to simulate the movement:
		movement_data.picked_square_idx = std::countr_zero(processed_square);
		movement_data.placement_square_idx = PROCESSED_SQ_IDX;
		movement_data.picked_piece_type = calcPickedPieceType(game_state_ctx, processed_square); //what if it's empty though?
		//movement_data.placed_piece_type = not sure if I really need this one

		//i need a new orchestrator to not cause too much indirection again...
		//i think I went overboard with this exact function

		//int const DISCOVERED = evalSquareAttack(game_state_ctx, PROCESSED_SQ, ALLY_KING_MASK, discovered, discovered_mate_used, PROCESSED_SQ_IDX, sim_path_data);
		int const DISCOVERED = evalSquareAttack(game_state_ctx, PROCESSED_SQ, ALLY_KING_MASK, discovered, discovered_mate_used, std::countr_zero(ALLY_KING_MASK), sim_path_data);

		int constexpr NO_PIN = 0;

		if (DISCOVERED == NO_PIN)
		{

			switch_teams_sim = getSwitchedTeamsData(game_state_ctx);
			switchTeams(game_state_ctx, switch_teams_sim, EXECUTE);

			movement_data.picked_square_idx = HOLD_PICKED_IDX;
			movement_data.picked_piece_type = HOLD_PICKED_PIECE_TYPE;
			movement_data.placement_square_idx = HOLD_PLACEMENT_SQUARE_IDX;

			BoardUpdatingSystem::reverseMovement(game_state_ctx, VALID_PIECE_PLACEMENT);

			return true;
		}
	}

	switch_teams_sim = getSwitchedTeamsData(game_state_ctx);
	switchTeams(game_state_ctx, switch_teams_sim, EXECUTE);

	movement_data.picked_square_idx = HOLD_PICKED_IDX;
	movement_data.picked_piece_type = HOLD_PICKED_PIECE_TYPE;
	movement_data.placement_square_idx = HOLD_PLACEMENT_SQUARE_IDX;

	BoardUpdatingSystem::reverseMovement(game_state_ctx, VALID_PIECE_PLACEMENT);

	return false;
}

inline constexpr int EvalCheckAndMateSystem::getPieceIndex(const int PIECE_TYPE, const int COLOR)
{
	return PIECE_TYPE + (COLOR * 6);
}

EvalCheckAndMateSystem::PickedPiecesComponent EvalCheckAndMateSystem::findPickedPieces(const GameStateContextComponent& GAME_STATE_CTX, const AttackEvalFlagsInfo::eval_flags eval_flags, uint64_t PROCESSED_SQ)
{
	using enum AttackEvalFlagsInfo::eval_flags;
	using enum pieceInfo::piece;

	const bool IS_CHECK	    = (check	 & eval_flags) != none;
	const bool IS_CHECKMATE = (checkmate & eval_flags) != none;

	auto& [BOARD, MOVEMENT_DATA, _1, _2] = GAME_STATE_CTX;

	const uint64_t ENEMY_KING_MASK = getPieceIndex(king, MOVEMENT_DATA.defender_color);
	const int ENEMY_KING_SQ_IDX	   = std::countr_zero(BOARD.pieces[ENEMY_KING_MASK]) * IS_CHECK;

	const uint64_t ALLY_KING_MASK  = getPieceIndex(king, MOVEMENT_DATA.attacker_color);
	const int ALLY_KING_SQ_IDX	   = std::countr_zero(BOARD.pieces[ALLY_KING_MASK]) * IS_CHECK;

	const int PROCESSED_SQ_IDX     = std::countr_zero(PROCESSED_SQ) * IS_CHECKMATE;
	
	PickedPiecesComponent picked_pieces_data
	{
		ENEMY_KING_SQ_IDX,
		ALLY_KING_SQ_IDX,
		PROCESSED_SQ_IDX
	};

	return picked_pieces_data;
}

inline EvalCheckAndMateSystem::SwitchTeamsComponent EvalCheckAndMateSystem::getSwitchedTeamsData(const GameStateContextComponent& game_state_ctx)
{
	SwitchTeamsComponent switch_teams_bundle
	{
		game_state_ctx.movement_data.allies,
		game_state_ctx.movement_data.enemies,
		game_state_ctx.movement_data.attacker_color,
		game_state_ctx.movement_data.defender_color
	};

	auto& [allies, enemies, attacker_color, defender_color] = switch_teams_bundle;

	//reverse allies an enemies
	allies ^= enemies;
	enemies ^= allies;
	allies ^= enemies;
	//reverse attackers and defenders
	attacker_color ^= 1;
	defender_color ^= 1;

	return switch_teams_bundle;
}

inline void EvalCheckAndMateSystem::switchTeams(GameStateContextComponent& game_state_ctx, const SwitchTeamsComponent& SWITCH_TEAMS_SIM, const uint64_t IF_IMPOSING)
{
	auto& [allies, enemies, attacker_color, defender_color] = SWITCH_TEAMS_SIM;

	game_state_ctx.movement_data.allies			= (IF_IMPOSING & allies)		 | (~IF_IMPOSING & game_state_ctx.movement_data.allies);
	game_state_ctx.movement_data.enemies		= (IF_IMPOSING & enemies)		 | (~IF_IMPOSING & game_state_ctx.movement_data.enemies);
	game_state_ctx.movement_data.attacker_color = (IF_IMPOSING & attacker_color) | (~IF_IMPOSING & game_state_ctx.movement_data.attacker_color);
	game_state_ctx.movement_data.defender_color = (IF_IMPOSING & defender_color) | (~IF_IMPOSING & game_state_ctx.movement_data.defender_color);
}

int EvalCheckAndMateSystem::evalSquareAttack(GameStateContextComponent& game_state_ctx)
{
	return 0;
}

int EvalCheckAndMateSystem::evalSquareAttack(GameStateContextComponent& game_state_ctx, uint64_t VALID_PIECE_PLACEMENT, uint64_t ALLY_KING_MASK, const CheckTypeFlagsInfo::eval_flags eval_flags_1, const EvalBoardStateEditInfo::eval_flags eval_flags_2, const int PICKED_SQ_IDX, ExtractPathComponent& path_data)
{
	//adjusts board movement depending on how we're accessing squareAttack evaluation. Either as:
	// * 0 - discovered mate
	// * 1 - check
	//this matters because when accessing via checkmate movement requires different placement compared to the square later being processed
	BoardUpdatingSystem::updateBoards(game_state_ctx, VALID_PIECE_PLACEMENT);

	const uint64_t ATTACKING_SQ = selectPlacementMask(game_state_ctx, eval_flags_2, VALID_PIECE_PLACEMENT, ALLY_KING_MASK);

	const int ATTACK_VALUE = simSquareAttack(game_state_ctx, ATTACKING_SQ, eval_flags_1, PICKED_SQ_IDX, path_data);

	BoardUpdatingSystem::reverseMovement(game_state_ctx, VALID_PIECE_PLACEMENT);

	return ATTACK_VALUE;
}

uint64_t EvalCheckAndMateSystem::selectPlacementMask(GameStateContextComponent& game_state_ctx, EvalBoardStateEditInfo::eval_flags eval_flags, const uint64_t VALID_PIECE_PLACEMENT, const uint64_t ALLY_KING_MASK)
{
	using enum EvalBoardStateEditInfo::eval_flags;

	const uint64_t IF_PLAYER_PLACEMENT = -static_cast<int>(eval_flags == check_used);
	const uint64_t IF_DISCOVERED_MATE  = -static_cast<int>(eval_flags == discovered_mate_used);

	const uint64_t PLAYER_PLACEMENT_MASK = IF_PLAYER_PLACEMENT & VALID_PIECE_PLACEMENT;
	const uint64_t DICOVERED_MATE_MASK   = IF_DISCOVERED_MATE & ALLY_KING_MASK;

	const uint64_t SELECTED_MASK = PLAYER_PLACEMENT_MASK | DICOVERED_MATE_MASK;

	return SELECTED_MASK;
}

int EvalCheckAndMateSystem::simSquareAttack(GameStateContextComponent& game_state_ctx, uint64_t VALID_PIECE_PLACEMENT, CheckTypeFlagsInfo::eval_flags eval_flags, const int PICKED_SQ_IDX, ExtractPathComponent& path_data)
{
	/*
	* orchestrator function that is responsible for evaluating whether a square was attacked.
	* It's purpose is to direct the exact execution method which varies between imposed or discovered check types.
	* The reason for why it varies stems from how both types require changing ally and enemy pieces.
	* Namely imposition requires this change to center it's POV from the enemy king's side.
	* switching teams either proceeds or doesn't based on the evaluation flag.
	*/

	const uint64_t IF_IMPOSING = 0ULL - static_cast<uint64_t>(eval_flags);

	//adjust team data accordingly
	SwitchTeamsComponent switch_teams_sim = getSwitchedTeamsData(game_state_ctx);
	switchTeams(game_state_ctx, switch_teams_sim, IF_IMPOSING);

	//obtain either a positive check result or negative
	const int IS_ATTACK_FOUND = isSquareAttacked(game_state_ctx, PICKED_SQ_IDX, path_data);

	//readjust team data accordingly
	switch_teams_sim = getSwitchedTeamsData(game_state_ctx);
	switchTeams(game_state_ctx, switch_teams_sim, IF_IMPOSING);

	//adjust the check result to synchronize with the former multiplexor found within isSquareAttackedSimulated
	constexpr int DISCOVERED_VALUE = 1;
	constexpr int IMPOSING_VALUE = 2;

	//calculate the result table:
	/*
	 * 0 - no check
	 * 1 - discovered check
	 * 2 - imposed check
	*/
	const int DISCOVERED_ATTACK = IS_ATTACK_FOUND * (1 - (int)eval_flags) * DISCOVERED_VALUE;
	const int IMPOSED_ATTACK = IS_ATTACK_FOUND * (int)eval_flags * IMPOSING_VALUE;

	const int ATTACK_VALUE = DISCOVERED_ATTACK + IMPOSED_ATTACK;

	return ATTACK_VALUE;
}

int EvalCheckAndMateSystem::isSquareAttacked(GameStateContextComponent& game_state_ctx, const int PICKED_SQ_IDX, ExtractPathComponent& path_data)
{
	/*
	* check if any one of enemy piece is attacking the processed square
	* this usually means the king
	*/

	using enum pieceInfo::piece;
	
	auto& [BOARD, movement_data, _1, _2] = game_state_ctx;

	//hold indicies and piece type state
	const int HOLD_REAL_SQ_IDX	   = movement_data.picked_square_idx;
	const int HOLD_REAL_PIECE_TYPE = movement_data.picked_piece_type;

	//swap indicies
	movement_data.picked_square_idx = PICKED_SQ_IDX;

	//loop through all piece types
	constexpr pieceInfo::piece ATTACKERS[] = {knight, rook, bishop, queen};
	
	bool is_attack_found = false;

	//check pawn attacks
	const uint64_t VALID_PAWN_MOVES = MoveValidationSystem::findPawnAttacks(BOARD, movement_data);
	const uint64_t ENEMY_PAWN_LOCATION = BOARD.pieces[getPieceIndex(pawn, movement_data.defender_color)];
	is_attack_found |= (VALID_PAWN_MOVES & ENEMY_PAWN_LOCATION) > 0;

	//check sliding piece attacks
	for(const auto piece : ATTACKERS)
	{
		movement_data.picked_piece_type = getPieceIndex(piece, movement_data.attacker_color); //set this value to access the exact validation type inside isAnyPieceAttacking
		is_attack_found |= isAnyPieceAttacking(game_state_ctx, piece, path_data, is_attack_found);
	}

	//reset to real idx and piece type
	movement_data.picked_square_idx = HOLD_REAL_SQ_IDX;
	movement_data.picked_piece_type = HOLD_REAL_PIECE_TYPE;

	//implicit bool to int conversion
	return is_attack_found;
}

bool EvalCheckAndMateSystem::isAnyPieceAttacking(GameStateContextComponent& game_state_ctx, pieceInfo::piece piece, ExtractPathComponent& path_data, bool is_attack_found)
{
	auto& [board, movement_data, _1, _2] = game_state_ctx;

	//THIS HAS TO ADJUST FOR PAWNS TO ACCOUNT ONLY DIAGONALS
	uint64_t valid_moves = MoveValidationSystem::validator(board, movement_data, path_data); //find where the attacker can move

	//find if it's movement profile intersects with the piece we're inspecting
	const int OPPOSITE_PIECE = getPieceIndex(piece, movement_data.defender_color);  
	const bool IS_ATTACK_FOUND = (board.pieces[OPPOSITE_PIECE] & valid_moves) > 0;

	return IS_ATTACK_FOUND;
}

int EvalCheckAndMateSystem::isKingCheckmate(GameStateContextComponent& game_state_ctx, const uint64_t VALID_PIECE_PLACEMENT)
{
	uint64_t ATTACK_PATH = getAttackerPath(game_state_ctx);
	//what about knights? //knights can't be blocked, only overtaken. blocking only works for sliding pieces

	bool isBlock = isBlockPossible(game_state_ctx, VALID_PIECE_PLACEMENT, ATTACK_PATH);

	// 1. ACCOUNT FOR BLOCKERS
	if (isBlock == true)
	{
		return false;
	}

	//run code to see if there's any pieces that could block this attack
	// - once you find a move also check whether it causes a discovered check, if so discard it
	// - (king should also work under the discovered system, however you must ensure it doesn't move within the radius of an enemy king)
	// - en passant can cause a discovered check
	
	bool kingMove = canKingMove();

	//2. ACCOUNT FOR POSSIBLE KING MOVES
	if (kingMove == true)
	{
		return false;
	}

	//run code to see if the king has any available moves to cover from an attack

	return true;
}

uint64_t EvalCheckAndMateSystem::getAttackerPath(GameStateContextComponent& game_state_ctx)
{
	/*
	* This function traces the exact ray with which a sliding piece is threatening a king.
	* The ray is then used for determining exact squares the defender can block (or overtake the attacker)
	* to prevent a checkmate.
	*/

	using enum pieceInfo::piece;
	using enum ExtractRayTypeInfo::ray_type;

	auto& [board, movement_data, path_data, _] = game_state_ctx;

	//enact a hypothetical move
	BoardUpdatingSystem::updateBoards(game_state_ctx, (1ULL << movement_data.placement_square_idx));

	//hold previous movement state
	const int HOLD_PICKED_SQ_IDX = movement_data.picked_square_idx;

	//adjust passing state
	//the board has updated, but the movement state hasn't yet therefore:
	//previous placement_sqaure_idx is where the current attacker resides
	const int ATTACKER_ORIGIN = movement_data.placement_square_idx;
	movement_data.picked_square_idx = ATTACKER_ORIGIN; //starting validation from the kings POV

	//find the origin of the king
	const int DEFENDING_KING_TYPE = getPieceIndex(king, movement_data.defender_color);
	const int DEFENDING_KING_ORIGIN = std::countr_zero(board.pieces[DEFENDING_KING_TYPE]);

	//set state the exact state required to obtain the check_ray
	const uint64_t KING_AND_ATTACKER = (1ULL << DEFENDING_KING_ORIGIN) | (1ULL << ATTACKER_ORIGIN);
	const uint64_t ATTACKER_BOARD    = (1ULL << ATTACKER_ORIGIN);
	constexpr uint64_t EXTRACTED_RAY = 0;

	ExtractPathComponent extract_path{ {KING_AND_ATTACKER, ATTACKER_BOARD, EXTRACTED_RAY}, check_ray };

	//enact validation to extract the exact sliding piece ray attacking the king
	const uint64_t ATTACK_PATH = MoveValidationSystem::validator(board, movement_data, extract_path);

	//reverse movement state
	movement_data.picked_square_idx = HOLD_PICKED_SQ_IDX;

	//reverse the hypothetical move
	BoardUpdatingSystem::reverseMovement(game_state_ctx, (1ULL << movement_data.placement_square_idx));

	return ATTACK_PATH;
}

bool EvalCheckAndMateSystem::isBlockPossible(GameStateContextComponent& game_state_ctx, const uint64_t VALID_PIECE_PLACEMENT, uint64_t ATTACK_PATH)
{
	using enum AttackEvalFlagsInfo::eval_flags;
	using enum ProcessedSquareInfo::squareInfo;
	using enum ExtractRayTypeInfo::ray_type;

	ExtractPathComponent path_data{ {}, attack_ray };

	while (ATTACK_PATH > 0)
	{
		//Isolate the least significant square
		uint64_t processed_square = ATTACK_PATH & (0ULL - ATTACK_PATH);
		
		//Remove the least significant square for subsequent iterations
		ATTACK_PATH &= ~processed_square;
		
		const int IS_BLOCKABLE = isSquareAttackedSimulated(game_state_ctx, VALID_PIECE_PLACEMENT, checkmate, processed_square, path_data); //PASS FOR CHECKMATE

		constexpr int IS_BLOCKABLE_VALUE = 4;

		if (IS_BLOCKABLE == IS_BLOCKABLE_VALUE)
		{
			return true;
		}
	}

	return false;
}

bool EvalCheckAndMateSystem::canKingMove()
{
	//okay so here you do a similar thing. first you make the system start up and then have it 

	return 0;
}

//void EvalCheckAndMateSystem::setCheckSimFrame(InitGameState::Board& board, MovementData& movement_data, PositionalEvalComponent& pos_eval, CheckSimFrameComponent& check_sim_frame)
//{
//	check_sim_frame.new_board = board;
//	check_sim_frame.new_movement_data = movement_data;
//	check_sim_frame.new_pos_eval = pos_eval;
//}


//void EvalCheckAndMateSystem::setDiscoverCheckColor(const MovementData& MOVEMENT_DATA, CheckTypeColorComponent& check_type, const uint64_t VALID_KING_MOVES)
//{
//	using enum pieceInfo::piece;
//
//	int constexpr CORRECTION = 6;
//
//	check_type.ATTACKER_COLOR = MOVEMENT_DATA.attacker_color;
//	check_type.DEFENDER_COLOR = MOVEMENT_DATA.defender_color;
//
//	check_type.ATTACKER_COLOR_OFFSET = MOVEMENT_DATA.attacker_color * CORRECTION;
//	check_type.DEFENDER_COLOR_OFFSET = MOVEMENT_DATA.defender_color * CORRECTION;
//
//	check_type.ATK_PIECES[0] = pawn + check_type.ATTACKER_COLOR_OFFSET;
//	check_type.ATK_PIECES[1] = knight + check_type.ATTACKER_COLOR_OFFSET;
//	check_type.ATK_PIECES[2] = rook + check_type.ATTACKER_COLOR_OFFSET;
//	check_type.ATK_PIECES[3] = bishop + check_type.ATTACKER_COLOR_OFFSET;
//	check_type.ATK_PIECES[4] = queen + check_type.ATTACKER_COLOR_OFFSET;
//	check_type.ATK_PIECES[5] = king + check_type.ATTACKER_COLOR_OFFSET;
//
//	check_type.DEF_PIECES[0] = pawn + check_type.DEFENDER_COLOR_OFFSET;
//	check_type.DEF_PIECES[1] = knight + check_type.DEFENDER_COLOR_OFFSET;
//	check_type.DEF_PIECES[2] = rook + check_type.DEFENDER_COLOR_OFFSET;
//	check_type.DEF_PIECES[3] = bishop + check_type.DEFENDER_COLOR_OFFSET;
//	check_type.DEF_PIECES[4] = queen + check_type.DEFENDER_COLOR_OFFSET;
//	check_type.DEF_PIECES[5] = king + check_type.DEFENDER_COLOR_OFFSET;
//
//	check_type.VALID_KING_MOVES = VALID_KING_MOVES;
//}
//
//void EvalCheckAndMateSystem::setImpositionCheckColor(const MovementData& MOVEMENT_DATA, CheckTypeColorComponent& check_type, const uint64_t VALID_KING_MOVES)
//{
//	using enum pieceInfo::piece;
//
//	int constexpr CORRECTION = 6;
//
//	check_type.ATTACKER_COLOR = MOVEMENT_DATA.defender_color;
//	check_type.DEFENDER_COLOR = MOVEMENT_DATA.attacker_color;
//
//	check_type.ATTACKER_COLOR_OFFSET = MOVEMENT_DATA.defender_color * CORRECTION;
//	check_type.DEFENDER_COLOR_OFFSET = MOVEMENT_DATA.attacker_color * CORRECTION;
//
//	check_type.ATK_PIECES[0] = pawn + check_type.ATTACKER_COLOR_OFFSET;
//	check_type.ATK_PIECES[1] = knight + check_type.ATTACKER_COLOR_OFFSET;
//	check_type.ATK_PIECES[2] = rook + check_type.ATTACKER_COLOR_OFFSET;
//	check_type.ATK_PIECES[3] = bishop + check_type.ATTACKER_COLOR_OFFSET;
//	check_type.ATK_PIECES[4] = queen + check_type.ATTACKER_COLOR_OFFSET;
//	check_type.ATK_PIECES[5] = king + check_type.ATTACKER_COLOR_OFFSET;
//
//	check_type.DEF_PIECES[0] = pawn + check_type.DEFENDER_COLOR_OFFSET;
//	check_type.DEF_PIECES[1] = knight + check_type.DEFENDER_COLOR_OFFSET;
//	check_type.DEF_PIECES[2] = rook + check_type.DEFENDER_COLOR_OFFSET;
//	check_type.DEF_PIECES[3] = bishop + check_type.DEFENDER_COLOR_OFFSET;
//	check_type.DEF_PIECES[4] = queen + check_type.DEFENDER_COLOR_OFFSET;
//	check_type.DEF_PIECES[5] = king + check_type.DEFENDER_COLOR_OFFSET;
//
//	check_type.VALID_KING_MOVES = VALID_KING_MOVES;
//}

//void EvalCheckAndMateSystem::setCheckType(CheckTypeFrameComponent& check_type, const CheckTypeColorComponent& FIRST_CHECK_COLOR_TYPE, const CheckTypeColorComponent& SECOND_CHECK_COLOR_TYPE, bool IF_KING)
//{
//	check_type.first_check_color_type = FIRST_CHECK_COLOR_TYPE;
//	check_type.second_check_color_type = SECOND_CHECK_COLOR_TYPE;
//	check_type.IF_KING = IF_KING;
//}
//
//int EvalCheckAndMateSystem::isKingCheck(CheckSimFrameComponent& check_sim_frame, CheckTypeFrameComponent& check_type, ExtractRayTypeInfo::ray_type ray_type)
//{
//	//this function runs whenever a piece on the board is moved
//	//it determines whether an opposing piece is forcing a check
//	//the way it works is by casting rays from the position of the king piece,
//	//and checking if the encountered pieces matches a corresponding enemy piece.
//	//It calculates both discovered checks and imposed checks
//
//	using enum occupancyInfo::occupancy;
//	using enum pieceInfo::piece;
//
//	auto& DISCOVERED = check_type.first_check_color_type;
//	auto& IMPOSED = check_type.second_check_color_type;
//	auto& IS_KING = check_type.IF_KING;
//
//	//FIND POSSIBLE DISCOVERED CHECK
//	const uint64_t DISCOVERED_CHECK = -(
//		findCheck(
//			check_sim_frame,
//			DISCOVERED,
//			IS_KING,
//			ray_type
//		)
//	);
//
//	//FIND POSSIBLE CHECK IMPOSED BY THE ATTACKER
//	const uint64_t IMPOSED_CHECK = -(
//		findCheck(
//			check_sim_frame,
//			IMPOSED,
//			IS_KING,
//			ray_type
//		)
//	);
//
//	//calculate result
//	/*
//	* 0 - no check
//	* 1 - discovered check
//	* 2 - imposed check
//	*/
//
//
//	const     uint64_t ANY_CHECK = DISCOVERED_CHECK | IMPOSED_CHECK;
//	constexpr uint64_t DISCOVERED_CHECK_MASK = 1;
//	constexpr uint64_t IMPOSED_CHECK_MASK = 2;
//
//	const uint64_t FINAL_RESULT =
//		ANY_CHECK & DISCOVERED_CHECK & ~IMPOSED_CHECK & DISCOVERED_CHECK_MASK |
//		ANY_CHECK & ~DISCOVERED_CHECK & IMPOSED_CHECK & IMPOSED_CHECK_MASK |
//		ANY_CHECK & DISCOVERED_CHECK & IMPOSED_CHECK & DISCOVERED_CHECK_MASK
//		;
//
//	return (int)(FINAL_RESULT);
//}
//
//bool EvalCheckAndMateSystem::findCheck(const CheckSimFrameComponent& check_sim_frame, CheckTypeColorComponent& check_type, const bool IF_KING, ExtractRayTypeInfo::ray_type ray_type)
//{
//	using enum pieceInfo::piece;
//
//	auto& COLOR = check_type.ATTACKER_COLOR;
//	auto& OPP_COLOR = check_type.DEFENDER_COLOR;
//	auto& ALLY_PIECES = check_type.ATK_PIECES;
//	auto& OPP_PIECES = check_type.DEF_PIECES;
//
//	FoundOccupation found_occupation;
//
//	//determines the target index, whether it's a king or an attacking piece
//	found_occupation = findOccupation(
//		check_sim_frame,
//		COLOR, 
//		OPP_COLOR,
//		IF_KING
//	);
//	
//	//does the same but for allies and enemies
//	const int PICKED_SQUARE_IDX = findTargetIndex(
//		check_sim_frame,
//		ALLY_PIECES,
//		IF_KING
//	);
//
//	//here is the transient state. only reads the simulated data for movement and adjusts it in this local struct
//	//now I just gotta fix isPieceAtkHelper so that it passes this locally created struct to the validation function it contains
//	MovementData probe_sim_movement = check_sim_frame.new_movement_data;
//	probe_sim_movement.allies = found_occupation.allies;
//	probe_sim_movement.enemies = found_occupation.enemies;
//	probe_sim_movement.picked_square_idx = PICKED_SQUARE_IDX;
//
//	bool attack_found = false;
//
//	TracePathComponent empty_path_data;
//
//	isPieceAtkHelper(probe_sim_movement, check_sim_frame, empty_path_data, attack_found, pawn,   ALLY_PIECES, OPP_PIECES, ray_type);
//	isPieceAtkHelper(probe_sim_movement, check_sim_frame, empty_path_data, attack_found, knight, ALLY_PIECES, OPP_PIECES, ray_type);
//	isPieceAtkHelper(probe_sim_movement, check_sim_frame, empty_path_data, attack_found, rook,   ALLY_PIECES, OPP_PIECES, ray_type);
//	isPieceAtkHelper(probe_sim_movement, check_sim_frame, empty_path_data, attack_found, bishop, ALLY_PIECES, OPP_PIECES, ray_type);
//	isPieceAtkHelper(probe_sim_movement, check_sim_frame, empty_path_data, attack_found, queen,  ALLY_PIECES, OPP_PIECES, ray_type);
//	isPieceKingAtkHelper(probe_sim_movement, check_sim_frame, check_type, empty_path_data, attack_found, king, ALLY_PIECES, OPP_PIECES, ray_type, IF_KING);
//
//	return attack_found;
//}
//
//EvalCheckAndMateSystem::FoundOccupation EvalCheckAndMateSystem::findOccupation(const CheckSimFrameComponent& check_sim_frame, const int COLOR, const int OPP_COLOR, bool IF_KING)
//{
//	//find the correct occupancy. varies based on whether the function gets subopted
//
//	auto& NEW_BOARD = check_sim_frame.new_board;
//	auto& NEW_MOVEMENT_DATA = check_sim_frame.new_movement_data;
//
//	const uint64_t IF_KING_MASK = -(IF_KING);
//
//	const uint64_t ALLIES_OCCUPANCY  = (IF_KING_MASK & NEW_BOARD.occupancy[COLOR])	   | (~IF_KING_MASK & NEW_BOARD.occupancy[OPP_COLOR]);
//	const uint64_t ENEMIES_OCCUPANCY = (IF_KING_MASK & NEW_BOARD.occupancy[OPP_COLOR]) | (~IF_KING_MASK & NEW_BOARD.occupancy[COLOR]);
//
//	return { ALLIES_OCCUPANCY, ENEMIES_OCCUPANCY };
//}
//
//int EvalCheckAndMateSystem::findTargetIndex(const CheckSimFrameComponent& check_sim_frame, const int(&PIECES)[6], bool IF_KING)
//{
//	/*
//	* this function is responsible for ensuring that the picked_square_idx gets adjusted, given that
//	* findCheck function is sub-opted from it's original purpose of finding a check for also determining
//	* whether the attacking piece can be overtaken inside the checkmate function. The logic for finding any of all possible
//	* types of attacking pieces is attacking the square in question is identical.
//	* Making this adjustment is required for the validator to find the correct moves in isPieceAtkHelper
//	*/
//
//	using enum pieceInfo::piece;
//
//	auto& NEW_BOARD = check_sim_frame.new_board;
//	auto& new_movement_data = check_sim_frame.new_movement_data;
//
//	const uint64_t IF_KING_MASK = -(IF_KING);
//
//	int64_t  const KING_BOARD = (int64_t)NEW_BOARD.pieces[ PIECES[king] ];
//	uint64_t const KING_IDX   = std::countr_zero(static_cast<uint64_t>(KING_BOARD) );
//
//	uint64_t const ATTACKING_PIECE_IDX = new_movement_data.picked_square_idx; //no clue why but right now this is placement. I'll change it to picked
//
//	const int PICKED_SQUARE_IDX = (IF_KING_MASK & KING_IDX) | (~IF_KING_MASK & ATTACKING_PIECE_IDX);
//
//	return PICKED_SQUARE_IDX;
//}
//
//void EvalCheckAndMateSystem::isPieceAtkHelper(MovementData& probe_sim_movement, const CheckSimFrameComponent& check_sim_frame, TracePathComponent& path_data, bool& attack_found, const int PIECE, const int(&PIECES)[6], const int(&OPP_PIECES)[6], ExtractRayTypeInfo::ray_type ray_type)
//{
//	using enum pieceInfo::piece;
//
//	auto& new_board = check_sim_frame.new_board;
//	probe_sim_movement.picked_piece_type = PIECES[PIECE];														//set the exact piece type of the attacker
//	uint64_t valid_moves = MoveValidationSystem::validator(new_board, probe_sim_movement, path_data, ray_type); //find where the attacker can move
//	attack_found |= ( (new_board.pieces[ OPP_PIECES[PIECE] ] & valid_moves) > 0);								//find if it's movement profile intersects with the piece we're inspecting
//}

//void EvalCheckAndMateSystem::isPieceKingAtkHelper(MovementData& probe_sim_movement, const CheckSimFrameComponent& check_sim_frame, CheckTypeColorComponent& check_type, TracePathComponent& path_data, bool& attack_found, const int PIECE, const int(&PIECES)[6], const int(&OPP_PIECES)[6], ExtractRayTypeInfo::ray_type ray_type, const bool IF_KING)
//{
//	/*
//	* This function exists to determine whether a king is capable of blocking an imposed check
//	* First it orients itself to the attacking piece, and it applies validated king moves for that piece
//	* Secondly, if a king is found within proximity, then it also checks whether overtaking the attacker
//	* would put the king within the radius of an opponents king, rendering the overtake unfeasible.
//	*/
//
//	using enum pieceInfo::piece;
//	using enum kingSuboptInfo::subopt;
//
//	//guard clause to prevent king imposing a check on another king
//	if (IF_KING == IS_KING)
//	{
//		return;
//	}
//
//	auto& new_board = check_sim_frame.new_board;
//
//	probe_sim_movement.picked_piece_type = PIECES[PIECE]; //set the exact piece type of the attacker	
//
//	uint64_t valid_moves{};
//	//this part is required when the checkmate function subopts the check function to determine possible blocks. king can overtake a piece imposing check
//	MoveValidationSystem::KingValidationBundle king_validation_bundle;
//	auto& kVB = king_validation_bundle;
//	kVB = MoveValidationSystem::kingMoves(probe_sim_movement); //king requires specialized function due to adjacency
//	valid_moves = kVB.king_mask;
//
//	//setting movement data 
//	MovementData probe_king_sim_movement;
//	probe_king_sim_movement = probe_sim_movement;
//	probe_king_sim_movement.picked_piece_type = PIECES[king];
//	probe_king_sim_movement.picked_square_idx = std::countr_zero(new_board.pieces[PIECES[PIECE]]);
//	probe_king_sim_movement.placement_square_idx = probe_sim_movement.picked_square_idx;
//
//	const int REASSIGN_ONE = probe_king_sim_movement.attacker_color;
//	probe_king_sim_movement.attacker_color = probe_king_sim_movement.defender_color;
//	probe_king_sim_movement.defender_color = REASSIGN_ONE;
//
//	const uint64_t REASSIGN_TWO = probe_king_sim_movement.allies; //reassign enemies and allies
//	probe_king_sim_movement.allies = probe_king_sim_movement.enemies;
//	probe_king_sim_movement.enemies = REASSIGN_TWO;
//
//	const uint64_t VALID_KING_MOVES = MoveValidationSystem::validator(new_board, probe_king_sim_movement, path_data, ray_type);
//	check_type.VALID_KING_MOVES = VALID_KING_MOVES;
//	
//	//attack_found |= ((VALID_KING_MOVES & valid_moves) > 0);
//}

//bool EvalCheckAndMateSystem::isKingCheckmate(const CheckSimFrameComponent& check_sim_frame, CheckTypeFrameComponent& check_type_frame, StalemateDataComponent& stalemate_data)
//{
//	/*
//	* checkmate is determined by moving the king to all available squares and reducing them retroactively
//	* by invoking the isKingCheck function for all of them if the king there is under attack
//	*/
//
//	using enum pieceInfo::piece;
//	using enum occupancyInfo::occupancy;
//	using enum kingSuboptInfo::subopt;
//	using enum ExtractRayTypeInfo::ray_type;
//	
//	auto& CTF = check_type_frame;7//	const auto& DISCOVERED_CHECK_TYPE = check_type_frame.first_check_color_type;
//	const auto& IMPOSITION_CHECK_TYPE = check_type_frame.second_check_color_type;
//
//	CTF.IF_KING = NO_KING;
//
//	CheckSimFrameComponent probe_check_sim_frame = check_sim_frame;
//	auto& pCSM = probe_check_sim_frame;
//
//	TracePathComponent path_data = findAttackPathHelper(check_sim_frame, DISCOVERED_CHECK_TYPE, stalemate_data);
//	uint64_t check_ray_path = path_data.check_ray;
//
//	
//	constexpr int CHECK = 2;
//
//	while (check_ray_path > 0)
//	{
//		//Isolate the least significant square
//		uint64_t processed_square = check_ray_path & (0ULL - check_ray_path);
//		
//		//Remove the least significant square for subsequent iterations
//		check_ray_path &= ~processed_square;
//
//		//update state. moves the piece to that position for evaluation
//		//pCSM = check_sim_frame;
//		//setCheckType(check_type, DISCOVERED_CHECK_TYPE, IMPOSITION_CHECK_TYPE, NO_KING);
//		/*pCSM.new_movement_data.picked_square_idx = pCSM.new_movement_data.placement_square_idx;
//		pCSM.new_movement_data.placement_square_idx = std::countr_zero(processed_square);
//
//		BoardUpdatingSystem::updateBoards(
//			pCSM.new_board,
//			pCSM.new_movement_data,
//			pCSM.new_pos_eval,
//			processed_square
//		);*/
//
//		pCSM.new_movement_data.picked_square_idx = pCSM.new_movement_data.placement_square_idx;
//		int check_case = isKingCheck(pCSM, CTF, attack_ray);
//
//		if (check_case == CHECK)
//		{
//			return false; //block available
//		}		
//	}
//
//	//pCSM = check_sim_frame;
//	uint64_t const ATTACKED_KING_MASK = pCSM.new_board.pieces[CTF.first_check_color_type.DEF_PIECES[king]];
//	uint64_t process_king_moves = CTF.second_check_color_type.VALID_KING_MOVES;
//	uint64_t valid_processed_moves = process_king_moves;
//
//	//simulates the king moving, and prunes moves where the king would be exposed were he to overtake
//	if(CTF.second_check_color_type.VALID_KING_MOVES > 0ULL)
//	{	
//		CTF.IF_KING = IS_KING;
//
//		while (process_king_moves > 0)
//		{
//			//Isolate the least significant square
//			uint64_t processed_square = process_king_moves & (0ULL - process_king_moves);
//
//			//Remove the least significant square for subsequent iterations
//			process_king_moves &= ~processed_square;
//
//			//set check state and 
//			pCSM.new_movement_data.picked_square_idx = std::countr_zero(ATTACKED_KING_MASK);
//			pCSM.new_movement_data.placement_square_idx = std::countr_zero(processed_square);
//
//			int check_case = isKingCheck(pCSM, CTF, attack_ray);
//
//
//			valid_processed_moves &= check_case * processed_square;
//		}
//	}
//
//	if (valid_processed_moves > 0)
//	{
//		return true; //block via king available
//	}
//
//	//set the picked piece to the enemy king, to find the moves available to the king and other relevant data
//	pCSM = check_sim_frame;
//	pCSM.new_movement_data.picked_piece_type = CTF.first_check_color_type.DEF_PIECES[king]; //king + DEFENDER_COLOR_OFFSET;
//	pCSM.new_movement_data.allies  = pCSM.new_board.occupancy[pCSM.new_movement_data.defender_color];
//	pCSM.new_movement_data.enemies = pCSM.new_board.occupancy[pCSM.new_movement_data.attacker_color];
//		
//	//uint64_t const ATTACKED_KING_MASK = pCSM.new_board.pieces[CTF.first_check_color_type.DEF_PIECES[king]];
//	pCSM.new_movement_data.picked_square_idx = std::countr_zero(ATTACKED_KING_MASK);
//	
//	//find potential valid king moves
//	uint64_t valid_king_moves = MoveValidationSystem::validator(pCSM.new_board, pCSM.new_movement_data, path_data, attack_ray);
//
//	/*
//	 * perform check calculations for each potential king movement,
//	 * to extrude viable legal moves, if any exist
//	 * otherwise pass a checkmate found flag
//	*/
//	
//	while(valid_king_moves > 0)
//	{
//		//Isolate and remove the least significant king position
//		uint64_t processed_square = valid_king_moves & (0ULL - valid_king_moves);
//		
//		//Remove the least significant square for subsequent iterations
//		valid_king_moves &= ~processed_square;
//		//static int isKingCheck(CheckSimFrameComponent&, checkTypeFrameComponent&, ExtractRayTypeInfo::ray_type);
//		int check_case = isKingCheck(pCSM, CTF, attack_ray);
//
//		if (check_case != CHECK)
//		{
//			return false; //king moves found
//		}
//	}
//
//	return true; //checkmate found
//}
//

//TracePathComponent EvalCheckAndMateSystem::findAttackPathHelper(const CheckSimFrameComponent& check_sim_frame, const CheckTypeColorComponent& DISCOVERED_CHECK_TYPE, StalemateDataComponent& stalemate_data)
//{
//	/*
//	* This function traces the exact ray with which a sliding piece is threatening a king.
//	* The ray is then used for determining exact squares the defender can block (or overtake the attacker)
//	* to prevent a checkmate.
//	*/
//
//	using enum pieceInfo::piece;
//	using enum occupancyInfo::occupancy;
//	using enum ExtractRayTypeInfo::ray_type;
//
//	const auto& BOARD = check_sim_frame.new_board;
//	const auto& MOVEMENT_DATA = check_sim_frame.new_movement_data;
//
//	//new board instance that will populates only the king and attacker
//	InitGameState::Board probe_board = BOARD;
//	const uint64_t ISOLATE_ATK_PIECE = (1ULL << MOVEMENT_DATA.placement_square_idx);
//	const uint64_t ISOLATE_DEF_KING = probe_board.pieces[DISCOVERED_CHECK_TYPE.DEF_PIECES[king]]; //probe_board.pieces[DISCOVERED_CHECK_TYPE.DEFENDER_COLOR_OFFSET + king];
//
//	//reassign board occupancy to those pieces
//	probe_board.occupancy[DISCOVERED_CHECK_TYPE.ATTACKER_COLOR] = ISOLATE_ATK_PIECE;
//	probe_board.occupancy[DISCOVERED_CHECK_TYPE.DEFENDER_COLOR] = ISOLATE_DEF_KING;
//	probe_board.occupancy[all] = ISOLATE_ATK_PIECE | ISOLATE_DEF_KING;
//
//	//reassign board piece occupancy to those two pieces
//	std::fill(std::begin(probe_board.pieces), std::end(probe_board.pieces), 0ULL);
//	probe_board.pieces[MOVEMENT_DATA.picked_piece_type] = ISOLATE_ATK_PIECE;
//	probe_board.pieces[DISCOVERED_CHECK_TYPE.DEF_PIECES[king]] = ISOLATE_DEF_KING;
//
//	//create a new movement data probe, for the validator function to execute properly
//	MovementData probe_movement_data;
//	probe_movement_data.placed_piece_type = DISCOVERED_CHECK_TYPE.DEF_PIECES[king];
//	probe_movement_data.picked_piece_type = MOVEMENT_DATA.picked_piece_type;
//	probe_movement_data.allies = ISOLATE_ATK_PIECE;
//	probe_movement_data.enemies = ISOLATE_DEF_KING;
//	//given that a movement occurred, we simply update the movement data
//	probe_movement_data.picked_square_idx = MOVEMENT_DATA.placement_square_idx;
//	probe_movement_data.placement_square_idx = std::countr_zero(probe_board.occupancy[MOVEMENT_DATA.defender_color]); //king attack assumed implicitly
//
//	//update specific path data state, for proper universalRay interaction
//	TracePathComponent sim_path_data{};
//	sim_path_data.king_and_attacker = probe_board.occupancy[all];
//	sim_path_data.attacker_origin = ISOLATE_ATK_PIECE;
//
//	//execute and only obtain the exact ray, stored in path_data state
//	MoveValidationSystem::validator(probe_board, probe_movement_data, sim_path_data, check_ray);
//
//	return sim_path_data;
//}
//
////ISSUES
////1. en passant not accounted for in finding blockers
////2. overtaking with a king when in checkmate must also ensure the piece isn't pinned
//
////SOLUTION
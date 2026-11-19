#include "eval_check_and_mate_system.h"

//determine calculation type
int EvalCheckAndMateSystem::checkmateHandler(GameStateContextComponent& game_state_ctx, uint64_t VALID_PIECE_PLACEMENT)
{
	/*using enum kingSuboptInfo::subopt;*/
	using enum ExtractRayTypeInfo::ray_type;
	using enum AttackEvalFlagsInfo::eval_flags;
	using enum ProcessedSquareInfo::squareInfo;

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

int EvalCheckAndMateSystem::isSquareAttackedSimulated(GameStateContextComponent& game_state_ctx, uint64_t VALID_PIECE_PLACEMENT, const AttackEvalFlagsInfo::eval_flags eval_flags, const uint64_t PROCESSED_SQ, ExtractPathComponent& path_data)
{
	using enum AttackEvalFlagsInfo::eval_flags;
	using enum CheckTypeFlagsInfo::eval_flags;
	using enum EvalBoardStateEditInfo::eval_flags;

	if ( (AttackEvalFlagsInfo::check & eval_flags) != none)
	{
		const auto [IMPOSED_CHECK_SQ_IDX, DISCOVERED_CHECK_SQ_IDX, _1] = findPickedPieces(game_state_ctx, eval_flags, PROCESSED_SQ);

		int const IMPOSING   = simSquareAttack(game_state_ctx, VALID_PIECE_PLACEMENT, imposing,   IMPOSED_CHECK_SQ_IDX,    path_data);
		int const DISCOVERED = simSquareAttack(game_state_ctx, VALID_PIECE_PLACEMENT, discovered, DISCOVERED_CHECK_SQ_IDX, path_data);

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

	if ( (AttackEvalFlagsInfo::checkmate & eval_flags) != none)
	{
		const auto [_1, _2, PROCESSED_SQ_IDX] = findPickedPieces(game_state_ctx, eval_flags, PROCESSED_SQ);

		int constexpr CHECK_FALLBACK = 2;
		int constexpr ELIGIBLE_BLOCKERS_VALUE = 4;

		bool eval_eligibility = evalBlockerEligibility(game_state_ctx, VALID_PIECE_PLACEMENT, PROCESSED_SQ_IDX, PROCESSED_SQ, path_data);

		return (CHECK_FALLBACK * (1 - eval_eligibility)) + ELIGIBLE_BLOCKERS_VALUE * eval_eligibility;
	}

	return 0;
}

int EvalCheckAndMateSystem::simSquareAttack(GameStateContextComponent& game_state_ctx, uint64_t VALID_PIECE_PLACEMENT, CheckTypeFlagsInfo::eval_flags eval_flags, const int PICKED_SQ_IDX, ExtractPathComponent& path_data)
{
	using enum CheckOrCheckmateBlock::eval_flags;

	//enact the exact move the player just made to test wether it constitutes a check
	BoardUpdatingSystem::updateBoards(game_state_ctx, VALID_PIECE_PLACEMENT);

	const int ATTACK_VALUE = evalSquareAttack(game_state_ctx, VALID_PIECE_PLACEMENT, eval_flags, PICKED_SQ_IDX, path_data, check);

	//reverse the move to bring back the inital state. board_updating_sytem later decided whether a move can occur or not
	BoardUpdatingSystem::reverseMovement(game_state_ctx, VALID_PIECE_PLACEMENT);

	return ATTACK_VALUE;
}

//imposed, discovered full calculation
inline void EvalCheckAndMateSystem::switchTeams(GameStateContextComponent& game_state_ctx, const SwitchTeamsComponent& SWITCH_TEAMS_SIM, const uint64_t IF_IMPOSING)
{
	auto& [allies, enemies, attacker_color, defender_color] = SWITCH_TEAMS_SIM;

	game_state_ctx.movement_data.allies			= (IF_IMPOSING & allies)		 | (~IF_IMPOSING & game_state_ctx.movement_data.allies);
	game_state_ctx.movement_data.enemies		= (IF_IMPOSING & enemies)		 | (~IF_IMPOSING & game_state_ctx.movement_data.enemies);
	game_state_ctx.movement_data.attacker_color = (IF_IMPOSING & attacker_color) | (~IF_IMPOSING & game_state_ctx.movement_data.attacker_color);
	game_state_ctx.movement_data.defender_color = (IF_IMPOSING & defender_color) | (~IF_IMPOSING & game_state_ctx.movement_data.defender_color);
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
	allies  ^= enemies;
	enemies ^= allies;
	allies  ^= enemies;
	//reverse attackers and defenders
	attacker_color ^= 1;
	defender_color ^= 1;

	return switch_teams_bundle;
}

int EvalCheckAndMateSystem::evalSquareAttack(GameStateContextComponent& game_state_ctx, uint64_t VALID_PIECE_PLACEMENT, CheckTypeFlagsInfo::eval_flags eval_flags, const int PICKED_SQ_IDX, ExtractPathComponent& path_data, CheckOrCheckmateBlock::eval_flags check_or_checkmate)
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
	const int IS_ATTACK_FOUND = isSquareAttacked(game_state_ctx, PICKED_SQ_IDX, path_data, check_or_checkmate);

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

int EvalCheckAndMateSystem::isSquareAttacked(GameStateContextComponent& game_state_ctx, const int PICKED_SQ_IDX, ExtractPathComponent& path_data, CheckOrCheckmateBlock::eval_flags check_or_checkmate_block)
{
	/*
	* check if any one of enemy piece is attacking the processed square
	* this usually means the king
	*/

	using enum pieceInfo::piece;
	using enum CheckOrCheckmateBlock::eval_flags;

	auto& [BOARD, movement_data, _1, _2] = game_state_ctx;

	//hold indicies and piece type state
	const int HOLD_REAL_SQ_IDX	   = movement_data.picked_square_idx;
	const int HOLD_REAL_PIECE_TYPE = movement_data.picked_piece_type;

	//swap indicies
	movement_data.picked_square_idx = PICKED_SQ_IDX;

	//loop through all attacker piece types
	constexpr pieceInfo::piece ATTACKERS[] = {knight, rook, bishop, queen};

	bool is_attack_found = false;
	
	//check for diagonal attacks
	const uint64_t IF_ENEMY_PRESENT = 0ULL-(BOARD.occupancy[movement_data.attacker_color] & (1ULL << PICKED_SQ_IDX)); //check if the picked square is occupied by the attacker
	const uint64_t VALID_PAWN_ATTACKS = MoveValidationSystem::findPawnAttacks(BOARD, movement_data);
	const uint64_t ENEMY_PAWN_LOCATION = BOARD.pieces[getPieceIndex(pawn, movement_data.defender_color)];
	const bool ATTACK_POSSIBLE = (IF_ENEMY_PRESENT & VALID_PAWN_ATTACKS & ENEMY_PAWN_LOCATION) > 0;

	//1. check - finds if a pawn is instigating a check
	//2. checkmate block - finds if a pawn is overtaking the attacking piece, thus preventing a checkmate
	is_attack_found |= (IF_ENEMY_PRESENT & VALID_PAWN_ATTACKS & ENEMY_PAWN_LOCATION) > 0;

	//check for pawn advancements blocking a checkmate
	const uint64_t VALID_PAWN_ADVANCEMENTS = MoveValidationSystem::advancePawn(BOARD, movement_data);
	const uint64_t IF_CHECKMATE_BLOCK = -(check_or_checkmate_block == checkmate_block);
	is_attack_found |= -(VALID_PAWN_ADVANCEMENTS > 0) & IF_CHECKMATE_BLOCK;

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

	uint64_t valid_moves = MoveValidationSystem::validator(board, movement_data, path_data); //find where the attacker can move

	//find if it's movement profile intersects with the piece we're inspecting
	const int OPPOSITE_PIECE = getPieceIndex(piece, movement_data.defender_color);  
	const bool IS_ATTACK_FOUND = (board.pieces[OPPOSITE_PIECE] & valid_moves) > 0;

	return IS_ATTACK_FOUND;
}

//finding eligible blockers when a checkmate occurs
bool EvalCheckAndMateSystem::evalBlockerEligibility(GameStateContextComponent& game_state_ctx, uint64_t VALID_PIECE_PLACEMENT, const int PROCESSED_SQ_IDX, const uint64_t PROCESSED_SQ, ExtractPathComponent& sim_path_data)
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
	HoldMovementDataComponent hold_movement_data = 
	{
		movement_data.picked_square_idx,
		movement_data.picked_piece_type,
		movement_data.placement_square_idx,
		movement_data.placed_piece_type
	};

	uint64_t unfiltered_blockers_mask = findBlockers(game_state_ctx, PROCESSED_SQ_IDX);

	//switch teams to prepare state

	SwitchTeamsComponent switch_teams_sim{};
	performSwitch(game_state_ctx, switch_teams_sim);

	const uint64_t ALLY_KING_MASK = calcAllyKingMask(game_state_ctx); 

	ProcessBlockersContext process_blockers_ctx
	{
		ALLY_KING_MASK,
		unfiltered_blockers_mask,
		PROCESSED_SQ_IDX,
		PROCESSED_SQ
	};

	const bool CAN_BLOCK_CHECKMATE = processBlockers(game_state_ctx, process_blockers_ctx, sim_path_data);

	restoreMovementState(game_state_ctx, switch_teams_sim, hold_movement_data);

	BoardUpdatingSystem::reverseMovement(game_state_ctx, VALID_PIECE_PLACEMENT);

	return CAN_BLOCK_CHECKMATE;
}

uint64_t EvalCheckAndMateSystem::findBlockers(GameStateContextComponent& game_state_ctx, const int PROCESSED_SQ_IDX)
{
	using enum pieceInfo::piece;
	using enum ExtractRayTypeInfo::ray_type;
	auto& [board, movement_data, _1, _2] = game_state_ctx;

	//adjust to new state/idx
	setNewPickedSquareIdx(game_state_ctx, PROCESSED_SQ_IDX);

	//find the intersects to dissect each one systematically. while loop their uint board mask
	//find all blockers
	uint64_t unfiltered_blockers_mask = 0ULL;

	//find pawn blockers when the pawn is attacking
	const uint64_t VALID_PAWN_ATTACKS = MoveValidationSystem::findPawnAttacks(board, movement_data); // all valid attacks for enemy pawns
	const uint64_t ENEMY_PAWN_LOCATION = board.pieces[getPieceIndex(pawn, movement_data.defender_color)]; // all enemy pawn positions
	const uint64_t PROCESSED_SQ_OCC = board.occupancy[movement_data.attacker_color];
	unfiltered_blockers_mask |= (VALID_PAWN_ATTACKS & ENEMY_PAWN_LOCATION & PROCESSED_SQ_OCC); // intersect for valid attacking pawns

	//find pawn blockers when the pawn is moving upwards
	const uint64_t VALID_PAWN_ADVANCEMENTS = MoveValidationSystem::advancePawn(board, movement_data);
	unfiltered_blockers_mask |= VALID_PAWN_ADVANCEMENTS;

	//find sliding piece blockers
	constexpr pieceInfo::piece ATTACKERS[] = { knight, rook, bishop, queen };

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

	return unfiltered_blockers_mask;
}

inline void EvalCheckAndMateSystem::setNewPickedSquareIdx(GameStateContextComponent& game_state_ctx, const int PROCESSED_SQ_IDX)
{
	//adjust to new state/idx
	game_state_ctx.movement_data.picked_square_idx = PROCESSED_SQ_IDX;
}

inline void EvalCheckAndMateSystem::performSwitch(GameStateContextComponent& game_state_ctx, SwitchTeamsComponent& switch_teams_sim)
{
	switch_teams_sim = getSwitchedTeamsData(game_state_ctx);
	constexpr uint64_t EXECUTE = ~0ULL;
	switchTeams(game_state_ctx, switch_teams_sim, EXECUTE);
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

bool EvalCheckAndMateSystem::processBlockers(GameStateContextComponent& game_state_ctx, ProcessBlockersContext& process_blockers_ctx, ExtractPathComponent& sim_path_data)
{
	/*
	* this process obtains all possible blockers of a particular blocking square that was passed from isBlockPossible
	* and evaluates whether any of these blockers can move to the square without incurring another check,
	*/
	using enum CheckTypeFlagsInfo::eval_flags;

	auto& [board, movement_data, _1, _2] = game_state_ctx;
	auto& [ALLY_KING_MASK, unfiltered_blockers_mask, PROCESSED_SQ_IDX, PROCESSED_SQ] = process_blockers_ctx;
	
	while (unfiltered_blockers_mask > 0ULL)
	{
		//Isolate the least significant square
		uint64_t processed_square = unfiltered_blockers_mask & (0ULL - unfiltered_blockers_mask);

		//Remove the least significant square for subsequent iterations
		unfiltered_blockers_mask &= ~processed_square;

		//prepare state to simulate the movement:
		movement_data.picked_square_idx    = std::countr_zero(processed_square);
		movement_data.placement_square_idx = PROCESSED_SQ_IDX;
		movement_data.picked_piece_type    = calcPickedPieceType(game_state_ctx, processed_square);
		movement_data.placed_piece_type    = -1;

		ValidPiecePlacementsContext vpp_ctx{
			PROCESSED_SQ,
			ALLY_KING_MASK
		};

		int const DISCOVERED = evalOneBlocker(game_state_ctx, vpp_ctx, discovered, std::countr_zero(ALLY_KING_MASK), sim_path_data);

		int constexpr NO_PIN = 0;
		if (DISCOVERED == NO_PIN)
		{
			return true;
		}
	}

	return false;
}

int inline EvalCheckAndMateSystem::calcPickedPieceType(const GameStateContextComponent& game_state_ctx, const uint64_t processed_square)
{
	using enum pieceInfo::piece;

	auto& BOARD = game_state_ctx.board;

	constexpr int WHITE = 0;
	constexpr int BLACK = 1;

	const uint64_t FIND_TYPE = 
		white_pawn   & -( (BOARD.pieces[white_pawn]   & processed_square) > 0) |
		white_knight & -( (BOARD.pieces[white_knight] & processed_square) > 0) |
		white_rook   & -( (BOARD.pieces[white_rook]   & processed_square) > 0) |
		white_bishop & -( (BOARD.pieces[white_bishop] & processed_square) > 0) |
		white_queen  & -( (BOARD.pieces[white_queen]  & processed_square) > 0) |
		white_king   & -( (BOARD.pieces[white_king]   & processed_square) > 0) |
		black_pawn   & -( (BOARD.pieces[black_pawn]   & processed_square) > 0) |
		black_knight & -( (BOARD.pieces[black_knight] & processed_square) > 0) |
		black_rook   & -( (BOARD.pieces[black_rook]   & processed_square) > 0) |
		black_bishop & -( (BOARD.pieces[black_bishop] & processed_square) > 0) |
		black_queen  & -( (BOARD.pieces[black_queen]  & processed_square) > 0) |
		black_king   & -( (BOARD.pieces[black_king]   & processed_square) > 0);

	return (int)FIND_TYPE;
}

int EvalCheckAndMateSystem::evalOneBlocker(GameStateContextComponent& game_state_ctx, ValidPiecePlacementsContext& vpp_ctx, CheckTypeFlagsInfo::eval_flags eval_flags, const int PICKED_SQ_IDX, ExtractPathComponent& path_data)
{
	using enum pieceInfo::piece;
	using enum CheckOrCheckmateBlock::eval_flags;

	BoardUpdatingSystem::updateBoards(game_state_ctx, vpp_ctx.VALID_PIECE_PLACEMENT);

	//king may have just moved, so locate it on the updated board
	const int KING_BOARD_IDX = getPieceIndex(king, game_state_ctx.movement_data.attacker_color);
	const int KING_SQ_IDX = std::countr_zero(game_state_ctx.board.pieces[KING_BOARD_IDX]);

	const int ATTACK_VALUE = evalSquareAttack(game_state_ctx, vpp_ctx.VALID_PIECE_PLACEMENT, eval_flags, KING_SQ_IDX, path_data, checkmate_block);

	BoardUpdatingSystem::reverseMovement(game_state_ctx, vpp_ctx.VALID_PIECE_PLACEMENT);

	return ATTACK_VALUE;
}

inline void EvalCheckAndMateSystem::restoreMovementState(GameStateContextComponent& game_state_ctx, SwitchTeamsComponent& switch_teams_sim, HoldMovementDataComponent& hold_movement_data)
{
	auto& movement_data = game_state_ctx.movement_data;

	performSwitch(game_state_ctx, switch_teams_sim);

	movement_data.picked_square_idx		= hold_movement_data.HOLD_PICKED_IDX;
	movement_data.picked_piece_type		= hold_movement_data.HOLD_PICKED_PIECE_TYPE;
	movement_data.placement_square_idx	= hold_movement_data.HOLD_PLACEMENT_SQUARE_IDX;
	movement_data.placed_piece_type		= hold_movement_data.HOLD_PLACED_PIECE_TYPE;
}

//checkmate pipeline
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

//for tomorrow:
// PRIORITY FIXES
//1. -1 should only be for empty squares... check if you messed up
//2. finish canKingMove
//3. test the code
//5. handle checkmate in the main game loop
//6. figure out whether to add en-passant as well or nah
//7. clean up the comments mess
//8. clean up dead structs in header
//9. combine namespaces maybe
//10. merge the refactor into dev branch and then pull from the dev branch into main branch
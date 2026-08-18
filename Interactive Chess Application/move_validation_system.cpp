#include "move_validation_system.h"

uint64_t MoveValidationSystem::universalRay(const MovementData& movement_data, const uint64_t direction_bitfield, const uint64_t full_ray)
{
	using enum whichPlayerInfo::playerInfo;

	const uint64_t SEL_RAY_HALF		= -(int64_t)(direction_bitfield & 0x01);		//0 - lsb, 1 - msb
	const uint64_t SEL_RAY_ROTATION = -(int64_t)((direction_bitfield >> 1) & 0x01); //0 - horizontal, 1 - vertical

	//find the appropriate half ray
	const uint64_t DIRECTIONAL_RAY_MASK = rayHalvingHelper(movement_data, SEL_RAY_HALF, SEL_RAY_ROTATION, full_ray);

	//adjust ray when enemy blockers are present
	const uint64_t RAY_TO_ENEMY_BLOCKERS_MASK = findBlockersHelper(movement_data, SEL_RAY_HALF, DIRECTIONAL_RAY_MASK, enemy);

	//adjust ray when ally blockers are present
	const uint64_t RAY_TO_ALLY_BLOCKERS_MASK = findBlockersHelper(movement_data, SEL_RAY_HALF, DIRECTIONAL_RAY_MASK, ally);

	// if blockers are both empty then result is a default halved ray
	// if only one blocker exit than that blocker is the result
	// if both blockers exist then result is the combination of the two

	//there exist five possible states for how slider attack path is calculated:

	/*
	* enemy and ally blockers have to be combined, because individually they cannot calculate the exact stopping point
	* 
	* REGULAR CASES:
	* in two possible cases a ray can encounter either enemy blockers or friendly blockers
	* but both can also be present within the full ray, therein we have to intersect to see which one comes first
	* because of that we need two auxiliary if selectors, that enforce whether one or two blockers exist within a single path
	* a ray may not have any blockers, as such the default directional ray is the result
	* 
	* SPECIAL CASE:
	* ray to allies defaults to 0 if a blocker is in front. 0 usually means there are no blockers, but here it instead
	* provides a false positive. To solve this edge case an intersect with the directional ray and allies is made
	* to isolate this specific instance. If allies are present yet ray detects 0, then a blocker can only be right in front.
	*/

	const uint64_t IF_RAY_TO_ENEMY_BLOCKERS		= -(RAY_TO_ENEMY_BLOCKERS_MASK != 0);
	const uint64_t IF_RAY_TO_ALLY_BLOCKERS		= -(RAY_TO_ALLY_BLOCKERS_MASK != 0);
	//auxiliary selectors
	const uint64_t IF_ONLY_ONE_BLOCKER_EXISTS	= IF_RAY_TO_ENEMY_BLOCKERS ^ IF_RAY_TO_ALLY_BLOCKERS;
	const uint64_t IF_BOTH_BLOCKERS_EXIST		= IF_RAY_TO_ENEMY_BLOCKERS & IF_RAY_TO_ALLY_BLOCKERS;
	//special case
	const uint64_t IF_NO_PATH_FORWARD			= -( ( (DIRECTIONAL_RAY_MASK & movement_data.allies) != 0) && IF_RAY_TO_ALLY_BLOCKERS == 0);

	const uint64_t UNIVERSAL_RAY_MASK = 
		(IF_ONLY_ONE_BLOCKER_EXISTS & ~IF_NO_PATH_FORWARD & RAY_TO_ENEMY_BLOCKERS_MASK)						| //only enemy blockers
		(IF_ONLY_ONE_BLOCKER_EXISTS & ~IF_NO_PATH_FORWARD & RAY_TO_ALLY_BLOCKERS_MASK)						| //only ally blockers
		(IF_BOTH_BLOCKERS_EXIST & (RAY_TO_ENEMY_BLOCKERS_MASK & RAY_TO_ALLY_BLOCKERS_MASK) )				| //both blockers
		(~IF_BOTH_BLOCKERS_EXIST & ~IF_ONLY_ONE_BLOCKER_EXISTS & ~IF_NO_PATH_FORWARD & DIRECTIONAL_RAY_MASK); //full directional ray

	return UNIVERSAL_RAY_MASK;
}

uint64_t MoveValidationSystem::rayHalvingHelper(const MovementData& movement_data, const uint64_t SEL_RAY_HALF, const uint64_t SEL_RAY_ROTATION, const uint64_t full_ray)
{
	/*
	* a ray is initially constructed via full_ray, obtained from defined constants in ray_transposition_info
	* this ray gets transposed to fit exactly where the piece is situated
	* sel_ray_dir handles which half of the full ray needs to be selected, 
	* whereas sel_transposed_ray handles which ray rotation has to be selected
	*/

	using enum rayTranspositionInfo::rays;

	//isolating selection
	const uint64_t IF_RAY_HALF = -(int64_t)(SEL_RAY_HALF == 0);
	const uint64_t IF_DIAGONAL = -( (full_ray == anti_diagonal) | (full_ray == diagonal) );

	uint64_t transposed_ray_mask = 0;

	//the implementation of the ray transformation mechanic varies, whether it's diagonal or nondiagonal
	switch (full_ray)
	{
	case anti_diagonal:
	case diagonal:
		transposed_ray_mask = diagonalTransformation(movement_data, full_ray);
		break;
	default:
		transposed_ray_mask = nonDiagonalTransformation(movement_data, SEL_RAY_ROTATION, full_ray);
		break;
	}

	//ray halving creates a mask that intersects with full ray to isolate the specific direction
	//**1ULL is at first idx. from there there are 63 possible shifts
	//**+1 corrects so that the origin square isn't included, even at idx 0
	const uint64_t IF_UPPER_HALF = IF_RAY_HALF  & ~( (1ULL << (movement_data.picked_square_idx + 1) ) - 1);
	const uint64_t IF_LOWER_HALF = ~IF_RAY_HALF & ( (1ULL << movement_data.picked_square_idx) - 1);
	const uint64_t HALF_MASK = IF_UPPER_HALF | IF_LOWER_HALF;

	return transposed_ray_mask & HALF_MASK; //intersected ray and valid half for the ray, to provide direction
}

uint64_t MoveValidationSystem::diagonalTransformation(const MovementData& movement_data, const uint64_t full_ray)
{
	/*
	* here a diagonal or anti-diagonal ray gets transposed to the origin square
	* both diagonal and anti-diagonal can shift either right or left, and all four approaches
	* require their own logic
	* **wrap-around, doesn't occur because shifting the ray up or down clips the excess bits
	* 
	* ° -> origin
	* / -> diagonal
	* 
	* DIAGONAL (/):
	* /° -> requires subtracting indexed file from rank
	* °/ -> requires subtracting indexed rank from file
	* 
	* ANTI-DIAGONAL (\):
	* **the most intuitive solution I could come up with was by observing the following:
	* one-dimensionally, you subtract total distance by distance of origin,
	* but two-dimensionally, things change:
	* +1 change in in file responds to -1 change in diagonal ray rank, therefore
	* 
	* \° -> sum indexed ranks and files, then subtract by 7 (max distance)
	* °\ -> sum indexed ranks and files, then subtract by 7 (max distance) and correct for negation
	*/

	using enum rayTranspositionInfo::rays;

	const int RANK = (int)(movement_data.picked_square_idx / 8);
	const int FILE = (int)(movement_data.picked_square_idx % 8);
	constexpr int VERTICAL_MOVE = 8;
	constexpr int MAX_DISTANCE_IDX = 7;

	const int OPPOSITE_DISTANCE = (RANK + FILE) - MAX_DISTANCE_IDX;

	const uint64_t IF_SHIFT_1 = -(int64_t)((RANK - FILE) >= 0);		//diagonal left	 - °/
	const uint64_t IF_SHIFT_2 = -(int64_t)((FILE - RANK) >= 0);		//diagonal right - /°
	const uint64_t IF_SHIFT_3 = -(int64_t)(OPPOSITE_DISTANCE >= 0);	//anti-diagonal right - \°
	const uint64_t IF_SHIFT_4 = -(int64_t)(OPPOSITE_DISTANCE < 0);	//anti-diagonal left - °\

	const int SHIFT_1 = (RANK - FILE)		 * VERTICAL_MOVE;
	const int SHIFT_2 = (FILE - RANK)		 * VERTICAL_MOVE;
	const int SHIFT_3 = (OPPOSITE_DISTANCE)  * VERTICAL_MOVE;
	const int SHIFT_4 = (-OPPOSITE_DISTANCE) * VERTICAL_MOVE;  //corrected for sum negation

	const uint64_t IF_DIAGONAL		= -(full_ray == diagonal);
	const uint64_t IF_ANTI_DIAGONAL = -(full_ray == anti_diagonal);

	const uint64_t TRANSPOSED_DIAGONAL_MASK = 
		(IF_DIAGONAL	  & IF_SHIFT_1 & (full_ray << SHIFT_1) ) |
		(IF_DIAGONAL	  & IF_SHIFT_2 & (full_ray >> SHIFT_2) ) |
		(IF_ANTI_DIAGONAL & IF_SHIFT_3 & (full_ray << SHIFT_3) ) |
		(IF_ANTI_DIAGONAL & IF_SHIFT_4 & (full_ray >> SHIFT_4)
	);

	return TRANSPOSED_DIAGONAL_MASK;
}

uint64_t MoveValidationSystem::nonDiagonalTransformation(const MovementData& movement_data, const uint64_t SEL_RAY_ROTATION, const uint64_t full_ray)
{
	/*
	* transposes the ray to the origin of the piece
	* % 8 handles horizontal shifts, used for a vertical ray
	* / 8 handles vertical shifts, used for a horizontal ray
	*/

	constexpr int CHESS_ROW = 8;

	const uint64_t IF_RAY_ROTATION = -(SEL_RAY_ROTATION == 0);

	//determine which ray shift gets used
	const uint64_t X_AXIS_SHIFT = ~IF_RAY_ROTATION & (movement_data.picked_square_idx / CHESS_ROW);
	const uint64_t Y_AXIS_SHIFT = IF_RAY_ROTATION & (movement_data.picked_square_idx % CHESS_ROW);
	const uint64_t TRANSPOSITION_TYPE = X_AXIS_SHIFT | Y_AXIS_SHIFT;

	//move the ray to piece origin
	const uint64_t MOVE_VERTICAL_RAY_MASK = IF_RAY_ROTATION & (full_ray << TRANSPOSITION_TYPE);
	const uint64_t MOVE_HORIZONTAL_RAY_MASK = ~IF_RAY_ROTATION & (full_ray << (TRANSPOSITION_TYPE * CHESS_ROW) );
	
	//select and return the proper transposed ray
	return MOVE_VERTICAL_RAY_MASK | MOVE_HORIZONTAL_RAY_MASK;
}

uint64_t MoveValidationSystem::findBlockersHelper(const MovementData& movement_data, const uint64_t SEL_RAY_HALF, const uint64_t DIRECTIONAL_RAY_MASK, const whichPlayerInfo::playerInfo PLAYER)
{
	/*
	* previous logic applied to halving the ray is applied to blockers as well
	* blockers are found via intersecting enemies and directional ray
	* then, depending on direction the upper or lower half must be formed, by isolating and tailing the blockers
	* this half intersects with the directional ray, to exclude pieces behind the first found blocker
	* enemy blockers have to be inclusive with the isolated bit (they overtake), whereas allied blockers must be exclusive (they don't overtake)
	* **right bit-shift, can overflow, but the result is 0 - 1 = 0xF..F, which is a desired result
	*/

	const uint64_t PLAYER_CHOICE[] = {movement_data.enemies, movement_data.allies};
	const uint64_t BLOCKER_MASK = DIRECTIONAL_RAY_MASK & PLAYER_CHOICE[PLAYER];

	//find isolated bits
	const uint64_t LOWEST_BLOCKER_BIT_MASK = BLOCKER_MASK & (0ULL - BLOCKER_MASK);
	const uint64_t HIGHEST_BLOCKER_BIT_MASK = std::bit_floor(BLOCKER_MASK);

	//tailing isolated bits
	//**inclusion requires a shift, negation reverses the need for a shift
	const uint64_t LOWER_HALF_INC_MASK = (LOWEST_BLOCKER_BIT_MASK << 1) - 1;
	const uint64_t LOWER_HALF_EXC_MASK = LOWEST_BLOCKER_BIT_MASK - 1;
	const uint64_t UPPER_HALF_INC_MASK = ~(HIGHEST_BLOCKER_BIT_MASK - 1);
	const uint64_t UPPER_HALF_EXC_MASK = ~( (HIGHEST_BLOCKER_BIT_MASK << 1) - 1);

	//enemies - ~~, allies - ~
	//determine whether pieces need to be included or excluded
	const uint64_t IF_WHICH_PLAYER = -(PLAYER == 0);
	const uint64_t LOWER_HALF_MASK = (IF_WHICH_PLAYER & LOWER_HALF_INC_MASK) | (~IF_WHICH_PLAYER & LOWER_HALF_EXC_MASK);
	const uint64_t UPPER_HALF_MASK = (IF_WHICH_PLAYER & UPPER_HALF_INC_MASK) | (~IF_WHICH_PLAYER & UPPER_HALF_EXC_MASK);

	const uint64_t IF_RAY_HALF = -(SEL_RAY_HALF == 0);
	const uint64_t HALF_MASK = (IF_RAY_HALF & LOWER_HALF_MASK) | (~IF_RAY_HALF & UPPER_HALF_MASK);

	return HALF_MASK & DIRECTIONAL_RAY_MASK;
}

uint64_t MoveValidationSystem::pawnValidation(const InitGameState::Board& board, MovementData& movement_data)
{
	/*
	* Pawns have their forward advancement movements validated first
	* **Initial double movement requires validating current pawn rank and potential occupancy
	* Followed by attacks and lastly special moves
	*/

	using enum occupancyInfo::occupancy;
	using enum pieceInfo::piece;

	constexpr int RANKS = 8;
	constexpr int INIT_WHITE_PAWNS_RANK = 1;
	constexpr int INIT_BLACK_PAWNS_RANK = 6;

	const uint64_t IF_WHITE = -(int64_t)(movement_data.picked_piece_type == white_pawn);
	const uint64_t IF_BLACK = -(int64_t)(movement_data.picked_piece_type == black_pawn);

	//save the initial pawn rank of the current selected pieces color, 
	//evaluate if the selected pieces rank corresponds to the appropriate color coded initial pawn rank
	const uint64_t INIT_POS_MASK = (IF_WHITE & (uint64_t)INIT_WHITE_PAWNS_RANK) | (IF_BLACK & (uint64_t)INIT_BLACK_PAWNS_RANK);
	const int64_t RANK_EVAL = (movement_data.picked_square_idx / RANKS) == (int64_t)INIT_POS_MASK;
	const uint64_t IF_INIT = -RANK_EVAL;	

	//preventing progression to occupied squares (regular move)
	//**working for both colors
	constexpr uint64_t VERT_ADJUST = 8;

	const uint64_t REGULAR_WHITE_MOVE_MASK = (1ULL << (movement_data.picked_square_idx + VERT_ADJUST) );
	const uint64_t REGULAR_BLACK_MOVE_MASK = ( (1ULL << movement_data.picked_square_idx) >> VERT_ADJUST);
	const uint64_t REGULAR_WHITE_MOVE_BLOCKED_MASK = board.occupancy[all] & REGULAR_WHITE_MOVE_MASK;
	const uint64_t REGULAR_BLACK_MOVE_BLOCKED_MASK = board.occupancy[all] & REGULAR_BLACK_MOVE_MASK;
	const uint64_t REGULAR_BLOCKED_MASK = (IF_WHITE & REGULAR_WHITE_MOVE_BLOCKED_MASK) | (IF_BLACK & REGULAR_BLACK_MOVE_BLOCKED_MASK);
	const uint64_t IF_OCCUPIED_FIRST = -(int64_t)(REGULAR_BLOCKED_MASK != 0);

	//preventing progression to occupied squares (initial double move)
	//**working for both colors
	constexpr uint64_t TWO_SQUARES = 0X101ULL;
	uint64_t double_vert_adjust = 16;

	const uint64_t DOUBLE_WHITE_MOVE_MASK = (TWO_SQUARES << (movement_data.picked_square_idx + VERT_ADJUST) );
	const uint64_t DOUBLE_BLACK_MOVE_MASK = ( (TWO_SQUARES << movement_data.picked_square_idx) >> double_vert_adjust);
	const uint64_t DOUBLE_WHITE_MOVE_BLOCKED_MASK = board.occupancy[all] & DOUBLE_WHITE_MOVE_MASK;
	const uint64_t DOUBLE_BLACK_MOVE_BLOCKED_MASK = board.occupancy[all] & DOUBLE_BLACK_MOVE_MASK;
	const uint64_t DOUBLE_BLOCKED_MASK = (IF_WHITE & DOUBLE_WHITE_MOVE_BLOCKED_MASK) | (IF_BLACK & DOUBLE_BLACK_MOVE_BLOCKED_MASK);
	const uint64_t IF_OCCUPIED_INIT = -(int64_t)(DOUBLE_BLOCKED_MASK != 0);

	const uint64_t ADVANCE_MASK = 
		(IF_INIT  & IF_WHITE & ~IF_OCCUPIED_FIRST & ~IF_OCCUPIED_INIT  & DOUBLE_WHITE_MOVE_MASK)  |
		(~IF_INIT & IF_WHITE & ~IF_OCCUPIED_FIRST & REGULAR_WHITE_MOVE_MASK)					  |
		(IF_INIT  & IF_BLACK & ~IF_OCCUPIED_FIRST & ~IF_OCCUPIED_INIT  & DOUBLE_BLACK_MOVE_MASK)  |
		(~IF_INIT & IF_BLACK & ~IF_OCCUPIED_FIRST & REGULAR_BLACK_MOVE_MASK)
	;

	uint64_t pawn_mask = 0;
	pawn_mask |= ADVANCE_MASK;

	//ATTACKS HANDLING
	constexpr uint64_t NOT_A = 0xfefefefefefefefe;
	constexpr uint64_t NOT_H = 0x7f7f7f7f7f7f7f7f;

	constexpr uint64_t RIGHT_ATTACK = 9;
	constexpr uint64_t LEFT_ATTACK = 7;

	//regular attacks with handled wrap around
	const uint64_t REGULAR_ATTACK_MASK = 
		(IF_WHITE & NOT_H & (board.occupancy[black] & (1ULL << (movement_data.picked_square_idx + LEFT_ATTACK) ) ) )   |
		(IF_WHITE &	NOT_A & (board.occupancy[black] & (1ULL << (movement_data.picked_square_idx + RIGHT_ATTACK) ) ) )  |
		(IF_BLACK & NOT_A & (board.occupancy[white] & ( (1ULL << movement_data.picked_square_idx) >> LEFT_ATTACK) ) )  |
		(IF_BLACK &	NOT_H & (board.occupancy[white] & ( (1ULL << movement_data.picked_square_idx) >> RIGHT_ATTACK) ) )
	;

	pawn_mask |= REGULAR_ATTACK_MASK;

	//PAWN PROMOTION (TO QUEEN)
	constexpr int BACK_RANK_WHITE_SIDE_IDX = 0;
	constexpr int BACK_RANK_BLACK_SIDE_IDX = 7;
	
	const int EVAL_WHITE_PROMOTION = (movement_data.placement_square_idx / RANKS) == BACK_RANK_BLACK_SIDE_IDX;
	const int EVAL_BLACK_PROMOTION = (movement_data.placement_square_idx / RANKS) == BACK_RANK_WHITE_SIDE_IDX;
	
	const uint64_t IF_WHITE_PAWN_PROMOTE = -(int64_t)(IF_WHITE != 0ULL && EVAL_WHITE_PROMOTION);
	const uint64_t IF_BLACK_PAWN_PROMOTE = -(int64_t)(IF_BLACK != 0ULL && EVAL_BLACK_PROMOTION);

	movement_data.promoted_piece_type = (int) (
		(IF_WHITE_PAWN_PROMOTE & white_queen) |
		(IF_BLACK_PAWN_PROMOTE & black_queen)
	);

	/* 
	* EN-PASSANT
	* enemy must make the move to your properly situated pawn
	* it must be an initial double move
	* only legal for a specified pawn at that specific turn
	* **conditions for en-passant determined in board state
	*/

	constexpr int SIDE = 1;
	const uint64_t PICKED_BIT_MASK = 1ULL << movement_data.picked_square_idx;

	constexpr int UP_LEFT    = 7; 
	constexpr int UP_RIGHT   = 9;
	constexpr int DOWN_LEFT  = 9;
	constexpr int DOWN_RIGHT = 7;

	uint64_t IF_BLACK_PAWN_LEFT   = -(int64_t)( ( (PICKED_BIT_MASK >> SIDE) & board.pieces[black_pawn] ) > 0 );
	uint64_t IF_BLACK_PAWN_RIGHT  = -(int64_t)( ( (PICKED_BIT_MASK << SIDE) & board.pieces[black_pawn] ) > 0 );
	uint64_t IF_WHITE_PAWN_RIGHT  = -(int64_t)( ( (PICKED_BIT_MASK << SIDE) & board.pieces[white_pawn] ) > 0 );
	uint64_t IF_WHITE_PAWN_LEFT   = -(int64_t)( ( (PICKED_BIT_MASK >> SIDE) & board.pieces[white_pawn] ) > 0 );

	const uint64_t EN_PASSANT_MASK =

		(movement_data.passant_mask & IF_BLACK_PAWN_LEFT  & (PICKED_BIT_MASK << UP_LEFT) )    |
		(movement_data.passant_mask & IF_BLACK_PAWN_RIGHT & (PICKED_BIT_MASK << UP_RIGHT) )   |
		(movement_data.passant_mask & IF_WHITE_PAWN_LEFT  & (PICKED_BIT_MASK >> DOWN_LEFT) )  |
		(movement_data.passant_mask & IF_WHITE_PAWN_RIGHT & (PICKED_BIT_MASK >> DOWN_RIGHT) )
	;
	pawn_mask |= EN_PASSANT_MASK;

	//flag that passant has been used for the upcoming board update
	movement_data.passant_used = (EN_PASSANT_MASK > 0);

	//update board state
	return pawn_mask;
}

uint64_t MoveValidationSystem::knightValidation(const InitGameState::Board&, MovementData& movement_data)
{
	uint64_t knight_mask = 0;

	constexpr uint64_t NOT_A_MASK = 0xfefefefefefefefe;
	constexpr uint64_t NOT_B_MASK = 0xfdfdfdfdfdfdfdfd;
	constexpr uint64_t NOT_G_MASK = 0xbfbfbfbfbfbfbfbf;
	constexpr uint64_t NOT_H_MASK = 0x7f7f7f7f7f7f7f7f;

	const uint64_t PICKED_BIT_MASK = (1ULL << movement_data.picked_square_idx);

	constexpr int UP_RIGHT = 17;
	constexpr int RIGHT_UP = 10;
	constexpr int RIGHT_DOWN = 6;
	constexpr int DOWN_RIGHT = 15;

	knight_mask |= (PICKED_BIT_MASK & NOT_H_MASK)			   << UP_RIGHT   & ~movement_data.allies;
	knight_mask |= (PICKED_BIT_MASK & NOT_G_MASK & NOT_H_MASK) << RIGHT_UP   & ~movement_data.allies;
	knight_mask |= (PICKED_BIT_MASK & NOT_G_MASK & NOT_H_MASK) >> RIGHT_DOWN & ~movement_data.allies;
	knight_mask |= (PICKED_BIT_MASK & NOT_H_MASK)			   >> DOWN_RIGHT & ~movement_data.allies;
		
	constexpr int DOWN_LEFT = 17;
	constexpr int LEFT_DOWN = 10;
	constexpr int LEFT_UP = 6;
	constexpr int UP_LEFT = 15;

	knight_mask |= (PICKED_BIT_MASK & NOT_A_MASK)			   >> DOWN_LEFT & ~movement_data.allies;
	knight_mask |= (PICKED_BIT_MASK & NOT_B_MASK & NOT_A_MASK) >> LEFT_DOWN & ~movement_data.allies;
	knight_mask |= (PICKED_BIT_MASK & NOT_B_MASK & NOT_A_MASK) << LEFT_UP   & ~movement_data.allies;
	knight_mask |= (PICKED_BIT_MASK & NOT_A_MASK)			   << UP_LEFT   & ~movement_data.allies;

	return knight_mask;
}

uint64_t MoveValidationSystem::rookValidation(const InitGameState::Board&, MovementData& movement_data)
{
	using enum rayTranspositionInfo::rays;
	using enum rayDirectionInfo::rays;

	uint64_t rook_mask = 0;

	rook_mask |= universalRay(movement_data, north, vertical);	//north
	rook_mask |= universalRay(movement_data, east, horizontal);	//east
	rook_mask |= universalRay(movement_data, south, vertical);	//south
	rook_mask |= universalRay(movement_data, west, horizontal);	//west

	return rook_mask;
}

uint64_t MoveValidationSystem::bishopValidation(const InitGameState::Board&, MovementData& movement_data)
{
	using enum rayTranspositionInfo::rays;
	using enum rayDirectionInfo::rays;

	uint64_t bishop_mask = 0;

	bishop_mask |= universalRay(movement_data, north_east, diagonal);		//north-east
	bishop_mask |= universalRay(movement_data, south_east, anti_diagonal);	//south-east
	bishop_mask |= universalRay(movement_data, south_west, diagonal);		//south-west
	bishop_mask |= universalRay(movement_data, north_west, anti_diagonal);	//north-west

	return bishop_mask;
}

uint64_t MoveValidationSystem::queenValidation(const InitGameState::Board&, MovementData& movement_data)
{
	using enum rayTranspositionInfo::rays;
	using enum rayDirectionInfo::rays;

	uint64_t queen_mask = 0;

	queen_mask |= universalRay(movement_data, north, vertical);				//north
	queen_mask |= universalRay(movement_data, north_east, diagonal);		//north-east
	queen_mask |= universalRay(movement_data, east, horizontal);			//east
	queen_mask |= universalRay(movement_data, south_east, anti_diagonal);	//south-east
	queen_mask |= universalRay(movement_data, south, vertical);				//south
	queen_mask |= universalRay(movement_data, south_west, diagonal);		//south-west
	queen_mask |= universalRay(movement_data, west, horizontal);			//west
	queen_mask |= universalRay(movement_data, north_west, anti_diagonal);	//north-west

	return queen_mask;
}

uint64_t MoveValidationSystem::kingValidation(const InitGameState::Board& board, MovementData& movement_data)
{
	using enum pieceInfo::piece;
	using enum kingMoveIndicies::move;

	constexpr int UP = 8;
	constexpr int DOWN = -8;
	constexpr int RIGHT = 1;
	constexpr int LEFT = -1;
	
	constexpr int UP_RIGHT = 9;
	constexpr int UP_LEFT = 7;
	constexpr int DOWN_LEFT = -9;
	constexpr int DOWN_RIGHT = -7;

	constexpr int ALL_SHIFT_DIRS[] = 
	{
		UP, DOWN, RIGHT, LEFT,
		UP_RIGHT, UP_LEFT, DOWN_LEFT, DOWN_RIGHT
	};

	const uint64_t PICKED_BIT_MASK = 1ULL << movement_data.picked_square_idx;

	constexpr uint64_t NOT_A_FILE = ~0x1010101010101010;
	constexpr uint64_t NOT_H_FILE = ~0x8080808080808080;

	const uint64_t CLIPPED_MOVED_BITS_MASK[] =
	{
		(PICKED_BIT_MASK << UP),
		(PICKED_BIT_MASK >> -DOWN),
		(PICKED_BIT_MASK << RIGHT)	     & NOT_A_FILE,
		(PICKED_BIT_MASK >> -LEFT)		 & NOT_H_FILE,

		(PICKED_BIT_MASK << UP_RIGHT)    & NOT_A_FILE,
		(PICKED_BIT_MASK << UP_LEFT)     & NOT_H_FILE,
		(PICKED_BIT_MASK >> -DOWN_LEFT)  & NOT_H_FILE,
		(PICKED_BIT_MASK >> -DOWN_RIGHT) & NOT_A_FILE
	};
	
	/*
	 * here a geometric series sum is used to compute a 3x3 buffer on the spot,
	 * this buffer is responsible for preventing two enemy kings moving to adjacent squares
	 * this calculation has to run 8 times to construct masks for all possible directions the king can be moved to.
	 * shifting the already computed mask causes issues with
	 ** the mathematical formula was written by AI. Eventually I'll learn more about deriving mathematical series on my own,
	 ** but for now this works, since I understand the reasoning for why I choose this approach.
	*/
	
	constexpr int ALL_DIRECTIONS = 8;
	uint64_t ENEMY_KING_ADJACENT[8] = {};
	
	const uint64_t ENEMY_KING = movement_data.enemies & (board.pieces[white_king] | board.pieces[black_king]);
	
	constexpr uint64_t H_FILE_MASK = 0x8080808080808080;
	constexpr uint64_t A_FILE_MASK = 0x101010101010101;
	const uint64_t IF_A_FILE_OVERLAP = -( (A_FILE_MASK & (1ULL << movement_data.placement_square_idx) ) > 0);
	const uint64_t IF_H_FILE_OVERLAP = -( (H_FILE_MASK & (1ULL << movement_data.placement_square_idx) ) > 0);

	//start of the mathematical formula for a 3x3 king buffer, adjusted by placement and direction
	const int TARGET_SQUARE_IDX = movement_data.picked_square_idx - 9;

	for (int i = 0; i < ALL_DIRECTIONS; i++) 
	{
		int DIRECTED_SQUARE_IDX = TARGET_SQUARE_IDX + ALL_SHIFT_DIRS[i];

		const uint64_t CRUDE_KING_BUFFER_MASK = (TARGET_SQUARE_IDX >= 0) ?
			(  7ULL <<  DIRECTED_SQUARE_IDX       ) * 65793ULL			 : //prevents upper bound mask from clipping
			( (7ULL << (DIRECTED_SQUARE_IDX += 8) ) * 65793ULL) >> 17;	   //prevents lower bound mask from clipping
		
		const uint64_t KING_BUFFER_MASK = (IF_A_FILE_OVERLAP  & ~H_FILE_MASK	   & CRUDE_KING_BUFFER_MASK) |
									   	  (IF_H_FILE_OVERLAP  & ~A_FILE_MASK	   & CRUDE_KING_BUFFER_MASK) |
										  (~IF_A_FILE_OVERLAP & ~IF_H_FILE_OVERLAP & CRUDE_KING_BUFFER_MASK)
		;

		//append conditional mask to static array
		ENEMY_KING_ADJACENT[i] = -( (KING_BUFFER_MASK & ENEMY_KING) > 0);
	};

	//possible moves, no enemy king in sight, no overtaking allies
	uint64_t king_mask = 0;
	king_mask |= CLIPPED_MOVED_BITS_MASK[up]		 & ~ENEMY_KING_ADJACENT[up]			& ~movement_data.allies;
	king_mask |= CLIPPED_MOVED_BITS_MASK[down]		 & ~ENEMY_KING_ADJACENT[down]		& ~movement_data.allies;
	king_mask |= CLIPPED_MOVED_BITS_MASK[right]		 & ~ENEMY_KING_ADJACENT[right]		& ~movement_data.allies;
	king_mask |= CLIPPED_MOVED_BITS_MASK[left]		 & ~ENEMY_KING_ADJACENT[left]		& ~movement_data.allies;

	king_mask |= CLIPPED_MOVED_BITS_MASK[up_right]   & ~ENEMY_KING_ADJACENT[up_right]	& ~movement_data.allies;
	king_mask |= CLIPPED_MOVED_BITS_MASK[up_left]    & ~ENEMY_KING_ADJACENT[up_left]	& ~movement_data.allies;
	king_mask |= CLIPPED_MOVED_BITS_MASK[down_left]  & ~ENEMY_KING_ADJACENT[down_left]  & ~movement_data.allies;
	king_mask |= CLIPPED_MOVED_BITS_MASK[down_right] & ~ENEMY_KING_ADJACENT[down_right] & ~movement_data.allies;

	return king_mask;
}

uint64_t MoveValidationSystem::validator(const InitGameState::Board& board, MovementData& movement_data)
{
	//this is how you define the type of the jump table. it's defining the type of the function pointer
	//uint64_t specifies the return value (can be empty if void), (*) is the placeholder of the function pointer
	//(here it's only * because the access system is being handled via the "using" keyword),
	//and the second parentheses contains function parameters, that have to be uniform for the jump table to work
	//for the sake of determinism
	using a_validator = uint64_t(*)(const InitGameState::Board&, MovementData&);

	//array of function pointers
	//static keyword in front of a data type enforces this table belongs only to this source file
	//this will prevent naming conflicts when the linker joins the code, if the same name exists in another file
	//the function will also be written inside the data segment, preventing rebuilding of the code and thus
	//improving execution time
	static a_validator jump_table[] = {
		&MoveValidationSystem::pawnValidation,
		&MoveValidationSystem::knightValidation,
		&MoveValidationSystem::rookValidation,
		&MoveValidationSystem::bishopValidation,
		&MoveValidationSystem::queenValidation,
		&MoveValidationSystem::kingValidation
	};

	constexpr int COLOR_CORRECTION = 6;
	constexpr int FIRST_PIECE = 0;
	constexpr int LAST_PIECE = 12;
	uint64_t valid_moves = 0;

	if (movement_data.picked_piece_type < LAST_PIECE && movement_data.picked_piece_type >= FIRST_PIECE) {
		//accesses the function pointer via index, then execute it with the provided arguments
		//modulo six accounts for figure color correction, as their legal moves don't differ based on color
		///however I'm counting them separately as this fulfills another architectural purpose
		valid_moves = jump_table[movement_data.picked_piece_type % COLOR_CORRECTION](board, movement_data);
	}

	return valid_moves;
}
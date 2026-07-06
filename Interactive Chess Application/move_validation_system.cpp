#include "move_validation_system.h"

uint64_t MoveValidationSystem::universalRay(MovementData& movement_data, const uint64_t direction_bitfield, const uint64_t full_ray)
{
	using enum whichPlayerInfo::playerInfo;

	const uint64_t SEL_RAY_HALF		= -(int64_t)(direction_bitfield & 0x01); //0 - lsb, 1 - msb
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

uint64_t MoveValidationSystem::rayHalvingHelper(MovementData& movement_data, const uint64_t SEL_RAY_HALF, const uint64_t SEL_RAY_ROTATION, const uint64_t full_ray)
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
	if (full_ray == anti_diagonal || full_ray == diagonal) 
	{ 
		transposed_ray_mask = diagonalTransformation(movement_data, full_ray);
	}
	else 
	{
		transposed_ray_mask = nonDiagonalTransformation(movement_data, SEL_RAY_ROTATION, full_ray);
	}

	//ray halving creates a mask that intersects with full ray to isolate the specific direction
	//**1ULL is at first idx. from there there are 63 possible shifts
	//**+1 corrects so that the origin square isn't included, even at idx 0
	const uint64_t IF_UPPER_HALF = IF_RAY_HALF & ~( (1ULL << (movement_data.picked_square_idx + 1) ) - 1);
	const uint64_t IF_LOWER_HALF = ~IF_RAY_HALF & ( (1ULL << movement_data.picked_square_idx) - 1);
	const uint64_t HALF_MASK = IF_UPPER_HALF | IF_LOWER_HALF;

	return transposed_ray_mask & HALF_MASK; //intersected ray and valid half for the ray, to provide direction
}

uint64_t MoveValidationSystem::diagonalTransformation(MovementData& movement_data, const uint64_t full_ray)
{
	/*
	* here a diagonal or anti-diagonal ray gets transposed to the origin square
	* both diagonal and anti-diagonal can shift either right or left, and all four approaches
	* require their own logic
	* **wrap-around, doesn't occur because shifting the ray up or down clips the excess bits
	* 
	* °  -> origin
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

uint64_t MoveValidationSystem::nonDiagonalTransformation(MovementData& movement_data, const uint64_t SEL_RAY_ROTATION, const uint64_t full_ray)
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

uint64_t MoveValidationSystem::findBlockersHelper(MovementData& movement_data, const uint64_t SEL_RAY_HALF, const uint64_t DIRECTIONAL_RAY_MASK, const whichPlayerInfo::playerInfo PLAYER)
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
	const uint64_t UPPER_HALF_EXC_MASK = ~((HIGHEST_BLOCKER_BIT_MASK << 1) - 1);

	//enemies - ~~, allies - ~
	//determine whether pieces need to be included or excluded
	const uint64_t IF_WHICH_PLAYER = -(PLAYER == 0);
	const uint64_t LOWER_HALF_MASK = (IF_WHICH_PLAYER & LOWER_HALF_INC_MASK) | (~IF_WHICH_PLAYER & LOWER_HALF_EXC_MASK);
	const uint64_t UPPER_HALF_MASK = (IF_WHICH_PLAYER & UPPER_HALF_INC_MASK) | (~IF_WHICH_PLAYER & UPPER_HALF_EXC_MASK);

	const uint64_t IF_RAY_HALF = -(SEL_RAY_HALF == 0);
	const uint64_t HALF_MASK = (IF_RAY_HALF & LOWER_HALF_MASK) | (~IF_RAY_HALF & UPPER_HALF_MASK);

	return HALF_MASK & DIRECTIONAL_RAY_MASK;
}

uint64_t MoveValidationSystem::pawnValidation(InitGameState::Board& board, MovementData& movement_data)
{
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
	uint64_t IF_INIT = -RANK_EVAL;	

	//preventing progression to occupied squares (regular move)
	//**working for both colors
	constexpr uint64_t VERT_ADJUST = 8;

	const uint64_t REGULAR_WHITE_MOVE_MASK = (1ULL << (movement_data.picked_square_idx + VERT_ADJUST) );
	const uint64_t REGULAR_BLACK_MOVE_MASK = ((1ULL << movement_data.picked_square_idx) >> VERT_ADJUST);
	const uint64_t REGULAR_WHITE_MOVE_BLOCKED_MASK = board.occupancy[all] & REGULAR_WHITE_MOVE_MASK;
	const uint64_t REGULAR_BLACK_MOVE_BLOCKED_MASK = board.occupancy[all] & REGULAR_BLACK_MOVE_MASK;
	const uint64_t REGULAR_BLOCKED_MASK = (IF_WHITE & REGULAR_WHITE_MOVE_BLOCKED_MASK) | (IF_BLACK & REGULAR_BLACK_MOVE_BLOCKED_MASK);
	const uint64_t IF_OCCUPIED_FIRST = -(int64_t)(REGULAR_BLOCKED_MASK != 0);

	//preventing progression to occupied squares (initial double move)
	//**working for both colors
	constexpr uint64_t TWO_SQUARES = 0X101ULL;
	constexpr uint64_t DOUBLE_VERT_ADJUST = 16;

	const uint64_t DOUBLE_WHITE_MOVE_MASK = (TWO_SQUARES << (movement_data.picked_square_idx + VERT_ADJUST) );
	const uint64_t DOUBLE_BLACK_MOVE_MASK = ( (TWO_SQUARES << movement_data.picked_square_idx) >> DOUBLE_VERT_ADJUST);
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
		(IF_BLACK & NOT_A & (board.occupancy[white] & ((1ULL << movement_data.picked_square_idx) >> LEFT_ATTACK) ) )  |
		(IF_BLACK &	NOT_H & (board.occupancy[white] & ((1ULL << movement_data.picked_square_idx) >> RIGHT_ATTACK) ) )
	;

	pawn_mask |= REGULAR_ATTACK_MASK;

	//pawn promotion
	/*constexpr int BACK_RANK_WHITE_SIDE = 0;
	constexpr int BACK_RANK_BLACK_SIDE = 7;
	
	const int EVAL_WHITE_PROMOTION = (movement_data.placement_square_idx / RANKS) == BACK_RANK_BLACK_SIDE;
	const int EVAL_BLACK_PROMOTION = (movement_data.placement_square_idx / RANKS) == BACK_RANK_WHITE_SIDE;
	
	const uint64_t IF_WHITE_PAWN_PROMOTE = -(int64_t)(IF_WHITE != 0ULL && EVAL_WHITE_PROMOTION);
	const uint64_t IF_BLACK_PAWN_PROMOTE = -(int64_t)(IF_BLACK != 0ULL && EVAL_BLACK_PROMOTION);

	if ( (IF_WHITE_PAWN_PROMOTE | IF_BLACK_PAWN_PROMOTE) != 0ULL) 
	{
		movement_data.picked_piece_type = (int)( (IF_WHITE_PAWN_PROMOTE & white_queen) | (IF_BLACK_PAWN_PROMOTE & black_queen) );
	}*/

	//en-passant


	return pawn_mask;
}

//following functions have a board argument, because pawn validation required it, and all functions need same
//parameters for the jump table to execute
uint64_t MoveValidationSystem::knightValidation(InitGameState::Board&, MovementData& movement_data)
{
	uint64_t knight_mask = 0;

	constexpr uint64_t NOT_A_MASK = 0xfefefefefefefefe;
	constexpr uint64_t NOT_B_MASK = 0xfdfdfdfdfdfdfdfd;
	constexpr uint64_t NOT_G_MASK = 0xbfbfbfbfbfbfbfbf;
	constexpr uint64_t NOT_H_MASK = 0x7f7f7f7f7f7f7f7f;

	constexpr int UP_RIGHT = 17;
	constexpr int RIGHT_UP = 10;
	constexpr int RIGHT_DOWN = 6;
	constexpr int DOWN_RIGHT = 15;

	knight_mask |= ((1ULL << movement_data.picked_square_idx) & NOT_H_MASK) << UP_RIGHT & ~movement_data.allies;
	knight_mask |= ((1ULL << movement_data.picked_square_idx) & NOT_G_MASK & NOT_H_MASK) << RIGHT_UP & ~movement_data.allies;
	knight_mask |= ((1ULL << movement_data.picked_square_idx) & NOT_G_MASK & NOT_H_MASK) >> RIGHT_DOWN & ~movement_data.allies;
	knight_mask |= ((1ULL << movement_data.picked_square_idx) & NOT_H_MASK) >> DOWN_RIGHT & ~movement_data.allies;

	constexpr int DOWN_LEFT = 17;
	constexpr int LEFT_DOWN = 10;
	constexpr int LEFT_UP = 6;
	constexpr int UP_LEFT = 15;

	knight_mask |= ((1ULL << movement_data.picked_square_idx) & NOT_A_MASK) >> DOWN_LEFT & ~movement_data.allies;
	knight_mask |= ((1ULL << movement_data.picked_square_idx) & NOT_B_MASK & NOT_A_MASK) >> LEFT_DOWN & ~movement_data.allies;
	knight_mask |= ((1ULL << movement_data.picked_square_idx) & NOT_B_MASK & NOT_A_MASK) << LEFT_UP & ~movement_data.allies;
	knight_mask |= ((1ULL << movement_data.picked_square_idx) & NOT_A_MASK) << UP_LEFT & ~movement_data.allies;

	return knight_mask;
}

uint64_t MoveValidationSystem::rookValidation(InitGameState::Board&, MovementData& movement_data)
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

uint64_t MoveValidationSystem::bishopValidation(InitGameState::Board&, MovementData& movement_data)
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

uint64_t MoveValidationSystem::queenValidation(InitGameState::Board&, MovementData& movement_data)
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

uint64_t MoveValidationSystem::kingValidation(InitGameState::Board&, MovementData& movement_data)
{
	uint64_t king_mask = 0;

	constexpr uint64_t NOT_A_FILE = ~0x1010101010101010;
	constexpr uint64_t NOT_H_FILE = ~0x8080808080808080;

	king_mask |= ((1ULL << movement_data.picked_square_idx) << 8) & ~movement_data.allies;
	king_mask |= ((1ULL << movement_data.picked_square_idx) >> 8) & ~movement_data.allies;
	king_mask |= ((1ULL << movement_data.picked_square_idx) << 1) & NOT_A_FILE & ~movement_data.allies;
	king_mask |= ((1ULL << movement_data.picked_square_idx) >> 1) & NOT_H_FILE & ~movement_data.allies;

	king_mask |= (((1ULL << movement_data.picked_square_idx) << 9) & NOT_A_FILE) & ~movement_data.allies;
	king_mask |= (((1ULL << movement_data.picked_square_idx) << 7) & NOT_H_FILE) & ~movement_data.allies;
	king_mask |= (((1ULL << movement_data.picked_square_idx) >> 9) & NOT_H_FILE) & ~movement_data.allies;
	king_mask |= (((1ULL << movement_data.picked_square_idx) >> 7) & NOT_A_FILE) & ~movement_data.allies;

	return king_mask;
}

uint64_t MoveValidationSystem::validator(InitGameState::Board& board, MovementData& movement_data)
{
	//this is how you define the type of the jump table. it's defining the type of the function pointer
	//uint64_t specifies the return value (can be empty if void), (*) is the placeholder of the function pointer
	//(here it's only * because the access system is being handled via the "using" keyword),
	//and the second parentheses contains function parameters, that have to be uniform for the jump table to work
	//for the sake of determinism
	using a_validator = uint64_t(*)(InitGameState::Board&, MovementData&);

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

	uint64_t valid_moves = 0;

	if (movement_data.picked_piece_type < 12 && movement_data.picked_piece_type >= 0) {
		//accesses the function pointer via index, then execute it with the provided arguments
		//modulo six accounts for figure color correction, as their legal moves don't differ based on color
		///however I'm counting them separately as this fulfills another architectural purpose
		valid_moves = jump_table[movement_data.picked_piece_type % 6](board, movement_data);
	}

	return valid_moves;
}
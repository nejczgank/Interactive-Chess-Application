#include "move_validation_system.h"
#include "occupancy_info.h"

uint64_t MoveValidationSystem::universalRay(MovementData& movement_data, uint64_t direction_bitfield, uint64_t full_ray)
{
	uint64_t significant_bit_truth_mask = -(int64_t)(direction_bitfield & 0x01);
	uint64_t transposition_truth_mask = -(int64_t)((direction_bitfield >> 1) & 0x01);

	//find the appropriate half ray
	uint64_t halved_ray = rayHalvingHelper(movement_data, &significant_bit_truth_mask, &transposition_truth_mask, &full_ray);
	//adjust ray for enemy blockers
	uint64_t ray_to_enemy_blockers = findEnemyBlockersHelper(movement_data, &significant_bit_truth_mask, &halved_ray);
	//adjust ray for ally blockers
	uint64_t ray_to_ally_blockers = findAllyBlockersHelper(movement_data, &significant_bit_truth_mask, &halved_ray);

	// if blockers are both empty then result is halved ray
	// if only one blocker exit than that blocker is the result
	// if both blockers exist then result is the combination of the two
	uint64_t if_ray_to_enemy_blockers = -(ray_to_enemy_blockers != 0);
	uint64_t if_ray_to_ally_blockers = -(ray_to_ally_blockers != 0);
	uint64_t if_only_one_blocker_exists = if_ray_to_enemy_blockers ^ if_ray_to_ally_blockers;
	uint64_t if_both_blockers_exist = if_ray_to_enemy_blockers & if_ray_to_ally_blockers;
	uint64_t if_no_path_forward = -(((halved_ray & movement_data.allies) != 0) && if_ray_to_ally_blockers == 0);

	uint64_t universal_ray = 
		(if_only_one_blocker_exists & ~if_no_path_forward & ray_to_enemy_blockers) |
		(if_only_one_blocker_exists & ~if_no_path_forward & ray_to_ally_blockers)  |
		(if_both_blockers_exist & (ray_to_enemy_blockers & ray_to_ally_blockers))  |
		(~if_both_blockers_exist & ~if_only_one_blocker_exists & ~if_no_path_forward & halved_ray); //added ~if_no_path_forward

	return universal_ray;
}

uint64_t MoveValidationSystem::rayHalvingHelper(MovementData& movement_data, uint64_t* significant_bit_truth_mask, uint64_t* transposition_truth_mask, uint64_t* full_ray)
{
	uint64_t if_significant_bit_mask = -(int64_t)(*significant_bit_truth_mask == 0);
	uint64_t if_diagonal = -((*full_ray == 0x102040810204080) | (*full_ray == 0x8040201008040201)); //defaults to true?

	uint64_t determined_transposed_ray = 0;
	//replace this with a switch, if, lookup table maybe? just don't let it execute two functions like that
	if (*full_ray == 0x102040810204080 || *full_ray == 0x8040201008040201) {
		determined_transposed_ray = diagonalTransformation(movement_data, full_ray);
	}
	else {
		determined_transposed_ray = nonDiagonalTransformation(movement_data, transposition_truth_mask, full_ray);
	}

	uint64_t determined_halving = (if_significant_bit_mask & ~((1ULL << (movement_data.picked_square_idx + 1)) - 1)) | (~if_significant_bit_mask & ((1ULL << movement_data.picked_square_idx) - 1));

	return determined_transposed_ray & determined_halving;
}

uint64_t MoveValidationSystem::diagonalTransformation(MovementData& movement_data, uint64_t* full_ray)
{
	uint64_t if_right_starting_diagonal = -(*full_ray == 0x102040810204080);
	uint64_t if_left_starting_diagonal = -(*full_ray == 0x8040201008040201);

	int rank = (int)(movement_data.picked_square_idx / 8);
	int file = (int)(movement_data.picked_square_idx % 8);

	uint64_t if_shift_1 = -((rank - file) >= 0);
	uint64_t if_shift_2 = -((file - rank) >= 0);

	int opposite_distance = (rank + file) - 7;

	uint64_t if_shift_3 = -(opposite_distance >= 0);
	uint64_t if_shift_4 = -(opposite_distance < 0);

	int safe_shift_1 = (rank - file) * 8;
	int safe_shift_2 = (file - rank) * 8;
	int safe_shift_3 = (opposite_distance) * 8;
	int safe_shift_4 = (-opposite_distance) * 8;

	uint64_t determined_transposing = (if_left_starting_diagonal & if_shift_1 & (*full_ray << safe_shift_1)) |
		(if_left_starting_diagonal & if_shift_2 & (*full_ray >> safe_shift_2)) |
		(if_right_starting_diagonal & if_shift_3 & (*full_ray << safe_shift_3)) |
		(if_right_starting_diagonal & if_shift_4 & (*full_ray >> safe_shift_4)
	);

	return determined_transposing;
}

uint64_t MoveValidationSystem::nonDiagonalTransformation(MovementData& movement_data, uint64_t* transposition_truth_mask, uint64_t* full_ray)
{
	uint64_t if_transposition_mask = -(*transposition_truth_mask == 0);
	uint64_t determined_transposing = (if_transposition_mask & (movement_data.picked_square_idx % 8)) | (~if_transposition_mask & (movement_data.picked_square_idx / 8));
	uint64_t transposed_ray = (if_transposition_mask & (*full_ray << determined_transposing)) | (~if_transposition_mask & (*full_ray << (determined_transposing * 8)));

	return transposed_ray;
}

uint64_t MoveValidationSystem::findEnemyBlockersHelper(MovementData& movement_data, uint64_t* significant_bit_truth_mask, uint64_t* halved_ray)
{
	uint64_t found_blockers = *halved_ray & movement_data.enemies;
	uint64_t if_significant_bit_mask = -(*significant_bit_truth_mask == 0);
	uint64_t determined_halving = (if_significant_bit_mask & (((found_blockers & (0ULL - found_blockers)) - 1) | (found_blockers & (0ULL - found_blockers)))) | (~if_significant_bit_mask & ~(std::bit_floor(found_blockers) - 1));

	return determined_halving & *halved_ray;
}

uint64_t MoveValidationSystem::findAllyBlockersHelper(MovementData& movement_data, uint64_t* significant_bit_truth_mask, uint64_t* halved_ray)
{

	uint64_t found_blockers = *halved_ray & movement_data.allies;
	uint64_t if_significant_bit_mask = -(*significant_bit_truth_mask == 0);
	uint64_t determined_halving = (if_significant_bit_mask & (((found_blockers & (0ULL - found_blockers)) - 1))) | (~if_significant_bit_mask & ~((std::bit_floor(found_blockers) - 1) | std::bit_floor(found_blockers)));

	return determined_halving & *halved_ray;
}

uint64_t MoveValidationSystem::pawnValidation(InitGameState::Board& board, MovementData& movement_data)
{
	uint64_t pawnMask = 0;
	uint64_t if_white = 0;
	uint64_t if_black = 0;

	if (movement_data.picked_piece_type == 0) { if_white = ~0ULL; }
	if (movement_data.picked_piece_type == 6) { if_black = ~0ULL; }

	uint64_t determine_init_pos = (if_white & 0x1) | (if_black & 0x6);
	uint64_t test = movement_data.picked_square_idx / 8;
	uint64_t if_init = -(int64_t)((movement_data.picked_square_idx / 8) == determine_init_pos);

	//preventing progression to occupied squares (regular move)
	uint64_t if_occupied_first = -(int64_t)((if_white & (board.occupancy[occupancyInfo::all] & (1ULL << (movement_data.picked_square_idx + 8)))) | (if_black & (board.occupancy[occupancyInfo::all] & ((1ULL << movement_data.picked_square_idx) >> 8))));
	//if_occupied_second shifts two vertical bits, meaning the determine_regular_move doesn't require extra if_occupied_first check
	uint64_t if_occupied_second = -(int64_t)((if_white & (board.occupancy[occupancyInfo::all] & (0x101ULL << (movement_data.picked_square_idx + 8)))) | (if_black & (board.occupancy[occupancyInfo::all] & ((0x101ULL << movement_data.picked_square_idx) >> 16))));

	uint64_t determine_regular_move = (if_init & if_white & ~if_occupied_second & (0x101ULL << (movement_data.picked_square_idx + 8)))   |
									  (~if_init & if_white & ~if_occupied_first & (1ULL << (movement_data.picked_square_idx + 8)))       |
									  (if_init & if_black & ~if_occupied_second & ((0x101ULL << movement_data.picked_square_idx) >> 16)) |
									  (~if_init & if_black & ~if_occupied_first & ((1ULL << movement_data.picked_square_idx) >> 8))
	;

	pawnMask |= determine_regular_move;

	//handling wrap around for attacks
	uint64_t not_a = 0xfefefefefefefefe;
	uint64_t not_h = 0x7f7f7f7f7f7f7f7f;

	uint64_t determine_atk = (if_white & not_h & (board.occupancy[occupancyInfo::black] & (1ULL << (movement_data.picked_square_idx + 7)))) |
							 (if_white & (board.occupancy[occupancyInfo::black] & (1ULL << (movement_data.picked_square_idx + 9))) |
							 (if_black & not_a & (board.occupancy[occupancyInfo::white] & ((1ULL << movement_data.picked_square_idx) >> 7))) |
							 (if_black & (board.occupancy[occupancyInfo::white] & (1ULL << movement_data.picked_square_idx) >> 9)))
	;

	pawnMask |= determine_atk;

	//en-passant
	//pawn promotion

	return pawnMask;
}

//following functions have a board argument, because pawn validation required it, and all functions need same
//parameters for the jump table to execute
uint64_t MoveValidationSystem::knightValidation(InitGameState::Board&, MovementData& movement_data)
{
	uint64_t knight_mask = 0;

	uint64_t not_a = 0xfefefefefefefefe;
	uint64_t not_b = 0xfdfdfdfdfdfdfdfd;
	uint64_t not_g = 0xbfbfbfbfbfbfbfbf;
	uint64_t not_h = 0x7f7f7f7f7f7f7f7f;

	knight_mask |= ((1ULL << movement_data.picked_square_idx) & not_h) << 17 & ~movement_data.allies;
	knight_mask |= ((1ULL << movement_data.picked_square_idx) & not_g & not_h) << 10 & ~movement_data.allies;
	knight_mask |= ((1ULL << movement_data.picked_square_idx) & not_g & not_h) >> 6 & ~movement_data.allies;
	knight_mask |= ((1ULL << movement_data.picked_square_idx) & not_h) >> 15 & ~movement_data.allies;

	knight_mask |= ((1ULL << movement_data.picked_square_idx) & not_a) >> 17 & ~movement_data.allies;
	knight_mask |= ((1ULL << movement_data.picked_square_idx) & not_b & not_a) >> 10 & ~movement_data.allies;
	knight_mask |= ((1ULL << movement_data.picked_square_idx) & not_b & not_a) << 6 & ~movement_data.allies;
	knight_mask |= ((1ULL << movement_data.picked_square_idx) & not_a) << 15 & ~movement_data.allies;

	return knight_mask;
}

uint64_t MoveValidationSystem::rookValidation(InitGameState::Board&, MovementData& movement_data)
{
	uint64_t rook_mask = 0;

	rook_mask |= universalRay(movement_data, 0x0, 0x101010101010101); //north
	rook_mask |= universalRay(movement_data, 0x2, 0xff);			   //east
	rook_mask |= universalRay(movement_data, 0x1, 0x101010101010101); //south
	rook_mask |= universalRay(movement_data, 0x3, 0xff);			   //west

	return rook_mask;
}

uint64_t MoveValidationSystem::bishopValidation(InitGameState::Board&, MovementData& movement_data)
{
	uint64_t bishop_mask = 0;

	bishop_mask |= universalRay(movement_data, 0x0, 0x8040201008040201); //north-east
	bishop_mask |= universalRay(movement_data, 0x1, 0x102040810204080);  //south-east
	bishop_mask |= universalRay(movement_data, 0x1, 0x8040201008040201);  //south-west
	bishop_mask |= universalRay(movement_data, 0x0, 0x102040810204080); //north-west

	return bishop_mask;
}

uint64_t MoveValidationSystem::queenValidation(InitGameState::Board&, MovementData& movement_data)
{
	uint64_t queen_mask = 0;

	queen_mask |= universalRay(movement_data, 0x0, 0x101010101010101);  //north
	queen_mask |= universalRay(movement_data, 0x0, 0x8040201008040201); //north-east
	queen_mask |= universalRay(movement_data, 0x2, 0xff);				 //east
	queen_mask |= universalRay(movement_data, 0x1, 0x102040810204080);  //south-east
	queen_mask |= universalRay(movement_data, 0x1, 0x101010101010101);  //south
	queen_mask |= universalRay(movement_data, 0x1, 0x8040201008040201); //south-west
	queen_mask |= universalRay(movement_data, 0x3, 0xff);				 //west
	queen_mask |= universalRay(movement_data, 0x0, 0x102040810204080);  //north-west

	return queen_mask;
}

uint64_t MoveValidationSystem::kingValidation(InitGameState::Board&, MovementData& movement_data)
{
	uint64_t kingMask = 0;

	uint64_t not_a_file = ~0x1010101010101010;
	uint64_t not_h_file = ~0x8080808080808080;

	kingMask |= ((1ULL << movement_data.picked_square_idx) << 8) & ~movement_data.allies;
	kingMask |= ((1ULL << movement_data.picked_square_idx) >> 8) & ~movement_data.allies;
	kingMask |= ((1ULL << movement_data.picked_square_idx) << 1) & not_a_file & ~movement_data.allies;
	kingMask |= ((1ULL << movement_data.picked_square_idx) >> 1) & not_h_file & ~movement_data.allies;

	kingMask |= (((1ULL << movement_data.picked_square_idx) << 9) & not_a_file) & ~movement_data.allies;
	kingMask |= (((1ULL << movement_data.picked_square_idx) << 7) & not_h_file) & ~movement_data.allies;
	kingMask |= (((1ULL << movement_data.picked_square_idx) >> 9) & not_h_file) & ~movement_data.allies;
	kingMask |= (((1ULL << movement_data.picked_square_idx) >> 7) & not_a_file) & ~movement_data.allies;

	return kingMask;
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
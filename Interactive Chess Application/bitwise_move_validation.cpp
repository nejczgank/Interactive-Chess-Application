//FIX DISPLAY
//REMBER THAT ONCE THIS WORKS AND DISPLAY HAS BEEN FIXED TO PUSH IT TO THE MAIN BRANCH

#include "bitwise_move_validation.h"

BitwiseMoveValidation::BitwiseMoveValidation(InitGameState::Board& all_boards, int picked_square_idx, int placement_square_idx) {
	
	all_boards_ = all_boards;

	white_pawns_ = all_boards.white_pawns;
	white_knights_ = all_boards.white_knights;
	white_rooks_ = all_boards.white_rooks;
	white_bishops_ = all_boards.white_bishops;
	white_queens_ = all_boards.white_queens;
	white_king_ = all_boards.white_king;

	black_pawns_ = all_boards.black_pawns;
	black_knights_ = all_boards.black_knights;
	black_rooks_ = all_boards.black_rooks;
	black_bishops_ = all_boards.black_bishops;
	black_queens_ = all_boards.black_queens;
	black_king_ = all_boards.black_king;

	white_occupancy_ = all_boards.white_occupancy;
	black_occupancy_ = all_boards.black_occupancy;
	all_occupancy_ = all_boards.all_occupancy;

	picked_square_idx_ = picked_square_idx;
	placement_square_idx_ = placement_square_idx;

	compressed_piece_type_ = 0;
	allies_ = 0;
	enemies_ = 0;
	universal_ray_ = 0;
}

void BitwiseMoveValidation::determinePickedPiece() {
	//check every board to see if the picked_square_idx is present on that board and compact it

	//DONT FORGET THAT YOU'LL HAVE TO UPDATE all_boards_
	//though, I'll try to make it branchless now that I better understand this concept

	uint64_t InitGameState::Board::* whiteBoardSelectors[]{
		&InitGameState::Board::white_pawns,
		&InitGameState::Board::white_knights,
		&InitGameState::Board::white_rooks,
		&InitGameState::Board::white_bishops,
		&InitGameState::Board::white_queens,
		&InitGameState::Board::white_king,
	};

	if ((white_occupancy_ & (1ULL << picked_square_idx_)) != 0) {
		for (int i = 1; i <= 6; i++) {
			if (all_boards_.*whiteBoardSelectors[i-1] & (1ULL << picked_square_idx_)) {
				//compressed_piece_type_ += i; //this doesn't shift the bit to its adequate place
				compressed_piece_type_ = (1ULL << (i-1));
				allies_ = white_occupancy_;
				enemies_ = black_occupancy_;
				return;
			}
		}
	}

	uint64_t InitGameState::Board::* blackBoardSelectors[]{
		&InitGameState::Board::black_pawns,
		&InitGameState::Board::black_knights,
		&InitGameState::Board::black_rooks,
		&InitGameState::Board::black_bishops,
		&InitGameState::Board::black_queens,
		&InitGameState::Board::black_king,
	};

	if ((black_occupancy_ & (1ULL << picked_square_idx_)) != 0) {
		for (int i = 1; i <= 6; i++) {
			if (all_boards_.*blackBoardSelectors[i-1] & (1ULL << picked_square_idx_)) {
				//compressed_piece_type_ += i + 6;
				compressed_piece_type_ = (1ULL << (i + 6 - 1));
				allies_ = black_occupancy_;
				enemies_ = white_occupancy_;
				return;
			}
		}
	}
}

void BitwiseMoveValidation::callPieceTypesValidator() {
	
	determinePickedPiece();

	uint64_t is_pawn   = -((compressed_piece_type_ & 0x1)	   | (compressed_piece_type_ >> 6 & 0x1));
	uint64_t is_knight = -((compressed_piece_type_ >> 1 & 0x1) | (compressed_piece_type_ >> 7 & 0x1));
	uint64_t is_rook   = -((compressed_piece_type_ >> 2 & 0x1) | (compressed_piece_type_ >> 8 & 0x1));
	uint64_t is_bishop = -((compressed_piece_type_ >> 3 & 0x1) | (compressed_piece_type_ >> 9 & 0x1));
	uint64_t is_queen  = -((compressed_piece_type_ >> 4 & 0x1) | (compressed_piece_type_ >> 10 & 0x1));
	uint64_t is_king   = -((compressed_piece_type_ >> 5 & 0x1) | (compressed_piece_type_ >> 11 & 0x1));
	
	uint64_t determine_legal_moves = ((is_pawn & pawnValidation())     |
									  (is_knight & knightValidation()) |
									  (is_rook & rookValidation())	   |
									  (is_bishop & bishopValidation()) |
									  (is_queen & queenValidation())   |
									  (is_king & kingValidaiton())
	);
}

uint64_t BitwiseMoveValidation::universalRay(uint64_t direction_bitfield, uint64_t full_ray) {

	uint64_t significant_bit_truth_mask = -(int64_t)(direction_bitfield & 0x01);
	uint64_t transposition_truth_mask = -(int64_t)((direction_bitfield >> 1) & 0x01);

	//find the appropriate half ray
	uint64_t halved_ray = rayHalvingHelper(&significant_bit_truth_mask, &transposition_truth_mask, &full_ray);
	//adjust ray for enemy blockers
	uint64_t ray_to_enemy_blockers = findEnemyBlockersHelper(&significant_bit_truth_mask, &halved_ray);
	//adjust ray for ally blockers
	uint64_t ray_to_ally_blockers = findAllyBlockersHelper(&significant_bit_truth_mask, &halved_ray);

	// if blockers are both empty then result is halved ray
	// if only one blocker exit than that blocker is the result
	// if both blockers exist then result is the combination of the two
	uint64_t if_ray_to_enemy_blockers = -(ray_to_enemy_blockers != 0);
	uint64_t if_ray_to_ally_blockers = -(ray_to_ally_blockers != 0);
	uint64_t if_only_one_blocker_exists = if_ray_to_enemy_blockers ^ if_ray_to_ally_blockers;
	uint64_t if_both_blockers_exist = if_ray_to_enemy_blockers & if_ray_to_ally_blockers;
	uint64_t if_no_path_forward = -(((halved_ray & allies_) != 0) && if_ray_to_ally_blockers == 0);

	uint64_t universal_ray =	(if_only_one_blocker_exists & ~if_no_path_forward & ray_to_enemy_blockers) |
								(if_only_one_blocker_exists & ~if_no_path_forward & ray_to_ally_blockers)  |
								(if_both_blockers_exist & (ray_to_enemy_blockers & ray_to_ally_blockers))  |
								(~if_both_blockers_exist & ~if_only_one_blocker_exists & halved_ray);

	return universal_ray;
}

uint64_t BitwiseMoveValidation::rayHalvingHelper(uint64_t* significant_bit_truth_mask, uint64_t* transposition_truth_mask, uint64_t* full_ray) {

	uint64_t if_significant_bit_mask = -(*significant_bit_truth_mask == 0);
	uint64_t if_diagonal = -((*full_ray == 0x102040810204080) | (*full_ray == 0x8040201008040201));

	uint64_t determined_transposed_ray = (if_diagonal & (diagonalTransformation(full_ray))) | (~if_diagonal & (nonDiagonalTransformation(transposition_truth_mask, full_ray)));
	uint64_t determined_halving = (if_significant_bit_mask & ~((1ULL << (picked_square_idx_ + 1)) - 1)) | (~if_significant_bit_mask & ((1ULL << picked_square_idx_) - 1));

	return determined_transposed_ray & determined_halving;
}

uint64_t BitwiseMoveValidation::diagonalTransformation(uint64_t* full_ray) {
	
	uint64_t if_right_starting_diagonal = -(*full_ray == 0x102040810204080);
	uint64_t if_left_starting_diagonal = -(*full_ray == 0x8040201008040201);

	int rank = (int)(picked_square_idx_ / 8);
	int file = (int)(picked_square_idx_ % 8);

	uint64_t if_shift_1 = -((rank - file) >= 0);
	uint64_t if_shift_2 = -((file - rank) >= 0);

	int opposite_distance = (rank + file) - 7;

	uint64_t if_shift_3 = -(opposite_distance >= 0);
	uint64_t if_shift_4 = -(opposite_distance < 0);

	int safe_shift_1 = (rank - file) * 8;
	int safe_shift_2 = (file - rank) * 8;
	int safe_shift_3 = (opposite_distance) * 8;
	int safe_shift_4 = (-opposite_distance) * 8;

	uint64_t determined_transposing = (if_left_starting_diagonal & if_shift_1 & (*full_ray << safe_shift_1))  | 
									  (if_left_starting_diagonal & if_shift_2 & (*full_ray >> safe_shift_2))  | 
									  (if_right_starting_diagonal & if_shift_3 & (*full_ray << safe_shift_3)) | 
									  (if_right_starting_diagonal & if_shift_4 & (*full_ray >> safe_shift_4)
	);

	return determined_transposing;
}

uint64_t BitwiseMoveValidation::nonDiagonalTransformation(uint64_t* transposition_truth_mask, uint64_t* full_ray) {

	uint64_t if_transposition_mask = -(*transposition_truth_mask == 0);
	uint64_t determined_transposing = (if_transposition_mask & (picked_square_idx_ % 8)) | (~if_transposition_mask & (picked_square_idx_ / 8));
	uint64_t transposed_ray = (if_transposition_mask & (*full_ray << determined_transposing)) | (~if_transposition_mask & (*full_ray << (determined_transposing * 8)));

	return transposed_ray;
}

uint64_t BitwiseMoveValidation::findEnemyBlockersHelper(uint64_t* significant_bit_truth_mask, uint64_t* halved_ray) {
	
	uint64_t found_blockers = *halved_ray & enemies_;
	uint64_t if_significant_bit_mask = -(*significant_bit_truth_mask == 0);
	uint64_t determined_halving = (if_significant_bit_mask & (((found_blockers & (0ULL - found_blockers)) - 1) | (found_blockers & (0ULL - found_blockers)))) | (~if_significant_bit_mask & ~(std::bit_floor(found_blockers) - 1));

	return determined_halving & *halved_ray;
}

uint64_t BitwiseMoveValidation::findAllyBlockersHelper(uint64_t* significant_bit_truth_mask, uint64_t* halved_ray) {

	uint64_t found_blockers = *halved_ray & allies_;
	uint64_t if_significant_bit_mask = -(*significant_bit_truth_mask == 0);
	uint64_t determined_halving = (if_significant_bit_mask & (((found_blockers & (0ULL - found_blockers)) - 1))) | (~if_significant_bit_mask & ~((std::bit_floor(found_blockers) - 1) | std::bit_floor(found_blockers)));

	return determined_halving & *halved_ray;
}

uint64_t BitwiseMoveValidation::pawnValidation() {
	return 0;
}

uint64_t BitwiseMoveValidation::knightValidation() {

	uint64_t knight_mask = 0;

	uint64_t not_a = 0xfefefefefefefefe;
	uint64_t not_b = 0xfdfdfdfdfdfdfdfd;
	uint64_t not_g = 0xbfbfbfbfbfbfbfbf;
	uint64_t not_h = 0x7f7f7f7f7f7f7f7f;

	knight_mask |= ((1ULL << picked_square_idx_) & not_h) << 17 & ~allies_;
	knight_mask |= ((1ULL << picked_square_idx_) & not_g & not_h) << 10 & ~allies_;
	knight_mask |= ((1ULL << picked_square_idx_) & not_g & not_h) >> 6 & ~allies_;
	knight_mask |= ((1ULL << picked_square_idx_) & not_h) >> 15 & ~allies_;

	knight_mask |= ((1ULL << picked_square_idx_) & not_a) >> 17 & ~allies_;
	knight_mask |= ((1ULL << picked_square_idx_) & not_b & not_a) >> 10 & ~allies_;
	knight_mask |= ((1ULL << picked_square_idx_) & not_b & not_a) << 6 & ~allies_;
	knight_mask |= ((1ULL << picked_square_idx_) & not_a) << 15 & ~allies_;

	return knight_mask;
}

uint64_t BitwiseMoveValidation::rookValidation() {

	uint64_t rook_mask = 0;
	
	rook_mask |= universalRay(0x0, 0x101010101010101); //north
	rook_mask |= universalRay(0x2, 0xff);			   //east
	rook_mask |= universalRay(0x1, 0x101010101010101); //south
	rook_mask |= universalRay(0x3, 0xff);			   //west

	return rook_mask;
}

uint64_t BitwiseMoveValidation::bishopValidation() {

	uint64_t bishop_mask = 0;

	bishop_mask |= universalRay(0x0, 0x8040201008040201); //north-east
	bishop_mask |= universalRay(0x1, 0x102040810204080);  //south-east
	bishop_mask |= universalRay(0x1, 0x102040810204080);  //south-west
	bishop_mask |= universalRay(0x0, 0x8040201008040201); //north-west

	return bishop_mask;
}

uint64_t BitwiseMoveValidation::queenValidation() {

	uint64_t queen_mask = 0;

	queen_mask |= universalRay(0x0, 0x101010101010101);  //north
	queen_mask |= universalRay(0x0, 0x8040201008040201); //north-east
	queen_mask |= universalRay(0x2, 0xff);				 //east
	queen_mask |= universalRay(0x1, 0x102040810204080);  //south-east
	queen_mask |= universalRay(0x1, 0x101010101010101);  //south
	queen_mask |= universalRay(0x1, 0x8040201008040201); //south-west
	queen_mask |= universalRay(0x3, 0xff);				 //west
	queen_mask |= universalRay(0x0, 0x102040810204080);  //north-west

	return queen_mask;
}

uint64_t BitwiseMoveValidation::kingValidaiton() {
	return 0;
}

void BitwiseMoveValidation::moveValidation() { //wtf was this // ohhh its meant to include the placement_square to see if the movement can be performed

}

void BitwiseMoveValidation::updateBoards() {

}

void BitwiseMoveValidation::checkPositionsBoard() {

}


//pawns (determining direction, decoupling captures and moves, extra initial move, en-passant)
//knights (no-blocking mechanic)
//rooks
//bishops
//queens
//kings

//check
//check-mate
//en-passant
//castling
//ties

//using bitmasks, avoiding if statements and loops
//final rendition a universal function with delta direction for raycasts

BitwiseMoveValidation::~BitwiseMoveValidation() = default;
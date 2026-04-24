#include "bitwise_move_validation.h"

uint64_t InitGameState::Board::* BitwiseMoveValidation::white_selectors_[]
{
	&InitGameState::Board::white_pawns,
	&InitGameState::Board::white_knights,
	&InitGameState::Board::white_rooks,
	&InitGameState::Board::white_bishops,
	&InitGameState::Board::white_queens,
	&InitGameState::Board::white_king,
};

uint64_t InitGameState::Board::* BitwiseMoveValidation::black_selectors_[]
{
	&InitGameState::Board::black_pawns,
	&InitGameState::Board::black_knights,
	&InitGameState::Board::black_rooks,
	&InitGameState::Board::black_bishops,
	&InitGameState::Board::black_queens,
	&InitGameState::Board::black_king,
};

uint64_t InitGameState::Board::* BitwiseMoveValidation::all_selectors_[]
{
	&InitGameState::Board::white_pawns,
	&InitGameState::Board::white_knights,
	&InitGameState::Board::white_rooks,
	&InitGameState::Board::white_bishops,
	&InitGameState::Board::white_queens,
	&InitGameState::Board::white_king,
	&InitGameState::Board::black_pawns,
	&InitGameState::Board::black_knights,
	&InitGameState::Board::black_rooks,
	&InitGameState::Board::black_bishops,
	&InitGameState::Board::black_queens,
	&InitGameState::Board::black_king,
};

uint64_t InitGameState::Board::* BitwiseMoveValidation::color_selectors_[]
{
	&InitGameState::Board::white_occupancy,
	&InitGameState::Board::black_occupancy
};

BitwiseMoveValidation::BitwiseMoveValidation(InitGameState::Board& all_boards) : 
	//initializer list
	//makes it so I don't have to use all_boards_-> for every subsequent board call
	all_boards_(&all_boards),
	white_pawns_(all_boards.white_pawns),
	white_knights_(all_boards.white_knights),
	white_rooks_(all_boards.white_rooks),
	white_bishops_(all_boards.white_bishops),
	white_queens_(all_boards.white_queens),
	white_king_(all_boards.white_king),
	black_pawns_(all_boards.black_pawns),
	black_knights_(all_boards.black_knights),
	black_rooks_(all_boards.black_rooks),
	black_bishops_(all_boards.black_bishops),
	black_queens_(all_boards.black_queens),
	black_king_(all_boards.black_king),
	white_occupancy_(all_boards.white_occupancy),
	black_occupancy_(all_boards.black_occupancy),
	all_occupancy_(all_boards.all_occupancy),
	evaluateThisBoard{} //initialized object for the AI class
{ 
	picked_square_idx_ = 0;
	placement_square_idx_ = 0;

	picked_piece_type_ = 0;
	placed_piece_type_ = 0;
	allies_ = 0;
	enemies_ = 0;
}

void BitwiseMoveValidation::setUpdatedState(int picked_square_idx, int placement_square_idx)
{
	picked_square_idx_ = picked_square_idx;
	placement_square_idx_ = placement_square_idx;
}

void BitwiseMoveValidation::determinePickedPiece()
{
	//find the picked piece
	if ((white_occupancy_ & (1ULL << picked_square_idx_)) != 0) {
		for (int i = 0; i < 6; i++) {
			if (*all_boards_.*white_selectors_[i] & (1ULL << picked_square_idx_)) {
				picked_piece_type_ = (1ULL << i);
				allies_ = white_occupancy_;
				enemies_ = black_occupancy_;
				return;
			}
		}
	}

	if ((black_occupancy_ & (1ULL << picked_square_idx_)) != 0) {
		for (int i = 0; i < 6; i++) {
			if (*all_boards_.*black_selectors_[i] & (1ULL << picked_square_idx_)) {
				picked_piece_type_ = (1ULL << (i + 6));
				allies_ = black_occupancy_;
				enemies_ = white_occupancy_;
				return;
			}
		}
	}
}

void BitwiseMoveValidation::determinePlacedPiece()
{
	//find the placed square
	if ((white_occupancy_ & (1ULL << placement_square_idx_)) != 0) {
		for (int i = 0; i < 6; i++) {
			if (*all_boards_.*white_selectors_[i] & (1ULL << placement_square_idx_)) {
				placed_piece_type_ = (1ULL << i);
				return;
			}
		}
	}

	if ((black_occupancy_ & (1ULL << placement_square_idx_)) != 0) {
		for (int i = 0; i < 6; i++) {
			if (*all_boards_.*black_selectors_[i] & (1ULL << placement_square_idx_)) {
				placed_piece_type_ = (1ULL << (i + 6));
				return;
			}
		}
	}
}

bool BitwiseMoveValidation::callPieceTypesValidator()
{
	determinePickedPiece();

	uint64_t if_pawn = -(0x1 & picked_piece_type_);
	int picked_piece_idx = (int)((if_pawn & 0x0) | (~if_pawn & std::countr_zero(picked_piece_type_)));
	picked_piece_idx = picked_piece_idx % 6;

	//array of member function pointers
	static uint64_t(BitwiseMoveValidation::*jumpTable[])() = {
		&BitwiseMoveValidation::pawnValidation,
		&BitwiseMoveValidation::knightValidation,
		&BitwiseMoveValidation::rookValidation,
		&BitwiseMoveValidation::bishopValidation,
		&BitwiseMoveValidation::queenValidation,
		&BitwiseMoveValidation::kingValidation
	};

	uint64_t valid_moves = 0;

	if (picked_piece_idx < 6 && picked_piece_idx >= 0) {
		//this for running the current instance of BitwiseMoveValidation
		valid_moves = (this->*jumpTable[picked_piece_idx])();
	}

	determinePlacedPiece();

	uint64_t piece_placement = movementValidation(&valid_moves);
	if (piece_placement == 0) {
		PlayerInput::outOfScope();
		return false;
	}

	updateBoards(&piece_placement);

	return true;
	//if validMoves = 0, then return 0 and let the external logic handle an invalid placement
}

uint64_t BitwiseMoveValidation::universalRay(uint64_t direction_bitfield, uint64_t full_ray)
{
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

uint64_t BitwiseMoveValidation::rayHalvingHelper(uint64_t* significant_bit_truth_mask, uint64_t* transposition_truth_mask, uint64_t* full_ray)
{
	uint64_t if_significant_bit_mask = -(int64_t)(*significant_bit_truth_mask == 0);
	uint64_t if_diagonal = -((*full_ray == 0x102040810204080) | (*full_ray == 0x8040201008040201));

	uint64_t determined_transposed_ray = 0;
	//replace this with a switch, if, lookup table maybe? just don't let it execute two functions like that
	if (*full_ray == 0x102040810204080) {
		determined_transposed_ray = diagonalTransformation(full_ray);
	}
	else if (*full_ray == 0x8040201008040201) {
		determined_transposed_ray = nonDiagonalTransformation(transposition_truth_mask, full_ray);
	}

	uint64_t determined_halving = (if_significant_bit_mask & ~((1ULL << (picked_square_idx_ + 1)) - 1)) | (~if_significant_bit_mask & ((1ULL << picked_square_idx_) - 1));

	return determined_transposed_ray & determined_halving;
}

uint64_t BitwiseMoveValidation::diagonalTransformation(uint64_t* full_ray)
{
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

uint64_t BitwiseMoveValidation::nonDiagonalTransformation(uint64_t* transposition_truth_mask, uint64_t* full_ray)
{
	uint64_t if_transposition_mask = -(*transposition_truth_mask == 0);
	uint64_t determined_transposing = (if_transposition_mask & (picked_square_idx_ % 8)) | (~if_transposition_mask & (picked_square_idx_ / 8));
	uint64_t transposed_ray = (if_transposition_mask & (*full_ray << determined_transposing)) | (~if_transposition_mask & (*full_ray << (determined_transposing * 8)));

	return transposed_ray;
}

uint64_t BitwiseMoveValidation::findEnemyBlockersHelper(uint64_t* significant_bit_truth_mask, uint64_t* halved_ray)
{
	uint64_t found_blockers = *halved_ray & enemies_;
	uint64_t if_significant_bit_mask = -(*significant_bit_truth_mask == 0);
	uint64_t determined_halving = (if_significant_bit_mask & (((found_blockers & (0ULL - found_blockers)) - 1) | (found_blockers & (0ULL - found_blockers)))) | (~if_significant_bit_mask & ~(std::bit_floor(found_blockers) - 1));

	return determined_halving & *halved_ray;
}

uint64_t BitwiseMoveValidation::findAllyBlockersHelper(uint64_t* significant_bit_truth_mask, uint64_t* halved_ray)
{

	uint64_t found_blockers = *halved_ray & allies_;
	uint64_t if_significant_bit_mask = -(*significant_bit_truth_mask == 0);
	uint64_t determined_halving = (if_significant_bit_mask & (((found_blockers & (0ULL - found_blockers)) - 1))) | (~if_significant_bit_mask & ~((std::bit_floor(found_blockers) - 1) | std::bit_floor(found_blockers)));

	return determined_halving & *halved_ray;
}

uint64_t BitwiseMoveValidation::pawnValidation()
{
	uint64_t pawnMask = 0;

	uint64_t if_white = -(picked_piece_type_ & 0x1); //1st one because it's a pawn
	uint64_t if_black = -((picked_piece_type_ >> 6) & 0x1); //7th one because it's a pawn

	uint64_t determine_init_pos = (if_white & 0x1) | (if_black & 0x6);
	uint64_t test = picked_square_idx_ / 8;
	uint64_t if_init = -(int64_t)((picked_square_idx_ / 8) == determine_init_pos);

	//preventing progression to occupied squares
	uint64_t if_occupied = -(int64_t)((if_white & (all_occupancy_ & (1ULL << (picked_square_idx_ + 8)))) | (if_black & (all_occupancy_ & ((1ULL << picked_square_idx_) >> 8))));

	uint64_t determine_regular_move = (if_init & if_white & (0x101ULL << (picked_square_idx_ + 8)))					|
									  (~if_init & if_white & ~if_occupied & (1ULL << (picked_square_idx_ + 8)))		|
									  (if_init & if_black & ((0x101ULL << picked_square_idx_) >> 16))				|
									  (~if_init & if_black & ~if_occupied & ((1ULL << picked_square_idx_) >> 8))
	;

	pawnMask |= determine_regular_move;

	//handling wrap around for attacks
	uint64_t not_a = 0xfefefefefefefefe;
	uint64_t not_h = 0x7f7f7f7f7f7f7f7f;

	uint64_t determine_atk = (if_white & not_h & (black_occupancy_ & (1ULL << (picked_square_idx_ + 7))))		|
							 (if_white & (black_occupancy_ & (1ULL << (picked_square_idx_ + 9)))				|
							 (if_black & not_a & (white_occupancy_ & ((1ULL << picked_square_idx_) >> 7)))		|
							 (if_black & (white_occupancy_ & (1ULL << picked_square_idx_) >> 9)))	  
	;

	pawnMask |= determine_atk;

	//en-passant
	//pawn promotion

	return pawnMask;
}

uint64_t BitwiseMoveValidation::knightValidation() 
{
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

uint64_t BitwiseMoveValidation::rookValidation()
{
	uint64_t rook_mask = 0;
	
	rook_mask |= universalRay(0x0, 0x101010101010101); //north
	rook_mask |= universalRay(0x2, 0xff);			   //east
	rook_mask |= universalRay(0x1, 0x101010101010101); //south
	rook_mask |= universalRay(0x3, 0xff);			   //west

	return rook_mask;
}

uint64_t BitwiseMoveValidation::bishopValidation()
{
	uint64_t bishop_mask = 0;

	bishop_mask |= universalRay(0x0, 0x8040201008040201); //north-east
	bishop_mask |= universalRay(0x1, 0x102040810204080);  //south-east
	bishop_mask |= universalRay(0x1, 0x8040201008040201);  //south-west
	bishop_mask |= universalRay(0x0, 0x102040810204080); //north-west

	return bishop_mask;
}

uint64_t BitwiseMoveValidation::queenValidation()
{
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

uint64_t BitwiseMoveValidation::kingValidation()
{
	uint64_t kingMask = 0;

	uint64_t not_a_file = ~0x1010101010101010;
	uint64_t not_h_file = ~0x8080808080808080;

	kingMask |= ((1ULL << picked_square_idx_) << 8);
	kingMask |= ((1ULL << picked_square_idx_) >> 8);
	kingMask |= ((1ULL << picked_square_idx_) << 1) & not_a_file;
	kingMask |= ((1ULL << picked_square_idx_) >> 1) & not_h_file;

	kingMask |= (((1ULL << picked_square_idx_) << 9) & not_a_file);
	kingMask |= (((1ULL << picked_square_idx_) << 7) & not_h_file);
	kingMask |= (((1ULL << picked_square_idx_) >> 9) & not_h_file);
	kingMask |= (((1ULL << picked_square_idx_) >> 7) & not_a_file);

	return kingMask;
}

uint64_t BitwiseMoveValidation::movementValidation(uint64_t* found_moves)
{
	// 0   - invalid move
	// 0 < - valid move

	return *found_moves & (1ULL << placement_square_idx_);
}

void BitwiseMoveValidation::updateBoards(uint64_t* piece_placement) 
{
	//piece_placement will give me the board where the piece is supposed to be
	//placed_piece_type_ will give me the exact piece type being captured, if any

	//PLACED_PIECE_TYPE_ IS 0 BECAUSE THERE IS NO PIECE TO OVERTAKE, THIS IS CAUSING THE ISSUE

	int color = ((0x3f & placed_piece_type_) == 0 ? 0 : 1);
	int opposite_color = ((0x7c0 & placed_piece_type_) == 0 ? 0 : 1);

	uint64_t if_pawn = -(0x1 & placed_piece_type_);
	int placed_piece_idx = (int)((if_pawn & 0x0) | (~if_pawn & std::countr_zero(placed_piece_type_)));

	clearPieceHelper(piece_placement, placed_piece_idx, color, opposite_color);

	color = ((0x3f & picked_piece_type_) == 0 ? 0 : 1);
	opposite_color = ((0x7c0 & picked_piece_type_) == 0 ? 0 : 1);

	if_pawn = -(0x1 & picked_piece_type_);
	int picked_piece_idx = (int)((if_pawn & 0x0) | (~if_pawn & std::countr_zero(picked_piece_type_)));

	placeNewPieceHelper(piece_placement, picked_piece_idx, color, opposite_color);

}

void BitwiseMoveValidation::clearPieceHelper(uint64_t* piece_placement, int placed_piece_idx, int color, int opposite_color)
{
	if (piece_placement == 0) {
		return;
	}
	
	//attributing the correct amount of points
	int materialCounts[] = { 1, 3, 3, 5, 9, 0 }; //validate this is correct //king evaluation will have to be dynamic later on based on the state of the game
	const int piece_value = placed_piece_idx % 6;

	evaluateThisBoard.materialCount(color * materialCounts[piece_value], opposite_color * materialCounts[piece_value]);

	//white/black occupancy
	//clear the value from the placed color board
	*all_boards_.*color_selectors_[color] &= ~*piece_placement;

	//specific figure
	//clear the value from the specific placed figure board
	*all_boards_.*all_selectors_[placed_piece_idx] &= ~*piece_placement;

	//white/black occupancy (opposite)
	//set the newly occupied value for the placed color board
	*all_boards_.*color_selectors_[opposite_color] |= *piece_placement;
}

void BitwiseMoveValidation::placeNewPieceHelper(uint64_t* piece_placement, int picked_piece_idx, int color, int opposite_color)
{
	//set the newly occupied value for the placed figure board
	*all_boards_.*all_selectors_[picked_piece_idx] |= *piece_placement;
	//clear the value from the picked color board
	*all_boards_.*color_selectors_[color] &= ~(1ULL << picked_square_idx_);
	//clear the value from the pdk figure board
	*all_boards_.*all_selectors_[picked_piece_idx] &= ~(1ULL << picked_square_idx_);
	//clear the pdk value from the all occupancy board
	all_occupancy_ &= ~(1ULL << picked_square_idx_);
	//update allies and enemies
	allies_ = *all_boards_.*color_selectors_[color];
	enemies_ = *all_boards_.*color_selectors_[opposite_color];
}

void BitwiseMoveValidation::checkPositionsBoard() {}

//special moves, I have yet to implement:
//check
//check-mate
//en-passant

//pawn promotion 
//(requires additional user input, or defaulting the piece to a queen 
//for easier implementation in conjunction with the ai)

//castling
//stalemate
//draws

BitwiseMoveValidation::~BitwiseMoveValidation() = default;
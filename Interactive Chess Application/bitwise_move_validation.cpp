#include "bitwise_move_validation.h"

namespace P {
	constexpr int W_PAWN = 0;
	constexpr int W_KNIGHT = 1;
	constexpr int W_ROOK = 2;
	constexpr int W_BISHOP = 3;	
	constexpr int W_QUEEN = 4;
	constexpr int W_KING = 5;
	constexpr int B_PAWN = 6;
	constexpr int B_KNIGHT = 7;
	constexpr int B_ROOK = 8;
	constexpr int B_BISHOP = 9;
	constexpr int B_QUEEN = 10;
	constexpr int B_KING = 11;
}

namespace OCC {
	using Type = int;
	constexpr int W_OCC = 0;
	constexpr int B_OCC = 1;
	constexpr int ALL_OCC = 2;
}

namespace moveInfo {
	using Type = int;
	constexpr int picked = 0;
	constexpr int placed = 1;
}

BitwiseMoveValidation::BitwiseMoveValidation(InitGameState::Board& all_boards) :
	//initializer list
	all_boards_(&all_boards),
	p_(all_boards_->pieces),
	occ_(all_boards_->occupancy),
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

void BitwiseMoveValidation::getMovementInfo(moveInfo::Type flag)
{
	//dynamically access the proper value, based on whether we are searching for a
	//picked piece to compute it's validation or a placed piece to determine placement
	int* lookupIdx[] = {&picked_square_idx_, &placement_square_idx_};
	int* movementPointer = lookupIdx[flag];

	//find the color of the piece to adjust the offset, needed for determining subsequent values
	int color = ((occ_[OCC::W_OCC] & (1ULL << *movementPointer)) != 0) ? 0 : 1;
	const int correction = 6 * color;

	uint64_t if_color = -(color == 0);
	uint64_t if_flag_option = -(flag == 0);
	
	//find what the allies or enemies are based on what color piece was picked
	allies_ = (if_color & if_flag_option & occ_[OCC::W_OCC]) | (~if_color & if_flag_option & occ_[OCC::B_OCC]);
	enemies_ = (if_color & if_flag_option & occ_[OCC::B_OCC]) | (~if_color & if_flag_option & occ_[OCC::W_OCC]);

	placed_piece_type_ = -1;

	for (int i = 0; i < 6; i++) { //move through 6 possible figure types
		if (p_[i + correction] & (1ULL << *movementPointer)) { //adjust which type gets selected
			(flag == 0 ? picked_piece_type_ : placed_piece_type_) = i + correction; //save the number to be later used by the jump table
			return;
		}
	}
}

uint64_t BitwiseMoveValidation::validator() 
{
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

	if (picked_piece_type_ < 12 && picked_piece_type_ >= 0) {
		//this for running the current instance of BitwiseMoveValidation
		valid_moves = (this->*jumpTable[picked_piece_type_ % 6])();
	}

	return valid_moves;
}

//uint64_t BitwiseMoveValidation::validator(int picked_piece_type)
//{
//	//array of member function pointers
//	static uint64_t(BitwiseMoveValidation::*jumpTable[])() = {
//		&BitwiseMoveValidation::pawnValidation,
//		&BitwiseMoveValidation::knightValidation,
//		&BitwiseMoveValidation::rookValidation,
//		&BitwiseMoveValidation::bishopValidation,
//		&BitwiseMoveValidation::queenValidation,
//		&BitwiseMoveValidation::kingValidation
//	};
//
//	uint64_t valid_moves = 0;
//
//	if (picked_piece_type < 12 && picked_piece_type >= 0) {
//		//this for running the current instance of BitwiseMoveValidation
//		valid_moves = (this->*jumpTable[picked_piece_type % 6])();
//	}
//
//	return valid_moves;
//}

bool BitwiseMoveValidation::callPieceTypesValidator()
{
	//determinePickedPiece();
	getMovementInfo(moveInfo::picked);

	uint64_t valid_moves = validator();

	//determinePlacedPiece();
	getMovementInfo(moveInfo::placed);

	uint64_t piece_placement = movementValidation(&valid_moves);
	if (piece_placement == 0ULL) {
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
	uint64_t if_diagonal = -((*full_ray == 0x102040810204080) | (*full_ray == 0x8040201008040201)); //defaults to true?

	uint64_t determined_transposed_ray = 0;
	//replace this with a switch, if, lookup table maybe? just don't let it execute two functions like that
	if (*full_ray == 0x102040810204080 || *full_ray == 0x8040201008040201) {
		determined_transposed_ray = diagonalTransformation(full_ray);
	}
	else {
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
	uint64_t if_white = 0;
	uint64_t if_black = 0;

	if (picked_piece_type_ == 0) { if_white = ~0ULL; }
	if (picked_piece_type_ == 6) { if_black = ~0ULL; }

	uint64_t determine_init_pos = (if_white & 0x1) | (if_black & 0x6);
	uint64_t test = picked_square_idx_ / 8;
	uint64_t if_init = -(int64_t)((picked_square_idx_ / 8) == determine_init_pos);

	//preventing progression to occupied squares (regular move)
	uint64_t if_occupied_first = -(int64_t)((if_white & (occ_[OCC::ALL_OCC] & (1ULL << (picked_square_idx_ + 8)))) | (if_black & (occ_[OCC::ALL_OCC] & ((1ULL << picked_square_idx_) >> 8))));
	uint64_t if_occupied_second = -(int64_t)((if_white & (occ_[OCC::ALL_OCC] & (0x101ULL << (picked_square_idx_ + 8)))) | (if_black & (occ_[OCC::ALL_OCC] & ((0x101ULL << picked_square_idx_) >> 16))));

	uint64_t determine_regular_move = (if_init  & if_white  & ~if_occupied_second & (0x101ULL << (picked_square_idx_ + 8)))					|
									  (~if_init & if_white  & ~if_occupied_first  & (1ULL << (picked_square_idx_ + 8)))						|
									  (if_init  & if_black  & ~if_occupied_second & ((0x101ULL << picked_square_idx_) >> 16))				|
									  (~if_init & if_black  & ~if_occupied_first  & ((1ULL << picked_square_idx_) >> 8))
	;

	pawnMask |= determine_regular_move;

	//handling wrap around for attacks
	uint64_t not_a = 0xfefefefefefefefe;
	uint64_t not_h = 0x7f7f7f7f7f7f7f7f;

	uint64_t determine_atk = (if_white & not_h & (occ_[OCC::B_OCC] & (1ULL << (picked_square_idx_ + 7)))) |
							 (if_white & (occ_[OCC::B_OCC] & (1ULL << (picked_square_idx_ + 9)))				|
							 (if_black & not_a & (occ_[OCC::W_OCC] & ((1ULL << picked_square_idx_) >> 7)))		|
							 (if_black & (occ_[OCC::W_OCC] & (1ULL << picked_square_idx_) >> 9)))
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

	kingMask |= ((1ULL << picked_square_idx_) << 8) & ~allies_;
	kingMask |= ((1ULL << picked_square_idx_) >> 8) & ~allies_;
	kingMask |= ((1ULL << picked_square_idx_) << 1) & not_a_file & ~allies_;
	kingMask |= ((1ULL << picked_square_idx_) >> 1) & not_h_file & ~allies_;

	kingMask |= (((1ULL << picked_square_idx_) << 9) & not_a_file) & ~allies_;
	kingMask |= (((1ULL << picked_square_idx_) << 7) & not_h_file) & ~allies_;
	kingMask |= (((1ULL << picked_square_idx_) >> 9) & not_h_file) & ~allies_;
	kingMask |= (((1ULL << picked_square_idx_) >> 7) & not_a_file) & ~allies_;

	return kingMask;
}

void BitwiseMoveValidation::initAttackTable(OCC::Type color_flag)
{
	/*	Board board;

	board.pieces[0] = white_pawns_;
	board.pieces[1] = white_knights_;
	board.pieces[2] = white_rooks_;
	board.pieces[3] = white_bishops_;
	board.pieces[4] = white_queens_;
	board.pieces[5] = white_king_;
	board.pieces[6] = black_pawns_;
	board.pieces[7] = black_knights_;
	board.pieces[8] = black_rooks_;
	board.pieces[9] = black_bishops_;
	board.pieces[10] = black_queens_;
	board.pieces[11] = black_king_;

	board.occupancy[0] = white_occupancy_;
	board.occupancy[1] = black_occupancy_;
	board.occupancy[2] = white_occupancy_ | black_occupancy_;*/

	//uint64_t white_attack_table = 0;
	//uint64_t black_attack_table = 0;

	//int correction = 0;
	//correction = (color_flag == 0) ? 0 : 6;

	//for (int i = 0; i < 6; i++) {

	//	uint64_t curr_bitboard = all_boards_->pieces[i+correction];

	//	while(curr_bitboard != 0)
	//	{

	//	  uint64_t attack = validator();
	//	  //or the result into the initial attack table
	//	}
	//}
	//
}

void BitwiseMoveValidation::attackTable() 
{
	//implement attack table in here
	//its purpose is to fetch all square positions where pieces have a legal move to attack at
	//on every move a piece makes the table gets updated to reflect the new holistic attack state of the board
	//this is done efficiently by only adjusting for the moved piece, and those who've had their move blocked by
	//the previous pieces position
	//doing so is the most efficient way I've thought of designing this system
	//which in turn aids both check/checkmate algorithm as well as castling algorithm

	//initialize white's attack table
	initAttackTable(OCC::W_OCC);
	initAttackTable(OCC::B_OCC);
	//how do I ensure every piece 

	//initialize black's attack table
	//find which attacks have intersected the piece that moved and update them
	//find which attacks have intersected the new position of the piece that moved and update them
	
	

}

uint64_t BitwiseMoveValidation::movementValidation(uint64_t* found_moves)
{
	// 0   - invalid move
	// 0 < - valid move

	return *found_moves & (1ULL << placement_square_idx_);
}

void BitwiseMoveValidation::updateBoards(uint64_t* piece_placement) 
{
	int color = 0;
	int opposite_color = 0;

	color = (picked_piece_type_ >= 6); // 0 - white, 1 - black
	opposite_color = 1 - color;
	
	clearPickedPieceHelper(color);
	
	//a piece was overtaken
	if (placed_piece_type_ != -1) {
		clearNewSpotHelper(piece_placement, color, opposite_color);
	}

	placeNewPieceHelper(piece_placement, color, opposite_color);

	//adjust all occupancy
	occ_[OCC::ALL_OCC] = occ_[OCC::W_OCC] | occ_[OCC::B_OCC];
}

void BitwiseMoveValidation::clearPickedPieceHelper(int color) 
{
	//clearing the boards for the picked piece
	all_boards_->occupancy[color] &= ~(1ULL << picked_square_idx_);
	all_boards_->pieces[picked_piece_type_] &= ~(1ULL << picked_square_idx_);
	allies_ &= ~(1ULL << picked_square_idx_);
}

void BitwiseMoveValidation::clearNewSpotHelper(uint64_t *piece_placement, int color, int opposite_color)
{
	//passes the value of the overtaken piece to the AI class
	int materialCounts[] = { 1, 3, 3, 5, 9, 0 };
	evaluateThisBoard.materialCount(opposite_color * materialCounts[placed_piece_type_ % 6], color * materialCounts[placed_piece_type_ % 6]);

	//prepares the spot for the new piece to take hold on the board
	all_boards_->occupancy[opposite_color] &= ~*piece_placement;
	all_boards_->pieces[placed_piece_type_] &= ~*piece_placement;
	enemies_ &= ~*piece_placement;
}

void BitwiseMoveValidation::placeNewPieceHelper(uint64_t *piece_placement, int color, int opposite_color)
{
	//places the new piece
	all_boards_->occupancy[color] |= *piece_placement;
	all_boards_->pieces[picked_piece_type_] |= *piece_placement;
	allies_ |= *piece_placement;
}

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
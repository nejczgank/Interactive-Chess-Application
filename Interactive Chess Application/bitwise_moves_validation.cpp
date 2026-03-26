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
}

void BitwiseMoveValidation::determinePickedPiece() {
	//check every board to see if the picked_square_idx is present on that board and compact it

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
				compressed_piece_type_ += i;
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
				compressed_piece_type_ += i + 6;
				allies_ = black_occupancy_;
				enemies_ = white_occupancy_;
				return;
			}
		}
	}
}

void BitwiseMoveValidation::callPieceTypesValidator() {
	directionHelper();
}

void BitwiseMoveValidation::directionHelper() {

	// south ray
	uint64_t halving_mask = (1ULL << picked_square_idx_) - 1;
	uint64_t full_transposed_ray = (0x101010101010101 << picked_square_idx_ % 8);
	uint64_t south = halving_mask & full_transposed_ray;

	//blockers conditional preperation
	uint64_t blockers = white_occupancy_ & south;
	uint64_t has_blockers = -(blockers != 0);
	//uint64_t blocker_halving_mask = (((1ULL << std::countr_zero(blockers) + 1) - 1) & has_blockers) | (((1ULL << std::countr_zero(blockers) + 1) - 1) & ~has_blockers);

	//blockers conditional evaluation
	//uint64_t msb_blocker_mask = (1ULL << (sizeof(blockers) * 8 - std::countl_zero(std::bit_floor(blockers))));
	uint64_t msb_blocker_mask = std::bit_floor(blockers);
	uint64_t msb_halving_mask = ~(msb_blocker_mask - 1);
	//branchless if statement assesing the final ray outcome with or without blockers
	//(condition_a_value & truth_mask) | (condition_b_value & ~truth_mask
	uint64_t adjusted_south = (has_blockers & (msb_halving_mask & south)) | (~has_blockers & south);


	//tole morm popravit ker moj lsb trik ne deluje zaradi napačnega indeksiranja
	//preveri točno kako je že bilo treba pridobiti 
	std::cout << halving_mask;
}

void BitwiseMoveValidation::pawnValidation() {
	
}

void BitwiseMoveValidation::knightValidation() {

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

	std::cout << knight_mask;
}

void BitwiseMoveValidation::rookValidation() {

}

void BitwiseMoveValidation::bishopValidation() {

}

void BitwiseMoveValidation::queenValidation() {

}

void BitwiseMoveValidation::kingValidaiton() {

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
#include "special_move_validation_system.h"

bool SpecialMoveValidationSystem::isKingCheck(InitGameState::Board& board, int color)
{
	//this function runs whenever a piece on the board is moved
	//it determines whether an opposing piece is forcing a check
	//the way it works is by casting rays from the position of the king piece,
	//and checking if the encountered pieces match a corresponding profile,
	//that takes into consideration color as well as piece type
	//**the function also includes guard rails to exit preemptively when a check is discovered
	//**I think this may be more efficient for the ai compared to establishing an attack table
	
	int const color_offset = ((color == 0) ? occupancyInfo::white : occupancyInfo::black) * 6;
	int const opposite_color = (color == 0) ? occupancyInfo::black : occupancyInfo::white;
	int const opposite_color_offset = opposite_color * 6;
	int piece[6] = {
		pieceInfo::pawn + color_offset,
		pieceInfo::knight + color_offset,
		pieceInfo::rook + color_offset,
		pieceInfo::bishop + color_offset,
		pieceInfo::queen + color_offset,
		pieceInfo::king + color_offset
	};
	int enemy_piece[6] = {
		pieceInfo::pawn + opposite_color_offset,
		pieceInfo::knight + opposite_color_offset,
		pieceInfo::rook + opposite_color_offset,
		pieceInfo::bishop + opposite_color_offset,
		pieceInfo::queen + opposite_color_offset,
		pieceInfo::king + opposite_color_offset
	};

	//create a dummy move_data
	MovementData movement_data;

	//assign general movement state
	movement_data.allies = board.occupancy[color];
	int const king_bitboard = (int)board.pieces[piece[pieceInfo::king]]; //converts uint64_t board representation to int num
	movement_data.picked_square_idx = std::countr_zero(static_cast<uint64_t>(king_bitboard)); //converts board value num to index num

	//add knight state, compute knight moves
	movement_data.picked_piece_type = piece[pieceInfo::knight];
	uint64_t valid_moves = MoveValidationSystem::validator(board, movement_data);

	//check if opposing knights are attacking the king
	if ((board.pieces[enemy_piece[pieceInfo::knight]] & valid_moves) != 0)
	{
		return true;
	}

	//add pawn state, compute knight moves
	movement_data.picked_piece_type = piece[pieceInfo::pawn];
	valid_moves = MoveValidationSystem::validator(board, movement_data);

	//check if opposing pawn are attacking the king
	if((board.pieces[enemy_piece[pieceInfo::pawn]] & valid_moves) != 0)
	{
		return true;
	}
	
	//add rook state, compute rook moves
	movement_data.picked_piece_type = piece[pieceInfo::rook];
	movement_data.enemies = board.occupancy[opposite_color];
	valid_moves = MoveValidationSystem::validator(board, movement_data);

	//check if opposing rook or queen is attacking the king
	if
	(
		((board.pieces[enemy_piece[pieceInfo::rook]] | board.pieces[enemy_piece[pieceInfo::queen]]) //rook or queen
		& valid_moves) != 0 //intersect
	)
	{
		return true;
	}

	//add bishop state, compute bishop moves
	movement_data.picked_piece_type = piece[pieceInfo::bishop];
	movement_data.enemies = board.occupancy[opposite_color];
	valid_moves = MoveValidationSystem::validator(board, movement_data);

	//check if opposing bishop or queen is attacking the king
	if
	(
		((board.pieces[enemy_piece[pieceInfo::bishop]] | board.pieces[enemy_piece[pieceInfo::queen]]) //bishop or queen
		& valid_moves) != 0 //intersect
	)
	{
		return true;
	}

	return false;
}

int SpecialMoveValidationSystem::isKingCheckmate() 
{
	//checkmate is determined by moving the king to all available squares and reducing them retroactively
	//by invoking the isKingCheck function for all of them if the king there is under attack
	//**the function also invokes stalemate if only the square the king is currently occupying is safe
	//**clockwise direction of checks

	return 0;
}
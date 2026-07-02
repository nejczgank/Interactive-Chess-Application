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
	
	using enum occupancyInfo::occupancy;
	using enum pieceInfo::piece;

	int const color_offset = ((color == 0) ? white : black) * 6;
	int const opposite_color = (color == 0) ? black : white;
	int const opposite_color_offset = opposite_color * 6;

	int piece[6] = {
		pawn + color_offset,
		knight + color_offset,
		rook + color_offset,
		bishop + color_offset,
		queen + color_offset,
		king + color_offset
	};

	int enemy_piece[6] = {
		pawn + opposite_color_offset,
		knight + opposite_color_offset,
		rook + opposite_color_offset,
		bishop + opposite_color_offset,
		queen + opposite_color_offset,
		king + opposite_color_offset
	};

	//create a dummy move_data
	MovementData movement_data;

	//assign general movement state
	movement_data.allies = board.occupancy[color];
	int64_t const king_bitboard = (int64_t)board.pieces[piece[king]]; //converts uint64_t board representation to int64
	movement_data.picked_square_idx = std::countr_zero(static_cast<uint64_t>(king_bitboard)); //converts board value num to index num

	//add knight state, compute knight moves
	movement_data.picked_piece_type = piece[knight];
	uint64_t valid_moves = MoveValidationSystem::validator(board, movement_data);

	//check if opposing knights are attacking the king
	if ((board.pieces[enemy_piece[knight]] & valid_moves) != 0)
	{
		return true;
	}

	//add pawn state, compute knight moves
	movement_data.picked_piece_type = piece[pawn];
	valid_moves = MoveValidationSystem::validator(board, movement_data);

	//check if opposing pawn are attacking the king
	if((board.pieces[enemy_piece[pawn]] & valid_moves) != 0)
	{
		return true;
	}
	
	//add rook state, compute rook moves
	movement_data.picked_piece_type = piece[rook];
	movement_data.enemies = board.occupancy[opposite_color];
	valid_moves = MoveValidationSystem::validator(board, movement_data);

	//check if opposing rook or queen is attacking the king
	if
	(
		((board.pieces[enemy_piece[rook]] | board.pieces[enemy_piece[queen]]) //rook or queen
		& valid_moves) != 0 //intersect
	)
	{
		return true;
	}

	//add bishop state, compute bishop moves
	movement_data.picked_piece_type = piece[bishop];
	movement_data.enemies = board.occupancy[opposite_color];
	valid_moves = MoveValidationSystem::validator(board, movement_data);

	//check if opposing bishop or queen is attacking the king
	if
	(
		((board.pieces[enemy_piece[bishop]] | board.pieces[enemy_piece[queen]]) //bishop or queen
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
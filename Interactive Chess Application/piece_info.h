#pragma once
namespace pieceInfo
{
	enum piece : int
	{
		//type and color
		white_pawn = 0,
		white_knight = 1,
		white_rook = 2,
		white_bishop = 3,
		white_queen = 4,
		white_king = 5,
		black_pawn = 6,
		black_knight = 7,
		black_rook = 8,
		black_bishop = 9,
		black_queen = 10,
		black_king = 11,

		//types only
		pawn = 0,
		knight = 1,
		rook = 2,
		bishop = 3,
		queen = 4,
		king = 5
	};
}
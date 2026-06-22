#include "board_updating_system.h"

uint64_t BoardUpdatingSystem::movementValidation(MovementData& movement_data, uint64_t& found_moves)
{
	// 0   - invalid move
	// 0 < - valid move

	return found_moves & (1ULL << movement_data.placement_square_idx);
}

void BoardUpdatingSystem::updateBoards(InitGameState::Board& board, MovementData& movement_data,  PositionalEvalComponent& pos_eval_data, uint64_t piece_placement)
{
	int color = 0;
	int opposite_color = 0;

	color = (movement_data.picked_piece_type >= 6); // 0 - white, 1 - black
	opposite_color = 1 - color;

	clearPickedPieceHelper(board, movement_data, color);

	//a piece was overtaken
	if (movement_data.placed_piece_type != -1) {
		clearNewSpotHelper(board, movement_data, pos_eval_data, piece_placement, color, opposite_color);
	}

	placeNewPieceHelper(board, movement_data, piece_placement, color, opposite_color);

	//adjust all occupancy
	board.occupancy[occupancyInfo::all] = board.occupancy[occupancyInfo::white] | board.occupancy[occupancyInfo::black];
}

void BoardUpdatingSystem::clearPickedPieceHelper(InitGameState::Board& board, MovementData& movement_data, int color)
{
	//clearing the boards for the picked piece
	board.occupancy[color] &= ~(1ULL << movement_data.picked_square_idx);
	board.pieces[movement_data.picked_piece_type] &= ~(1ULL << movement_data.picked_square_idx);
	movement_data.allies &= ~(1ULL << movement_data.picked_square_idx);
}

void BoardUpdatingSystem::clearNewSpotHelper(InitGameState::Board& board, MovementData& movement_data, PositionalEvalComponent& pos_eval_data, uint64_t& piece_placement, int color, int opposite_color)
{
	//passes the value of the overtaken piece to the AI system
	int const materialCounts[] = { 1, 3, 3, 5, 9, 0 };

	int const own_material_count = color * materialCounts[movement_data.placed_piece_type % 6];
	int const opposite_material_count = opposite_color * materialCounts[movement_data.placed_piece_type % 6];

	//calculates the value of the capture
	//it's important to do so since the AI will simulate board movement and captures, recursively accessing newer states
	PositionalEvalSystem::materialCount(pos_eval_data, own_material_count, opposite_material_count);

	//prepares the spot for the new piece to take hold on the board
	board.occupancy[opposite_color] &= ~piece_placement;
	board.pieces[movement_data.placed_piece_type] &= ~piece_placement;
	movement_data.enemies &= ~piece_placement;
}

void BoardUpdatingSystem::placeNewPieceHelper(InitGameState::Board& board, MovementData& movement_data, uint64_t& piece_placement, int color, int opposite_color)
{
	//places the new piece
	board.occupancy[color] |= piece_placement;
	board.pieces[movement_data.picked_piece_type] |= piece_placement;
	movement_data.allies |= piece_placement;
}
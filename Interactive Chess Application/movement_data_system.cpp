#include "movement_data_system.h"

void GetMovementInfoSystem::executeMovementInfo(const InitGameState::Board& board, MovementData& movement_data, int sel_movement, int picked_square_idx, int placement_square_idx)
{
	using enum occupancyInfo::occupancy;

	//generic function for obtaining data when a piece moves or for when it is placed
	
	movement_data.picked_square_idx = picked_square_idx;
	movement_data.placement_square_idx = placement_square_idx;

	//sets which index needs to be handled, based on the flag
	const int lookupIdx[] = { picked_square_idx, placement_square_idx };
	const int movementType = lookupIdx[sel_movement];

	//checks for the color of the selected figure
	const int color = ((board.occupancy[white] & (1ULL << movementType)) != 0) ? 0 : 1;
	//calculates the offset needed for the jump table execution
	//used for determining the exact colored piece when assessing valid moves
	const int correction = 6 * color;

	//twos compliment bit masks for determining allied or rival pieces
	//black - empty mask, white - full mask
	const uint64_t if_color = -(color == 0);
	const uint64_t if_flag_option = -(sel_movement == 0);

	//setting allied or rivaled pieces
	movement_data.allies = (if_color & if_flag_option & board.occupancy[white]) | (~if_color & if_flag_option & board.occupancy[black]);
	movement_data.enemies = (if_color & if_flag_option & board.occupancy[black]) | (~if_color & if_flag_option & board.occupancy[white]);

	//preemptive flag for later updating of board state
	//when the variable is set and remains -1, the placed piece didn't overtake any other piece
	movement_data.placed_piece_type = -1;

	//looping all colored pieces, finding the right piece and setting the value
	//of the desired variable (via flag) between 0-11 for the jump table execution
	for (int i = 0; i < 6; i++) {
		if (board.pieces[i + correction] & (1ULL << movementType)) {
			(sel_movement == 0 ? movement_data.picked_piece_type : movement_data.placed_piece_type) = i + correction;
			return;
		}
	}
}




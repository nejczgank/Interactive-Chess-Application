#include "display.h"

char Display::compressed_board_[64];

void Display::displayBoard(InitGameState::Board* board) {
    constructBoard(*board);
    addRanksFiles();
    std::fill(compressed_board_, compressed_board_ + 64, 0); //clear state
}

void Display::constructBoard(InitGameState::Board &board) {

	const int PIECE_TYPES_BOARDS = 12;

    std::unordered_map<int, char> piece_symbol = {
        {0, 'p'},
        {1, 'n'},
        {2, 'r'},
        {3, 'b'},
        {4, 'q'},
        {5, 'k'},
        {6, 'P'},
        {7, 'N'},
        {8, 'R'},
        {9, 'B'},
        {10, 'Q'},
        {11, 'K'},
    };
    
    //member data pointer for generic access of boards
    uint64_t InitGameState::Board::* piece_selectors[] = {
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
            &InitGameState::Board::black_king
    };

	for (int i = 0; i < PIECE_TYPES_BOARDS; i++) {

        uint64_t curr_bitboard = board.*piece_selectors[i];		for (int j = 0; j < 64; j++) {

            if (curr_bitboard == 0) {
                break;
            }
			
            //if current bitboard bit and a shifted bit of 1 equals 1 using an and operator
            if ((curr_bitboard & (1ULL << j)) != 0) {

                //append the appropriate symbol to the compressed array
                Display::compressed_board_[j] = piece_symbol[i];

                //flip the bit off for faster loop exit
                curr_bitboard = curr_bitboard ^ (1Ull << j);
            }
		}
	}
}

void Display::addRanksFiles() {
            
    int square_idx = 56;

    for (int i = 8; i > 0; i--) {

        std::cout << i << "   ";

        for (int j = square_idx; j < (square_idx + 8); j++) {

            if (compressed_board_[j] == 0) {
                std::cout << "  ";
            }
            else {
                std::cout << compressed_board_[j] << " ";
            }
        }
        square_idx -= 8;
        std::cout << '\n';
    }

    char file = 'a';

    std::cout << "\n    ";
    for (int i = 0; i < 8; i++) {
        std::cout << file++ << " ";
    }

    std::cout << '\n';
}



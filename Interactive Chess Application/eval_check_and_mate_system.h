#pragma once
#include <iostream>
#include <cstdint>
#include <bit>
#include <cmath>
#include <algorithm>
#include <iterator>
#include <span>
#include "occupancy_info.h"
#include "init_game_state.h"
#include "move_validation_system.h"
#include "piece_info.h"
#include "board_updating_system.h"
#include "stalemate_data_component.h"
#include "trace_path_component.h"
#include "game_state_component.h"
#include "king_subopt_info.h"
#include "extract_ray_type_info.h"
#include "en_passant_state_component.h"
#include "game_state_component.h"
#include "attack_eval_flags_info.h"
#include "processed_square_info.h"
#include "extract_path_component.h"
#include "check_type_eval_flags_info.h"
#include "eval_board_state_edit_info.h"
#include "check_or_checkmate_block.h"


class EvalCheckAndMateSystem {
public:
	//static void initState(InitGameState::Board&, MovementData&, const uint64_t, StalemateDataComponent&, const int, EvalCheckAndMateComponent&, TracePathComponent& path_data);
	static int checkmateHandler(GameStateContextComponent&, uint64_t);

private:
	//intermediary struct definitions
	struct FoundOccupation
	{
		uint64_t allies{};
		uint64_t enemies{};
	};

	struct PickedPiecesComponent
	{
		int imposed_check_sq_idx{};
		int discovered_check_sq_idx{};
		int processed_sq_idx{};
	};

	struct SwitchTeamsComponent
	{
		uint64_t allies{};
		uint64_t enemies{};
		uint64_t attacker_color{};
		uint64_t defender_color{};
	};

	struct ValidPiecePlacementsContext
	{
		const uint64_t& VALID_PIECE_PLACEMENT;
		const uint64_t& ALLY_KING_MASK;
	};

	struct HoldMovementDataComponent
	{
		int const HOLD_PICKED_IDX{};
		int const HOLD_PICKED_PIECE_TYPE{};
		int const HOLD_PLACEMENT_SQUARE_IDX{};
		int const HOLD_PLACED_PIECE_TYPE{};
	};

	struct ProcessBlockersContext
	{
		const uint64_t& ALLY_KING_MASK;
		uint64_t& unfiltered_blockers_mask;
		const int& PROCESSED_SQ_IDX;
		const uint64_t& PROCESSED_SQ;
	};

	struct KingMovementComponent
	{
		uint64_t king_moves;
		int king_pos;
		uint64_t king_board;
	};

	//determine calculation type
	static inline constexpr int getPieceIndex(const int, const int);
	static PickedPiecesComponent findPickedPieces(const GameStateContextComponent&, const AttackEvalFlagsInfo::eval_flags, const uint64_t);
	static int isSquareAttackedSimulated(GameStateContextComponent&, uint64_t, const AttackEvalFlagsInfo::eval_flags, const uint64_t, ExtractPathComponent&);
	static int simSquareAttack(GameStateContextComponent&, uint64_t, CheckTypeFlagsInfo::eval_flags, int, ExtractPathComponent&);

	//imposed, discovered full calculation
	static inline SwitchTeamsComponent getSwitchedTeamsData(const GameStateContextComponent&);
	static inline void switchTeams(GameStateContextComponent&, const SwitchTeamsComponent&, const uint64_t);
	static int evalSquareAttack(GameStateContextComponent&, uint64_t, CheckTypeFlagsInfo::eval_flags, const int, ExtractPathComponent&, CheckOrCheckmateBlock::eval_flags);
	static int isSquareAttacked(GameStateContextComponent&, const int, ExtractPathComponent&, CheckOrCheckmateBlock::eval_flags);
	static bool isAnyPieceAttacking(GameStateContextComponent&, pieceInfo::piece, ExtractPathComponent&, bool);
	
	//finding eligible blockers when a checkmate occurs
	static bool evalBlockerEligibility(GameStateContextComponent&, uint64_t, const int, const uint64_t, ExtractPathComponent&);
	static uint64_t findBlockers(GameStateContextComponent&, const int);
	static inline void setNewPickedSquareIdx(GameStateContextComponent&, const int);
	static inline void performSwitch(GameStateContextComponent&, SwitchTeamsComponent&);
	static inline uint64_t calcAllyKingMask(const GameStateContextComponent&);
	static bool processBlockers(GameStateContextComponent&, ProcessBlockersContext&, ExtractPathComponent&);
	static inline int calcPickedPieceType(const GameStateContextComponent&, uint64_t);
	static int evalOneBlocker(GameStateContextComponent&, ValidPiecePlacementsContext&, CheckTypeFlagsInfo::eval_flags, const int, ExtractPathComponent&);
	static inline void restoreMovementState(GameStateContextComponent&, SwitchTeamsComponent&, HoldMovementDataComponent&);
	
	//checkmate pipeline
	static int isKingCheckmate(GameStateContextComponent&, const uint64_t);
	static uint64_t getAttackerPath(GameStateContextComponent&);
	static bool isBlockPossible(GameStateContextComponent&, const uint64_t);
	static bool canKingMove(GameStateContextComponent&);
	static KingMovementComponent findKingMoves(GameStateContextComponent&, SwitchTeamsComponent&, HoldMovementDataComponent&);
	static inline void kingMoveReversal(GameStateContextComponent&, HoldMovementDataComponent&);
};


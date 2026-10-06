Ideas and pitfalls that I found out after researching why my code sucks!

LONG PARAMETER LIST:
Passing five separate parameters into a low-level function—especially when three or four of them are always moved around together—is a classic code smell known as a Long Parameter List.
Creating wrapper structs to group related arguments is not just a good idea; it is a fundamental refactoring pattern called Introduce Parameter Object.

there exist two types of parameter bundles: Contexts and Frames

Contexts (reference bundles):

example:
`// A lightweight view of the core state needed for board updates
struct BoardStateContext {
    InitGameState::Board& board;
    MovementData& movement;
    PositionalEvalComponent& pos_eval;
    EnPassantStateComponent& en_passant;
};`

Then your function signature cleanly becomes:

`int isSquareAttacked(BoardStateContext& ctx, uint64_t valid_placement, uint8_t target_sq);`

Frames (deep copies):

example:
`struct BoardStateFrame {
    InitGameState::Board board;
    MovementData movement;
    PositionalEvalComponent pos_eval;
    EnPassantStateComponent en_passant;
};`


If internal methods need convenience access inside member functions or local scopes, C++17 Structured Bindings give you clean, readable aliases without typing auto& repeatedly:
`// Clean unpacking in C++17
auto& [board, movement, pos_eval] = sim_frame;
// Now 'board', 'movement', and 'pos_eval' are direct references to frame members!`

CONTROL COUPLING:
Occurs when functions take in too much context that isn't specifically relevant to their query. At best it makes the code less readable and at worst it completely throws off your entire
architecture when invariants shift. Just as I experienced when designing my isCheckmate function.

example:
Caller ---> passes Mode (1,2,3,4) ---> isCheckmate (Contains massive switch statements inside)

to ensure that our code nevertheless executes as it should (simple query) we dedicate external or internal helpers. Dedicating external helpers touches on another subject tied to
control coupling that is:

FACADE (or orchestrator) PATTERN:
By pulling the lower-level primitives together into a clean, single-purpose workflow we achieve true decoupling, while also respecting the single query principle. This principle makes 
it so the caller doesn't have to manage the setup details.

example:
`[ GOOD: Orchestrator / Wrapper ]
Caller ---> calls checkKingCheck()  ---> [ Wrapper ] ---> 1. Selects King Square
                                                     ---> 2. Calls pure isSquareAttacked(sq_idx)`
The One Thing to Guard Against!
The only way a wrapper function like this causes "harm" is if you let it grow too large or handle too many unrelated jobs (turning it into a "God function").

QUERYING ARCHITECTURE FLOW:
When you're designing low-level systems (like this C++ engine), here is a simple 3-question checklist you can ask yourself every time you're about to add a parameter or create a new 
function:

1. "Is this function answering a question or executing a workflow?"
 -   If it's answering a question (Query): Keep it pure. Give it explicit inputs (square_idx, board_state), let it do the math, and return an answer (bool, uint64_t). It should never 
    care why the caller wants to know.
 -   If it's executing a workflow (Orchestrator): This is where wrappers live. Its job is to gather state, resolve target indices, coordinate helpers, and handle control flow.

2. "Am I passing a parameter that tells the function how to run itself?"
 -   The Flag Rule: If you pass a bool, an enum Mode, or an int type whose only job is to drive an internal switch or if/else block inside the function, stop. That's a control couple.
     The Fix: Split it into explicit functions, or resolve the decision before calling the pure engine function.

3. "Does this function know too much about its caller's context?"
 -   If a low-level function like isSquareAttacked has to know whether it's checking an enemy king vs. a checkmate ray square, it knows too much about the higher-level rules. 
     Keep the low-level functions dumb and fast, and let higher-level wrappers carry the domain context.
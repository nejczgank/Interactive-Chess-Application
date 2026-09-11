//some refresher
//operator precedence is why you use a . when accepting a reference and a -> when accepting a pointer as a parameter
//& - structname.var = value
//* - structname->var = value
//or
//* - *structname.var = value
//what operator precedence means is that the above line will throw an error
//due to . having priority over *
//this makes the compiler read structname.var = value first, throwing an error and then attempting to dereference
//therefore, if we want to use a longer scheme, we have to first explicitly assign precedence, like this:
//(*structname).var = value

//why use a pointer over a reference:
//1. - pointers can be null, making it useful when a parameter is optional
//2. - pointers can change what they point to, for example:
/*
int* lookupIdx[] = { &picked_square_idx_, &placement_square_idx_ };
int* movementPointer = lookupIdx[flag];
*/
//the following code designates an array of values holding memory addresses
//this means that a variable can be used to select one such value
//allowing for logic that enables generic functionality
//3. - pointer arithmetic

//i++ - post-increment (modify after evaluating)
//++i - pre-increment (modify before evaluating)

//post-increment creates a temporary value of i in memory, which can be used for assigning some variable to it
//if that's not necessary, then we're wasting resources, making the pre-increment more viable

//watch out when declaring namespaces inside headerfiles. if you don't use constexpr the linker will collide your constants and crash your program.
//this works for primitives, however non-primitives require the use of the inline keyword that tells the linker to merge all of these variables into a single global variable

namespace moveInfo {
	using Type = int;
	constexpr int picked = 0;
	constexpr int placed = 1;
}

using Type = int - this transforms the namespace into a data type from strictly a data value

now because with dod you have a bunch of structs which you pass as references to functions what you can do to not have to declare struct name each time is the following:
1. struct ref trick

struct MoveCtx {
    const int sq;
    const uint64_t b; // knight bitfield
    const uint64_t not_allies;

    // A fast constructor that automatically extracts the data
    MoveCtx(const MovementData& md) 
        : sq(md.picked_square_idx)
        , b(1ULL << md.picked_square_idx)
        , not_allies(~md.allies) {}
};

uint64_t MoveValidationSystem::knightValidation(const MovementData& movement_data)
{
    const MoveCtx c(movement_data; // Unpacks everything instantly
    uint64_t knight_mask = 0;

    // Look at how readable this is. No noise, just raw math.
    knight_mask |= (c.b & not_h) << 17 & c.not_allies;
    knight_mask |= (c.b & not_g & not_h) << 10 & c.not_allies;
    knight_mask |= (c.b & not_g & not_h) >> 6  & c.not_allies;
    knight_mask |= (c.b & not_h) >> 15 & c.not_allies;

    knight_mask |= (c.b & not_a) >> 17 & c.not_allies;
    knight_mask |= (c.b & not_b & not_a) >> 10 & c.not_allies;
    knight_mask |= (c.b & not_b & not_a) << 6  & c.not_allies;
    knight_mask |= (c.b & not_a) << 15 & c.not_allies;

    return knight_mask;
}

2. structural binding
uint64_t MoveValidationSystem::knightValidation(const MovementData& movement_data)
{
    // Assuming MovementData layout is: struct { int picked_square_idx; uint64_t allies; ... }
    const auto& [sq, allies] = movement_data; 
    
    const uint64_t knight = 1ULL << sq;
    const uint64_t not_allies = ~allies;

    // ... your bitwise math using 'knight' and 'not_allies' ...
}

better naming conventions:
-don't describle what a variable does in all it's technicallity, ensure it fits into the context

three types of 64 bit unsigned integers, to write better structured code:

1. SELECTORS (Type 3: Pure 0x0 or 0xF... masks acting as 'if' statements)
   -> Prefix: 'if_', 'sel_'
   -> Examples: if_forward_dir, if_only_one_blocker, if_diagonal, sel_bitfield_option
sel (selector) is similar to if, but sel has broader meaning and I use it to denote extraction from bitfields
or if I find any other meaning later I'll wrap it under sel.
if properly portrays control flow for me

2. INTERMEDIARY STATE (Type 2: Bitmasks representing lines or shapes of squares)
   -> Prefix: 'mask_'
   -> Examples: mask_file_a

3. GAME STATE BITBOARDS (Type 1: Bitmasks representing actual piece placements)
   -> Prefix: 'bb_'
   -> Examples: bb_enemies, bb_allies, bb_pawn_attacks


LAMBDAS:
auto lambdaName = [ capture ]( parameters ){ body };
- capture : obtains local variables
  there are a few types of captures available:
  - [&var]    -> by reference. enables direct modification
  - [var]     -> by value. it's read only by default
  - [&] / [=] -> captures all locally scoped variables by reference or value
  - !!! member variables of structs cannot be extracted separately
- parameters : obtains variable parameters for figuring out the result
- body : contains the necessary code
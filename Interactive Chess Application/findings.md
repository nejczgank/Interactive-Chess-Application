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
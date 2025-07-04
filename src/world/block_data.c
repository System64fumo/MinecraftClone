#include "world.h"

// TODO: Load this from a file

// Type, Translucent, Face textures
// t,	t,		   f,f,f,f,f,f
uint8_t block_data[MAX_BLOCK_TYPES][8] = {
	[0 ... 255] = {0, 0, 31, 31, 31, 31, 31, 31},	// Null
	[0] =  {BTYPE_REGULAR,	1, 0,   0,   0,   0,   0,   0  },	// Air
	[1] =  {BTYPE_REGULAR,	0, 3,   3,   3,   3,   3,   3  },	// Dirt
	[2] =  {BTYPE_REGULAR,	0, 4,   4,   4,   4,   3,   1  },	// Grass
	[3] =  {BTYPE_REGULAR,	0, 2,   2,   2,   2,   2,   2  },	// Stone
	[4] =  {BTYPE_REGULAR,	0, 17,  17,  17,  17,  17,  17 },	// Cobblestone
	[5] =  {BTYPE_REGULAR,	0, 5,   5,   5,   5,   5,   5  },	// Planks
	[6] =  {BTYPE_CROSS,	1, 16,  16,  16,  16,  16,  16 },	// Sapling
	[7] =  {BTYPE_REGULAR,	0, 18,  18,  18,  18,  18,  18 },	// Bedrock
	[8] =  {BTYPE_LIQUID,	1, 206, 206, 206, 206, 206, 206},	// Flowing water
	[9] =  {BTYPE_LIQUID,	1, 206, 206, 206, 206, 206, 206},	// Stationary water
	[10] = {BTYPE_LIQUID,	0, 238, 238, 238, 238, 238, 238},	// Flowing lava
	[11] = {BTYPE_LIQUID,	0, 238, 238, 238, 238, 238, 238},	// Stationary lava
	[12] = {BTYPE_REGULAR,	0, 19,  19,  19,  19,  19,  19 },	// Sand
	[13] = {BTYPE_REGULAR,	0, 20,  20,  20,  20,  20,  20 },	// Gravel
	[14] = {BTYPE_REGULAR,	0, 33,  33,  33,  33,  33,  33 },	// Gold Ore
	[15] = {BTYPE_REGULAR,	0, 34,  34,  34,  34,  34,  34 },	// Iron Ore
	[16] = {BTYPE_REGULAR,	0, 35,  35,  35,  35,  35,  35 },	// Coal Ore
	[17] = {BTYPE_REGULAR,	0, 21,  21,  21,  21,  22,  22 },	// Wood
	[18] = {BTYPE_LEAF,		1, 53,  53,  53,  53,  53,  53 },	// Leaves
	[19] = {BTYPE_REGULAR,	0, 49,  49,  49,  49,  49,  49 },	// Sponge
	[20] = {BTYPE_REGULAR,	1, 50,  50,  50,  50,  50,  50 },	// Glass
	[21] = {BTYPE_REGULAR,	0, 161, 161, 161, 161, 161, 161},	// Lapis ore
	[22] = {BTYPE_REGULAR,	0, 145, 145, 145, 145, 145, 145},	// Lapis block
	[23] = {BTYPE_REGULAR,	0, 47,  46,  46,  46,  63 , 63 },	// Dispenser
	[24] = {BTYPE_REGULAR,	0, 193, 193, 193, 193, 209, 177},	// Sandstone
	[25] = {BTYPE_REGULAR,	0, 75,  75,  75,  75,  75,  76 },	// Noteblock
	[26] = {BTYPE_SLAB,		1, 151, 0,   151, 150, 5,   135},	// Bed (Part 1)
	[35] = {BTYPE_REGULAR,	0, 65,  65,  65,  65,  65,  65 },	// White Wool
	[37] = {BTYPE_CROSS,	1, 14,  14,  14,  14,  14,  14 },	// Dandelion
	[38] = {BTYPE_CROSS,	1, 13,  13,  13,  13,  13,  13 },	// Blue flower
	[39] = {BTYPE_CROSS,	1, 30,  30,  30,  30,  30,  30 },	// Brown mushroom
	[40] = {BTYPE_CROSS,	1, 29,  29,  29,  29,  29,  29 },	// Red mushroom
	[41] = {BTYPE_REGULAR,	0, 24,  24,  24,  24,  24,  24 },	// Gold block
	[42] = {BTYPE_REGULAR,	0, 23,  23,  23,  23,  23,  23 },	// Iron block
	[43] = {BTYPE_REGULAR,	0, 6,   6,   6,   6,   7,   7  },	// Full Slab
	[44] = {BTYPE_SLAB,		1, 6,   6,   6,   6,   7,   7  },	// Slab
	[45] = {BTYPE_REGULAR,	0, 8,   8,   8,   8,   8,   8  },	// Bricks
	[46] = {BTYPE_REGULAR,	0, 9,   9,   9,   9,   11,  10 },	// TNT
	[47] = {BTYPE_REGULAR,	0, 36,  36,  36,  36,  5,   5  },	// Bookshelf
	[48] = {BTYPE_REGULAR,	0, 37,  37,  37,  37,  37,  37 },	// Mossy cobble
	[49] = {BTYPE_REGULAR,	0, 38,  38,  38,  38,  38,  38 },	// Obsidian
	[51] = {BTYPE_CROSS,	1, 32,  32,  32,  32,  32,  32 },	// Fire
	[52] = {BTYPE_REGULAR,	0, 66,  66,  66,  66,  66,  66 },	// Spawner
	[56] = {BTYPE_REGULAR,	0, 51,  51,  51,  51,  51,  51 },	// Diamond ore
	[57] = {BTYPE_REGULAR,	0, 25,  25,  25,  25,  25,  25 },	// Diamond block
	[58] = {BTYPE_REGULAR,	0, 60,  61,  60,  61,  5,   44 },	// Crafting table
	[61] = {BTYPE_REGULAR,	0, 45,  46,  46,  46,  63,  63 },	// Furnace
	[62] = {BTYPE_REGULAR,	0, 62,  46,  46,  46,  63,  63 },	// Furnace Lit
	[73] = {BTYPE_REGULAR,	0, 52,  52,  52,  52,  52,  52 },	// Redstone ore
	[74] = {BTYPE_REGULAR,	0, 52,  52,  52,  52,  52,  52 },	// Redstone ore
	[79] = {BTYPE_REGULAR,	0, 68,  68,  68,  68,  68,  68 },	// Ice
	[80] = {BTYPE_REGULAR,	0, 67,  67,  67,  67,  67,  67 },	// Snow block
	[82] = {BTYPE_REGULAR,	0, 73,  73,  73,  73,  73,  73 },	// Clay block
	[84] = {BTYPE_REGULAR,	0, 75,  75,  75,  75,  75,  76 },	// Jukebox
	[86] = {BTYPE_REGULAR,	0, 120, 119, 119, 119, 103, 103},	// Pumpkin
	[87] = {BTYPE_REGULAR,	0, 104, 104, 104, 104, 104, 104},	// Netherrack
	[88] = {BTYPE_REGULAR,	0, 105, 105, 105, 105, 105, 105},	// Soul sand
	[89] = {BTYPE_REGULAR,	0, 106, 106, 106, 106, 106, 106},	// Glowstone
	[91] = {BTYPE_REGULAR,	0, 121, 119, 119, 119, 103, 103},	// Jack o lantern
	[95] = {BTYPE_REGULAR,	0, 28,  27,  27,  27,  26,  26 },	// Chest
};
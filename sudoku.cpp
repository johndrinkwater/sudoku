#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if __GNUC__
#include <inttypes.h>
typedef uint64_t u64;
typedef uint16_t u16;
typedef uint8_t u8;
inline int popcount(u16 x) { return __builtin_popcount(x); }
inline int bitscan(u64 x) { return __builtin_ctzl(x); }
#elif _WIN32
/* are these right? + fixup for bool + qsort maybe? */
#include <intrin.h>
typedef unsigned __int64 u64;
typedef unsigned __int16 u16;
typedef unsigned __int8 u8;
inline int popcount(u16 x) { return __popcnt(x); }
inline int bitscan(u64 x) { unsigned long i; _BitScanForward64(&i,x); return i; }
#endif

#define SKIPPABLE (1<<15)
#define BOARDSIZE 9*9
/* after removing duplicates + self, this is the remaining peers to test */
#define PEERSSIZE 9+9+2
u16 board[ BOARDSIZE ];
u16 foundboard[ BOARDSIZE ];
u16 tests[ BOARDSIZE ][ 3 ][ 9 ];
u16 peers[ BOARDSIZE ][9*3];
bool discovered = false;
const char *puzzle =
/*
	"4.....8.5.3..........7......2.....6.....8.4......1.......6.3.7.5..2.....1.4......";
*/
/* Quick */
/*
	"..3.2.6..9..3.5..1..18.64....81.29..7.......8..67.82....26.95..8..2.3..9..5.1.3..";
*/
/* Very hard */
/*
	"85...24..72......9..4.........1.7..23.5...9...4...........8..7..17..........36.4.";
*/
/* Long duration */
	".....6....59.....82....8....45........3........6..3.54...325..6..................";


int compare_u16 ( const void *a, const void *b ) {
	const u16 *da = (const u16 *) a;
	const u16 *db = (const u16 *) b;
	return (*da > *db) - (*da < *db);
}

void SetupConstraints( ) {

	u16 firstsq[9] = { 0, 3, 6, 27, 30, 33, 54, 57, 60 };
	u16 offsets[9] = { 0, 1, 2, 9, 10, 11, 18, 19, 20 };
	u16 pi = 0;

	for ( u16 c = 0; c < BOARDSIZE; c++ ) {
		u16 col = c % 9;
		u16 row = c - col;
		pi = 0;
		u16 sq = ( (c / 27) * 3) + ( col / 3 );

		for ( u16 k = 0; k < 9; k++ ) {

			peers[c][pi++] = tests[c][0][k] = row + k;
			peers[c][pi++] = tests[c][1][k] = col + (k * 9);
			peers[c][pi++] = tests[c][2][k] = firstsq[sq] + offsets[k];
		}
		qsort( peers[c], 9*3, sizeof(u16), compare_u16 );
		pi = 0;
		for (u16 k = 0; k < 9*3; k++ ) {
			if (peers[c][k] == c) {	continue; }
			if ( peers[c][k] != peers[c][pi] ) { peers[c][++pi] = peers[c][k]; }
		}
		if ( c == 0 ) {
			/* above compaction fails for the first entry of c == 0 */
			for (u16 k = 0; k < PEERSSIZE; k++ ) {
				peers[c][k] = peers[c][k+1];
			}
		}
	}
}

inline u16 valuefromtile( char tile ) {

	switch( tile ) {
		case '.': return 511;
		case '1': return 1<<0;
		case '2': return 1<<1;
		case '3': return 1<<2;
		case '4': return 1<<3;
		case '5': return 1<<4;
		case '6': return 1<<5;
		case '7': return 1<<6;
		case '8': return 1<<7;
		case '9': return 1<<8;
		default:  return 511;
	}
}

inline char tilevalue( u16 tile ) {

	switch( tile & ~SKIPPABLE ) {
		case 1<<0: return '1';
		case 1<<1: return '2';
		case 1<<2: return '3';
		case 1<<3: return '4';
		case 1<<4: return '5';
		case 1<<5: return '6';
		case 1<<6: return '7';
		case 1<<7: return '8';
		case 1<<8: return '9';
		default:   return '.';
	}
}

inline void tilevaluemulti( u16 tile, char* string ) {

	u8 len = 0;
	if ( tile & ( 1<<0 ) ) { string[len++] = '1'; }
	if ( tile & ( 1<<1 ) ) { string[len++] = '2'; }
	if ( tile & ( 1<<2 ) ) { string[len++] = '3'; }
	if ( tile & ( 1<<3 ) ) { string[len++] = '4'; }
	if ( tile & ( 1<<4 ) ) { string[len++] = '5'; }
	if ( tile & ( 1<<5 ) ) { string[len++] = '6'; }
	if ( tile & ( 1<<6 ) ) { string[len++] = '7'; }
	if ( tile & ( 1<<7 ) ) { string[len++] = '8'; }
	if ( tile & ( 1<<8 ) ) { string[len++] = '9'; }
	/* if ( tile & SKIPPABLE ) { string[len++] = 'S'; } */
	string[len] = '\0';
}

void PrintBoard( u16 *board, bool multi = false ) {

	u16 max = 0;
	if ( multi == true ) {
		for ( u16 i = 0; i < BOARDSIZE; i++ ) {
			if ( popcount( board[i] & ~SKIPPABLE ) > max ) { max = popcount( board[i] & ~SKIPPABLE ); }
			if ( max == 9 ) { break; }
		}
	}
	for ( int i = 0; i < BOARDSIZE; i++ ) {

		if ( multi == true ) {
			char tilestring[10];
			tilevaluemulti( board[i], tilestring );
			printf ( "%*s ", max, tilestring );
		} else {
			printf( "%c ", tilevalue( board[i] ) );
		}
		if ( ( i % 9 == 2) | ( i % 9 == 5) ) { printf( "| " ); }
		if ( i % 9 == 9 - 1 ) { printf( "\n" ); }
	}
}

inline void ReadPuzzle( u16 *board, char *input ) {

	for ( int i = 0; i < BOARDSIZE; i++ ) {
		board[i] = valuefromtile( input[i] );
	}
}

bool PushAndCheckChange( u16 *board, u16 cell, u16 value, u16* changes ) {

	u16 tilemask = 0;
	u16 tile = 0;
	u16 requested = 0;

	board[ cell ] = value;
	requested = popcount( value );

	if ( requested == 1 ) {
		board[ cell ] |= SKIPPABLE;
	} else if ( requested == 0 ) {
		return false;
	} else {
		return true; /* multivalue updates require no updates */
	}

	for ( u16 k = 0; k < PEERSSIZE; k++ ) {

		if ( board[ peers[ cell ][k] ] & SKIPPABLE ) { continue; }

		board[ peers[ cell ][k] ] &= ~value;

		/* to avoid some cycles
		if ( popcount( board[ peers[ cell ][k] ] ) == 1 ) {
			board[ peers[ cell ][k] ] |= SKIPPABLE;
		} */

		/* to avoid some cycles
		if ( ! PushAndCheckChange( board, k, board[ peers[ cell ][k] ] & ~value ) ) { return false; }
		*/
	}

	for ( u16 x = 0; x < 2; x++ ) {
		tilemask = 0;
		for ( u16 k = 0; k < 9; k++ ) {
			/* 0 row 1 col 2 sqs */
			tile = board[ tests[ cell ][x][k] ] & ~SKIPPABLE;
			if ( !tile ) { return false; }
			if ( popcount( tile ) == 1 ) {
				if ( tilemask & tile ) { return false; }
				tilemask |= tile;
			}
		}
	}
	return true;
}

inline u16 SetupBoard( u16 *board ) {

	u16 changes = 0;
	for ( u16 c = 0; c < BOARDSIZE; c++ ) {

		PushAndCheckChange( board, c, board[c], &changes );
	}
	return changes;
}

bool SolveRecursive( u16* board ) {

	u16 *boards[9];
	u16 mask = 0;
	u16 requested = 0;
	u16 *changes = 0;
	u16 cell = 0;
	u16 min = 10;

	for ( u16 c = 0; c < BOARDSIZE; c++ ) {

		if ( board[ c ] & SKIPPABLE ) { continue; }
		u16 count = popcount( board[ c ] );
		/*	we sadly have this because it really improves perf for some trade off:
			slightly incorrect data in board - which gets resolved in future recursions. */
		if ( count == 1 ) {
			board[ c ] |= SKIPPABLE;
			continue;
		}
		if ( count < min ) {
			cell = c; min = count;
		}
		if ( count == 2 ) {
			break;
		}
	}

	if ( min >= 10 ) {
		discovered = true;
		memcpy( foundboard, board, BOARDSIZE * sizeof( u16 ) );
		return true;
	}

	for ( u16 c = 0; c < min; c++ ) {
		boards[c] = ( u16 *)malloc( BOARDSIZE * sizeof( u16 ) );
		memcpy( boards[c], board, BOARDSIZE * sizeof( u16 ) );

		requested = 1 << ( bitscan( board[cell] & ~mask ) );
		if ( PushAndCheckChange( boards[c], cell, requested, changes ) ) { SolveRecursive( boards[c] ); }
		mask |= requested;

		free( boards[c] );
		if ( discovered ) { return true; }
	}
	return false;
}

int main ( int argc, char **argv ) {

	SetupConstraints( );

	ReadPuzzle( board, ( char* )puzzle );
	PrintBoard( board );

	u16 changes = 0;
	do {
		changes = SetupBoard( board );
	} while ( changes > 0 );

	SolveRecursive( board );

	if ( discovered ) {
		printf( "Solution found\n" );
		PrintBoard( foundboard );
	} else {
		printf( "This seems to be impossible. Did you input faulty data?\n" );
	}

	return 0;
}

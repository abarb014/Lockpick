#ifndef LOCKPICK_H
#define LOCKPICK_H

/* Remember to add an srand() somewhere here to ensure that the game STAYS RANDOM. */

#include <stdlib.h>

#define POT_LOW 0x01F
#define POT_HIGH 0x3F0
#define POT_RANGE (0x3F0 - 0x01F)

/* Main structure for a level of the lock pick game. */
typedef struct level
{
	unsigned char difficulty;			// Measure of how tough a level is. Valid values are 1 (easiest) to 5 (hardest).
	unsigned char sections;				// How many different areas the potentiometer will be divided into. Valid values are 2 to 10.
	unsigned char winSection;			// Winning section in the range of sections above.
} level;

/* Changes a given level into a new level that is generated
   based on a given difficulty. */
void GenerateLevel(unsigned char diff, level* current_level)
{
	// Difficulty can only be from 1 - 5.
	unsigned char temp_diff = diff;
	if (temp_diff > 5)
		temp_diff = 5;
	else if (temp_diff == 0)
		temp_diff = 1;
	current_level->difficulty = temp_diff;
	
	// Sections are twice the difficulty. Range is 2 - 10.
	current_level->sections = temp_diff * 2;
	
	// Generate a random winning section.
	current_level->winSection = rand() % current_level->sections + 1;
}

/* Tests a selected input and returns 1 if the test is on a winning section, or
   it returns 0 if it is on a losing section. */
unsigned char TestSelection(unsigned short selection, level current_level)
{
    // Find the winning range
    unsigned short slice = POT_RANGE / current_level.sections;

    unsigned short win_low = POT_LOW + ((current_level.winSection - 1) * slice);
    unsigned short win_high = POT_LOW + (current_level.winSection * slice);

    if ( (selection >= win_low) && (selection <= win_high) )
        return 1;
    else
        return 0;
}

#endif


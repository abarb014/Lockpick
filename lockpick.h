#ifndef LOCKPICK_H
#define LOCKPICK_H

/* Remember to add an srand() somewhere here to ensure that the game STAYS RANDOM. */

#include <stdlib.h>
#include "io.c"

#define POT_LOW 0x01F
#define POT_HIGH 0x3F1
#define POT_RANGE (0x3F1 - 0x01F)

/* Main structure for a level of the lock pick game. */
typedef struct level
{
	/* Game control variables */
	unsigned char difficulty;			// Measure of how tough a level is. Valid values are 1 (easiest) to 5 (hardest).
	unsigned char stage;				// The level the user is currently on.
	unsigned char sections;				// How many different areas the potentiometer will be divided into. Valid values are 2 to 10.
	unsigned char winSection;			// Winning section in the range of sections above.
	unsigned char screen;				// Keeps track of what screen to display.
	
	/* User variables */
	unsigned char bobbyPins;			// Keeps track of the user's remaining lives.
	unsigned char beatStage;			// 1 if the user passed the current stage, 0 if failure.
	
	/* Signals */
	unsigned char updateScreen;			// Signal used to update the screen. Surprisingly.
} level;

/* Initializes the game structure with values that can be immediately
   modified by the generate level function. */
void InitLevel(level* current_level)
{
	current_level->difficulty = 0;
	current_level->stage = 0;
	current_level->sections = 0;
	current_level->winSection = 0;
	current_level->screen = 0;
	
	current_level->bobbyPins = 0;
	current_level->beatStage = 0;
	
	current_level->updateScreen = 1;
}

/* Changes a given level into a new level that is generated
   based on a given difficulty. */
void GenerateLevel(level* current_level)
{
	// Difficulty can only be from 1 - 5.
	current_level->difficulty += 1;
	
	// Sections are twice the difficulty. Range is 2 - 10.
	current_level->sections = current_level->difficulty * 2;
	
	// Generate a random winning section.
	current_level->winSection = rand() % current_level->sections + 1;
	
	// Go to the next stage.
	current_level->stage += 1;
	
	// Reset lives.
	current_level->bobbyPins = 5;
	
	// Go to the display level screen.
	current_level->screen = 1;
	
	// Signal the game logic to refresh the screen.
	current_level->updateScreen = 1;
	
	// Tell the game that the stage isn't won yet.
	current_level->beatStage = 0;
}

/* Tests a selected input and returns 1 if the test is on a winning section, or
   it returns 0 if it is on a losing section. */
void TestSelection(unsigned short selection, level* current_level)
{
    unsigned short slice = POT_RANGE / current_level->sections;

    unsigned short win_low = POT_LOW + ((current_level->winSection - 1) * slice);
    unsigned short win_high = POT_LOW + (current_level->winSection * slice);

	unsigned char win;
    if ( (selection >= win_low) && (selection <= win_high) )
        win = 1;
    else
        win = 0;
		
	// Change data depending on a win or lose.
	if (!win)
	{
		current_level->bobbyPins -= 1;
	}
	
	current_level->beatStage = win;
}

/* Changes the LCD to display a new message, but only once. */
void ChangeScreen(level* current_level)
{
	if (current_level->updateScreen == 1)
	{
		LCD_ClearScreen();
		
		// Welcome
		if (current_level->screen == 0)
		{
			LCD_DisplayString(1, "    Welcome       to Lockpick  ");
		}
		
		// Next level
		else if (current_level->screen == 1)
		{
			LCD_DisplayString(6, "Level");
			LCD_Cursor(25);
			LCD_WriteData(current_level->stage + '0');
		}
		
		// Waiting for user input
		else if (current_level->screen == 2)
		{
			LCD_DisplayString(1, "Level: ");
			LCD_WriteData(current_level->stage + '0');
			LCD_AppendString(17, "Bobby Pins: ");
			LCD_WriteData(current_level->bobbyPins + '0');
		}
		
		// Level complete
		else if (current_level->screen == 3)
		{
			LCD_DisplayString(7, "You");
			LCD_AppendString(23, "Win!");
		}
		
		// Game over
		else if (current_level->screen == 4)
		{
			LCD_DisplayString(7, "You");
			LCD_AppendString(22, "Lose!");
		}
		
		// This shouldn't ever display
		else
		{
			LCD_DisplayString(1, "Error Code: 0x01");
		}
		
		current_level->updateScreen = 0;
	}
}

/* Signals the game logic to update the screen. */
void ResetScreen(level* current_level)
{
	current_level->updateScreen = 1;
}

/* Changes the screen variable to a specified screen. */
void GotoScreen(unsigned char i, level* current_level)
{
	current_level->screen = i;
	current_level->updateScreen = 1;
}

/* Checks to see if the game has been won. */
unsigned char CheckWin(level* current_level)
{
	if (current_level->beatStage)
		return 1;
	else
		return 0;
}

/* Checks to see if the player is alive or not. */
unsigned char Alive(level* current_level)
{
	if (current_level->bobbyPins > 0)
		return 1;
	else
		return 0;
}

#define C4 261.63
#define D4 293.66
#define E4 329.63
#define F4 349.23
#define G4 392.00
#define A4 440.00
#define B4 493.88
#define C5 523.25
#define REST 0

/* PWM Code */
//NOTE*** THIS NEW CODE TARGETS PB6 NOT PB3

void set_PWM(double frequency) {
	
	
	// Keeps track of the currently set frequency
	// Will only update the registers when the frequency
	// changes, plays music uninterrupted.
	static double current_frequency;
	if (frequency != current_frequency) {

		if (!frequency) TCCR3B &= 0x08; //stops timer/counter
		else TCCR3B |= 0x03; // resumes/continues timer/counter
		
		// prevents OCR3A from overflowing, using prescaler 64
		// 0.954 is smallest frequency that will not result in overflow
		if (frequency < 0.954) OCR3A = 0xFFFF;
		
		// prevents OCR3A from underflowing, using prescaler 64					// 31250 is largest frequency that will not result in underflow
		else if (frequency > 31250) OCR3A = 0x0000;
		
		// set OCR3A based on desired frequency
		else OCR3A = (short)(8000000 / (128 * frequency)) - 1;

		TCNT3 = 0; // resets counter
		current_frequency = frequency;
	}
}

void PWM_on() {
	TCCR3A = (1 << COM3A0);
	// COM3A0: Toggle PB6 on compare match between counter and OCR3A
	TCCR3B = (1 << WGM32) | (1 << CS31) | (1 << CS30);
	// WGM32: When counter (TCNT3) matches OCR3A, reset counter
	// CS31 & CS30: Set a prescaler of 64
	set_PWM(0);
}

void PWM_off() {
	TCCR3A = 0x00;
	TCCR3B = 0x00;
}

/* Songs used in the game. */
unsigned char current_note;
unsigned char current_time;
unsigned char timer;

double welcome_notes[] = {F4, REST, C4, REST, C4, D4, E4, F4, REST, C4, REST, F4, -1};
double welcome_times[] = {1,    1,   1,   1,   1,  1,  1,  1,   1,   1,   1,   1};
	
double win_notes[] = {A4, REST, A4, REST, C5, REST, C5, REST, G4, REST, G4, A4, -1};
double win_times[] = {1,  1,     1,    1,  1,    1,  1,    1,  1,    1,  1,   1};

double lose_notes[] = {F4, REST, F4, REST, D4, REST, G4, REST, F4, REST, D4, F4, REST, F4, REST, D4, REST, G4, REST, F4, REST, D4, -1};
double lose_times[] = {1,     1,  1,    1,  1,    1,   1,    1,  6,  1, 6, 1,     1,  1,    1,  1,    1,   1,    1,  6, 1, 6};
#endif

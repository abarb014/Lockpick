#include <avr/io.h>
#include "lockpick.h"
#include "timer.h"

void ADC_init()
{
	ADCSRA |= (1 << ADEN) | (1 << ADSC) | (1 << ADATE);
}

typedef struct task
{
	int state;
	unsigned long period;
	unsigned long elapsedTime;
	int (*TickFct)(int);
} task;

task tasks[4];

unsigned char tasksNum = 4;
unsigned long tasksPeriodGCD = 100;
unsigned long periodPotSample = 100;
unsigned long periodGameLogic = 200;
unsigned long periodGameSound = 100;
unsigned long periodUpdateScreen = 200;

// Shared Variables
level current_level;

unsigned short potLoc = 0x000;
unsigned short old_potLoc = 0x000;

enum PS_States {PS_start, PS_sample};
int TickFct_PotSample(int state);

enum GL_States {GL_start, GL_init, GL_welcome, GL_genLevel, GL_dispLevel, GL_wait, GL_test, GL_win, GL_gameOver, GL_reset};
int TickFct_GameLogic(int state);

enum US_States {US_start, US_update};
int TickFct_UpdateScreen(int state);

enum GS_States {GS_start, GS_update};
int TickFct_GameSound(int state);

int main(void)
{
	DDRA = 0xC6; PORTA = 0x39;
	DDRB = 0xFF; PORTB = 0x00;
	DDRC = 0xFF; PORTC = 0x00;
	DDRD = 0xFF; PORTD = 0x00;
	
	unsigned char i = 0;
	
	tasks[i].state = PS_start;
	tasks[i].period = periodPotSample;
	tasks[i].elapsedTime = periodPotSample;
	tasks[i].TickFct = &TickFct_PotSample;
	i++;
	tasks[i].state = GL_start;
	tasks[i].period = periodGameLogic;
	tasks[i].elapsedTime = periodGameLogic;
	tasks[i].TickFct = &TickFct_GameLogic;
	i++;
	tasks[i].state = US_start;
	tasks[i].period = periodUpdateScreen;
	tasks[i].elapsedTime = periodUpdateScreen;
	tasks[i].TickFct = &TickFct_UpdateScreen;
	i++;
	tasks[i].state = GS_start;
	tasks[i].period = periodGameSound;
	tasks[i].elapsedTime = periodGameSound;
	tasks[i].TickFct = &TickFct_GameSound;
	i++;
	
	LCD_init();
	LCD_ClearScreen();
	ADC_init();
	TimerSet(tasksPeriodGCD);
	PWM_on();
	TimerOn();
	
	srand(ADC);
	
	while(1)
	{
		for (unsigned char i = 0; i < tasksNum; i++)
		{
			if (tasks[i].elapsedTime >= tasks[i].period)
			{
				tasks[i].state = tasks[i].TickFct(tasks[i].state);
				tasks[i].elapsedTime = 0;
			}
			
			tasks[i].elapsedTime += tasksPeriodGCD;
		}

		while (!TimerFlag);
		TimerFlag = 0;
	}
	
	return 0;
}

int TickFct_PotSample(int state)
{
	switch (state)
	{
		case PS_start:
			state = PS_sample;
			break;
		case PS_sample:
			state = PS_sample;
			break;
		default:
			state = PS_start;
			break;
	}
	
	switch (state)
	{
		case PS_sample:
			old_potLoc = potLoc;
			potLoc = ADC;
			if (potLoc < 0x01F)
				potLoc = 0x01F;
			else if (potLoc > 0x3F1)
				potLoc = 0x3F1;
			break;
		default:
			break;
	}
	
	return state;
}

int TickFct_GameLogic(int state)
{
	/* Local variables */
	static unsigned char i = 0;
	static unsigned char stage_flag = 1;
	
	/* Input collection */
	unsigned char input = ~PINA;
	unsigned char A3 = input & 0x08;
	// unsigned char A4 = input & 0x10;

	switch (state)
	{
		case GL_start:
			state = GL_init;
			break;
		case GL_init:
			PORTC = 1;
			state = GL_welcome;
			break;
		case GL_welcome:
			PORTC = 2;
			if (i == 15)
				state = GL_genLevel;
			break;
		case GL_genLevel:
			PORTC = 3;
			state = GL_dispLevel;
			break;
		case GL_dispLevel:
			PORTC = 4;
			if (i == 15)
				state = GL_wait;
			break;
		case GL_wait:
			PORTC = 5;
			current_note = 0;
			current_time = 0;
			timer = 0;
			if (A3 && Alive(&current_level))
				state = GL_test;
			else if (!Alive(&current_level))
				state = GL_gameOver;
			break;
		case GL_test:
			PORTC = 6;
			if (CheckWin(&current_level))
				state = GL_win;
			else
				state = GL_wait;
			break;
		case GL_win:
			PORTC = 7;
			if (i == 15)
				state = GL_genLevel;
			break;
		case GL_gameOver:
			PORTC = 8;
			if (A3)
				state = GL_reset;
			else
				state = GL_gameOver;
			break;
		case GL_reset:
			PORTC = 9;
			state = GL_init;
			break;
		default:
			state = GL_start;
			break;
	}
	
	switch (state)
	{
		case GL_init:
			current_note = 0;
			current_time = 0;
			timer = 0;
			InitLevel(&current_level);
			break;
		case GL_welcome:
			i += 1;
			break;
		case GL_genLevel:
			current_note = 0;
			current_time = 0;
			timer = 0;
			i = 0;
			stage_flag = 1;
			GenerateLevel(&current_level);
			break;
		case GL_dispLevel:
			i += 1;
			break;
		case GL_wait:
			if (stage_flag)
			{
				GotoScreen(2, &current_level);
				stage_flag = 0;
			}
			break;
		case GL_test:
			stage_flag = 1;
			TestSelection(potLoc, &current_level);
			break;
		case GL_win:
			if (stage_flag)
			{
				GotoScreen(3, &current_level);
				stage_flag = 0;
				i = 0;
			}
			i += 1;
			break;
		case GL_gameOver:
			if (!stage_flag)
			{
				GotoScreen(4, &current_level);
				stage_flag = 1;
			}
			break;
		case GL_reset:
			i = 0;
			stage_flag = 1;
			break;
		default:
			break;
	}
	
	return state;
}

int TickFct_UpdateScreen(int state)
{
	switch (state)
	{
		case US_start:
			state = US_update;
			break;
		case US_update:
			state = US_update;
			break;
		default:
			state = US_start;
			break;
	}
	
	switch (state)
	{
		case US_update:
			ChangeScreen(&current_level);
			break;
		default:
			break;
	}
	
	return state;
}

int TickFct_GameSound(int state)
{
	static double note;
	static double time;
	
	switch (state)
	{
		case GS_start:
			state = GS_update;
			break;
		case GS_update:
			state = GS_update;
			break;
		default:
			state = GS_start;
			break;
	}
	
	switch (state)
	{
		case GS_update:
			if (current_level.screen == 0)
			{
				note = welcome_notes[current_note];
				time = welcome_times[current_time];
				
				if (note != -1)
				{
					if (timer < time)
					{
						set_PWM(note);
						timer += 1;
					}
					else
					{
						current_note += 1;
						current_time += 1;
						timer = 0;
					}
				}
				else
				{
					set_PWM(0);
				}
			}
			else if (current_level.screen == 3)
			{
				note = win_notes[current_note];
				time = win_times[current_time];
				
				if (note != -1)
				{
					if (timer < time)
					{
						set_PWM(note);
						timer += 1;
					}
					else
					{
						current_note += 1;
						current_time += 1;
						timer = 0;
					}
				}
				else
				{
					set_PWM(0);
				}	
			}
			else if (current_level.screen == 2)
			{
				if (potLoc > old_potLoc + 8)
					set_PWM(440.00);
				else if (potLoc < old_potLoc - 8)
					set_PWM(220.00);
				else
					set_PWM(0);
			}
			else if (current_level.screen == 4)
			{
				note = lose_notes[current_note];
				time = lose_times[current_time];
				
				if (note != -1)
				{
					if (timer < time)
					{
						set_PWM(note);
						timer += 1;
					}
					else
					{
						current_note += 1;
						current_time += 1;
						timer = 0;
					}
				}
				else
				{
					set_PWM(0);
				}
			}
			break;
		default:
			break;
	}
	
	return state;
}
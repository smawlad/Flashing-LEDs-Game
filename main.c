// -------
// main.c
// -------

#include <driverlib.h>
#include <msp430.h>
#include <string.h>
#include "myGpio.h"
#include "myClocks.h"
#include "myLcd.h"

#define RED_ON          0x0001      					// Enable and turn on the red LED
#define RED_OFF         0xFFFE      					// Turn off the red LED
#define GREEN_ON	0x0080						// Enable and turn on the green LED
#define GREEN_OFF	0xFF7F						// Turn off the green LED
#define ENABLE_PINS     0xFFFE      					// Required to use inputs and outputs
#define BUTTON1		0x0002						// P1.1 is button 1
#define BUTTON2		0x0004						// P1.2 is button 2
#define UP 		0x0010						// Timer_A Up mode
#define CONTINUOUS      0x0020          				// Timer_A Continuous mode
#define ACLK		0x0100						// Timer_A SMCLK source
#define DEVELOPMENT 	0x5A80						// Stop the watchdog timer
#define BOUNCE_DELAY	0xA000						// Delay for Button Bounce
#define MS_10		400						// Approximate value to count for 10ms
#define SMCLK		0x0200						// Timer_A SMCLK source


void main (void)
{
	void DisplayMultipleWords(char words[250]);		
	void ScrollWords(char words[250]);
	void DisplayNumber(unsigned long int number);
	void DisplayWord(char words[250]);
	int  delay(int count);
	int  GetRandomSequence(int sequence);
	void ShowSequence(int sequence, int i);
	int  GetUserSequence(int sequence, int gameOver, int i);
	void GameOverMessage(int score);

    int i;												// Used in for loops

    int score;											// Number correct so far

    int sequence;										// Random sequence of 16 LED blinks

    int gameOver;										// Is game over?

    int delayCount;										// Number of 10ms delays needed

    WDTCTL = WDTPW | WDTHOLD;                           // Stop watchdog timer

	initGPIO();											// Initialize GPIO
    initClocks();    									// Initialize clocks
    myLCD_init();										// Initialize Liquid Crystal Display
    PM5CTL0 = ENABLE_PINS;               				// Enable to turn on LEDs

    TA0CTL   = TA0CTL | (SMCLK + CONTINUOUS);			// SMCLK:  Counts faster than ACLK
                                                        // CONTINUOUS:  Count 0 to 0xFFFF
    TA0CCTL0 = CCIE;									// Timer_0 interrupt

    TA1CTL   = TA1CTL | (ACLK  + UP);                   // Count up from 0 with ACLK
    TA1CCR0  = MS_10;									// Duration approximatley 10ms

    _BIS_SR(GIE);										// Activate all interrupts

	while(1)											// Infinite loop
	{
		gameOver = 0;									// Reset gameOver flag back to 0 for new game
		sequence = 0;									// Reset sequence back to 0 for new game
		score    = 0;									// Reset the score back to 0 for new game

		P1OUT = P1OUT | RED_ON;							// Turn on the red LED light
		P9OUT = P9OUT | GREEN_ON;						// Turn on the green LED light

		ScrollWords("PRESS S1 TO BEGIN");				// Scroll message across LCD


		sequence = GetRandomSequence(sequence);			// Get a random sequence of LEDs for game

		while(!gameOver)								// Keep looping while game is playing
		{												//
			for(i=0; i<16; i=i+1)						// This loops 16 times because we show the user
			{											// a sequence of up to 16 LED blinks
				P1OUT = P1OUT & RED_OFF;				// Turn off the red LED light
				P9OUT = P9OUT & GREEN_OFF;				// Turn off the green LED light
				delayCount = 50;						// Delay for (50*10ms) = 500ms
				while(delayCount = delay(delayCount));	// Wait for delay to end

				if(!gameOver)							     // If the game isn't over yet
				{
					DisplayWord("ROUND");				     // Display word to LCD screen
					delayCount = 50;					     // Delay for (50*10ms) = 500ms
					while(delayCount = delay(delayCount));   // Wait for delay to be over
					DisplayNumber(i+1);					     // Display number to LCD screen
					delayCount = 50;					     // Delay for (50*10ms) = 500ms
					while(delayCount = delay(delayCount));   // Wait for delay to be over

					ShowSequence(sequence,i);			     // Show the user a sequence of LEDs

					// User enters sequence of button pushes for LEDs
					gameOver = GetUserSequence(sequence,gameOver,i);

					if(!gameOver)						// If the user answered correctly
					{
						score += 1;						// Increment Score
					}
				} // end if(!gameOver)
				else									// Otherwise, if the game is over
				{
					break;								// Stop the game
				}
			} // end for(i=0; i<16; i=i+1)
			gameOver = 1;								// If the user has played 16 rounds, end the
														// game
		} // end while(!gameOver)

		// Loop in the GAME OVER message until user starts a new game
		GameOverMessage(score);
	} // end while(1)
} // end main


// Timer_0 Interrupt Service Routine
#pragma vector=TIMER0_A0_VECTOR
__interrupt void Timer_A0 (void)
{
	TA0CTL = TA0CTL & (~TAIFG);							// Reset Timer_0 so it keeps counting
}


//***************************************************************************************************
// DisplayWord()
//
// The function displays a single word of up to 6 characters on the LCD screen.  If the word is
// longer than 6 characters, it will display an error message.
//
// This function has one argument and does not return a value.
//
//      Arg 1  - 'words' specifies the text to be displayed on the LCD screen
//***************************************************************************************************
void DisplayWord(char words[250])
{
	void clearLCD(void);								// Declare functions used

	unsigned int i;										// Used to get index of current character
	unsigned int length;								// Keeps track of the length of the word
	char character;										// Current character to be displayed

	clearLCD();											// Make sure that the LCD screen is blank
	length = strlen(words);								// Get the length of the desired word
	if (length<=6)										// If the word has 6 or less characters
	{
		for(i = 1;i<=length;i=i+1)						// Loop through all of the LCD locations
		{
			character = words[i-1];						// Get the current letter for current location
			if(character)								// If the character exists
			{
				myLCD_showChar(character,i);			// Show the character on the LCD screen
			} // end if(character)
		} // end for(i = 1;i<=6;i++)
	} // end if (length<=6)
	else												// If the word has more than 6 characters
	{
		myLCD_showChar('E',1);							// Display an error message
		myLCD_showChar('R',2);
		myLCD_showChar('R',3);
		myLCD_showChar('O',4);
		myLCD_showChar('R',5);
	}
} // end DisplayWord


//***************************************************************************************************
// DisplayMultipleWords()
//
// The function displays a message on the LCD screen word by word.  Any amount of blank space will
// determine the end of a word and the beginning of another.
//
// This function has one argument and does not return a value.
//
//      Arg 1  - 'words' specifies the text to display on the LCD screen
//***************************************************************************************************
void DisplayMultipleWords(char words[250])
{
	void clearLCD(void);								// Declare functions used
	int delay(int count);

	unsigned int i;										// Used to get index of current character
	unsigned int j;										// Used to get location of current character
	unsigned int length;								// Keeps track of the length of the message
	unsigned int delayCount;							// Determines the length of the delay
	char character;										// Current character to be displayed

	clearLCD();											// Make sure that the LCD screen is blank
	length = strlen(words);								// Get the length of the desired message
	for(i = 0;i<length;i=i+1)								// Loop through all of the characters in words
	{
		for(j = 1;j<=6;j=j+1)								// Loop through all of the LCD locations
		{
			character = words[i];						// Get the current character for current location
			if(character)								// If the character exists
			{
				if(character != 32)						// If character isn't a space...
				{
					myLCD_showChar(character,j);		// Show character on the LCD
					i++;								// Move on to the next character
				}
				else									// Otherwise, if the character is a space
				{
					delayCount = 40;					// Delay for (40*10ms) = 400ms between words
					while(delayCount = delay(delayCount));// Wait for delay to be over
					clearLCD();							// Make sure that the LCD screen is blank
					break;								// Break out of for(j = 1;j<=6;j++)
				} // end else
			} // end if(character)
		} // end for(j = 1;j<=6;j++)
	} // end for(i = 0;i<length;i++)
	delayCount = 40;									// Delay for (40*10ms) = 400ms
	while(delayCount = delay(delayCount));				// Wait for delay to be over

	clearLCD();											// Make sure that the LCD screen is blank
} // end DisplayMultipleWords


//***************************************************************************************************
// ScrollWords()
//
// The function scrolls text across the LCD screen.
//
// This function has one argument and does not return a value.
//
//      Arg 1  - 'words' specifies the text to be shown on the LCD screen
//***************************************************************************************************
void ScrollWords(char words[250])
{
	void clearLCD(void);								// Declare functions used
	int delay(int count);

	unsigned int i;										// Used to get index of current character
	unsigned int j;										// Used to get location of current character
	unsigned int length;								// Keeps track of the length of the word
	unsigned int delayCount;							// Determines the length of the delay
	char character;										// Current character to be displayed
	unsigned int offset;								// Offset window determines which six
														//  characters will be displayed during each
														//  shift

	clearLCD();											// Make sure that the LCD screen is blank
	length = strlen(words);								// Get the length of the desired message
	offset=0;											// Start with an offset of 0
	i=0;												// Start at index 0

	while(offset<length+6)								// Loop as long as you haven't shifted all
	{													// of the characters off the LCD screen
		i=offset;										// Move index to the starting offset position
		for(j = 1;j<=6;j=j+1)								// Loop through all of the LCD slots
		{
			character = words[i-6];						// Get the current character for LCD location
			if(character && (i>=6) && (i<=length+6))	// If character exists and you haven't
			{											//  reached the end of the message
				myLCD_showChar(character,j);			// Show the character on the LCD
			}
			else										// Otherwise, if it doesn't exist...
			{
				myLCD_showChar(' ',j);					// Pad the rest of the locations with spaces
			}
			i++;										// Move on to the next character
		} // end for(j = 1;j<=6;j++)

		delayCount = 20;								// Delay for (20*10ms) = 200ms
		while(delayCount=delay(delayCount));			// Wait for delay to be over
		offset = offset + 1;							// Increment offset to start one index
														//  further than last time
	}
	clearLCD();											// Clear the LCD
} // end ScrollWords


//***************************************************************************************************
// DisplayNumber()
//
// The function displays a numerical value of up to 6 digits on the LCD screen.
//
// This function has one argument and does not return a value.
//
//      Arg 1  - 'number' specifies the number that should appear on the LCD screen
//***************************************************************************************************
void DisplayNumber(unsigned long int number)
{
	void clearLCD(void);								// Declare functions used

	unsigned long int digit;							// Determines the current digit to display
	int i;												// Used to get location of current digit
	long int q = 10000;									// Used in digit calculations
	long int r = 100000;								// Used in digit calculations
	int zeroFlag = 1;									// Flag that all leading 0s have been passed

	clearLCD();											// Make sure that the LCD is blank
	if(number == 0)										// If the number is 0...
	{
	    myLCD_showChar('0', 6 );						// Show a 0 on the sixth LCD slot
	}
	else												// Otherwise if the number isn't 0
	{
		for(i=1; i<=6; i=i+1)	    					// Loop through each of the LCD locations
		{
			if(i == 1)									// For the first location
			{
				digit = (number/100000)+48;				// Calculate what digit to show
				if(digit>57 || digit<48)				// If the digit is invalid
				{
					myLCD_showChar('E',1);				// Display an error message
					myLCD_showChar('R',2);
					myLCD_showChar('R',3);
					myLCD_showChar('O',4);
					myLCD_showChar('R',5);
					break;								// Break out of the function
				}
			}
			else										// For all of the other locations
			{
				digit=(number/q)-((number/r)*10)+48;	// Calculate what digit to show
				q=q/10;
				r=r/10;
			}
			if(!((digit == 48) && (zeroFlag == 1)))		// If the digit isn't a leading 0
			{
				zeroFlag = 0;							// Flag that you've gotten past all
														//  leading 0s
				myLCD_showChar(digit,(i));				// Show the digit on the LCD
			}
		} // end for (i=1; i<=6; i++)
	} // end else
} // end DisplayNumber


//***************************************************************************************************
// clearLCD()
//
// The function clears the LCD screen so that all slots display empty spaces.
//***************************************************************************************************
void clearLCD(void)
{
	int j;
	for(j=0;j<=6;j=j+1)									// Loop through all LCD locations
	{
		myLCD_showChar(' ',j);							// Pad all locations with spaces
	}
} // end clearLCD


//***************************************************************************************************
// delay()
//
// The function creates a delay in the program.
//
// This function has one argument and does not return a value.
//
//      Arg 1  - 'count' specifies how many Timer_1 counting cycles to delay for
//***************************************************************************************************
int delay(int count)
{
	if(TA1CTL & TAIFG)									// If Timer_1 is done counting
	{
		count = count-1;										// Decrement count
		TA1CTL = TA1CTL & (~TAIFG);								// Reset Timer_1
	}
	return count;										// Return the value of count
} // end delay


//***************************************************************************************************
// GetRandomSequence()
//
// The function uses a timer to get a random sequence of LEDs to show the user.
//
// This function has one argument and returns an integer value.
//
//      Return - 'sequence' specifies the 16 bit sequence that will be displayed to the user
//		Arg 1  - 'sequence' specifies the 16 bit sequence that will be displayed to the user
//***************************************************************************************************
int GetRandomSequence(int sequence)
{
	while(!sequence)									// While we haven't set sequence to anything
	{
		if(!(BUTTON1 & P1IN))							// Check to see if button 1 is pressed
		{
			sequence = TA0R;							// Set sequence equal to the current value
		}												//  that Timer_0 has counted up to
	}
	return sequence;
}


//***************************************************************************************************
// ShowSequence()
//
// The function shows the user a sequence of blinking LEDs.
//
// This function has two arguments and does not return anything.
//
//      Arg 1  - 'sequence' specifies the 16 bit sequence that the user is trying to enter
//      Arg 2  - 'i' determines how many blinking LEDs to display
//***************************************************************************************************
void ShowSequence(int sequence, int i)
{
	int delay(int count);								// Declare functions used
	void DisplayWord(char words[250]);

	int segment;										// Designates a segment of the entire
														// sequence to display
	int delayCount;										// Determines how long each delay should be
	int k;												// Used in the for loop to display i blinks

	for(k=0;k<=i;k=k+1)									// Makes sure to show the correct
	{													//  number of LEDs
		DisplayWord("WATCH"); 							// Display word to LCD
		delayCount = 25;								// Delay for (25*10ms) = 250ms
		while(delayCount = delay(delayCount));			// Wait for the delay to be over
		segment = (sequence >> k) & 0x01;				// Determines which LED should be on
		if (segment == 0)								// If we want the red LED
		{
			P1OUT = P1OUT | RED_ON; 					// Turn on the red LED
			P9OUT = P9OUT & GREEN_OFF; 					// Turn off the green LED
		}
		else											// Otherwise if we want the Green LED
		{
			P9OUT = P9OUT | GREEN_ON; 					// Turn on the green LED
			P1OUT = P1OUT & RED_OFF; 					// Turn off the red LED
		}
		delayCount = 25;								// Delay for (25*10ms) = 250ms
		while(delayCount = delay(delayCount));			// Wait for the delay to be over
		P1OUT &= RED_OFF;								// Turn off the red LED
		P9OUT &= GREEN_OFF;								// Turn off the green LED
	} // end for(k=0;k<=i;k+=1)
}


//***************************************************************************************************
// GetUserSequence()
//
// The function checks to make sure that the user enters the correct sequence of button pushes.
//
// This function has three arguments and returns either '0' or '1'.
//
//      Return - 'gameOver' tells the program whether or not the user entered a correct sequence
//      Arg 1  - 'sequence' specifies the 16 bit sequence that the user is trying to enter
//      Arg 2  - 'gameOver' determines whether or not the user has entered a correct sequence
//      Arg 3  - 'i' determines how many buttons pushes the function look for
//***************************************************************************************************
int GetUserSequence(int sequence, int gameOver, int i)
{
	void DisplayWord(char words[250]);					// Declare functions used
	int delay(int count);

	int btn_counts=0;									// Keeps track of how many buttons the user
														//  has pressed
	int delayCount;										// Determines how long the delay will be
	int segment;										// Determines which button the user should be
														//  pressing
														//  Can have 2 different values
														//  0 - The user should press BUTTON1
														//  1 - The user should press BUTTON2

	DisplayWord("GO");									// Display word to LCD
	while((btn_counts<=i) && !gameOver) 				// Wait for button input as long as the user
	{													//  hasn't entered a wrong sequence or
														//  hasn't gotten all of them right
		if((BUTTON1 & P1IN) == 0)						// Check if button 1 is pushed
		{
			while((BUTTON1 & P1IN) == 0)				// This loop accounts for button bounce
			{
				P1OUT = P1OUT | RED_ON;					// Turn on the red LED
				delayCount = 3;							// Delay for (3*10ms) = 30ms
				while(delayCount=delay(delayCount));	// Wait for delay to be over
			}
			P1OUT = P1OUT & RED_OFF;					// Turn off the red LED

			segment = (sequence >> btn_counts) & 0x01;	// Determines which button
														// should have been pressed
			if(segment != 0)							// If the user pressed the wrong button
			{
				gameOver = 1;							// GAME OVER
				break;									// Stop waiting for more button presses
			}
			btn_counts = btn_counts + 1;				// Incrememnt number of times a button has
		}												//  been pressed
		else if((BUTTON2 & P1IN) == 0)					// Check if button 2 is pushed
		{
			while((BUTTON2 & P1IN) == 0)				// This loop accounts for button bounce
			{
				P9OUT = P9OUT | GREEN_ON;				// Turn on the green LED
				delayCount = 3;							// Delay for (3*10ms) = 30ms
				while(delayCount=delay(delayCount));	// Wait for delay to be over
			}
			P9OUT &= GREEN_OFF;							// Turn off the green LED

			segment = (sequence >> btn_counts) & 0x01;	// Determines which button
														//  should have been pressed
			if(segment == 0)							// If the user pressed the wrong button
			{
				gameOver = 1;							// GAME OVER
				break;									// Stop waiting for more button presses
			}
			btn_counts = btn_counts + 1;				// Increment number of times a button has
		}												//  been pressed
	}
	return gameOver;									// Return the gameOver state
}


//***************************************************************************************************
// GameOverMessage()
//
// The function shows the user a game over message on the LCD screen.
//
// This function has one argument and does not return anything.
//
//      Arg 1  - 'score' specifies the user's score at the end of the game
//***************************************************************************************************
void GameOverMessage(int score)
{
	void ScrollWords(char words[250]);					// Declare functions used
	void DisplayNumber(unsigned long int number);

	P1OUT = P1OUT | RED_ON;								// Turn on the red LED
	P9OUT = P9OUT | GREEN_ON;							// Turn on the green LED

	if(score == 16)										// If the user answered everything correctly
	{
		ScrollWords("YOU WIN");							// Scroll message across LCD screen
	}
	else												// Otherwise if they didn't get everything
	{													//  correct
		ScrollWords("GAME OVER FINAL SCORE");			// Scroll words across LCD screen
		DisplayNumber(score);							// Display number to LCD screen
	}

	while(1)											// Loops as long as the user hasn't reset the
	{													//  game
		if(!(BUTTON1 & P1IN)&&!(BUTTON2 & P1IN))		// If both buttons are pressed
		{
			break;										// Break out of the infinite loop to start a
		}												//  new game
	}
}

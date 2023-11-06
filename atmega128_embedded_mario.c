#undef F_CPU
#define F_CPU 16000000
#include "avr_mcu_section.h"
#include <stdbool.h>
AVR_MCU(F_CPU, "atmega128");

#define	__AVR_ATmega128__	1
#include <avr/io.h>


// GENERAL INIT - USED BY ALMOST EVERYTHING ----------------------------------

static void port_init() {
	PORTA = 0b00011111;	DDRA = 0b01000000; // buttons & led
	PORTB = 0b00000000;	DDRB = 0b00000000;
	PORTC = 0b00000000;	DDRC = 0b11110111; // lcd
	PORTD = 0b11000000;	DDRD = 0b00001000;
	PORTE = 0b00100000;	DDRE = 0b00110000; // buzzer
	PORTF = 0b00000000;	DDRF = 0b00000000;
	PORTG = 0b00000000;	DDRG = 0b00000000;
}


// BUTTON HANDLING -----------------------------------------------------------

#define BUTTON_NONE		0
#define BUTTON_CENTER	1
#define BUTTON_LEFT		2
#define BUTTON_RIGHT	3
#define BUTTON_UP		4
#define BUTTON_DOWN		5
static int button_accept = 1;

static int button_pressed() {
	// up
	if (!(PINA & 0b00000001) & button_accept) { // check state of button 1 and value of button_accept
		button_accept = 0; // button is pressed
		return BUTTON_UP;
	}

	// left
	if (!(PINA & 0b00000010) & button_accept) { // check state of button 2 and value of button_accept
		button_accept = 0; // button is pressed
		return BUTTON_LEFT;
	}

	// center
	if (!(PINA & 0b00000100) & button_accept) { // check state of button 3 and value of button_accept
		button_accept = 0; // button is pressed
		return BUTTON_CENTER;
	}

	// right
	if (!(PINA & 0b00001000) & button_accept) { // check state of button 4 and value of button_accept
		button_accept = 0; // button is pressed
		return BUTTON_RIGHT;
	}


	// down
	if (!(PINA & 0b00010000) & button_accept) { // check state of button 5 and value of button_accept
		button_accept = 0; // button is pressed
		return BUTTON_DOWN;
	}

	return BUTTON_NONE;
}

static void button_unlock() {
	//check state of all buttons
	if (
		((PINA & 0b00000001)
		|(PINA & 0b00000010)
		|(PINA & 0b00000100)
		|(PINA & 0b00001000)
		|(PINA & 0b00010000)) == 31)
	button_accept = 1; //if all buttons are released button_accept gets value 1
}

// LCD HELPERS ---------------------------------------------------------------

#define		CLR_DISP	    0x00000001
#define		DISP_ON		    0x0000000C
#define		DISP_OFF	    0x00000008
#define		CUR_HOME      0x00000002
#define		CUR_OFF 	    0x0000000C
#define   CUR_ON_UNDER  0x0000000E
#define   CUR_ON_BLINK  0x0000000F
#define   CUR_LEFT      0x00000010
#define   CUR_RIGHT     0x00000014
#define   CG_RAM_ADDR		0x00000040
#define		DD_RAM_ADDR	  0x00000080
#define		DD_RAM_ADDR2	0x000000C0

//#define		ENTRY_INC	    0x00000007	//LCD increment
//#define		ENTRY_DEC	    0x00000005	//LCD decrement
//#define		SH_LCD_LEFT	  0x00000010	//LCD shift left
//#define		SH_LCD_RIGHT	0x00000014	//LCD shift right
//#define		MV_LCD_LEFT	  0x00000018	//LCD move left
//#define		MV_LCD_RIGHT	0x0000001C	//LCD move right

static void lcd_delay(unsigned int b) {
	volatile unsigned int a = b;
	while (a)
		a--;
}

static void lcd_pulse() {
	PORTC = PORTC | 0b00000100;	//set E to high
	lcd_delay(1400); 			//delay ~110ms
	PORTC = PORTC & 0b11111011;	//set E to low
}

static void lcd_send(int command, unsigned char a) {
	unsigned char data;

	data = 0b00001111 | a;					//get high 4 bits
	PORTC = (PORTC | 0b11110000) & data;	//set D4-D7
	if (command)
		PORTC = PORTC & 0b11111110;			//set RS port to 0 -> display set to command mode
	else
		PORTC = PORTC | 0b00000001;			//set RS port to 1 -> display set to data mode
	lcd_pulse();							//pulse to set D4-D7 bits

	data = a<<4;							//get low 4 bits
	PORTC = (PORTC & 0b00001111) | data;	//set D4-D7
	if (command)
		PORTC = PORTC & 0b11111110;			//set RS port to 0 -> display set to command mode
	else
		PORTC = PORTC | 0b00000001;			//set RS port to 1 -> display set to data mode
	lcd_pulse();							//pulse to set d4-d7 bits
}

static void lcd_send_command(unsigned char a) {
	lcd_send(1, a);
}

static void lcd_send_data(unsigned char a) {
	lcd_send(0, a);
}

static void lcd_init() {
	//LCD initialization
	//step by step (from Gosho) - from DATASHEET

	PORTC = PORTC & 0b11111110;

	lcd_delay(10000);

	PORTC = 0b00110000;				//set D4, D5 port to 1
	lcd_pulse();					//high->low to E port (pulse)
	lcd_delay(1000);

	PORTC = 0b00110000;				//set D4, D5 port to 1
	lcd_pulse();					//high->low to E port (pulse)
	lcd_delay(1000);

	PORTC = 0b00110000;				//set D4, D5 port to 1
	lcd_pulse();					//high->low to E port (pulse)
	lcd_delay(1000);

	PORTC = 0b00100000;				//set D4 to 0, D5 port to 1
	lcd_pulse();					//high->low to E port (pulse)

	lcd_send_command(0x28); // function set: 4 bits interface, 2 display lines, 5x8 font
	lcd_send_command(DISP_OFF); // display off, cursor off, blinking off
	lcd_send_command(CLR_DISP); // clear display
	lcd_send_command(0x06); // entry mode set: cursor increments, display does not shift

	lcd_send_command(DISP_ON);		// Turn ON Display
	lcd_send_command(CLR_DISP);		// Clear Display
}

static void lcd_send_text(char *str) {
	while (*str)
		lcd_send_data(*str++);
}

static void lcd_send_line1(char *str) {
	lcd_send_command(DD_RAM_ADDR);
	lcd_send_text(str);
}

static void lcd_send_line2(char *str) {
	lcd_send_command(DD_RAM_ADDR2);
	lcd_send_text(str);
}

static void screen_update(unsigned char *map){
	// Update the first line
	lcd_send_command(DD_RAM_ADDR);
	for (int i = 0; i < 16; i++)
	{
		lcd_send_data(map[i]);
	}

	// Update the second line
	lcd_send_command(DD_RAM_ADDR2);
	for (int i = 0; i < 16; i++)
	{
		lcd_send_data(map[i + 16]);
	}
}

static char MARIO[] = {
	// Race right
	0b01100,
	0b01111,
	0b01110,
	0b01110,
	0b01110,
	0b11111,
	0b01010,
	0b11011,
	// Face left
	0b00110,
	0b11110,
	0b01110,
	0b01110,
	0b01110,
	0b11111,
	0b01010,
	0b11011
};


// Map
#define EMPTY_CHAR ' '

static unsigned char MAP[2][16] = {
	{' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '},
	{' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '}
};

int main() {
	port_init();
	lcd_init();
	
	// Init CGRAM
	lcd_send_command(CG_RAM_ADDR);
	for (int i = 0; i < 8; i++)
	{
		lcd_send_data(MARIO[i]);
	}

	int movement_offset = 0;
	unsigned int start_col = 1;
	unsigned int start_row = 1;
	bool delete_prev = false;
	bool face_right = true;
	unsigned int pressed_button = BUTTON_NONE;


	int addr = start_row ? DD_RAM_ADDR2 : DD_RAM_ADDR;
	lcd_send_command(addr + start_col);

	lcd_send_data(0);


	// Game Loop
	while(1)
	{	
		// Waiting for the next input
		do
		{
			pressed_button = button_pressed();
			button_unlock();
		} while (pressed_button == BUTTON_NONE);
		

		// Process the input

		if(pressed_button == BUTTON_LEFT)
		{
			face_right = false;
		}	
		else
		{
			face_right = true;
		}


		// POSITION UPDATE
		// Update movement offset and start column
		if (movement_offset == 4 && face_right){
			start_col += 1;

			delete_prev = true;
			
			// The 'deleted' block's DDRAM should be updated before its CGRAM.
			int addr = start_row ? DD_RAM_ADDR2 : DD_RAM_ADDR;
			lcd_send_command(addr + start_col - 1);
			lcd_send_data(EMPTY_CHAR);
		} else if (movement_offset == 0 && !face_right)
		{
			start_col -= 1;
			delete_prev = true;

		} else if (movement_offset == 1 && !face_right)
		{
			int addr = start_row ? DD_RAM_ADDR2 : DD_RAM_ADDR;
			lcd_send_command(addr + start_col + 1);
			lcd_send_data(EMPTY_CHAR);
		}
		
		if (movement_offset == 0 && !face_right)
		{
			movement_offset = 4;
		} else
		{
			movement_offset = (face_right ? (movement_offset + 1) : (movement_offset - 1)) % 5;
		}


		// Update CGRAM
		lcd_send_command(CG_RAM_ADDR);

		// Char 1
		for (int i = 0; i < 8; i++)
		{
			int start_index = face_right ? 0 : 8;
			lcd_send_data(MARIO[start_index + i] >> movement_offset);
		}	
		
		// If offset is not 0, then the second character also should be updated.
		if(movement_offset)
		{
			// Char 2
			for (int i = 0; i < 8; i++)
			{	
				int start_index = face_right ? 0 : 8;
				char temp = 0;
				temp = MARIO[start_index + i] << (5 - movement_offset);

				lcd_send_data(temp);
			}
		}

		// Update map's encoding matrix
		for (int i = 0; i < 2; i++)
		{
			for (int j = 0; j < 16; j++)
			{
				if (i == start_row && j == start_col)
				{	
					
					if (delete_prev)
					{
						int prev_col = face_right ? (j - 1) : (j + 2);
						MAP[i][prev_col] = EMPTY_CHAR;
					}
					

					MAP[i][j] = 0;
					if (movement_offset){
						MAP[i][j + 1] = 1;
					} else
					{
						MAP[i][j + 1] = EMPTY_CHAR;
					}

				}	
			}
			
		}
		



		// Update DDRAM
		screen_update(MAP);

		delete_prev = false;
		button_unlock();
	}

		
}

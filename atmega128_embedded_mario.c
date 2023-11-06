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

#define EMPTY_CHAR ' '

static void clean_map(unsigned char *map){
	for (int i = 0; i < 32; i++)
	{
		map[i] = EMPTY_CHAR;
	}
	
}

static char MARIO[] = {
	// Race right
	0b00000,
	0b00000,
	0b01100,
	0b01111,
	0b01110,
	0b11111,
	0b01010,
	0b11011,
	// Face left
	0b00000,
	0b00000,
	0b00110,
	0b11110,
	0b01110,
	0b11111,
	0b01010,
	0b11011,
};

// Character map
// 2 3
// 0 1

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

	int col_offset = 0;
	int row_offset = 0;

	unsigned int start_col = 0;
	unsigned int start_row = 1;

	bool delete_prev = false;
	bool face_right = true;

	unsigned int pressed_button = BUTTON_NONE;

	bool horizontal_movement = false;

	int addr = start_row ? DD_RAM_ADDR2 : DD_RAM_ADDR;
	lcd_send_command(addr + start_col);

	lcd_send_data(0);


	// Game Loop
	while(1)
	{	
		horizontal_movement = false;
		// Waiting for the next input
		do
		{
			pressed_button = button_pressed();
			button_unlock();
		} while (pressed_button == BUTTON_NONE);


		// Zero custom
		lcd_send_command(CG_RAM_ADDR);

		for (int i = 0; i < 4*8; i++)
		{
			lcd_send_data(0);
		}
		


		// Process the input
		if(pressed_button == BUTTON_LEFT)
		{
			face_right = false;
			horizontal_movement = true;
		}	
		else if (pressed_button == BUTTON_RIGHT)
		{
			face_right = true;
			horizontal_movement = true;
		}
		else if (pressed_button == BUTTON_UP)
		{	
			if (row_offset == 7)
			{
				start_row = 0;
			}

			row_offset = (row_offset + 1) % 8;
			
		}
		else if (pressed_button == BUTTON_DOWN)
		{
			if (row_offset == 0 && start_row == 0)
			{
				row_offset = 7;
				start_row = 1;
			}
			else
			{
				row_offset -= 1;
			}

		}
		else 
		{
			continue;
		}

		// POSITION UPDATE
		// Update movement offset and start column

		if (horizontal_movement)
		{

			if (col_offset == 4 && face_right){
				start_col += 1;
			}
			
			if (col_offset == 0 && !face_right)
			{
				start_col -= 1;
				col_offset = 4;
			}
			else
			{
				col_offset = (face_right ? (col_offset + 1) : (col_offset - 1)) % 5;
			}

		}

		// ************** Update CGRAM ************

		// ***************LOWER LINE***************
		lcd_send_command(CG_RAM_ADDR);

		if (start_row)
		{
			// Char 0
			for (int i = 0; i < (8 - row_offset); i++)
			{
				int start_index = face_right ? 0 : 8;
				lcd_send_data(MARIO[start_index + i + row_offset] >> col_offset);
			}	
			for (int i = 0; i < row_offset; i++)
			{
				lcd_send_data(0);
			}
			
			
			// If offset is not 0, then the second character also should be updated.
			if(col_offset)
			{
				// Char 1
				for (int i = 0; i < (8 - row_offset); i++)
				{	
					int start_index = face_right ? 0 : 8;
					char temp = 0;
					temp = MARIO[start_index + i + row_offset] << (5 - col_offset);

					lcd_send_data(temp);
				}

				for (int i = 0; i < row_offset; i++)
				{
					lcd_send_data(0);
				}
				
			}
		}

		// ***************UPPER LINE***************
		// If the row offset is greather than 2, or we are standing on the upper line with 0 offset.
		if (row_offset >= 3 || !start_row)	
		{
			
			// Char 2 start position
			lcd_send_command(CG_RAM_ADDR + 16);

			// Mario is in the upper region.
			if (row_offset <= 2)
			{	
				// Char 2
				for (int i = 0; i < 8; i++)
				{
					int start_index = face_right ? 0 : 8;
					lcd_send_data(MARIO[start_index + i] >> col_offset);
				}

				// Char 3
				for (int i = 0; i < 8; i++)
				{
					int start_index = face_right ? 0 : 8;
					char temp = 0;
					temp = MARIO[start_index + i] << (5 - col_offset);

					lcd_send_data(temp);
				}
			}
			else
			{	
				// Char 2
				for (int i = 0; i < (8 - row_offset); i++)
				{
					lcd_send_data(0);
				}

				for (int i = 0; i < row_offset; i++)
				{
					int start_index = face_right ? 0 : 8;
					lcd_send_data(MARIO[start_index + i] >> col_offset);
				}


				// Char 3
				for (int i = 0; i < (8 - row_offset); i++)
				{
					lcd_send_data(0);
				}

				for (int i = 0; i < row_offset; i++)
				{
					int start_index = face_right ? 0 : 8;
					char temp = 0;
					temp = MARIO[start_index + i] << (5 - col_offset);

					lcd_send_data(temp);
				}
			}
		}

		// Update map's encoding matrix
		clean_map(MAP);

		MAP[0][start_col] = 2;
		MAP[0][start_col + 1] = 3;
		MAP[1][start_col] = 0;
		MAP[1][start_col + 1] = 1;
		



		// Update DDRAM
		screen_update(MAP);

		delete_prev = false;
		button_unlock();
	}

		
}

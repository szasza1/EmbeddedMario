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

// Actions
#define A_NONE -1
#define A_LEFT 0
#define A_RIGHT 1
#define A_JUMP 2
#define A_JUMP_R 3
#define A_JUMP_L 4

static int is_pressed() {

	if (!(PINA & 0b00001001)) {
		return A_JUMP_R;
	}

	if (!(PINA & 0b00000011)) {
		return A_JUMP_L;
	}

	if (!(PINA & 0b00000001)) {
		return A_JUMP;
	}

	if (!(PINA & 0b00001000)) {
		return A_RIGHT;
	}

	if (!(PINA & 0b00000010)) {
		return A_LEFT;
	}

	return A_NONE;
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

static char MARIO[] = {
	// Face right 
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
	// Platform 1
	0b00000,
	0b00000,
	0b00000,
	0b11111,
	0b11111,
	0b01110,
	0b01110,
	0b01110,
	// Enemy 1
	0b00000,
	0b00000,
	0b00000,
	0b01110,
	0b11111,
	0b10101,
	0b11011,
	0b01110,
	// Platform 2
	0b00000,
	0b00000,
	0b00000,
	0b11111,
	0b10101,
	0b11011,
	0b10101,
	0b11111,
	// Enemy 2
	0b00000,
	0b00000,
	0b00000,
	0b01010,
	0b11011,
	0b01110,
	0b11111,
	0b01010,
	// Fire
	0b00000,
	0b00000,
	0b00000,
	0b00010,
	0b01100,
	0b00110,
	0b01100,
	0b11111,
};


static char MARIO_ANIM[] = {
	// Right 1
	0b00000,
	0b00000,
	0b01100,
	0b01111,
	0b01111,
	0b01110,
	0b11010,
	0b00011,
	// Left 2
	0b00000,
	0b00000,
	0b01100,
	0b01111,
	0b11110,
	0b01110,
	0b01011,
	0b11000,
	// Left 1
	0b00000,
	0b00000,
	0b00110,
	0b11110,
	0b01111,
	0b01110,
	0b11010,
	0b00011,
	// Left 2
	0b00000,
	0b00000,
	0b00110,
	0b11110,
	0b11110,
	0b01110,
	0b01011,
	0b11000,
};

static char CG_CONTENT[] = {
	// Char 0 (Mario)
	0b00000,
	0b00000,
	0b00000,
	0b00000,
	0b00000,
	0b00000,
	0b00000,
	0b00000,
	// Char 1 (Mario)
	0b00000,
	0b00000,
	0b00000,
	0b00000,
	0b00000,
	0b00000,
	0b00000,
	0b00000,
	// Char 2 (Mario)
	0b00000,
	0b00000,
	0b00000,
	0b00000,
	0b00000,
	0b00000,
	0b00000,
	0b00000,
	// Char 3 (Mario)
	0b00000,
	0b00000,
	0b00000,
	0b00000,
	0b00000,
	0b00000,
	0b00000,
	0b00000,
	// Char 4
	0b00000,
	0b00000,
	0b00000,
	0b00000,
	0b00000,
	0b00000,
	0b00000,
	0b00000,
	// Char 5
	0b00000,
	0b00000,
	0b00000,
	0b00000,
	0b00000,
	0b00000,
	0b00000,
	0b00000,
	// Char 6
	0b00000,
	0b00000,
	0b00000,
	0b00000,
	0b00000,
	0b00000,
	0b00000,
	0b00000,
	// Char 7
	0b00000,
	0b00000,
	0b00000,
	0b00000,
	0b00000,
	0b00000,
	0b00000,
	0b00000,
};

// CG map
// 2 3
// 0 1

// LEVEL  ID:
// 0: Mario
// 2: Platform 1
// 3: Mushroom
static unsigned char LEVEL_DESC[4][16] = {
	//Level 1
	{' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '},
	{' ' ,' ',' ',6, ' ' ,2,' ',' ',' ',' ',3,' ',' ', 6,' ',5},
	//Level 2
	{' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '},
	{' ' ,' ',' ',' ',' ',' ',2,' ',' ',' ',' ',' ',5,' ',' ',6}
};

// Level object ids are in increasing order.
static unsigned char LEVEL_OBJECTS[3][4] = {
	{2,3,5,6}, //Level 1
	{13,3,0,0}, // Fire pos in level 1
	{2,5,6,0},  //Level 2
	{0,0,0,0}
};

static unsigned char MAP[2][16] = {
	{' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '},
	{' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '}
};


static void update_CG(){
	lcd_send_command(CG_RAM_ADDR);

	for (int i = 0; i < 32; i++)
	{
		lcd_send_data(CG_CONTENT[i]);
	}
}


#define MARIO_LEFT 0
#define MARIO_RIGHT 1
#define MARIO_UP 2
#define MARIO_DOWN 3

// Global variables
static unsigned int col_offset = 0;
static unsigned int row_offset = 0;

static unsigned int start_col = 0;
static unsigned int start_row = 1;

static bool face_right = true;

static bool jump = false;
static bool fall = false;
static unsigned int jump_off = 0;
static unsigned int fall_off = 0;

static unsigned int akt_level = 0; // Should be increased by 2!

static int score = 0;
static int health = 5;


static int h_animation_offset = 0;
static int h_anim_gap = 25;


static void update_mario_buffer(){

	int obj_ids[4] = {
		LEVEL_DESC[akt_level + 1][start_col],		//CG 0
		LEVEL_DESC[akt_level + 1][start_col + 1],	//CG 1
		LEVEL_DESC[akt_level][start_col],			//CG 2
		LEVEL_DESC[akt_level][start_col + 1]		//CG 3
	};

	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 8; j++)
		{
			if(obj_ids[i] >= 2 && obj_ids[i] <= 10) {				//TODO: Object ID valid check
				CG_CONTENT[i * 8 + j] = MARIO[obj_ids[i] * 8 + j];
			} else {
				CG_CONTENT[i * 8 + j] = 0;
			}
		}
		
	}
	
		
	// ***************LOWER LINE***************

	if (start_row)
	{	
		// Update char 0 and char 1
		for (int i = 0; i < (8 - row_offset); i++)
		{
			/*int start_index = face_right ? 0 : 8;
			// CG 0
			CG_CONTENT[i] |= MARIO[start_index + i + row_offset] >> col_offset;
			// CG 1
			if (col_offset) CG_CONTENT[i + 8] |= MARIO[start_index + i + row_offset] << (5 - col_offset);*/


			int start_index = face_right ? 0 : 16;
			int step = (h_animation_offset < h_anim_gap) ? 0 : 1;

			start_index += (step * 8);

			// CG 0
			CG_CONTENT[i] |= MARIO_ANIM[start_index + i + row_offset] >> col_offset;
			// CG 1
			if (col_offset) CG_CONTENT[i + 8] |= MARIO_ANIM[start_index + i + row_offset] << (5 - col_offset);
			h_animation_offset = (h_animation_offset + 1) % (2*h_anim_gap);
		}

	}

	// ***************UPPER LINE***************

	if (row_offset >= 1 || !start_row)	
	{
		if (!start_row)
		{	
			// Update char 2 and char 3
			for (int i = 0; i < (8 - row_offset); i++)
			{
				int start_index = face_right ? 0 : 8;
				// CG 2
				CG_CONTENT[i + 16] |= MARIO[start_index + i + row_offset] >> col_offset;
				//CG 3
				if (col_offset) CG_CONTENT[i + 24] |= MARIO[start_index + i + row_offset] << (5 - col_offset);
			}

		}
		else
		{	
			int cor = 8 - row_offset;

			for (int i = (8 - row_offset); i < 8; i++)
			{
				int start_index = face_right ? 0 : 8;
				// CG 2
				CG_CONTENT[i + 16] |= MARIO[start_index + i - cor] >> col_offset;
				//CG 3
				if (col_offset) CG_CONTENT[i + 24] |= MARIO[start_index + i - cor] << (5 - col_offset);
				
			}
			
		}
	}
}


static bool check_pixel(char src, int offset) {
	switch (offset)
	{
	case 0:
		return (0b10000 & src);
		break;
	case 1:
		return (0b01000 & src);
		break;
	case 2:
		return (0b00100 & src);
		break;
	case 3:
		return (0b00010 & src);
		break;
	case 4:
		return (0b00001 & src);
		break;
	default:
		break;
	}
}

static bool check_left_pixel(char src, int offset) {
	switch (offset)
	{
	case 1:
		return (0b01111 & src);
		break;
	case 2:
		return (0b00111 & src);
		break;
	case 3:
		return (0b00011 & src);
		break;
	case 4:
		return (0b00001 & src);
		break;
	default:
		break;
	}
}

static bool check_right_pixel(char src, int offset) {
	switch (offset)
	{
	case 1:
		return (0b10000 & src);
		break;
	case 2:
		return (0b11000 & src);
		break;
	case 3:
		return (0b11100 & src);
		break;
	case 4:
		return (0b11110 & src);
		break;
	default:
		break;
	}
}


static void load_level(unsigned int akt_level){
	for (int i = 0; i < 2; i++)
	{
		for (int j = 0; j < 16; j++)
		{	
			if (!LEVEL_DESC[akt_level + i][j] || LEVEL_DESC[akt_level + i][j] == ' '){

				MAP[i][j] = LEVEL_DESC[akt_level + i][j];

			} else {
				
				for (int z = 0; z < 4; z++)
				{
					if (LEVEL_OBJECTS[akt_level][z] == LEVEL_DESC[akt_level + i][j]) { MAP[i][j] = 4 + z;} 
				}

			}
			
		}
		
	}

	
	MAP[0][start_col] = 2;
	MAP[0][start_col + 1] = 3;
	MAP[1][start_col] = 0;
	MAP[1][start_col + 1] = 1;
	

}

static void init_CG_CONTENT(unsigned int akt_level){
	// Mario
	for (int i = 0; i < 8; i++)
	{
		CG_CONTENT[i] = MARIO[i];
	}

	// CGRAM 5
	int cg_index = 32;
	for (int i = 0; i < 4; i++)
	{
		if (LEVEL_OBJECTS[akt_level][i]){

			for (int j = 0; j < 8; j++)
			{
				CG_CONTENT[cg_index++] = MARIO[ LEVEL_OBJECTS[akt_level][i] * 8 + j ];
			}
		}
	}
}

static int update_map(){
	
	if(MAP[1][start_col] == 0) return 0; 	// The CG positions are not changed.

	int shift_mode = 0;

	if(MAP[1][start_col] == 1) {		// Right shift.

		int obj_id_0 = LEVEL_DESC[akt_level][start_col - 1]; 
		int obj_id_1 = LEVEL_DESC[akt_level + 1][start_col - 1]; 

		MAP[0][start_col - 1] = ' ';  
		MAP[1][start_col - 1] = ' ';

		
		for (int z = 0; z < 4; z++)
		{
			if (LEVEL_OBJECTS[akt_level][z] == obj_id_0) { MAP[0][start_col - 1] = 4 + z;}
			if (LEVEL_OBJECTS[akt_level][z] == obj_id_1) { MAP[1][start_col - 1] = 4 + z;}
		}
		
		shift_mode = 1;
	} else {							// Left shift.

		int obj_id_0 = LEVEL_DESC[akt_level][start_col + 2];
		int obj_id_1 = LEVEL_DESC[akt_level + 1][start_col + 2];

		MAP[0][start_col + 2] = ' ';  
		MAP[1][start_col + 2] = ' ';

		
		for (int z = 0; z < 4; z++)
		{
			if (LEVEL_OBJECTS[akt_level][z] == obj_id_0) { MAP[0][start_col + 2] = 4 + z;}
			if (LEVEL_OBJECTS[akt_level][z] == obj_id_1) { MAP[1][start_col + 2] = 4 + z;}
		}
		shift_mode = 2;
	}

	MAP[0][start_col] = 2;
	MAP[0][start_col + 1] = 3;
	MAP[1][start_col] = 0;
	MAP[1][start_col + 1] = 1;

	return shift_mode;
}

static void update_DD(int shift_mode){
	if(shift_mode == 1) { // Right shift
		lcd_send_command(DD_RAM_ADDR2 + start_col);
		lcd_send_data(MAP[1][start_col]);
		lcd_send_data(MAP[1][start_col + 1]);

		lcd_send_command(DD_RAM_ADDR + start_col);
		lcd_send_data(MAP[0][start_col]);
		lcd_send_data(MAP[0][start_col + 1]);

	} else {
		lcd_send_command(DD_RAM_ADDR2 + start_col);
		lcd_send_data(MAP[1][start_col]);
		lcd_send_data(MAP[1][start_col + 1]);

		lcd_send_command(DD_RAM_ADDR + start_col);
		lcd_send_data(MAP[0][start_col]);
		lcd_send_data(MAP[0][start_col + 1]);
	}

	lcd_send_command(DD_RAM_ADDR);
	lcd_send_data(health + '0');
	lcd_send_command(DD_RAM_ADDR + 15);
	lcd_send_data(score + '0');
}


static void delete_prev_col_DD(int shift_mode){
	if(shift_mode == 1) {
		lcd_send_command(DD_RAM_ADDR2 + (start_col - 1));
		lcd_send_data(MAP[1][start_col - 1]);

		lcd_send_command(DD_RAM_ADDR + (start_col - 1));
		lcd_send_data(MAP[0][start_col - 1]);
	} else {
		lcd_send_command(DD_RAM_ADDR2 + start_col + 2);
		lcd_send_data(MAP[1][start_col + 2]);

		lcd_send_command(DD_RAM_ADDR + start_col + 2);
		lcd_send_data(MAP[0][start_col + 2]);
	}
}


static void init_screen() {
	lcd_send_command(CG_RAM_ADDR);

	for (int i = 0; i < 64; i++)
	{
		lcd_send_data(CG_CONTENT[i]);
	}

	lcd_send_command(DD_RAM_ADDR);
	for (int i = 0; i < 16; i++)
	{
		lcd_send_data(MAP[0][i]);
	}

	// Update the second line
	lcd_send_command(DD_RAM_ADDR2);
	for (int i = 0; i < 16; i++)
	{
		lcd_send_data(MAP[1][i]);
	}

	lcd_send_command(DD_RAM_ADDR);
	lcd_send_data(health + '0');
	lcd_send_command(DD_RAM_ADDR + 15);
	lcd_send_data(score + '0');
}


static void update(){ 
	// Update map's encoding matrix
	int shift_mode = update_map(); 	

	// DDClear
	delete_prev_col_DD(shift_mode);

	// Update CGRAM
	update_CG();

	// Update DDRAM
	update_DD(shift_mode);
}


// -1: Nincs utkozes
// 0: Ervenytelen lepes
// >=2: Utkozott objektum ID
static int collide(int mode) {
	switch (mode)
	{
	case MARIO_RIGHT: 
	{	
		if (start_col == 15) return 0; // TODO:

		int obj_id = LEVEL_DESC[akt_level + start_row][start_col + 1]; // 2-5
		
		// detection
		if (obj_id >= 2 && obj_id <= 6){
			for (int i = row_offset; i < 8; i++)
			{
				if ( check_pixel(MARIO[(obj_id + 1) * 8 - 1 - i], col_offset) ) return obj_id;
			}
		}
		break;
	}
	case MARIO_LEFT:
	{	
		if (start_col == 0 && col_offset == 0 && akt_level == 0) return 0; //invalid movement

		int temp_c_start = col_offset ? start_col : start_col - 1;
		int temp_c_off = col_offset ? col_offset - 1 : 4;

		int obj_id = LEVEL_DESC[akt_level + start_row][temp_c_start];

		// detection
		if (obj_id >= 2 && obj_id <= 6){
			for (int i = row_offset; i < 8; i++)
			{
				if ( check_pixel(MARIO[(obj_id + 1) * 8 - 1 - i], (temp_c_off)) ) return obj_id;
			}
		}
		
		break;
	}
	case MARIO_DOWN:
	{
		if(start_row && !row_offset) return 0; // start_row = 1 and row_offset == 0

		int temp_r_start = row_offset ? start_row : 1; 
		int temp_r_off = row_offset ? row_offset - 1 : 7;
		
		int obj_id = LEVEL_DESC[akt_level + temp_r_start][start_col];

		if (!col_offset && obj_id >= 2 && obj_id <= 6 && MARIO[(obj_id + 1) * 8 - 1 - temp_r_off] != 0) return obj_id;

		if (col_offset) {
			int obj_id_2 = LEVEL_DESC[akt_level + temp_r_start][start_col + 1];

			char left_base = (obj_id <= 6 && obj_id >= 2) ? MARIO[(obj_id + 1) * 8 - 1 - temp_r_off] : 0;
			char right_base = (obj_id_2 <= 6 && obj_id_2 >= 2) ? MARIO[(obj_id_2 + 1) * 8 - 1 - temp_r_off] : 0;

			bool l_res = check_left_pixel(left_base, col_offset);
			bool r_res = check_right_pixel(right_base, col_offset);

			if (l_res) return obj_id;
			if (r_res) return obj_id_2;
		}

	}
	default:
		break;
	}

	return -1; // nincs collision
}


static bool process_collision(int mode, int akt_collision){
	if (akt_collision == 0) return true;
	if (akt_collision == 2 || akt_collision == 4){
		jump = false;
		return true;
	}

	switch (mode)
	{
	case MARIO_LEFT:
	case MARIO_RIGHT:
	{	
		if (akt_collision == 6){ // Fire
			// Death
		}

		if (akt_collision == 3 || akt_collision == 5) {
			health --;
			if(mode == MARIO_LEFT) move_m(MARIO_RIGHT, 2);
			else move_m(MARIO_LEFT, 2);
		}

		return false;
	}
	case MARIO_DOWN:
	{	

		if (akt_collision == 6){ // Fire
			// Death
		}

		if (akt_collision == 3) {
			score ++;
			health ++;

			if (LEVEL_DESC[akt_level + start_row][start_col] == akt_collision) LEVEL_DESC[akt_level + start_row][start_col] = ' '; 
			if (LEVEL_DESC[akt_level + start_row][start_col + 1] == akt_collision) LEVEL_DESC[akt_level + start_row][start_col + 1] = ' ';

			return false;
			
		} else if (akt_collision == 5) {
			health -= 2;
			if(mode == MARIO_LEFT) move_m(MARIO_RIGHT, 4);
			else move_m(MARIO_LEFT, 4);
		}
	}
	default:
		break;
	}

	return false;
}


static void update_position(int mode){
	switch (mode)
	{
	case MARIO_RIGHT:
	{
		face_right = true;

		int akt_collision = collide(mode);
		if (process_collision(MARIO_RIGHT, akt_collision)) return;


		if (col_offset == 4){
			start_col += 1;
		}

		col_offset = (face_right ? (col_offset + 1) : (col_offset - 1)) % 5;

		if (collide(MARIO_DOWN) == -1) {
			fall = true;
		}

		break;
	}
	case MARIO_LEFT:
	{
		face_right = false;

		int akt_collision = collide(mode);
		if (process_collision(MARIO_LEFT, akt_collision)) return;


		if (col_offset == 0)
		{
			start_col -= 1;
			col_offset = 4;
		} else 
		{
			col_offset = (face_right ? (col_offset + 1) : (col_offset - 1)) % 5;
		}

		if (collide(MARIO_DOWN) == -1) {
			fall = true;
		}

		break;
	}
	case MARIO_UP:
	{
		if (row_offset == 7)
		{
			start_row = 0;
		}

		row_offset = (row_offset + 1) % 8;

		break;
	}
	case MARIO_DOWN:
	{	
		int akt_collision = collide(mode);
		if (process_collision(MARIO_DOWN, akt_collision)) return;


		if (row_offset == 0 && start_row == 0)
		{
			row_offset = 7;
			start_row = 1;
		}
		else
		{
			row_offset -= 1;
		}
		break;
	}
	default:
		break;
	}
}



void update_position_n(int mode, int n){
	for (int i = 0; i < n; i++)
	{
		update_position(mode);
	}
}


void move_m(int mode, int n){ 
	if (n == 1) {
		update_position(mode);	
	} else {
		update_position_n(mode, n);
	}
	update_mario_buffer(); 
	update(); 
}


int main() {
	port_init();
	lcd_init();
	
	// Game variables
	unsigned int action = A_NONE;

	// Initialization
	load_level(akt_level);
	init_CG_CONTENT(akt_level);

	init_screen();

	// Game Loop
	while(1)
	{	
		action = is_pressed();
		
		

		if (fall) {
			if(fall_off == 0) {
				move_m(MARIO_DOWN, 1);
				fall_off ++;
			} else if (fall_off <= 3) {
				move_m(MARIO_DOWN, 2);
				fall_off ++;				
			} else {
				move_m(MARIO_DOWN, 3);
				// Standing ?
				if(collide(MARIO_DOWN) == -1){
					fall_off = 3;
				} else {
					fall_off = 0;
					fall = false;
				}
			}
		}

		if (jump) {
			if (jump_off <= 3){
				jump_off ++;
				move_m(MARIO_UP, 2);
			
			} else {
				jump = false;
				jump_off = 0;
				fall = true;
				move_m(MARIO_UP, 1);
			}
		}

		// Process the input
		if(action == A_LEFT)
		{	
			move_m(MARIO_LEFT, 2);
		}	
		else if (action == A_RIGHT)
		{
			move_m(MARIO_RIGHT, 2);
		}
		else if (action == A_JUMP && (!jump) && (!fall)) {
			jump = true;
			jump_off += 1;
			move_m(MARIO_UP, 3);
		}
				
	}
		
}

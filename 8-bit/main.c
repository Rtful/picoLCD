#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "LCD.h"
/*
Copyright (c) 2021, zadi15 (https://github.com/zadi15/)
License can be found at picoLCD/LICENSE
*/

unsigned int LCD_data_pins[8] = {0,1,2,3,4,5,6,7};
unsigned int LCD_E_pin = 15;
unsigned int LCD_RW_pin = 17;
unsigned int LCD_RS_pin = 16;

int main(void) {
    bi_decl(bi_program_description("This is a work-in-progress example of interfacing with LCD Displays using HD44780 chips on the Raspberry Pi Pico!"));

    stdio_init_all();

    LCD_init_pins();

    //Initialize and clear the LCD, prepping it for characters / instructions
    LCD_init();
    LCD_clear();
    LCD_print("Hello World!");
}

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "pico/multicore.h"
#include "pico/time.h"
#include "pico/sync.h"
#include "LCD.h"
/*
Copyright (c) 2021, zadi15 (https://github.com/zadi15/)
License can be found at picoLCD/LICENSE
*/

// core1 data
unsigned int LCD_data_pins[8] = {0,1,2,3,4,5,6,7};
unsigned int LCD_E_pin = 15;
unsigned int LCD_RW_pin = 17;
unsigned int LCD_RS_pin = 16;

#define FILENAME_BUF_SIZE 20

// shared data
static critical_section_t shared_lock;
static char shared_filename[FILENAME_BUF_SIZE] = {0};
static uint8_t shared_changed = 0;

static int shared_ch = EOF;

// core0 data
static char core0_filename[FILENAME_BUF_SIZE];

static void core1(void) {
    LCD_init_pins();

    //Initialize and clear the LCD, prepping it for characters / instructions
    LCD_init();
    LCD_create_char(1, (const char[8]){
        0b00110,
        0b00110,
        0b10110,
        0b11100,
        0b11110,
        0b00011,
        0b00011,
        0b00000,
    });
    LCD_create_char(2, (const char[8]){
        0b11111,
        0b11111,
        0b11111,
        0b01110,
        0b01110,
        0b01110,
        0b00100,
        0b00100,
    });
    LCD_create_char(3, (const char[8]){
        0b00111,
        0b00111,
        0b01111,
        0b01110,
        0b01110,
        0b11110,
        0b11100,
        0b11100,
    });
    LCD_clear();
    LCD_print("Hello World!\001");
    LCD_col_row(0, 3);
    LCD_print("Temp\002210\337C\37760\337C\00360\337C");
    LCD_col_row(0, 2);
    LCD_print("Tot 0h00m");
    LCD_col_row(11, 2);
    LCD_print("Rem 0h00m");
    while (1) {
        critical_section_enter_blocking(&shared_lock);
        if (shared_ch != EOF)
            LCD_print_char(shared_ch);
        shared_ch = EOF;
        critical_section_exit(&shared_lock);
    }
}

static void core0(void) {
    while (1) {
        int ch = stdio_getchar();
        if (ch > 255) break;
        critical_section_enter_blocking(&shared_lock);
        shared_ch = ch;
        critical_section_exit(&shared_lock);
    }
}

int main(void) {
    bi_decl(bi_program_description("This is a work-in-progress example of interfacing with LCD Displays using HD44780 chips on the Raspberry Pi Pico!"));

    stdio_init_all();
    critical_section_init(&shared_lock);

    multicore_launch_core1(core1);
    core0();
}

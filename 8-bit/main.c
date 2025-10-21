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

#define FILENAME_BUF_SIZE 21
#define CHANGED_FILENAME (1 << 0)
#define CHANGED_TOTAL (1 << 1)
#define CHANGED_REMAINING (1 << 2)
#define CHANGED_TEMP_N (1 << 3)
#define CHANGED_TEMP_E (1 << 4)
#define CHANGED_TEMP_B (1 << 5)

// core1 data
unsigned int LCD_data_pins[8] = {0,1,2,3,4,5,6,7};
unsigned int LCD_E_pin = 15;
unsigned int LCD_RW_pin = 17;
unsigned int LCD_RS_pin = 16;
static char core1_filename[FILENAME_BUF_SIZE] = {0};
static char core1_temp_n[4] = {0};
static char core1_temp_e[4] = {0};
static char core1_temp_b[4] = {0};
static char core1_total[6] = {0};
static char core1_remaining[6] = {0};

// shared data
static critical_section_t shared_lock;
static char shared_filename[FILENAME_BUF_SIZE] = {0};
static char shared_temp_n[4] = {0};
static char shared_temp_e[4] = {0};
static char shared_temp_b[4] = {0};
static char shared_total[6] = {0};
static char shared_remaining[6] = {0};
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
        int ch = shared_ch;
        uint8_t changed = shared_changed;
        memcpy(core1_temp_n, shared_temp_n, sizeof core1_temp_n);
        memcpy(core1_temp_e, shared_temp_e, sizeof core1_temp_e);
        memcpy(core1_temp_b, shared_temp_b, sizeof core1_temp_b);
        memcpy(core1_total, shared_total, sizeof core1_total);
        memcpy(core1_remaining, shared_remaining, sizeof core1_remaining);
        if (changed & CHANGED_FILENAME)
            strcpy(core1_filename, shared_filename);
        shared_ch = EOF;
        shared_changed = 0;
        critical_section_exit(&shared_lock);

        if (ch != EOF)
            LCD_print_char(ch);

        if (changed & CHANGED_TEMP_N) {
            LCD_col_row(5, 3);
            LCD_print(core1_temp_n);
        }
        if (changed & CHANGED_TEMP_E) {
            LCD_col_row(11, 3);
            LCD_print(core1_temp_e);
        }
        if (changed & CHANGED_TEMP_B) {
            LCD_col_row(16, 3);
            LCD_print(core1_temp_b);
        }
        if (changed & CHANGED_FILENAME) {
            LCD_col_row(0, 0);
            LCD_print("                    ");
            LCD_col_row(0, 0);
            LCD_print(core1_filename);
        }
    }
}

static void core0(void) {
    while (1) {
        int ch = stdio_getchar();
        if (ch > 255) break;
        switch (ch) {
            case 'F':
                char *end = &core0_filename[FILENAME_BUF_SIZE - 1];
                for (char *ptr = core0_filename; ch && ptr != end; ptr++)
                    *ptr = ch = stdio_getchar();
                *end = 0;
                while (ch)
                    ch = stdio_getchar();
                critical_section_enter_blocking(&shared_lock);
                shared_changed |= CHANGED_FILENAME;
                strcpy(shared_filename, core0_filename);
                critical_section_exit(&shared_lock);
                break;
            default:
                critical_section_enter_blocking(&shared_lock);
                shared_ch = ch;
                critical_section_exit(&shared_lock);
                break;
        }
    }
}

int main(void) {
    bi_decl(bi_program_description("This is a work-in-progress example of interfacing with LCD Displays using HD44780 chips on the Raspberry Pi Pico!"));

    stdio_init_all();
    critical_section_init(&shared_lock);

    multicore_launch_core1(core1);
    core0();
}

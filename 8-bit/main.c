#include <stdio.h>
#include <string.h>
#include <stdlib.h>
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

#define FILENAME_BUF_SIZE 256
#define CHANGED_FILENAME (1 << 0)
#define CHANGED_TOTAL (1 << 1)
#define CHANGED_REMAINING (1 << 2)
#define CHANGED_TEMP_N (1 << 3)
#define CHANGED_TEMP_E (1 << 4)
#define CHANGED_TEMP_B (1 << 5)
#define CHANGED_PROG (1 << 6)

#define PROG_BAR_LEN 16
#define SCROLL_SPACE 10

// core1 data
unsigned int LCD_data_pins[8] = {0,1,2,3,4,5,6,7};
unsigned int LCD_E_pin = 15;
unsigned int LCD_RW_pin = 17;
unsigned int LCD_RS_pin = 16;

// shared data
static critical_section_t shared_lock;
static char shared_filename[FILENAME_BUF_SIZE] = {0};
static char shared_temp_n[4] = {0};
static char shared_temp_e[4] = {0};
static char shared_temp_b[4] = {0};
static char shared_total[6] = {0};
static char shared_remaining[6] = {0};
static char shared_prog_row[21] = {0};
static uint8_t shared_changed = 0;

// core0 data
static char core0_filename[FILENAME_BUF_SIZE];

static void core1(void) {
    char core1_filename[FILENAME_BUF_SIZE + SCROLL_SPACE + 20] = {0};
    char core1_temp_n[4] = {0};
    char core1_temp_e[4] = {0};
    char core1_temp_b[4] = {0};
    char core1_total[6] = {0};
    char core1_remaining[6] = {0};
    char core1_prog_row[21] = {0};
    char *core1_filename_pos = core1_filename;
    char *core1_filename_end = core1_filename;
    bool core1_reprint_filename = false;
    int core1_filename_scroll = 0;
    memset(core1_filename, ' ', SCROLL_SPACE);
    core1_filename[SCROLL_SPACE] = 0;
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
    LCD_create_char(4, (const char[8]){
        0b01010,
        0b00000,
        0b01110,
        0b10001,
        0b10001,
        0b11111,
        0b10001,
        0b10001,
    });
    LCD_create_char(5, (const char[8]){
        0b01010,
        0b00000,
        0b01110,
        0b10001,
        0b10001,
        0b10001,
        0b10001,
        0b01110,
    });
    LCD_create_char(6, (const char[8]){
        0b01010,
        0b00000,
        0b10001,
        0b10001,
        0b10001,
        0b10001,
        0b10001,
        0b01110,
    });
    LCD_clear();
    LCD_instruction(0b00001100);
    LCD_col_row(0, 3);
    LCD_print("Temp\002   \337C\377  \337C\003  \337C");
    LCD_col_row(0, 2);
    LCD_print("Tot 0h00m");
    LCD_col_row(11, 2);
    LCD_print("Rem 0h00m");
    while (1) {
        sleep_ms(350);
        critical_section_enter_blocking(&shared_lock);
        const uint8_t changed = shared_changed;
        memcpy(core1_temp_n, shared_temp_n, sizeof core1_temp_n);
        memcpy(core1_temp_e, shared_temp_e, sizeof core1_temp_e);
        memcpy(core1_temp_b, shared_temp_b, sizeof core1_temp_b);
        memcpy(core1_total, shared_total, sizeof core1_total);
        memcpy(core1_remaining, shared_remaining, sizeof core1_remaining);
        memcpy(core1_prog_row, shared_prog_row, sizeof core1_prog_row);
        if (changed & CHANGED_FILENAME)
            core1_filename_end = stpcpy(core1_filename + SCROLL_SPACE, shared_filename);
        shared_changed = 0;
        critical_section_exit(&shared_lock);

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
        if (changed & CHANGED_TOTAL) {
            LCD_col_row(4, 2);
            LCD_print(core1_total);
        }
        if (changed & CHANGED_REMAINING) {
            LCD_col_row(15, 2);
            LCD_print(core1_remaining);
        }
        if (changed & CHANGED_PROG) {
            LCD_col_row(0, 1);
            LCD_print(core1_prog_row);
        }
        if (changed & CHANGED_FILENAME) {
            core1_filename_pos = core1_filename + SCROLL_SPACE;
            char buf[21];
            snprintf(buf, sizeof buf, "%20s", core1_filename);
            strcpy(core1_filename_end, buf);
            core1_reprint_filename = true;
        }
        if (core1_reprint_filename) {
            core1_reprint_filename = false;
            LCD_col_row(0, 0);
            for (int i = 0; i < 20; i++)
                LCD_print_char(core1_filename_pos[i]);
        }
        if (core1_filename_end - core1_filename > 30 && !core1_filename_scroll) {
            if (++core1_filename_pos == core1_filename_end)
                core1_filename_pos = core1_filename;
            core1_reprint_filename = true;
        }
        if (!core1_filename_scroll--)
            core1_filename_scroll = 4;
    }
}

static char filename_getchar(void) {
    static int saved = EOF;
    char ch;
    if (saved != EOF) {
        ch = saved;
        saved = EOF;
        return ch;
    }
    if ((ch = stdio_getchar()) == 0xc3) {
        const char ch2 = stdio_getchar();
        switch (ch2) {
            case 0x84: // Ä
                return '\004';
            case 0x96: // Ö
                return '\005';
            case 0x9c: // Ü
                return '\006';
            case 0xa4: // ä
                return '\341';
            case 0xb6: // ö
                return '\357';
            case 0xbc: // ü
                return '\365';
            default:
                saved = ch2;
                break;
        }
    }
    return ch;
}

static void read_buf(char *buf, size_t size) {
    char *const end = buf + size - 1;
    char ch = true;
    for (char *ptr = buf; ch && ptr != end; ptr++)
        *ptr = ch = stdio_getchar();
    *end = 0;
    while (ch)
        ch = stdio_getchar();
}

static void core0(void) {
    while (1) {
        int ch = stdio_getchar();
        char buf[4];
        int minutes;
        int hours;
        switch (ch) {
            case '\0':
                break;
            case 'F':
                char *const end = &core0_filename[FILENAME_BUF_SIZE - 1];
                for (char *ptr = core0_filename; ch && ptr != end; ptr++)
                    *ptr = ch = filename_getchar();
                *end = 0;
                while (ch)
                    ch = stdio_getchar();
                critical_section_enter_blocking(&shared_lock);
                shared_changed |= CHANGED_FILENAME;
                strcpy(shared_filename, core0_filename);
                critical_section_exit(&shared_lock);
                break;
            case 'P':
                read_buf(buf, sizeof buf);
                const int prog = atoi(buf);
                const int bar_len = prog * PROG_BAR_LEN / 100;
                critical_section_enter_blocking(&shared_lock);
                shared_changed |= CHANGED_PROG;
                for (int i = 0; i < bar_len && i < PROG_BAR_LEN; i++)
                    shared_prog_row[i] = '\377';
                for (int i = bar_len; i < PROG_BAR_LEN; i++)
                    shared_prog_row[i] = ' ';
                snprintf(shared_prog_row + PROG_BAR_LEN, sizeof shared_prog_row - PROG_BAR_LEN, "%3s%%", buf);
                critical_section_exit(&shared_lock);
                break;
            case 'T':
                read_buf(buf, sizeof buf);
                minutes = atoi(buf);
                hours = minutes / 60;
                minutes %= 60;
                critical_section_enter_blocking(&shared_lock);
                shared_changed |= CHANGED_TOTAL;
                snprintf(shared_total, sizeof shared_total, "%dh%02dm", hours, minutes);
                critical_section_exit(&shared_lock);
                break;
            case 'R':
                read_buf(buf, sizeof buf);
                minutes = atoi(buf);
                hours = minutes / 60;
                minutes %= 60;
                critical_section_enter_blocking(&shared_lock);
                shared_changed |= CHANGED_REMAINING;
                snprintf(shared_remaining, sizeof shared_remaining, "%dh%02dm", hours, minutes);
                critical_section_exit(&shared_lock);
                break;
            case 'N':
                read_buf(buf, sizeof buf);
                critical_section_enter_blocking(&shared_lock);
                shared_changed |= CHANGED_TEMP_N;
                snprintf(shared_temp_n, sizeof shared_temp_n, "%3s", buf);
                critical_section_exit(&shared_lock);
                break;
            case 'E':
                read_buf(buf, sizeof buf);
                critical_section_enter_blocking(&shared_lock);
                shared_changed |= CHANGED_TEMP_E;
                snprintf(shared_temp_e, sizeof shared_temp_e, "%2s\337", buf);
                critical_section_exit(&shared_lock);
                break;
            case 'B':
                read_buf(buf, sizeof buf);
                critical_section_enter_blocking(&shared_lock);
                shared_changed |= CHANGED_TEMP_B;
                snprintf(shared_temp_b, sizeof shared_temp_b, "%2s\337", buf);
                critical_section_exit(&shared_lock);
                break;
            default:
                while (ch = stdio_getchar());
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

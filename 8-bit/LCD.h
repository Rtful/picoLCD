#include <stdint.h>

// LSB first
extern unsigned int LCD_data_pins[8];

extern unsigned int LCD_E_pin;
extern unsigned int LCD_RW_pin;
extern unsigned int LCD_RS_pin;

void LCD_init_pins();
void LCD_instruction(uint8_t inst);
void LCD_data(uint8_t data);
void LCD_goto(uint8_t pos);
void LCD_print_char(char c);
void LCD_print(char *s);
void LCD_init();
void LCD_clear();
void LCD_col_row(uint8_t col, uint8_t row);
void LCD_create_char(uint8_t num, uint8_t rows[8]);

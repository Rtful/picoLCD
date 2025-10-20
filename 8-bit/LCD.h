#include <stdint.h>

// LSB first
extern unsigned int LCD_data_pins[8];

extern unsigned int LCD_E_pin;
extern unsigned int LCD_RW_pin;
extern unsigned int LCD_RS_pin;

void LCD_init_pins(void);
void LCD_instruction(const uint8_t inst);
void LCD_data(const uint8_t data);
void LCD_goto(const uint8_t pos);
void LCD_print_char(const char c);
void LCD_print(const char *s);
void LCD_init(void);
void LCD_clear(void);
void LCD_col_row(const uint8_t col, const uint8_t row);
void LCD_create_char(const uint8_t num, const uint8_t rows[8]);

#include "LCD.h"
#include "hardware/gpio.h"

// LSB first
extern unsigned int LCD_data_pins[8];

extern unsigned int LCD_E_pin;
extern unsigned int LCD_RW_pin;
extern unsigned int LCD_RS_pin;

void LCD_init_pins() {
    for (int i = 0; i < 8; i++) {
        gpio_init(LCD_data_pins[i]);
        gpio_set_dir(LCD_data_pins[i], true);
        gpio_put(LCD_data_pins[i], false);
    }
    gpio_init(LCD_E_pin);
    gpio_set_dir(LCD_E_pin, true);
    gpio_put(LCD_E_pin, false);
    gpio_init(LCD_RW_pin);
    gpio_set_dir(LCD_RW_pin, true);
    gpio_put(LCD_RW_pin, false);
    gpio_init(LCD_RS_pin);
    gpio_set_dir(LCD_RS_pin, true);
    gpio_put(LCD_RS_pin, false);
}

static void put_byte(uint8_t data) {
    for (int i = 0; i < 8; i++) {
        gpio_put(LCD_data_pins[i], data & (1 << i) ? true : false);
    }
}

static void lcd_wait() {
    gpio_put(LCD_RS_pin, false);
    for (int i = 0; i < 8; i++)
        gpio_set_dir(LCD_data_pins[i], false);
    gpio_put(LCD_RW_pin, true);

    for (;;) {
        gpio_put(LCD_E_pin, true);
        if (!gpio_get(LCD_data_pins[7]))
            break;
        gpio_put(LCD_E_pin, false);
    }
    gpio_put(LCD_E_pin, false);

    for (int i = 0; i < 8; i++)
        gpio_set_dir(LCD_data_pins[i], true);
}

void LCD_instruction(uint8_t inst) {
    lcd_wait();
    gpio_put(LCD_RW_pin, false);
    put_byte(inst);
    gpio_put(LCD_E_pin, true);
    gpio_put(LCD_E_pin, false);
}

void LCD_data(uint8_t data) {
    lcd_wait();
    gpio_put(LCD_RW_pin, false);
    gpio_put(LCD_RS_pin, true);
    put_byte(data);
    gpio_put(LCD_E_pin, true);
    gpio_put(LCD_E_pin, false);
}

void LCD_goto(uint8_t pos) {
    LCD_instruction(0x80 | (uint8_t) pos);
}

void LCD_print_char(char c) {
    LCD_data((uint8_t) c);
}

void LCD_print(char *s) {
    while (*s)
        LCD_print_char(*s++);
}

void LCD_init() {
    LCD_instruction(0b00111000);
    LCD_instruction(0b00111000);
    LCD_instruction(0b00111000);
    LCD_instruction(0b00001110);
    LCD_instruction(0b00000110);
}

void LCD_clear() {
    LCD_instruction(0x01);
    LCD_instruction(0x02);
    LCD_instruction(0x80);
}

void LCD_col_row(uint8_t col, uint8_t row) {
    switch (row) {
        case 0:
            LCD_goto(col);
            break;
        case 1:
            LCD_goto(col + 0x40);
            break;
        case 2:
            LCD_goto(col + 0x14);
            break;
        case 3:
            LCD_goto(col + 0x54);
            break;
    }
}

void LCD_create_char(uint8_t num, uint8_t rows[8]) {
    LCD_instruction(0x40 | ((num & 0x7) << 3));
    for (int i = 0; i < 8; i++)
        LCD_data(rows[i]);
}

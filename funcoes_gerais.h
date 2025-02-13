#ifndef FUNCOES_GERAIS_H
#define FUNCOES_GERAIS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "ssd1306.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"

// Declaração das variáveis globais
extern uint pos_y;
extern const int LED_B;
extern const int LED_R;
extern const int LED_G;
extern const int SW;
extern const int eixoX;
extern const int eixoY;
extern const int ADC_CHANNEL_0;
extern const int ADC_CHANNEL_1;
extern const int BotaoA;
extern const int BotaoB;
extern int valorAgua;
extern ssd1306_t display;
extern volatile bool botaoA_pressionado;
extern volatile bool botaoB_pressionado;
extern volatile bool button_pressionado;

void button_callback(uint gpio, uint32_t events);
void buttonAB_callback(uint gpio, uint32_t events);
void pwm_init_buzzer(uint pin);
void play_tone(uint pin, uint frequency, uint duration_ms);
void play_star_wars(uint pin);
void setup_pwm_led(uint led, uint *slice, uint16_t level);
void inicializacao();
void print_texto(char *msg, uint pos_x, uint pos_y, uint scale);
void print_retangulo(int x1, int x2, int y1, int y2);
void print_menu(uint posy);
void joystick_read_axis(uint16_t *vrx_value, uint16_t *vry_value);
void joystick_led_control();
void pwm_led();
void addAgua();

#endif // FUNCOES_GERAIS_H
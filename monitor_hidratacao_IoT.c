#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "ssd1306.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "funcoes_gerais.h"









int main()
{
    inicializacao();
    char *text = "";
    uint countDown = 0;
    uint countUp = 1;   // LEMBRAR DE AJUSTAR QUANDO ADICIONAR MAIS OPCOES AO MENU
    uint pos_y_anterior = 19; 
    uint menu = 1; // posicao das opcoes do menu - sera incrementado ou decrementado conforme o movimento do joystick

    print_menu(pos_y);

    

    
    while (true) {
    gpio_put(LED_B, 0);
    gpio_put(LED_G, 0);
    gpio_put(LED_R, 0);
    adc_select_input(0);
    // Lê o valor do ADC para o eixo Y
    uint adc_y_raw = adc_read();
    const uint bar_width = 40;
    const uint adc_max = (1 << 12) - 1;
    uint bar_y_pos = adc_y_raw * bar_width / adc_max; // bar_y_pos determinara se o Joystick foi pressionado para cima ou para baixo
    if(bar_y_pos < 14 && countDown < 1){ // LEMBRAR DE AJUSTAR QUANDO ADICIONAR MAIS OPCOES AO MENU
        pos_y += 12;
        countDown += 1;
        countUp -= 1;
        menu ++; // incrementa a posicao do menu
    }
    else{
        if(bar_y_pos > 24 && countUp < 1){ // LEMBRAR DE AJUSTAR QUANDO ADICIONAR MAIS OPCOES AO MENU
        pos_y -= 12;
        countDown -= 1;
        countUp += 1;
        menu --; // incrementa a posicao do menu
    }
    }
    sleep_ms(100);
    if(pos_y != pos_y_anterior){
        print_menu(pos_y);
       
    }

    /*switch (menu){
        case 1:
            


        
    }*/
    sleep_ms(100);
    pos_y_anterior = pos_y; // atualiza a posicao do cursor para a origem
}
}
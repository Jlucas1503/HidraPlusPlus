#include "funcoes_gerais.h"
#include <stdio.h>        // Biblioteca padrão de entrada e saída
#include "hardware/adc.h" // Biblioteca para manipulação do ADC no RP2040
#include "hardware/pwm.h" // Biblioteca para controle de PWM no RP2040
#include "hardware/clocks.h"
#include "pico/stdlib.h"  // Biblioteca padrão do Raspberry Pi Pico
#include "hardware/i2c.h" // Biblioteca para comunicação I2C no RP2040
#include "ssd1306.h"      // Biblioteca para controle do display OLED SSD1306
volatile bool button_pressionado = false;
// PINOS I2C
#define I2C_PORT i2c1 // Define o barramento I2C a ser utilizado
#define PINO_SCL 14
#define PINO_SDA 15
#define BUZZER_PIN 21
const int BotaoB = 6;
const int BotaoA = 5;

int valorAgua = 0; // Variável para armazenar a quantidade de água ingerida

const int LED_B = 12;                    // Pino para controle do LED azul via PWM
const int LED_R = 13;                    // Pino para controle do LED vermelho via PWM
const int LED_G = 11;                    // Pino para controle do LED verde via PWM
const int SW = 22;           // Pino de leitura do botão do joystick
volatile bool botaoA_pressionado = false;
volatile bool botaoB_pressionado = false;


// Pino de leitura do botão A


uint pos_y = 14; // Posição inicial do cursor no eixo Y
// Configura a interrupção para o botão

// Definição dos pinos usados para o joystick e LEDs
const int eixoX = 26;          // Pino de leitura do eixo X do joystick (conectado ao ADC)
const int eixoY = 27;          // Pino de leitura do eixo Y do joystick (conectado ao ADC)
const int ADC_CHANNEL_0 = 0; // Canal ADC para o eixo X do joystick
const int ADC_CHANNEL_1 = 1; // Canal ADC para o eixo Y do joystick

ssd1306_t display; // inicializa o objeto do display OLED






/********************* */
void button_callback(uint gpio, uint32_t events) {
    static uint64_t last_press_time = 0;
    uint64_t current_time = time_us_64();
    
    // Debounce de 200ms
    if ((current_time - last_press_time) > 200000) {
        if (gpio == SW) {
            button_pressionado = true;
        }
        last_press_time = current_time;
    }
}





/********************************** */
void buttonAB_callback(uint gpio, uint32_t events) {
    static uint64_t last_press_time = 0;
    uint64_t current_time = time_us_64();
    
    // Debounce de 200ms
    if ((current_time - last_press_time) < 200000) return;

    if (gpio == BotaoA) {
        botaoA_pressionado = true;
    } else if (gpio == BotaoB) {
        botaoB_pressionado = true;
    }
    last_press_time = current_time;
}
/*********************** */


///////////////////////////////////////////

/*Função para o programa Buzzer pwm1 */


// Inicializa o PWM no pino do buzzer
void pwm_init_buzzer(uint pin) {
    gpio_set_function(pin, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(pin);
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 4.0f); // Ajusta divisor de clock
    pwm_init(slice_num, &config, true);
    pwm_set_gpio_level(pin, 0); // Desliga o PWM inicialmente
}

// Toca uma nota com a frequência e duração especificadas
void play_tone(uint pin, uint frequency, uint duration_ms) {
    uint slice_num = pwm_gpio_to_slice_num(pin);
    uint32_t clock_freq = clock_get_hz(clk_sys);
    uint32_t top = clock_freq / frequency - 1;

    pwm_set_wrap(slice_num, top);
    pwm_set_gpio_level(pin, top / 2);

    uint64_t start_time = time_us_64();
    while ((time_us_64() - start_time) < duration_ms * 1000) {
        if (button_pressionado) {
            break;
        }
        sleep_ms(1);
    }
    
    pwm_set_gpio_level(pin, 0);
    sleep_ms(50);
}




// variaveis para os leds
//configuração do programa de JOYSTICK_LED ---- PWM
const float DIVIDER_PWM = 16.0;          // Divisor fracional do clock para o PWM
const uint16_t PERIOD = 4096;            // Período do PWM (valor máximo do contador)
uint16_t led_b_level, led_r_level = 100; // Inicialização dos níveis de PWM para os LEDs
uint slice_led_b, slice_led_r;           // Variáveis para armazenar os slices de PWM correspondentes aos LEDs

void setup_pwm_led(uint led, uint *slice, uint16_t level)
{
  gpio_set_function(led, GPIO_FUNC_PWM); // Configura o pino do LED como saída PWM
  *slice = pwm_gpio_to_slice_num(led);   // Obtém o slice do PWM associado ao pino do LED
  pwm_set_clkdiv(*slice, DIVIDER_PWM);   // Define o divisor de clock do PWM
  pwm_set_wrap(*slice, PERIOD);          // Configura o valor máximo do contador (período do PWM)
  pwm_set_gpio_level(led, level);        // Define o nível inicial do PWM para o LED
  pwm_set_enabled(*slice, true);         // Habilita o PWM no slice correspondente ao LED
}

//////////////////////////////////////////////



void inicializacao(){
    stdio_init_all();
    setup_pwm_led(LED_B, &slice_led_b, led_b_level); // Configura o PWM para o LED azul
    setup_pwm_led(LED_R, &slice_led_r, led_r_level); // Configura o PWM para o LED vermelho

    pwm_init_buzzer(BUZZER_PIN);


    // Inicializa o ADC e os pinos de entrada analógica
    adc_init();         // Inicializa o módulo ADC
    adc_gpio_init(eixoX); // Configura o pino 26 (eixo X) para entrada ADC
    adc_gpio_init(eixoY); // Configura o pino 27 (eixo Y) para entrada ADC

    // inicialização do display OLED
    i2c_init(I2C_PORT, 600 * 1000); // Inicializa o barramento I2C com velocidade de 400kHz
    gpio_set_function(PINO_SCL, GPIO_FUNC_I2C); // Configura o pino SCL
    gpio_set_function(PINO_SDA, GPIO_FUNC_I2C); // Configura o pino SDA
    gpio_pull_up(PINO_SCL); // Ativa o pull-up no pino SCL
    gpio_pull_up(PINO_SDA); // Ativa o pull-up no pino SDA
    display.external_vcc = false; // O display não possui tensão de alimentação externa
    ssd1306_init(&display, 128, 64, 0x3c, I2C_PORT); // Inicializa o display OLED
    ssd1306_clear(&display); // Limpa o display


  // Inicializa o pino do botão do joystick
    gpio_init(SW);             // Inicializa o pino do botão
    gpio_set_dir(SW, GPIO_IN); // Configura o pino do botão como entrada
    gpio_pull_up(SW);          // Ativa o pull-up no pino do botão para evitar flutuações

    // leds
    gpio_init(LED_B); // Inicializa o pino do LED azul
    gpio_init(LED_R); // Inicializa o pino do LED vermelho
    gpio_init(LED_G); // Inicializa o pino do LED verde
    gpio_set_function(LED_B, GPIO_OUT); // Configura o pino do LED azul como saida
    gpio_set_function(LED_R, GPIO_OUT); // Configura o pino do LED vermelho como saida
    gpio_set_function(LED_G, GPIO_OUT); // Configura o pino do LED verde como saida

    // botão A
    gpio_init(BotaoA); // Inicializa o pino do botão A
    gpio_set_dir(BotaoA, GPIO_IN); // Configura o pino do botão como entrada
    gpio_pull_up(BotaoA); // Ativa o pull-up no pino do botão para evitar flutuações

    gpio_init(BotaoB); // Inicializa o pino do botão B
    gpio_set_dir(BotaoB, GPIO_IN); // Configura o pino do botão como entrada
    gpio_pull_up(BotaoB); // Ativa o pull-up no pino do botão para evitar flutuações


    // callback para os botoes
    gpio_set_irq_enabled_with_callback(SW, GPIO_IRQ_EDGE_FALL, true, &button_callback);
    gpio_set_irq_enabled_with_callback(BotaoA, GPIO_IRQ_EDGE_FALL, true, &buttonAB_callback);

    //gpio_set_irq_enabled_with_callback(BotaoA, GPIO_IRQ_EDGE_FALL, true, &button_callback); // callback para o botão A
}

void print_texto(char *msg, uint pos_x, uint pos_y, uint scale){ //mensagem, posicao x, posicao y, escala
    ssd1306_draw_string(&display, pos_x, pos_y, scale, msg);
    ssd1306_show(&display);
}

void print_retangulo(int x1, int x2, int y1, int y2){
    ssd1306_draw_empty_square(&display, x1, x2, y1, y2);
    ssd1306_show(&display);
}

void print_menu(uint posy){
    ssd1306_clear(&display); // Limpa o display
    print_texto("HIDRATA", 52, 2, 1); // Imprime a palavra "HIDRATA" no display
    print_retangulo(2, posy, 120, 12); // Imprime um retangulo no display
    print_texto("Add 300ml", 10, 18, 1);
    print_texto("Atualizar server", 10, 30, 1);
    //print_texto("Mostrar agua", 10, 42, 1);


}

void joystick_read_axis(uint16_t *vrx_value, uint16_t *vry_value)
{
    // Reading the X-axis value of the joystick
    adc_select_input(ADC_CHANNEL_0); // Selects the ADC channel for the X-axis
    sleep_us(2);                     // Small delay for stability
    *vrx_value = adc_read();         // Reads the X-axis value (0-4095)

    // Reading the Y-axis value of the joystick
    adc_select_input(ADC_CHANNEL_1); // Selects the ADC channel for the Y-axis
    sleep_us(2);                     // Small delay for stability
    *vry_value = adc_read();         // Reads the Y-axis value (0-4095)
}

void joystick_led_control() {
    // Configuração inicial
    uint16_t vrx_value, vry_value;
    setup_pwm_led(LED_B, &slice_led_b, 0);
    setup_pwm_led(LED_R, &slice_led_r, 0);
    
    // Configura interrupção do botão
    gpio_set_irq_enabled_with_callback(SW, GPIO_IRQ_EDGE_FALL, true, &button_callback);

    printf("Controle LED por Joystick ativado\n");
    
    // Loop principal com verificação de interrupção
    while(!button_pressionado) {
        joystick_read_axis(&vrx_value, &vry_value);
        
        // Mapeia os valores do joystick para PWM
        pwm_set_gpio_level(LED_B, vrx_value);
        pwm_set_gpio_level(LED_R, vry_value);
        
        // Delay não-bloqueante
        sleep_ms(50);
    }
    
    // Cleanup ao sair
    pwm_set_gpio_level(LED_B, 0);
    pwm_set_gpio_level(LED_R, 0);
    button_pressionado = false;
    printf("Retornando ao menu principal\n");
}



// funcao para PWM LED -- programa 3

void pwm_led() {
    // Configuração inicial
    uint16_t led_b_level = 100;
    uint slice_led_g;
    setup_pwm_led(LED_B, &slice_led_b, led_b_level);
    
    // Configura interrupção do botão
    gpio_set_irq_enabled_with_callback(SW, GPIO_IRQ_EDGE_FALL, true, &button_callback);

    printf("Controle PWM do LED ativado\n");
    
    // Loop principal com verificação de interrupção
    while(!button_pressionado) {
        // Aumenta o brilho do LED verde
        led_b_level += 100;
        if (led_b_level > PERIOD) {
            led_b_level = 0;
        }
        pwm_set_gpio_level(LED_B, led_b_level);
        
        // Delay não-bloqueante
        sleep_ms(50);
    }
    
    // Cleanup ao sair
    pwm_set_gpio_level(LED_B, 0);
    button_pressionado = false;
    printf("Retornando ao menu principal\n");

}

/*
void addAgua(void) {
    // Zera as flags para BotaoA e BotaoB e aguarda um tempo para limpar eventos pendentes
    botaoA_pressionado = false;
    botaoB_pressionado = false;
    sleep_ms(300);

    // Habilita as interrupções para BotaoA e BotaoB utilizando a callback buttonAB_callback
    gpio_set_irq_enabled_with_callback(BotaoA, GPIO_IRQ_EDGE_FALL, true, &buttonAB_callback);
    gpio_set_irq_enabled(BotaoB, GPIO_IRQ_EDGE_FALL, true);

    // Loop principal: enquanto BotaoB não for pressionado (flag false), processa o BotaoA
    while (!botaoB_pressionado) {
        if (botaoA_pressionado) {
            valorAgua += 300;                    // Incrementa 300ml a cada pressionamento
            botaoA_pressionado = false;           // Reseta a flag para novo acionamento

            char msg[50];
            sprintf(msg, "Voce adicionou %d ml", valorAgua);
            ssd1306_clear(&display);
            print_texto(msg, 10, 35, 1);
        }
        sleep_ms(50); // Delay curto para polling
    }

    // Desabilita as interrupções para que outros modos não sejam afetados
    gpio_set_irq_enabled(BotaoA, GPIO_IRQ_EDGE_FALL, false);
    gpio_set_irq_enabled(BotaoB, GPIO_IRQ_EDGE_FALL, false);

    // Exibe mensagem de retorno ao menu e limpa o display
    ssd1306_clear(&display);
    print_texto("Retornando ao menu", 10, 35, 1);
    sleep_ms(1000);
    ssd1306_clear(&display);
    ssd1306_show(&display);
}
*/
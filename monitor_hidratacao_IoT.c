#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "ssd1306.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "string.h"
/*#include "funcoes_gerais.h"*/
#include "pico/cyw43_arch.h"
#include "lwip/tcp.h"

// PINOS I2C
#define I2C_PORT i2c1 // Define o barramento I2C a ser utilizado
#define PINO_SCL 14
#define PINO_SDA 15
#define BUZZER_PIN 21

const int LED_B = 12;                    // Pino para controle do LED azul via PWM
const int LED_R = 13;                    // Pino para controle do LED vermelho via PWM
const int LED_G = 11;                    // Pino para controle do LED verde via PWM
const int SW = 22;           // Pino de leitura do botão do joystick

volatile bool botaoA_pressionado = false;
volatile bool botaoB_pressionado = false;

const int BotaoB = 6;
const int BotaoA = 5;



uint pos_y = 14; // Posição inicial do cursor no eixo Y
// Configura a interrupção para o botão

// Definição dos pinos usados para o joystick e LEDs
const int eixoX = 26;          // Pino de leitura do eixo X do joystick (conectado ao ADC)
const int eixoY = 27;          // Pino de leitura do eixo Y do joystick (conectado ao ADC)
const int ADC_CHANNEL_0 = 0; // Canal ADC para o eixo X do joystick
const int ADC_CHANNEL_1 = 1; // Canal ADC para o eixo Y do joystick

ssd1306_t display; // inicializa o objeto do display OLED

/************* */
#define WIFI_SSID "brisa-1685641"  // Substitua pelo nome da sua rede Wi-Fi
#define WIFI_PASS "mklxybza" // Substitua pela senha da sua rede Wi-Fi

// Buffer para respostas HTTP
#define HTTP_RESPONSE "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n" \
                      "<!DOCTYPE html><html><body>" \
                      "<h1>Controle do LED</h1>" \
                      "<p><a href=\"/led/on\">Ligar LED</a></p>" \
                      "<p><a href=\"/led/off\">Desligar LED</a></p>" \
                      "</body></html>\r\n"


/**************************** */


/*void buttonB_callback(uint gpio, uint32_t events) {
    static uint64_t last_press_time = 0;
    uint64_t current_time = time_us_64();
    
    // Debounce de 200ms
    if ((current_time - last_press_time) < 200000) return;

    if (gpio == BotaoB) {
        botaoB_pressionado = true;
    }
    last_press_time = current_time;
}*/





/*FUNÇOES PARA CONEXAO WIFI*/

// Função de callback para processar requisições HTTP
static err_t http_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    if (p == NULL) {
        // Cliente fechou a conexão
        tcp_close(tpcb);
        return ERR_OK;
    }

    // Processa a requisição HTTP
    char *request = (char *)p->payload;

    /*if (strstr(request, "GET /led/on")) {
        gpio_put(LED_PIN, 1);  // Liga o LED
    } else if (strstr(request, "GET /led/off")) {
        gpio_put(LED_PIN, 0);  // Desliga o LED
    } */

    // Envia a resposta HTTP
    tcp_write(tpcb, HTTP_RESPONSE, strlen(HTTP_RESPONSE), TCP_WRITE_FLAG_COPY);

    // Libera o buffer recebido
    pbuf_free(p);

    return ERR_OK;
}

// Callback de conexão: associa o http_callback à conexão
static err_t connection_callback(void *arg, struct tcp_pcb *newpcb, err_t err) {
    tcp_recv(newpcb, http_callback);  // Associa o callback HTTP
    return ERR_OK;
}

// Função de setup do servidor TCP
static void start_http_server(void) {
    struct tcp_pcb *pcb = tcp_new();
    if (!pcb) {
        printf("Erro ao criar PCB\n");
        return;
    }

    // Liga o servidor na porta 80
    if (tcp_bind(pcb, IP_ADDR_ANY, 80) != ERR_OK) {
        printf("Erro ao ligar o servidor na porta 80\n");
        return;
    }

    pcb = tcp_listen(pcb);  // Coloca o PCB em modo de escuta
    tcp_accept(pcb, connection_callback);  // Associa o callback de conexão

    printf("Servidor HTTP rodando na porta 80...\n");
}

/******************************/












/********************* */
volatile bool button_pressionado = false;


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
/*********************** */


///////////////////////////////////////////

/*Função para o programa Buzzer pwm1 */

// Notas musicais para a música tema de Star Wars
const uint star_wars_notes[] = {
    330, 330, 330, 262, 392, 523, 330, 262,
    392, 523, 330, 659, 659, 659, 698, 523,
    415, 349, 330, 262, 392, 523, 330, 262,
    392, 523, 330, 659, 659, 659, 698, 523,
    415, 349, 330, 523, 494, 440, 392, 330,
    659, 784, 659, 523, 494, 440, 392, 330,
    659, 659, 330, 784, 880, 698, 784, 659,
    523, 494, 440, 392, 659, 784, 659, 523,
    494, 440, 392, 330, 659, 523, 659, 262,
    330, 294, 247, 262, 220, 262, 330, 262,
    330, 294, 247, 262, 330, 392, 523, 440,
    349, 330, 659, 784, 659, 523, 494, 440,
    392, 659, 784, 659, 523, 494, 440, 392
};

// Duração das notas em milissegundos
const uint note_duration[] = {
    500, 500, 500, 350, 150, 300, 500, 350,
    150, 300, 500, 500, 500, 500, 350, 150,
    300, 500, 500, 350, 150, 300, 500, 350,
    150, 300, 650, 500, 150, 300, 500, 350,
    150, 300, 500, 150, 300, 500, 350, 150,
    300, 650, 500, 350, 150, 300, 500, 350,
    150, 300, 500, 500, 500, 500, 350, 150,
    300, 500, 500, 350, 150, 300, 500, 350,
    150, 300, 500, 350, 150, 300, 500, 500,
    350, 150, 300, 500, 500, 350, 150, 300,
};

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

// Função principal para tocar a música
void play_star_wars(uint pin) {
    for (int i = 0; i < sizeof(star_wars_notes) / sizeof(star_wars_notes[0]); i++) {
        if (button_pressionado) {
            button_pressionado = false;
            return;
        }
        
        if (star_wars_notes[i] == 0) {
            uint64_t start = time_us_64();
            while ((time_us_64() - start) < note_duration[i] * 1000) {
                if (button_pressionado) {
                    button_pressionado = false;
                    return;
                }
                sleep_ms(1);
            }
        } else {
            play_tone(pin, star_wars_notes[i], note_duration[i]);
        }
    }
    button_pressionado = false;
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


    gpio_init(BotaoA);
    gpio_set_function(BotaoA, GPIO_IN);
    gpio_pull_up(BotaoA);

    gpio_set_irq_enabled_with_callback(SW, GPIO_IRQ_EDGE_FALL, true, &button_callback);
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
    print_texto("MENU", 52, 2, 1); // Imprime a palavra "menu" no display
    print_retangulo(2, posy, 120, 12); // Imprime um retangulo no display
    print_texto("add Agua", 10, 18, 1);
    print_texto("teste2", 10, 30, 1);
    print_texto("teste3", 10, 42, 1);


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












////////////////////////////////////////////////

// FUNCAO DE ADICIONAR AGUA
void addAgua(void) {
    gpio_set_irq_enabled_with_callback(SW, GPIO_IRQ_EDGE_FALL, true, &button_callback);
    

    int valorAgua = 0; // Variável para armazenar a quantidade de água ingerida
    while(!button_pressionado) {
        if (gpio_get(BotaoA) == 0) {
            valorAgua += 300;                    // Incrementa 300ml a cada pressionamento
            char msg[50];
            sprintf(msg, "Voce adicionou %d ml", valorAgua);
            ssd1306_clear(&display);
            print_texto(msg, 10, 35, 1);
            sleep_ms(1000); // Delay curto para polling
        }
        
    }
    // Exibe mensagem de retorno ao menu e limpa o display
    ssd1306_clear(&display);
    button_pressionado = false;
    print_texto("Retornando ao menu", 10, 35, 1);
    sleep_ms(1000);
    ssd1306_clear(&display);
    ssd1306_show(&display);
}











/********************************************************** */










int main(){
    printf("Iniciando...\n");
    //uint16_t vrx_value, vry_value, sw_value; // Variáveis para armazenar os valores do joystick (eixos X e Y) e botão
    inicializacao();
    
    


    char *text = "";
    uint countDown = 0;
    uint countUp = 2;
    uint pos_y_anterior = 19; 
    uint menu = 1; // posicao das opcoes do menu - sera incrementado ou decrementado conforme o movimento do joystick

    print_menu(pos_y);

  /*  
     */
    /******************************************************* */
    // trecho aproveitado do codigo do Joystick.c
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


    if(bar_y_pos < 14 && countDown < 2){
        pos_y += 12;
        countDown += 1;
        countUp -= 1;
        menu ++; // incrementa a posicao do menu
    }
    else{
        if(bar_y_pos > 24 && countUp < 2){
        pos_y -= 12;
        countDown -= 1;
        countUp += 1;
        menu --; // incrementa a posicao do menu
    }
}
    sleep_ms(100);
    if(pos_y != pos_y_anterior){
        print_menu(pos_y);
       // pos_y_anterior = pos_y;
    }

    if (button_pressionado) {
        button_pressionado = false;
        switch(menu) {
            case 1:
            // limpando o display
                ssd1306_clear(&display);
                print_texto("APERTE A ", 20, 18, 1.5);
                print_texto("PARA ADD 300ml", 20, 30, 1.5);
                addAgua();
                break;
            case 2:
                ssd1306_clear(&display);
                print_texto("PROGRAMA 2 ", 20, 18, 1.5);
                print_texto("BUZZER", 20, 30, 1.5);
                play_star_wars(BUZZER_PIN);
                break;

            case 3:
                ssd1306_clear(&display);
                print_texto("PROGRAMA 3 ", 20, 18, 1.5);
                print_texto("PWM LED", 20, 30, 1.5);
                pwm_led();
                break;
            default:
                gpio_put(LED_B, 0);
                gpio_put(LED_G, 0);
                gpio_put(LED_R, 0);
        }
    }
    sleep_ms(100);
    pos_y_anterior = pos_y; // atualiza a posicao do cursor para a origem

    }
}

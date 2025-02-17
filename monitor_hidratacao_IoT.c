#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "hardware/i2c.h"
#include "ssd1306.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "string.h"
/*#include "funcoes_gerais.h"*/
#include "pico/cyw43_arch.h"
#include "lwip/tcp.h"
#include "lwip/dns.h"
#include "lwip/init.h"
#include "lwip/pbuf.h"



// LINK PARA O CANAL PUBLICO DO THINGSPEAK : https://thingspeak.mathworks.com/channels/2838418


/*ETAPA DE VARIAVEIS GLOBAIS*/

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

volatile bool connection_established = false;

const int BotaoB = 6;
const int BotaoA = 5;

int valorAgua = 0; // Variável para armazenar a quantidade de água ingerida


uint pos_y = 14; // Posição inicial do cursor no eixo Y
// Configura a interrupção para o botão

// Definição dos pinos usados para o joystick e LEDs
const int eixoX = 26;          // Pino de leitura do eixo X do joystick (conectado ao ADC)
const int eixoY = 27;          // Pino de leitura do eixo Y do joystick (conectado ao ADC)
const int ADC_CHANNEL_0 = 0; // Canal ADC para o eixo X do joystick
const int ADC_CHANNEL_1 = 1; // Canal ADC para o eixo Y do joystick

ssd1306_t display; // inicializa o objeto do display OLED

struct tcp_pcb *tcp_client_pcb;
ip_addr_t server_ip;



/************* */
// CONEXAO COM WIFI

#define WIFI_SSID "brisa-1685641"  // Substitua pelo nome da sua rede Wi-Fi
#define WIFI_PASS "mklxybza" // Substitua pela senha da sua rede Wi-Fi
#define THINGSPEAK_HOST "api.thingspeak.com"
#define THINGSPEAK_PORT 80
#define API_KEY "EY5DTKZKKZO2HF0I" // chave para escrita no Thingspeak
#define API_KEY_READ "R35Z3BISK9VPH4AD" // cchave para leitura no Thingspeak
#define CHANNEL_ID "2838418"

// Buffer para respostas HTTP
#define HTTP_RESPONSE "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n" \
                      "<!DOCTYPE html><html><body>" \
                      "<h1>Controle do LED</h1>" \
                      "<p><a href=\"/led/on\">Ligar LED</a></p>" \
                      "<p><a href=\"/led/off\">Desligar LED</a></p>" \
                      "</body></html>\r\n"


/**************************** */








/*FUNÇOES PARA CONEXAO WIFI*/

static err_t http_recv_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    if (p == NULL) {
        tcp_close(tpcb);
        return ERR_OK;
    }
    printf("Resposta do ThingSpeak: %.*s\n", p->len, (char *)p->payload);
    pbuf_free(p);
    return ERR_OK;
}

// Callback quando a conexão TCP é estabelecida
static err_t http_connected_callback(void *arg, struct tcp_pcb *tpcb, err_t err) {
    if (err != ERR_OK) {
        printf("Erro na conexão TCP\n");
        return err;
    }


    printf("Conectado ao ThingSpeak!\n");
    connection_established = true;
    return ERR_OK;
}

// Resolver DNS e conectar ao servidor
static void dns_callback(const char *name, const ip_addr_t *ipaddr, void *callback_arg) {
    if (ipaddr) {
        printf("Endereço IP do ThingSpeak: %s\n", ipaddr_ntoa(ipaddr));
        tcp_client_pcb = tcp_new();
        tcp_connect(tcp_client_pcb, ipaddr, THINGSPEAK_PORT, http_connected_callback);
    } else {
        printf("Falha na resolução de DNS\n");
    }
}


// Callback específico para leitura do último valor
static err_t http_read_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    if (p == NULL) {
        tcp_close(tpcb);
        return ERR_OK;
    }
    
    // Converte o conteúdo para string
    char *payload = (char *)p->payload;
    
    // Busca o início do corpo da resposta HTTP (após os headers)
    char *body = strstr(payload, "\r\n\r\n");
    if (body) {
        body += 4; // pula o delimitador
    } else {
        // Caso não haja headers, usa a resposta inteira
        body = payload;
    }
    
    // Separa o valor numérico (pode ser que haja espaços ou quebras de linha)
    int valor = atoi(body);
    valorAgua = valor;
    
    printf("Resposta do ThingSpeak (leitura): %s\n", body);
    printf("VALOR DA ÁGUA: %d\n", valorAgua);
    
    pbuf_free(p);
    return ERR_OK;
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


int64_t timer_callback(alarm_id_t id, void *user_data) {
    // Código a ser executado periodicamente
    printf("Temporizador acionado!\n");
    
    // Retorna o tempo, em microsegundos, para a próxima execução do callback.
    // Retornando 1000000, o callback será acionado a cada 1 segundo.
    return 1000000;
}




/*********************** */


///////////////////////////////////////////
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
    gpio_init(BotaoB);
    gpio_set_function(BotaoB, GPIO_IN);
    gpio_pull_up(BotaoB);






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
    print_texto("hidra++", 52, 2, 1); // Imprime a palavra "menu" no display
    print_retangulo(2, posy, 120, 12); // Imprime um retangulo no display
    print_texto("add Agua", 10, 18, 1);
    print_texto("Atualizar server", 10, 30, 1);
    print_texto("ler server", 10, 42, 1);


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



















////////////////////////////////////////////////

// FUNCAO DE ADICIONAR AGUA
void addAgua(void) {
    gpio_set_irq_enabled_with_callback(SW, GPIO_IRQ_EDGE_FALL, true, &button_callback);
    
    while (!button_pressionado) {
        if (gpio_get(BotaoA) == 0) {
            valorAgua += 300;                    // Incrementa 300ml a cada pressionamento
            char msg[50];
            sprintf(msg, "Voce adicionou");
            char msg1[20];
            sprintf(msg1, "300ml de agua");
            char msg2[50];
            sprintf(msg2, "Total: %d", valorAgua);
            ssd1306_clear(&display);
            print_texto(msg, 10, 25, 1);
            print_texto(msg1, 10, 40, 1);
            print_texto(msg2, 10, 55, 1);
            sleep_ms(1000);
        }
        else if (gpio_get(BotaoB) == 0) {
            // Evita que o valor fique negativo
            if (valorAgua >= 300) {
                valorAgua -= 300; // Retira 300ml
            }
            char msg[50];
            sprintf(msg, "Voce retirou");
            char msg1[20];
            sprintf(msg1, "300ml de agua");
            char msg2[50];
            sprintf(msg2, "Total: %d", valorAgua);
            ssd1306_clear(&display);
            print_texto(msg, 10, 25, 1);
            print_texto(msg1, 10, 40, 1);
            print_texto(msg2, 10, 55, 1);
            sleep_ms(1000);
        }
    }
    // Exibe mensagem de retorno ao menu, toca alguns bipes e limpa o display
    ssd1306_clear(&display);
    button_pressionado = false;
    print_texto("Retornando ao menu", 10, 35, 1);
    
    // Toca alguns bipes no buzzer (ex: 2 bipes)
    for (int i = 0; i < 3; i++) {
        // Primeiro pisca o LED
    gpio_put(LED_G, 1);
    sleep_ms(300);
    gpio_put(LED_G, 0);
    sleep_ms(100);
    
    // Em seguida toca o buzzer
    play_tone(BUZZER_PIN, 5000, 100);
    }
    
    sleep_ms(2000);
    ssd1306_clear(&display);
    ssd1306_show(&display);
}

void atualizarServer() { // Requisição GET para mandar ao ThingSpeak
    // Se não houver conexão ativa, reconecta
    if(tcp_client_pcb == NULL){
        connection_established = false;
        dns_gethostbyname(THINGSPEAK_HOST, &server_ip, dns_callback, NULL);

        // Aguarda a conexão ser estabelecida
        uint32_t timeout = 0;
        while(!connection_established && timeout < 5000) { // timeout de 5s
            sleep_ms(100);
            timeout += 100;
        }
        if(!connection_established) {
            printf("Conexão não estabelecida.\n");
            return;
        }
    }

    char request[250];
    snprintf(request, sizeof(request),
             "GET /update?api_key=%s&field1=%d HTTP/1.1\r\n"
             "Host: %s\r\n"
             "Connection: close\r\n"
             "\r\n",
             API_KEY, valorAgua, THINGSPEAK_HOST);

    // Envia a requisição via conexão TCP estabelecida
    tcp_write(tcp_client_pcb, request, strlen(request), TCP_WRITE_FLAG_COPY);
    printf("tcp_client_pcb: %p\n", tcp_client_pcb);
    tcp_output(tcp_client_pcb);
    tcp_recv(tcp_client_pcb, http_recv_callback);

    // Aguarda um tempo para a resposta (ou utilize um flag controlado no callback)
    sleep_ms(500);

    // Exibe mensagem e fecha a conexão
    ssd1306_clear(&display);
    print_texto("Retornando ao menu", 10, 35, 1);
    sleep_ms(500);  

    tcp_close(tcp_client_pcb);
    tcp_client_pcb = NULL;
    for (int i = 0; i < 5; i++) {
            // Primeiro pisca o LED
    gpio_put(LED_B, 1);
    sleep_ms(300);
    gpio_put(LED_B, 0);
    sleep_ms(100);
    
    // Em seguida toca o buzzer
    play_tone(BUZZER_PIN, 3000, 100);
    }

    sleep_ms(1500);
    ssd1306_clear(&display);
    ssd1306_show(&display);
}


void lerServer(void) {
    // Se não houver conexão ativa, reconecta
    if (tcp_client_pcb == NULL) {
        connection_established = false;
        dns_gethostbyname(THINGSPEAK_HOST, &server_ip, dns_callback, NULL);
        // Aguarda a conexão ser estabelecida (exemplo com timeout)
        uint32_t timeout = 0;
        while (!connection_established && timeout < 5000) {
            sleep_ms(100);
            timeout += 100;
        }
        if (!connection_established) {
            printf("Conexão não estabelecida.\n");
            return;
        }
    }

    // Formata a requisição para obter o último valor de água
    char request[250];
    snprintf(request, sizeof(request),
             "GET /channels/%s/fields/1/last.txt?api_key=%s HTTP/1.1\r\n"
             "Host: %s\r\n"
             "Connection: close\r\n"
             "\r\n",
             CHANNEL_ID, API_KEY_READ, THINGSPEAK_HOST);

    // Envia a requisição e utiliza o callback específico para leitura
    tcp_write(tcp_client_pcb, request, strlen(request), TCP_WRITE_FLAG_COPY);
    tcp_output(tcp_client_pcb);
    tcp_recv(tcp_client_pcb, http_read_callback);

    // Aguarda um tempo para a resposta (ou utilize um flag controlado no callback)
    sleep_ms(500);

    //fecha a conexão
     

    tcp_close(tcp_client_pcb);
    tcp_client_pcb = NULL;

    sleep_ms(1000);
    ssd1306_clear(&display);
    ssd1306_show(&display);
}








/********************************************************** */










int main(){
    printf("Iniciando...\n");
    //uint16_t vrx_value, vry_value, sw_value; // Variáveis para armazenar os valores do joystick (eixos X e Y) e botão
    inicializacao();

    // Cria um temporizador que aciona a cada 1000ms (1 segundo)
    add_alarm_in_ms(1000, timer_callback, NULL, true);



    // Inicialização do meu WIFI
    if (cyw43_arch_init()) {
        printf("Falha ao iniciar Wi-Fi\n");
        return 1;
    }

    cyw43_arch_enable_sta_mode();
    printf("Conectando ao Wi-Fi...\n");

    if (cyw43_arch_wifi_connect_blocking(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_MIXED_PSK)) {
        printf("Falha ao conectar ao Wi-Fi\n");
        return 1;
    }
    printf("Wi-Fi conectado!\n");


    // Resolve o DNS do Thingspeak e conecta
    dns_gethostbyname(THINGSPEAK_HOST, &server_ip, dns_callback, NULL);
    


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
                for(int b = 0; b < 1 ; b++){
                ssd1306_clear(&display);
                print_texto("APERTE A ", 20, 18, 1);
                print_texto("PARA ADD 300ml", 20, 30, 1);
                sleep_ms(1500);

                ssd1306_clear(&display);
                print_texto("APERTE B ", 20, 18, 1);
                print_texto("PARA TIRAR 300ml", 20, 30, 1);
                sleep_ms(1500);
                }
                addAgua();
                break;
            case 2:
                ssd1306_clear(&display);
                print_texto("Mandando ao ", 20, 18, 1.5);
                print_texto("servidor...", 20, 30, 1.5);
                print_texto("Aguarde", 20, 42, 1);
                sleep_ms(5000);
                atualizarServer();
                break;

            case 3:
                ssd1306_clear(&display);
                print_texto("Puxando do ", 20, 18, 1.5);
                print_texto("servidor...", 20, 30, 1.5);
                print_texto("Aguarde", 20, 42, 1);
                sleep_ms(5000);
                lerServer();

                ssd1306_clear(&display);

                char conversao[20];
                sprintf(conversao, "%d", valorAgua);
                print_texto("Total ja registrado:", 20, 18, 1);
                print_texto(conversao, 20, 32, 1);
                sleep_ms(4000);
                ssd1306_clear(&display);
                print_texto("Retornando ao menu", 10, 35, 1);
                sleep_ms(1000); 
                break;
        }
    }
    sleep_ms(100);
    pos_y_anterior = pos_y; // atualiza a posicao do cursor para a origem

    }
}

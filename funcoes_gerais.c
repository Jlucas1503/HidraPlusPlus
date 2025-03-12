#include "monitor_hidratacao_IoT.h"

/* DEFINIÇÃO DAS VARIÁVEIS GLOBAIS */
volatile bool button_pressionado = false;
volatile bool connection_established = false;
int valorAgua = 0;
uint pos_y = 14;
ssd1306_t display;
struct tcp_pcb *tcp_client_pcb = NULL;
ip_addr_t server_ip;
uint16_t led_b_level = 100, led_r_level = 100;
uint slice_led_b, slice_led_r;

/*****************************************
 * FUNÇÕES DO MÓDULO DE HARDWARE
 *****************************************/
void setup_pwm_led(uint led, uint *slice, uint16_t level) {
    gpio_set_function(led, GPIO_FUNC_PWM);
    *slice = pwm_gpio_to_slice_num(led);
    pwm_set_clkdiv(*slice, DIVIDER_PWM);
    pwm_set_wrap(*slice, PERIOD);
    pwm_set_gpio_level(led, level);
    pwm_set_enabled(*slice, true);
}

void pwm_init_buzzer(uint pin) {
    gpio_set_function(pin, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(pin);
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 4.0f);
    pwm_init(slice_num, &config, true);
    pwm_set_gpio_level(pin, 0);
}

void play_tone(uint pin, uint frequency, uint duration_ms) {
    uint slice_num = pwm_gpio_to_slice_num(pin);
    uint32_t clock_freq = clock_get_hz(clk_sys);
    uint32_t top = clock_freq / frequency - 1;
    pwm_set_wrap(slice_num, top);
    pwm_set_gpio_level(pin, top / 2);
    
    uint64_t start_time = time_us_64();
    while ((time_us_64() - start_time) < duration_ms * 1000) {
        if (button_pressionado)
            break;
        sleep_ms(1);
    }
    pwm_set_gpio_level(pin, 0);
    sleep_ms(50);
}

void print_texto(char *msg, uint pos_x, uint pos_y, uint scale) {
    ssd1306_draw_string(&display, pos_x, pos_y, scale, msg);
    ssd1306_show(&display);
}

void print_retangulo(int x1, int x2, int y1, int y2) {
    ssd1306_draw_empty_square(&display, x1, x2, y1, y2);
    ssd1306_show(&display);
}

void print_menu(uint posy) {
    ssd1306_clear(&display);
    print_texto("hidra++", 52, 2, 1);
    print_retangulo(2, posy, 120, 12);
    print_texto("add Agua", 10, 18, 1);
    print_texto("Atualizar server", 10, 30, 1);
    print_texto("ler server", 10, 42, 1);
}

void joystick_read_axis(uint16_t *vrx_value, uint16_t *vry_value) {
    adc_select_input(ADC_CHANNEL_0);
    sleep_us(2);
    *vrx_value = adc_read();
    
    adc_select_input(ADC_CHANNEL_1);
    sleep_us(2);
    *vry_value = adc_read();
}

void inicializacao(void) {
    stdio_init_all();
    setup_pwm_led(LED_B, &slice_led_b, led_b_level);
    setup_pwm_led(LED_R, &slice_led_r, led_r_level);
    pwm_init_buzzer(BUZZER_PIN);
    
    adc_init();
    adc_gpio_init(eixoX);
    adc_gpio_init(eixoY);
    
    i2c_init(I2C_PORT, 600 * 1000);
    gpio_set_function(PINO_SCL, GPIO_FUNC_I2C);
    gpio_set_function(PINO_SDA, GPIO_FUNC_I2C);
    gpio_pull_up(PINO_SCL);
    gpio_pull_up(PINO_SDA);
    display.external_vcc = false;
    ssd1306_init(&display, 128, 64, 0x3c, I2C_PORT);
    ssd1306_clear(&display);
    
    gpio_init(SW);
    gpio_set_dir(SW, GPIO_IN);
    gpio_pull_up(SW);
    
    gpio_init(LED_B);
    gpio_init(LED_R);
    gpio_init(LED_G);
    gpio_set_function(LED_B, GPIO_OUT);
    gpio_set_function(LED_R, GPIO_OUT);
    gpio_set_function(LED_G, GPIO_OUT);
    
    gpio_init(BotaoA);
    gpio_set_function(BotaoA, GPIO_IN);
    gpio_pull_up(BotaoA);
    gpio_init(BotaoB);
    gpio_set_function(BotaoB, GPIO_IN);
    gpio_pull_up(BotaoB);
    
    gpio_set_irq_enabled_with_callback(SW, GPIO_IRQ_EDGE_FALL, true, button_callback);
}

/*****************************************
 * FUNÇÕES DE REDE (exemplos, conforme seu código original)
 *****************************************/
static err_t http_recv_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    if (p == NULL) {
        tcp_close(tpcb);
        return ERR_OK;
    }
    printf("Resposta do ThingSpeak: %.*s\n", p->len, (char *)p->payload);
    pbuf_free(p);
    return ERR_OK;
}

static err_t http_connected_callback(void *arg, struct tcp_pcb *tpcb, err_t err) {
    if (err != ERR_OK) {
        printf("Erro na conexão TCP\n");
        return err;
    }
    printf("Conectado ao ThingSpeak!\n");
    connection_established = true;
    return ERR_OK;
}

static err_t http_read_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    if (p == NULL) {
        tcp_close(tpcb);
        return ERR_OK;
    }
    char *payload = (char *)p->payload;
    char *body = strstr(payload, "\r\n\r\n");
    if (body)
        body += 4;
    else
        body = payload;
    
    valorAgua = atoi(body);
    printf("Resposta do ThingSpeak (leitura): %s\n", body);
    printf("VALOR DA AGUA: %d\n", valorAgua);
    pbuf_free(p);
    return ERR_OK;
}

void dns_callback(const char *name, const ip_addr_t *ipaddr, void *callback_arg) {
    if (ipaddr) {
        printf("Endereço IP: %s\n", ipaddr_ntoa(ipaddr));
        tcp_client_pcb = tcp_new();
        tcp_connect(tcp_client_pcb, ipaddr, THINGSPEAK_PORT, http_connected_callback);
    } else {
        printf("Falha na resolução DNS\n");
    }
}

void atualizarServer(void) {
    if (tcp_client_pcb == NULL) {
        connection_established = false;
        dns_gethostbyname(THINGSPEAK_HOST, &server_ip, dns_callback, NULL);
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
    char request[250];
    snprintf(request, sizeof(request),
             "GET /update?api_key=%s&field1=%d HTTP/1.1\r\n"
             "Host: %s\r\nConnection: close\r\n\r\n",
             API_KEY, valorAgua, THINGSPEAK_HOST);
    tcp_write(tcp_client_pcb, request, strlen(request), TCP_WRITE_FLAG_COPY);
    tcp_output(tcp_client_pcb);
    tcp_recv(tcp_client_pcb, http_recv_callback);
    sleep_ms(500);
    ssd1306_clear(&display);
    print_texto("Retornando ao menu", 10, 35, 1);
    sleep_ms(500);
    tcp_close(tcp_client_pcb);
    tcp_client_pcb = NULL;
}

void lerServer(void) {
    if (tcp_client_pcb == NULL) {
        connection_established = false;
        dns_gethostbyname(THINGSPEAK_HOST, &server_ip, dns_callback, NULL);
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
    char request[250];
    snprintf(request, sizeof(request),
             "GET /channels/%s/fields/1/last.txt?api_key=%s HTTP/1.1\r\n"
             "Host: %s\r\nConnection: close\r\n\r\n",
             CHANNEL_ID, API_KEY_READ, THINGSPEAK_HOST);
    tcp_write(tcp_client_pcb, request, strlen(request), TCP_WRITE_FLAG_COPY);
    tcp_output(tcp_client_pcb);
    tcp_recv(tcp_client_pcb, http_read_callback);
    sleep_ms(500);
    tcp_close(tcp_client_pcb);
    tcp_client_pcb = NULL;
    sleep_ms(1000);
    ssd1306_clear(&display);
    ssd1306_show(&display);
}

/*****************************************
 * FUNÇÕES DO MENU
 *****************************************/
void addAgua(void) {
    gpio_set_irq_enabled_with_callback(SW, GPIO_IRQ_EDGE_FALL, true, button_callback);
    while (!button_pressionado) {
        if (gpio_get(BotaoA) == 0) {
            valorAgua += 300;
            ssd1306_clear(&display);
            print_texto("Voce adicionou", 10, 25, 1);
            print_texto("300ml de agua", 10, 40, 1);
            char msg_total[50];
            sprintf(msg_total, "Total: %d", valorAgua);
            print_texto(msg_total, 10, 55, 1);
            sleep_ms(1000);
        } else if (gpio_get(BotaoB) == 0) {
            if (valorAgua >= 300)
                valorAgua -= 300;
            ssd1306_clear(&display);
            print_texto("Voce retirou", 10, 25, 1);
            print_texto("300ml de agua", 10, 40, 1);
            char msg_total[50];
            sprintf(msg_total, "Total: %d", valorAgua);
            print_texto(msg_total, 10, 55, 1);
            sleep_ms(1000);
        }
    }
    ssd1306_clear(&display);
    button_pressionado = false;
    print_texto("Retornando ao menu", 10, 35, 1);
    for (int i = 0; i < 3; i++) {
        gpio_put(LED_G, 1);
        sleep_ms(300);
        gpio_put(LED_G, 0);
        sleep_ms(100);
        play_tone(BUZZER_PIN, 5000, 100);
    }
    sleep_ms(2000);
    ssd1306_clear(&display);
    ssd1306_show(&display);
}

/*****************************************
 * CALLBACKS E TIMER
 *****************************************/
void button_callback(uint gpio, uint32_t events) {
    static uint64_t last_press_time = 0;
    uint64_t current_time = time_us_64();
    if ((current_time - last_press_time) > 200000) {
        if (gpio == SW)
            button_pressionado = true;
        last_press_time = current_time;
    }
}

int64_t timer_callback(alarm_id_t id, void *user_data) {
    printf("Temporizador acionado!\n");
    return 1000000; // 1 segundo para o próximo
}
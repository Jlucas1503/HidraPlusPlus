#include "monitor_hidratacao_IoT.h"

int main(void) {
    printf("Iniciando...\n");
    inicializacao();
    // Configura o temporizador
    add_alarm_in_ms(1000, timer_callback, NULL, true);

    // Configuração do Wi-Fi e demais funções devem ser chamadas conforme necessário
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
    dns_gethostbyname(THINGSPEAK_HOST, &server_ip, dns_callback, NULL);
    
    uint countDown = 0, countUp = 2, menu = 1, pos_y_anterior = 19;
    print_menu(pos_y);
    
    while (true) {
        uint adc_y_raw = adc_read();
        const uint bar_width = 40, adc_max = (1 << 12) - 1;
        uint bar_y_pos = adc_y_raw * bar_width / adc_max;
        
        if (bar_y_pos < 14 && countDown < 2) {
            pos_y += 12;
            countDown++;
            countUp--;
            menu++;
        } else if (bar_y_pos > 24 && countUp < 2) {
            pos_y -= 12;
            countDown--;
            countUp++;
            menu--;
        }
        sleep_ms(100);
        if (pos_y != pos_y_anterior)
            print_menu(pos_y);
        
        if (button_pressionado) {
            button_pressionado = false;
            switch(menu) {
                case 1:
                    ssd1306_clear(&display);
                    print_texto("APERTE A", 20, 18, 1);
                    print_texto("PARA ADD 300ml", 20, 30, 1);
                    sleep_ms(1500);
                    ssd1306_clear(&display);
                    print_texto("APERTE B", 20, 18, 1);
                    print_texto("PARA TIRAR 300ml", 20, 30, 1);
                    sleep_ms(1500);
                    addAgua();
                    break;
                case 2:
                    ssd1306_clear(&display);
                    print_texto("Mandando ao", 20, 18, 1);
                    print_texto("servidor...", 20, 30, 1);
                    print_texto("Aguarde", 20, 42, 1);
                    sleep_ms(5000);
                    atualizarServer();
                    break;
                case 3:
                    ssd1306_clear(&display);
                    print_texto("Puxando do", 20, 18, 1);
                    print_texto("servidor...", 20, 30, 1);
                    print_texto("Aguarde", 20, 42, 1);
                    sleep_ms(5000);
                    lerServer();
                    ssd1306_clear(&display);
                    {
                        char conversao[20];
                        sprintf(conversao, "%d", valorAgua);
                        print_texto("Total ja registrado:", 20, 18, 1);
                        print_texto(conversao, 20, 32, 1);
                    }
                    sleep_ms(4000);
                    ssd1306_clear(&display);
                    print_texto("Retornando ao menu", 10, 35, 1);
                    sleep_ms(1000); 
                    break;
            }
        }
        sleep_ms(100);
        pos_y_anterior = pos_y;
    }
    
    return 0;
}
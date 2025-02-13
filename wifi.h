#ifndef WIFI_H
#define WIFI_H




#define WIFI_SSID "brisa-1685641"  // Substitua pelo nome da sua rede Wi-Fi
#define WIFI_PASS "mklxybza" // Substitua pela senha da sua rede Wi-Fi


#include "lwip/tcp.h"

#ifdef __cplusplus
extern "C" {
#endif

// Retira o "static" para permitir o uso fora do arquivo (se necessário)
err_t http_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
err_t connection_callback(void *arg, struct tcp_pcb *newpcb, err_t err);
void start_http_server(void);

#ifdef __cplusplus
}
#endif



#endif // WIFI_H
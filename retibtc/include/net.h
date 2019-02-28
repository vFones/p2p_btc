#ifndef NET_H
#define NET_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdbool.h>

#include <fullio.h>
#include <utils.h>

#define LEN_ADDRESS 32

// rapid access to ip-port
struct s_net_ent{
  char addr[LEN_ADDRESS];
  unsigned short port;
};

#define NET_ENT sizeof(struct s_net_ent)
typedef struct s_net_ent *Net_ent;
/* wrapper di getsockname ma che restituisce l'indirizzo
   del socket locale tramite Net_ent */
int getsock_net_ent(int fd, Net_ent ent);

/* wrapper di getpeername ma che restituisce l'indirizzo
   del socket remoto tramite Net_ent */
int getpeer_net_ent(int fd, Net_ent ent);


/*funzione per comparare due net_ent.
  I puntatori sono di tipo void in modo da renderla compatibile
  con la funzione search_by_info() di list.h*/
bool compare_net_ent(void *x, void *y);

/*funzione per visitare net_ent.
  I puntatori sono di tipo void in modo da renderla compatibile
  con la funzione visit_list() di list.h*/
void visit_net_ent(void *args);

//funzione per l'inizializzazione rapida di un indirizzo
void fill_address(
    struct sockaddr_in *  socket_address, /*struttura da riempire*/
    sa_family_t           family, /*famiglia indirizzo (AF_INET)*/
    char *                ip_address,  /*indirizzo IP dato in dotted_decimal*/
    unsigned short                 port /*porta da assegnare*/
  );

bool sha_auth(int server_fd, char *pwd);

//funzione per invio rapido di char
int send_char(int fd, char n);
//funzione per ricezione rapida di char
int recv_char(int fd, char *n);


//funzione per invio rapido di short
int send_short(int fd, short n);
//funzione per ricezione rapida di short
int recv_short(int fd, short *n);

//funzione per invio rapido di un int
int send_int(int fd, int n);
//funzione per ricezione rapida di un int
int recv_int(int fd, int *n);


//funzione per invio rapido di net_ent
int send_net_ent(int fd, Net_ent n);
//funzione per ricezione rapida di net_ent
int recv_net_ent(int fd, Net_ent n);



#endif

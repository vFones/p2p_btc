#ifndef PEER_H
#define PEER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/select.h>
#include <pthread.h>
#include <signal.h>

#include <retibtc/fullio.h>
#include <retibtc/list.h>
#include <retibtc/sockwrap.h>
#include <retibtc/net.h>
#include <retibtc/blockchain.h>
#include <retibtc/utils.h>
#include <retibtc/p2p.h>
#include <retibtc/rwsincro.h>
#include <retibtc/vitc.h>

#define usg "Usage: ./peer \n \
      \t-a CENTRAL_SERVER_IP_ADDRESS\n \
      \t-b CENTRAL_SERVER_PORT\n \
      \t-h PASSWORD \n \
      \t-p PEER_PORT\n "


struct s_connected_ent{
  int fd;
  struct s_net_ent ent;
};
typedef struct s_connected_ent *Connected_ent;
#define CONNECTED_ENT sizeof(struct s_connected_ent)


// pacchetto inviato in fase di aggancio ad un peer ad un altro.
/*per mettersi daccordo sul numero di blocchi della bchain da scambiarsi
  in fase di sincronizzazione */
struct s_hook_peer
{
  struct s_connected_ent to_hook;
  int last_seq;
  int tails_number;
};
typedef struct s_hook_peer *Hook_peer;
#define HOOKPEER sizeof(struct s_hook_peer)


struct s_info_block_pkg
{
  struct s_info_block ib;
  char prev_id[LEN_ID]; // id del block precedente
  int seq; // numero di sequenza
};
typedef struct s_info_block_pkg *IB_pkg;
#define IB_PKG sizeof(struct s_info_block_pkg)


/*GLOBAL VAR*******************************************************************/
char pwd[BUFFLEN]; // password for p2p net
struct  s_net_ent server, // central server
                  my_service_ent; // my server ent
int maxfd;
int fd_open[FD_SETSIZE];

RW_sincro sincro_stuff;
//VARIABLES TO PROTECT WITH CRITIC SECTION
List conn_peer;
List conn_wallet;
Bchain block_chain;


int next_seq;
pthread_t reserved_tid;
bool flag_tid;

//signal stuff
bool exit_flag;
void sig_handler(int n);

/*THREAD FUNCTION PROTOTYPES***************************************************/
void *hook_p2p(void *argfd);
void *hook_w2p(void *argfd);
void *w_balance(void *argfd);
void *w_transaction(void *argfd);
void *p_block(void *argfd);



/*FUNZIONI PER L'AGGANCIO RETE*************************************************/

/*funzione pe l'aggancio alla rete*/
bool hook_network();

/*funzione per la sostituzione di un peer a cui sono connesso e che e' crashato*/
bool re_hook_peer(Connected_ent error_peer);

// funzione che implementa la richiesta di aggancio ad un peer
Connected_ent hook_to_peer(struct s_net_ent peer_ent);

/*funzione per ricevere/inviare (cioe' sincronizzare) la blockchain con il
  peer a cui ci si e' apena connessi*/
bool sincro_bchain(int fd, int block_to_sincro, int multi_tails);
void send_multitails(int fd);
void recv_multitails(int fd, int n_tails);


/*FUNCTIONS USED BY THREADS****************************************************/

/*aggiunta alla block_chain (in maniera sincronizzata) di un blocco*/
int sync_in_blockchain(Info_block block);

/*funzione di propagazione di un blocco*/
void flooding(IB_pkg pkg, struct s_net_ent pkg_sender);

/*funzione er avvertire un wallet nel caso in cui abbia icevuto una transazione*/
void warn_wallet(void *info);

/*UTILITY FUNCTIONS************************************************************/
void usage(void);
void print_peer_state();

/*FAST SEND/RECEIVE FUNCTIONS FOR PEER DATA STRUCTURE**************************/
int send_hook_peer(int fd, Hook_peer hp);

int recv_hook_peer(int fd, Hook_peer hp);

int send_info_block_pkg(int fd, IB_pkg pkg);

int recv_info_block_pkg(int fd, IB_pkg pkg);


/*LIST UTILIY FUNCTION*********************************************************/
/*
  I puntatori sono di tipo void in modo da renderla compatibile
  con la funzione search_by_info() di list.h
*/

/*funzione per stampare le info di un peer.*/
void visit_peer(void *args);

/*funzione per comparare due peer.*/
bool compare_peer(void *x, void *y);

/*funzione per comparare due peer in base al loro descrittore.
  usata per capire se un descrittore su cui ho letto un errore(o un EOF)
  si trova nella lista dei peer connessi.*/
bool compare_peer_fd(void *x, void *y);

bool compare_info_block_pkg(void *x, void *y);


#endif

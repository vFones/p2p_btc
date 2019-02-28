#include "peer.h"

bool hook_network()
{
  struct sockaddr_in server_addr;
  int server_fd;
  bool hooked = false;

  /*connect with server */
  fill_address(&server_addr, AF_INET, server.addr, server.port);
  server_fd = Socket(AF_INET, SOCK_STREAM, 0);
  Connect(server_fd, (struct sockaddr *)&server_addr);

  struct s_net_ent my_ent;
  getsock_net_ent(server_fd, &my_ent);
  printf("HOOK TO NETWORK");
  printf("\nPEER: %s:%d", my_ent.addr, my_ent.port);
  printf("\tSERVICE: %s:%d\n", my_service_ent.addr, my_service_ent.port);

  /*SENDING SHA-PASSWORD */
  if(!sha_auth(server_fd, pwd)) // wrong password
  {
    fprintf(stderr, "\nwrong password!\n");
    usage();
  }
  printf("successful connection!\n");

  /*STARTING HOOKING PROCEDURE******************************************/
  short n_peer;

  //send request for hook to peer-network
  send_short(server_fd, HOOK_PEER);

  send_net_ent(server_fd, &my_service_ent);

  //receive the number of peers to connect to
  recv_short(server_fd, &n_peer);


  if(n_peer == 0) // if i am the first peer
  {
    printf("I'm the first peer of p2p network, generating genesis block...\n");

    create_genesis_block(block_chain);

    // send 1 to confirm that the genesis block was created
    send_short(server_fd, 1);
    hooked = true;
  }
  else // I'm not the first peer, so n_peer tells me the number of peers I need to try to connect to
  {
    int i, connections = 0; //successful connections
    short conn;
    struct s_net_ent tmp_ent;
    Connected_ent tmp_peer = NULL;

    // create genesis block equal to other
    create_genesis_block(block_chain);

    for(i=0; i<n_peer; i++)
    {
      conn = 0;
      recv_net_ent(server_fd, &tmp_ent);
      if( (tmp_peer = hook_to_peer(tmp_ent) ) != NULL )
      {
        conn = 1; // connessione confermata
        connections++;
        add_to_list(conn_peer, tmp_peer);
      }

      send_short(server_fd, conn);
    }// fine for ricezione peer

    if(connections)
      hooked = true;
  }// fine caso else, che indica che non sono il primo peer

  close(server_fd);
  return hooked;
}

bool re_hook_peer(Connected_ent error_peer)
{
  struct sockaddr_in server_addr;
  int server_fd;
  bool hooked = false;
  struct s_net_ent my_ent;

  /*connect with server */
  fill_address(&server_addr, AF_INET, server.addr, server.port);
  server_fd = Socket(AF_INET, SOCK_STREAM, 0);
  Connect(server_fd, (struct sockaddr *)&server_addr);

  //my_ent
  getsock_net_ent(server_fd, &my_ent);
  printf("RE-HOOK PEER");
  printf("\nPEER: %s:%d", my_ent.addr, my_ent.port);
  printf("\tSERVICE: %s:%d\n", my_service_ent.addr, my_service_ent.port);

  /*SENDING SHA-PASSWORD */
  if(!sha_auth(server_fd, pwd)) // wrong password
  {
    fprintf(stderr, "\nwrong password!\n");
    usage();
  }
  printf("\nsuccessful authorizzation!\n");

  /*STARTING RE-HOOKING PROCEDURE**************************************************/
  send_short(server_fd, RE_HOOK_PEER);
  //send net_ent of crashed peer
  printf("\n\nI'm sending to server net_ent of crashed peer: [%s%d]\n\n", \
      error_peer->ent.addr, error_peer->ent.port);

  send_net_ent(server_fd, &(error_peer->ent) );

  //send my_service net_ent
  send_net_ent(server_fd, &my_service_ent);

  //try to connect to new peer received from server
  struct s_net_ent toconn_ent;
  Connected_ent toconn_peer;
  short connected = 0;
  while(!connected)
  {
    recv_net_ent(server_fd, &toconn_ent);
    if(!toconn_ent.port) // se la porta non e' diversa da 0
      break;
    toconn_peer = hook_to_peer(toconn_ent);
    connected = (toconn_peer == NULL) ? 0 : 1;
    send_short(server_fd, connected);
  }

  if(connected)
  {
    add_to_list(conn_peer, toconn_peer);
    hooked = true;
  }
  remove_from_list(conn_peer, error_peer, compare_peer);
  return hooked;
}

Connected_ent hook_to_peer(struct s_net_ent peer_ent)
{
  struct sockaddr_in peer_addr;
  Connected_ent peer = (Connected_ent)obj_malloc(CONNECTED_ENT);
  Hook_peer hp = (Hook_peer)obj_malloc(HOOKPEER);

  int peer_last_seq, peer_tails_number;
  int block_to_sincro, tails_to_sincro;
  bool sincro;

  if(compare_net_ent((void *)&peer_ent, (void *)&my_service_ent))
  {
    printf("Net ent sended by server is same as mine\n");
    printf("Mine:[%s:%d]\t from server [%s:%d]\n", my_service_ent.addr, \
      my_service_ent.port, peer_ent.addr, peer_ent.port);
    return NULL;
  }
  peer->ent = peer_ent;

  hp->to_hook.ent = my_service_ent;
  hp->last_seq = block_chain->count;
  hp->tails_number = get_tails_number(block_chain);

  // peer inviatomi dal server gia' presente in lista
  if( search_by_info(conn_peer, peer, compare_peer) != -1)
  {
    free(peer);
    free(hp);
    return NULL;
  }

  printf("Try connecting to [%s:%d]\n", peer->ent.addr, peer->ent.port);

  fill_address(&peer_addr, AF_INET, peer->ent.addr, peer->ent.port);
  peer->fd = Socket(AF_INET, SOCK_STREAM, 0);
  Connect(peer->fd, (struct sockaddr *) &peer_addr);

  send_short(peer->fd, HOOK_P2P);

  send_hook_peer(peer->fd, hp);
  recv_int(peer->fd, &peer_last_seq);
  recv_int(peer->fd, &peer_tails_number);
  int my_tails_number = get_tails_number(block_chain);

  block_to_sincro = block_chain->count - peer_last_seq;
  tails_to_sincro = my_tails_number - peer_tails_number;

  if(block_to_sincro > 0) // se io ho la bchain piu' aggiornata
    tails_to_sincro = my_tails_number;
  else
    if(block_to_sincro < 0) // se io NON ho la bchain piu' aggiornata
      tails_to_sincro = peer_tails_number;
    else //block_to_sincro == 0
      if(tails_to_sincro > 0) // stesso seq ma io ho piu' code nuove
        tails_to_sincro = my_tails_number;
      else
        if(tails_to_sincro < 0) // stesso seq ma io ho meno code vecchie
          tails_to_sincro = -peer_tails_number;

  sincro = sincro_bchain(peer->fd, block_to_sincro, tails_to_sincro);

  if(!sincro)
  {
    close(peer->fd);
    free(peer);
    free(hp);
    return NULL;
  }
  fd_open[peer->fd] = 1;
  if(peer->fd > maxfd)
    maxfd = peer->fd;

  return peer;
}


bool compare_peer(void *x, void *y)
{
  Connected_ent px = (Connected_ent)x;
  Connected_ent py = (Connected_ent)y;
  return compare_net_ent( (void*) &(px->ent), (void*) &(py->ent) );
}


bool compare_peer_fd(void *x, void *y)
{
  Connected_ent px = (Connected_ent)x;
  Connected_ent py = (Connected_ent)y;
  return (px->fd == py->fd);
}


void visit_peer(void *args)
{
  Connected_ent p = (Connected_ent)args;
  printf("fd=%d -->IP:port = %s:%d\n", p->fd, p->ent.addr, p->ent.port);
}


void print_peer_state()
{
  printf("\nѶ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ\n\n");
  printf("\t\tPEER: [%s:%d]\n\n", my_service_ent.addr, my_service_ent.port);


  printf("\nѶ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ\n\n");
  printf("\t\tCONNECTED PEER\n\n");
  visit_list(conn_peer, visit_peer);

  printf("\nѶ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ\n\n");
  printf("\t\tBLOCK-CHAIN\n");
  visit_bchain(block_chain, visit_trns);

  printf("\nѶ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ Ѷ\n\n");
  printf("\t\tCONNECTED WALLET\n\n");
  visit_list(conn_wallet, visit_peer);


}


void usage()
{
  fprintf(stderr, usg);
  exit(EXIT_FAILURE);
}


void sig_handler(int n)
{
  if (n == SIGINT)
    exit_flag = true;
}


int send_hook_peer(int fd, Hook_peer hp)
{
  if ( full_write(fd, hp, HOOKPEER) != 0 )
  {
    perror("send_hook_peer()");
    return -1;
  }
  return 0;
}


int recv_hook_peer(int fd, Hook_peer hp)
{
  if (full_read(fd, hp, HOOKPEER) != 0)
  {
    perror("rec_hook_peer()");
    return -1;
  }
  return 0;
}


bool sincro_bchain(int fd, int block_to_sincro, int tails_to_sincro)
{
  printf("\nstart of synchronization procedure of the blockchain\n");
  if(block_to_sincro == 0) // se abbiamo lo stesso numero di seq
  { /*
      Dando per gia' sincronizzati per i blocchi precedenti,
      se si chiama la funzione passando block_to_sincro=0, significa
      che i peer, scambiandosi la grandezze delle code,
      si sono resi conto che uno dei due ha piu' code dell'altro
      quindi si controlla tails_to_sincro per capire se si deve ricevere
      o no le code.
    */
    printf("\nblock_to_sincro = %d\n", block_to_sincro);
    if(tails_to_sincro > 0)
    { //ti invio tutte le mie code
      printf("\ntails_to_sincro > 0 ... [%d]\n", tails_to_sincro);
      send_multitails(fd);
    }
    else if(tails_to_sincro < 0)
    {// mi invii le tutte le tue code
      printf("\ntails_to_sincro < 0 ... [%d]\n", tails_to_sincro);
      recv_multitails(fd, abs(tails_to_sincro));
    }
    // se anche multitails == 0 significa che abbiama gia' la stessa bchain
    return true;
  }



  IB_pkg pkg_torecv;

  int n = abs(block_to_sincro); // +1 spiegato nel commento successivo

  int i_seq, last_seq;
  if(block_to_sincro > 0) // io ho la bchain piu' aggiornata
  {
    /*es: se tu ne hai 3 e io 5, ti devo inviare 2 blocchi
      rispettivamente di indice 4 e 5.
      Quindi 5-2+1=4, (il +1 perche' la bchain parte da 1 dato che l'indice 0 e' il blocco genesei)
       cioe' l'indice del primo blocco da mandarti.
      tuttavia il 3' blocco che hai tu potrebbe essere una multicoda,
      per cui ti invio anche il mio 3' blocco e tu semplicemente lo sovrascrivi, quindi i blocchi
      in questo caso sarebbero uno in piu', per cui ne incremento il numero (n), almeno che il primo
      blocco da sincronizzare non sia il blocco genesi (i_seq==0). In tal caso passo direttamente al successivo
    */
    i_seq = block_chain->count - block_to_sincro;

    if(i_seq==0)
      i_seq++;
    else
      n++;
    while(n)
    {
      if( (i_seq == block_chain->count) )
      { /*se l'unico blocco che mi e' rimasto da mandare e' l'ultimo allora ti invio la/e coda/e */
        send_multitails(fd);
      }
      else
      { /*altrimenti ti invio un normale blocco della sequenza (i_seq)*/
        Info_block ib;
        struct s_info_block_pkg pkg_tosend;

        printf("\nsequence number del blocco che sto inviando per la sincronizzazione %d\n", i_seq);

        ib = get_info_block_by_index(block_chain, i_seq, 1);
        get_prev_id(block_chain, pkg_tosend.prev_id, i_seq);
        pkg_tosend.ib = *ib;

        printf("\nALL ESTERNO DI get_info_block_by_index:\n");
        printf("\nprev_id = %s\nid = %s\nrand_sec %d\nconfirmed = %d", pkg_tosend.prev_id, pkg_tosend.ib.id, pkg_tosend.ib.rand_sec, pkg_tosend.ib.confirmed);


        // visit_info_block(&(pkg_tosend->ib), visit_trns);
        pkg_tosend.seq = i_seq;
        send_info_block_pkg(fd, &pkg_tosend);
      }
      i_seq++;
      n--;
    }
  }
  else // io ho la bchain meno aggiornata
  {
    // quindi rimuovo il mio ultimo blocco, sia esso oppure no una multicoda
    remove_tails(block_chain);

    // riferito all'esempio nel then, il primo indice da ricevere:
    i_seq = block_chain->count; // cioe' 3
    if(i_seq==0)
      i_seq++;
    // mentre l'indice dell'ultimo blocco da ricevere:
    last_seq = block_chain->count + block_to_sincro; // cioe' 5
    while(n)
    {
      if( i_seq == last_seq )
      {
        recv_multitails(fd, tails_to_sincro);
      }
      else
      {
        pkg_torecv = (IB_pkg)obj_malloc(IB_PKG);
        printf("\nsequence number del blocco che sto ricevendo per la sincronizzazione %d\n", i_seq);
        recv_info_block_pkg(fd, pkg_torecv);
        add_to_bchain(block_chain, &(pkg_torecv->ib), pkg_torecv->seq);
        printf("\nBLOCCO NUMERO %d ricevuto:\n", i_seq);
        visit_info_block(&pkg_torecv->ib, visit_trns);
      }
      i_seq++;
      n--;
    }
  }

  return true;
}

void send_multitails(int fd)
{
  printf("\nstart to send multitails\n");
  struct s_info_block_pkg pkg_tosend;
  List tails = get_tails(block_chain);
  int n_tails = tails->count;
  int seq = block_chain->count;
  int i = 1;
  Info_block ib;

  while(n_tails)
  {
    ib = get_info_block_by_index(block_chain, seq, i);

    pkg_tosend.ib = *ib;
    get_prev_id(block_chain, pkg_tosend.prev_id, seq);
    pkg_tosend.seq = seq;

    send_info_block_pkg(fd, &pkg_tosend);
    i++, n_tails--;
  }
}

void recv_multitails(int fd, int n_tails)
{
  printf("\nstart to receive multitails\n");
  remove_tails(block_chain); // remove old tails
  IB_pkg pkg;
  while(n_tails)
  {
    pkg = (IB_pkg)obj_malloc(IB_PKG);
    recv_info_block_pkg(fd, pkg);
    add_to_bchain(block_chain, &(pkg->ib), pkg->seq);
    n_tails--;
  }
}


int send_info_block_pkg(int fd, IB_pkg pkg)
{
  pkg->seq = htonl(pkg->seq);
  pkg->ib.rand_sec = htonl(pkg->ib.rand_sec);
  pkg->ib.confirmed = htonl(pkg->ib.confirmed);
  //sending info block in pieces
  send_trns(fd, (Trns)pkg->ib.info);


  full_write(fd, pkg->ib.id, LEN_ID);
  send_int(fd, pkg->ib.rand_sec);
  send_net_ent(fd, &(pkg->ib.creator));
  send_int(fd, pkg->ib.confirmed);

  //sending bc_block
  full_write(fd, pkg->prev_id, LEN_ID);
  send_int(fd, pkg->seq);
  return 0;
}


int recv_info_block_pkg(int fd, IB_pkg pkg)
{
  // receiving info block
  Trns recv_t = (Trns) obj_malloc(TRNS);
  recv_trns(fd, recv_t);
  pkg->ib.info = (Trns)recv_t;

  full_read(fd, pkg->ib.id, LEN_ID);
  recv_int(fd, &(pkg->ib.rand_sec));
  recv_net_ent(fd, &(pkg->ib.creator));
  recv_int(fd, &(pkg->ib.confirmed));

  // receiving sb_block
  full_read(fd, pkg->prev_id, LEN_ID);
  recv_int(fd, &(pkg->seq));

  pkg->seq = ntohl(pkg->seq);
  pkg->ib.confirmed = ntohl(pkg->ib.confirmed);
  pkg->ib.rand_sec = ntohl(pkg->ib.rand_sec);
  return 0;
}

bool compare_info_block_pkg(void *x, void *y)
{
  IB_pkg a = (IB_pkg)x;
  IB_pkg b = (IB_pkg)y;
  if( a->seq == b->seq
  &&  strncmp(a->prev_id, b->prev_id, LEN_ID) == 0
  &&  compare_info_block(&(a->ib), &(b->ib))
  )
    return true;

  return false;
}

//TODO: eseguita in sezione critica
int sync_in_blockchain(Info_block block)
{
  bool recreate = true;
  bool retry;
  int seq_chosen;
  while(recreate)
  {
    retry = true;
    while(retry)
    {
       rw_sincro_entry_section(sincro_stuff, WRITER);
        if( !(retry = flag_tid) )
        {
          next_seq = block_chain->count + 1;
          flag_tid = true;
          reserved_tid = pthread_self();
        }
       rw_sincro_exit_section(sincro_stuff, WRITER);

    }
    sleep(block->rand_sec);

    rw_sincro_entry_section(sincro_stuff, WRITER);

      if( pthread_equal(reserved_tid, pthread_self() )  )
      {
        seq_chosen = next_seq;
        add_to_bchain(block_chain, block, next_seq);
        recreate = false;
        flag_tid = false;
      }

    rw_sincro_exit_section(sincro_stuff, WRITER);

  }
  return seq_chosen;
}

// eseguita in sezione critica
void flooding(IB_pkg pkg, struct s_net_ent pkg_sender)
{
  Connected_ent tosend;

  int i;
  printf("\n***********FLOOODING*************\n");
  // se l'ho creato io
  if( compare_net_ent(&pkg_sender, &my_service_ent) )
  { // lo invio a tutti
    for(i = 1; i <= conn_peer->count; i++)
    {
      tosend = search_by_index(conn_peer, i);
      printf("\tSENDING TO: \n" );
      visit_net_ent(&(tosend->ent));

      send_short(tosend->fd, P_BLOCK);
      send_info_block_pkg(tosend->fd, pkg);
      printf("\n******flooded******\n");
    }
  }
  else // mi e' arrivato dal'esterno
  {// lo invio a tutti tranne che a quello che me l'ha inviato
    for(i = 1; i <= conn_peer->count; i++)
    {
      tosend = search_by_index(conn_peer, i);
      if(!compare_net_ent(&(tosend->ent), &pkg_sender))
      {
        printf("\tSENDING TO: \n");
        visit_net_ent(&(tosend->ent));
        send_short(tosend->fd, P_BLOCK);
        send_info_block_pkg(tosend->fd, pkg);
        printf("\n******flooded******\n");
      }
    }
  }
}

// eseguita in sezione critica
void warn_wallet(void *info)
{
  Trns t = (Trns)info;
  int found;
  struct s_connected_ent tmp;
  tmp.ent = t->dst;

  found = search_by_info(conn_wallet, &tmp, compare_peer);
  Connected_ent wallet;
  if(found != -1)
  {
    wallet = search_by_index(conn_wallet, found),
    send_short(wallet->fd, TRANSACTION);
    send_trns(wallet->fd, t);
  }
}

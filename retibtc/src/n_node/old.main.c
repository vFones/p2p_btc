#include "peer.h"

int main(int argc, char *argv[])
{
  srand(time(NULL));

  /*var used for argv parsing*/
  int opt;
  int flags[4] = { 0 };

/*ARGV PARSING*****************************************************************/
  while ((opt = getopt(argc, argv, "a:b:h:p:")) != -1)
  {
    switch(opt)
    {
      case 'a': // server Address
        strncpy(server.addr, optarg, LEN_ADDRESS);
        flags[0] = 1;
        break;

      case 'b': // server Port
        server.port = (unsigned short)atoi(optarg);
        flags[1] = 1;
        break;

      case 'h': // password to encrypt with Sha256
        strcpy(pwd, optarg);
        flags[2] = 1;
        break;

      case 'p': // my_service_ent Port
        my_service_ent.port = atoi(optarg);
        flags[3] = 1;
        break;

      default:
        usage();
        break;
    }
  }

  if ( !flags[2] || !flags[3] || optind < argc )
  {
    usage();
    exit(EXIT_FAILURE);
  }

  strncpy(my_service_ent.addr, "0.0.0.0", LEN_ADDRESS);

/*DEFAULT TEST SERVER IP:port adress*/
  if(!flags[0])
    strcpy(server.addr, "127.0.0.1");

  if(!flags[1])
    server.port = 7777;

/*SETUP FOR PEER AND WALLET REQUEST********************************************/
  //listen descriptor stuff
  struct sockaddr_in my_server_addr;
  int listfd;
  int optval = 1;

  listfd = Socket(AF_INET, SOCK_STREAM, 0);
  setsockopt(listfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int));
  fill_address(&my_server_addr, AF_INET, my_service_ent.addr, my_service_ent.port);

  Bind(listfd, (struct sockaddr*)&my_server_addr);
  Listen(listfd, BACKLOG);

  maxfd = listfd;
  fd_open[listfd] = 1;

/*TRY TO HOOK TO NETWORK*******************************************************/
  conn_peer = create_list();
  conn_wallet = create_list();
  block_chain = create_bchain();

  int attempts = 3; // 3 attempts at most to connect
  bool connesso = false; // flag

  while( attempts-- && !connesso )
    connesso = hook_network();

  if(!connesso && !attempts)
  {
    perror("\nimpossible to hook up to the network\n");
    exit(EXIT_FAILURE);
  }


  printViTCmsg("\nHOOKING PROCEDURE DONE.\n");


  /*SELECT STUFF*/
  fd_set rset;
  int n_ready;

  //setup fd_open & maxfd



  int i_fd;
/*START SERVER*****************************************************************/
  short request;
  pthread_t tid[BACKLOG]; // thread ID
  int argfd[BACKLOG]; // array for pass fd to thread
  int threads_created;
  // sincro_stuff = rw_sincro_create(W_PRIO);

  /*SIGNAL STUFF*/
  exit_flag = false;
  signal(SIGINT, sig_handler);

  while(1)
  {
    /*Descriptor Update************/
    FD_ZERO(&rset);

    FD_SET(listfd, &rset);
    for(i_fd=0; i_fd<=maxfd; i_fd++)
      if(fd_open[i_fd])
        FD_SET(i_fd, &rset);

    /*PRINT CONNECTED PEER AND BLOCKCHAIN*/
    print_peer_state();

    /*wait n_ready*****************/
    while( (n_ready = select(maxfd+1, &rset , NULL , NULL , NULL) ) < 0
            && errno == EINTR)
    {
      if(exit_flag)
        exit(EXIT_SUCCESS);
    }

    if(n_ready < 0)
    {
      perror("\nselect() error\n");
      exit(EXIT_FAILURE);
    }

    /*CHECK NEW CONNECTION*************************************************/
    if( FD_ISSET(listfd, &rset) )
    {
      n_ready--;
      printf("\nNew connection request\n");
      int connfd, keepalive = 1;
      connfd = Accept(listfd, NULL);
      setsockopt(connfd, SOL_SOCKET, SO_KEEPALIVE, &keepalive, sizeof(keepalive));
      fd_open[connfd] = 1;
      if(connfd > maxfd)
        maxfd = connfd;

    }



    /*CHECK REMAINING n_ready FD FOR SERVE THE REQUESTS********************/
    i_fd = listfd;
    threads_created = 0;
    flag_tid = false;

    while(n_ready)
    {
      i_fd++;
      if(!fd_open[i_fd])
        continue;

      if(FD_ISSET(i_fd, &rset))
      {
        n_ready--;
        int readed, sock_error;
        readed = recv_short(i_fd, &request);

        /*CHECK ERROR ON SOCKET******************************************/
        if(readed != 0)
        {
          printf("\n\n\tError on socket!\n");

          getsockopt(i_fd, SOL_SOCKET, SO_ERROR, &sock_error, NULL);
          fprintf(stderr, "%s\n", strerror(sock_error));

          fd_open[i_fd] = 0;
          close(i_fd);

          Connected_ent error_peer = (Connected_ent)obj_malloc(CONNECTED_ENT);
          error_peer->fd = i_fd;
          int error_peer_index;

          /*
          accesso sia in lettura, ma soprattutto in scrittura alla
          lista dei conn_peer per la rimozione del peer crashato e alla b_chain
          per l'eventuale sincro con il nuovo peer a cui mi sono connesso
          */
          //TODO: rw_sincro_entry_section
          rw_sincro_entry_section(sincro_stuff, WRITER);

            // if the descriptor referred to the connection with a peer
            if( (error_peer_index = search_by_info(conn_peer, error_peer, compare_peer_fd) ) != -1)
            {
              // remove it from the list and connect to another by asking the server
              error_peer = search_by_index(conn_peer, error_peer_index);
              // TODO: controlla la rimozione dalla lista in re_hook_peer
              if(re_hook_peer(error_peer) )
                printf("\ncrashed peer, replaced with another\n");
              else
                printf("\ncrashed peer, only removed from list \n");

            }
            else // the descriptor with the error referred to a wallet
            {
              printf("\nwallet descriptor closed and removed from list\n");
              remove_from_list(conn_wallet, error_peer, compare_peer_fd);
            }
            free(error_peer);


            if( i_fd == (maxfd) ) // if the closed i_fd was the maximum
            {// update maxfd to optimize fd_open scanning
              while(fd_open[--i_fd] == 0) ;// only for --i
              maxfd = i_fd;
              break; //go back to select
            }

          rw_sincro_exit_section(sincro_stuff, WRITER);
          continue; //go back to select
        }//end error checking on the socket


        /*LAUNCH OF THREAD TO SERVE THE REQUEST*******************************/
        // set the descriptor to be passed to the thread
        argfd[threads_created] = i_fd;

        switch(request)
        {
          case HOOK_P2P:
            pthread_create(&tid[ threads_created ], NULL,
                    hook_p2p, (void*) &argfd[threads_created]);
            break;

          case HOOK_W2P:
            pthread_create(&tid[ threads_created ], NULL,
                    hook_w2p, (void*) &argfd[threads_created]);
            break;

          case W_BALANCE:
            pthread_create(&tid[ threads_created ], NULL,
                    w_balance, (void*) &argfd[threads_created]);
            break;

          case W_TRANSACTION:
            pthread_create(&tid[ threads_created ], NULL,
                    w_transaction, (void*) &argfd[threads_created]);
          break;

          case P_BLOCK:
            pthread_create(&tid[ threads_created ], NULL,
                    p_block, (void*) &argfd[threads_created]);

            break;

          case SHUTDOWN_NET:
            printf("\nShutting down network...\n");
            exit(EXIT_SUCCESS);
            // peer_shutdown();
            break;
        }

        threads_created++;
      }
    }
    /*JOIN OF LAUNCHED THREADS BEFORE RECALL OF SELECT*********************/
    for(int i=0; i < threads_created;  i++)
      pthread_join(tid[i], NULL);
  }



/*END PEER********************************************************************/
  empty_list(conn_peer);
  free(conn_peer);

  empty_list(conn_wallet);
  free(conn_wallet);

  empty_bchain(block_chain);
  free(block_chain);

  // free(sincro_stuff);
  return 0;
}

void *hook_p2p(void *argfd)
{
  int fd = *(int *)argfd;
  int block_to_sincro, tails_to_sincro;
  bool sincro;
  Hook_peer hp = (Hook_peer)obj_malloc(HOOKPEER);
  int my_tails_number, my_last_seq;

  my_last_seq = block_chain->count;
  my_tails_number = get_tails_number(block_chain);

  recv_hook_peer(fd, hp);

  block_to_sincro = my_last_seq - hp->last_seq;
  tails_to_sincro = my_tails_number - hp->tails_number;



  if(block_to_sincro > 0) // se io ho la bchain piu' aggiornata
  {
    tails_to_sincro = my_tails_number;
    rw_sincro_entry_section(sincro_stuff, READER);
  }
  else
  if(block_to_sincro < 0) // se io NON ho la bchain piu' aggiornata
  {
    tails_to_sincro = hp->tails_number;
    rw_sincro_entry_section(sincro_stuff, WRITER);
  }
  else //block_to_sincro == 0
  if(tails_to_sincro > 0) // stesso seq ma io ho piu' code nuove
  {
    tails_to_sincro = my_tails_number;
    rw_sincro_entry_section(sincro_stuff, READER);
  }
  else
  if(tails_to_sincro < 0) // stesso seq ma io ho code vecchie
  {
    tails_to_sincro = -hp->tails_number;
    rw_sincro_entry_section(sincro_stuff, WRITER);
  }
  send_int(fd, my_last_seq);
  send_int(fd, my_tails_number);


  //TODO: test it

  sincro = sincro_bchain(fd, block_to_sincro, tails_to_sincro);

  if(sincro)
  {
    hp->to_hook.fd = fd;
    // if it is not already in list
    if(search_by_info(conn_peer, (void*)&hp->to_hook, compare_peer) == -1)
    {
      add_to_list(conn_peer, (void *)&hp->to_hook);
      printf("HOOK_P2P request served by thread.\nNew peer [%s:%d] connected!\n", hp->to_hook.ent.addr, hp->to_hook.ent.port);
    }
    else
    {
      remove_from_list(conn_peer, (void *)&hp->to_hook, compare_peer);
      add_to_list(conn_peer, (void *)&hp->to_hook);
      printf("HOOK_P2P request served.\nPeer [%s:%d] successfull RE-connected!\n", hp->to_hook.ent.addr, hp->to_hook.ent.port);
    }
  }
  //
  // if(block_to_sincro > 0) // if i have the most updated bchain
  // // rw_sincro_exit_section(sincro_stuff, READER);
  // else if(block_to_sincro < 0)
  // // rw_sincro_exit_section(sincro_stuff, WRITER);
  // else //block_to_sincro ==0
  // if(tails_to_sincro > 0) // stesso seq ma io ho piu' code
  // // rw_sincro_exit_section(sincro_stuff, READER);
  // else if(tails_to_sincro < 0) // stesso seq ma io code vecchie
  // // rw_sincro_exit_section(sincro_stuff, WRITER);

  pthread_exit(NULL);
}


void *hook_w2p(void *argfd)
{
  int fd = *(int *)argfd;
  Connected_ent wallet = (Connected_ent)obj_malloc(CONNECTED_ENT);
  int err;

  wallet->fd = fd;

  getpeer_net_ent(fd, &wallet->ent);

  printf("Serving wallet... %s:%d\n", wallet->ent.addr, wallet->ent.port);

  //rw_sincro_entry_section(sincro_stuff, WRITER);
    err = add_to_list(conn_wallet, wallet);
  //rw_sincro_exit_section(sincro_stuff, WRITER);

  if(!err)
  {
    send_short(fd, 1);
    printf("wallet hooked correctly\n");
  }
  else
  {
    send_short(fd, 0);
    printf("error wallet not hooked\n");
  }


  pthread_exit(NULL);
}



void *w_balance(void *argfd)
{
  int fd = *(int *)argfd;
  List wallet_trns;
  int n_trns;
  struct s_net_ent wallet_ent;
  getpeer_net_ent(fd, &wallet_ent);

  //create info to search in bchain
  struct s_trns w_ref;
  w_ref.src = wallet_ent;
  w_ref.dst = wallet_ent;

  // rw_sincro_entry_section(sincro_stuff, READER);
    wallet_trns = search_block_info(block_chain, &w_ref, is_src_or_dst_equal);
  // rw_sincro_exit_section(sincro_stuff, READER);

  if(wallet_trns != NULL)
    n_trns = wallet_trns->count;
  else
    n_trns = 0;

  send_int(fd, n_trns);

  for(int i = 1; i <= n_trns; i++)
  {
    Trns tmp = search_by_index(wallet_trns, i);
    send_trns(fd, tmp);
  }
  if(n_trns)
    printf("\nBalance of wallet[%s:%d] sent\n", wallet_ent.addr, wallet_ent.port);
  else
    printf("\nwallet[%s:%d] has not yet been subject to transactions\n", wallet_ent.addr, wallet_ent.port);


  printf("\nW_BALANCE REQUEST CORRECTLY SERVED\n");

  pthread_exit(NULL);

}

void *w_transaction(void *argfd)
{
  int fd = *(int *)argfd;
  Trns trns = (Trns) obj_malloc(TRNS);
  recv_trns(fd, trns);

  int seq_chosen;

  char *id = describe_trns(trns);
  int random = 5 + (rand()%11);

  Info_block block = create_info_block(trns, id, random, my_service_ent);
  printf("created Info_block to insert:\n");
  visit_info_block(block, visit_trns);

  printf("\nsend confirm to wallet\n");
  send_short(fd, 1);

  seq_chosen = sync_in_blockchain(block);

  /*se un wallet sta facendo una transazione NON a se stesso,
    allora devo controllare se il wallet destinatario e' agganciato a me*/
  if( !compare_net_ent(&(trns->src), &(trns->dst)) )
    warn_wallet( trns );


  struct s_info_block_pkg tosend_pkg;
  tosend_pkg.ib = *block;
  tosend_pkg.seq = seq_chosen;
  get_prev_id(block_chain, tosend_pkg.prev_id, seq_chosen);

  //TODO: rw_sincro
  rw_sincro_entry_section(sincro_stuff, WRITER);
  flooding(&tosend_pkg, my_service_ent);
  rw_sincro_exit_section(sincro_stuff, WRITER);
  printf("\nW_TRANSACTION REQUEST CORRECTLY SERVED\n");

  pthread_exit(NULL);
}



void *p_block(void *argfd)
{

  int fd = *(int *)argfd;
  IB_pkg received_pkg = (IB_pkg)obj_malloc(IB_PKG);

  recv_info_block_pkg(fd, received_pkg);

  visit_info_block(&(received_pkg->ib), visit_trns);

  List to_destroy = NULL;
  bool found;

  // TODO: rw_sincro_entry_section(sincro_stuff, WRITER);
  found = is_in_bchain(block_chain, &(received_pkg->ib), compare_trns);

  if(received_pkg->ib.confirmed) // se blocco da inserire
  {
    if(!found) // new block!
    {
      IB_pkg tosend = (IB_pkg)obj_malloc(IB_PKG);

      // o sei una nuova coda o sei il successivo in seq
      if(received_pkg->seq != block_chain->count) // seq successivo all'attuale
      {
        if(has_multi_tail(block_chain))
        {// rimuovi le teste che non hanno max rand_time
          to_destroy = extract_not_max_tails(block_chain);
        }
      }

      if(to_destroy != NULL)
      {
        for(int i = 1; i <= to_destroy->count; i++)
        {
          tosend->ib = *(Info_block)search_by_index(to_destroy, i);
          tosend->seq = received_pkg->seq;
          flooding(tosend, my_service_ent);
        }
      }
      printf("\adding to bchain \n");
      add_to_bchain(block_chain, &(received_pkg->ib), received_pkg->seq);

      //se il destinatario della transazione e' uno dei miei wallet
      warn_wallet( &(received_pkg->ib.info) );
      //segnao che ho inserito io
      reserved_tid = pthread_self();
      // flooding di quello arrivato
      struct s_connected_ent tmp;
      tmp.fd = fd;
      int i_sender_peer = search_by_info(conn_peer, &tmp, compare_peer_fd);
      Connected_ent sender_service_ent = search_by_index(conn_peer, i_sender_peer);
      flooding(received_pkg, sender_service_ent->ent);
    }
    /* else non fai niente
      gia' nella bchain quindi non devo continuare a diffonderlo perche
      significa che (sia nel caso in cui io sia oppure no il creatore), ne avro'
      gia' fatto il flooding
    */
    else
      printf("\n already in mine blockchain, doing nothing\n");


  }
  else // se il blocco si deve distruggere
  {
    if(found) // se l'ho trovato nella bchain
    {
      extract_info_block(block_chain, received_pkg->seq, &(received_pkg->ib) );


      if(compare_net_ent( &(received_pkg->ib.creator), &my_service_ent))
      {  // se sono il creatore
        received_pkg->ib.confirmed = 1;
        int seq_chosen;
        seq_chosen = sync_in_blockchain(&(received_pkg->ib));

        IB_pkg tosend_pkg = (IB_pkg)obj_malloc(IB_PKG);
        tosend_pkg->ib = received_pkg->ib;
        tosend_pkg->seq = seq_chosen;
        get_prev_id(block_chain, tosend_pkg->prev_id, seq_chosen);
        flooding(tosend_pkg, my_service_ent);
      }
      else
      { //flooding di quello che mi e' arrivato in modo che gli altrilo possano distruggere
        // capisco quale dei peer a cui sono connesso mi ha inviato il pkg
        struct s_connected_ent tmp;
        tmp.fd = fd;
        int i_sender_peer = search_by_info(conn_peer, &tmp, compare_peer_fd);
        Connected_ent sender_service_ent = search_by_index(conn_peer, i_sender_peer);
        //diffondo a tutti tranne a quello che me lo ha inviato
        flooding(received_pkg, sender_service_ent->ent);
      }
    }
    /* else  ASSUNZIONE:non fai niente perche' l'ho gia' distrutto*/
  }
  // rw_sincro_exit_section(sincro_stuff, WRITER);

  printf("\nP_BLOCK REQUEST CORRECTLY SERVED\n");
  pthread_exit(NULL);
}

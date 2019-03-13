#include "n_node.h"


int main(int argc, char **argv)
{
  int opt;
  int *flags = (int *)calloc(3, sizeof(int));
  while ( (opt = getopt(argc, argv, "n:w:")) != -1)
  {
    switch(opt)
    {
      case 'n':
        node_port = (unsigned short)atoi(optarg);
        flags[0] = 1;
        break;
      case 'w':
        wallet_port = (unsigned short)atoi(optarg);
        flags[1] = 1;
        break;
      default:
        flags[2] = 1;
        break;
    }
  }
  if( !flags[0] || !flags[1] || flags[2] )
    usage(MSG);
  free(flags);

  // pthread_mutexattr_init(&mtx_fd_attr);
  // pthread_mutexattr_setpshared(&mtx_fd_attr, PTHREAD_PROCESS_SHARED);
  // pthread_mutex_init(&mtx_fd, &mtx_fd_attr);


  //initialized just to silent warning..;
  pid_t node_server = 0;
  pid_t wallet_server = 0;
  // process used to handle node connection
  if ((node_server = fork()) < 0)
  {
    perror("node_server fork()");
    exit(EXIT_FAILURE);
  }


  //main proc creating wallet proc
  if (node_server)
  {
    if ((wallet_server = fork()) < 0)
    {
      perror("wallet_server fork()");
      exit(EXIT_FAILURE);
    }
    // TODO: change wait(NULL2) to SIGCHLD handler with p_select
  }

  if (node_server == 0)
    n_routine();


  // TODO: process used to serve wallet
  if(wallet_server == 0)
    w_routine();

  //main process
  if(node_server) // && wallet_server)
  {
    wait(&node_server);
    fprintf(stderr, "[%d] forked into [%d] and [%d]\n", getpid(), node_server, wallet_server);

    // pthread_mutex_destroy(&mtx_fd);
    // pthread_mutexattr_destroy(&mtx_fd_attr);
    // exit(EXIT_SUCCESS);

  }

}

#include "n_node.h"



int main(int argc, char **argv)
{
  int opt;
  int *flags = (int *)calloc(2, sizeof(int));

  while ((opt = getopt(argc, argv, "p:")) != -1)
  {
    switch(opt)
    {
      case 'p':
        service_port = (unsigned short)atoi(optarg);
        flags[0] = 1;
        break;
      default:
        flags[1] = 1;
        break;
    }
  }
  if( !flags[0] || flags[1] )
    usage(MSG);
  free(flags);

  // pthread_mutexattr_init(&mtx_fd_attr);
  // pthread_mutexattr_setpshared(&mtx_fd_attr, PTHREAD_PROCESS_SHARED);
  // pthread_mutex_init(&mtx_fd, &mtx_fd_attr);

  //initialized just to silent warning..



  exit_flag = 0;
  sig_act.sa_handler = sig_handler;
  sig_act.sa_flags = 0;
  sigaction(SIGINT, &sig_act, NULL);
  sigaction(SIGCHLD, &sig_act, NULL);


  n_routine();
}

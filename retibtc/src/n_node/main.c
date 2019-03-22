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

  n_routine();
}


void sig_handler(int sig_no)
{
  if(sig_no == SIGINT)
  {
    fprintf(stderr, "\nCaptured C-c, closing [%d].\n", getpid());
    exit_flag = 1;
  }
  if(sig_no == SIGUSR1)
    wakeup = 1;
}
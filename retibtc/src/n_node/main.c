#include "n_node.h"

int child_flag = 0;

// avoiding creation of zombie child
// POSIX because not using SA_RESTART
void sigchld_handl()
{
  int saved_errno;
  pid_t pid;
  //save errno current value
  do{
    saved_errno = errno;
    // waiting any sons
    pid = waitpid(WAIT_ANY, NULL, WNOHANG);
    errno = saved_errno;
  } while(pid > 0);
  //gapil implementation of sigchld handler
}

void sig_handler(int sig_no)
{
  if(sig_no == SIGINT)
  {
    printf("\nCaptured C-c, closing [%d].\n", getpid());
    exit_flag = 1;
  }
  if(sig_no == SIGCHLD)
  {
    printf("\nSons crashed.\n");
    child_flag = 1;
  }
}


int main(int argc, char **argv)
{
  int opt;
  int *flags = (int *)calloc(3, sizeof(int));
  while ((opt = getopt(argc, argv, "n:w:")) != -1)
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

  mkfifo(FIFOPATH, 0664); // u+rw, g+rw, o+r

  //initialized just to silent warning..
  pid_t node_server = 0;
  pid_t wallet_server = 0;

  exit_flag = 0;
  sig_act.sa_handler = sig_handler;
  sig_act.sa_flags = 0;
  sigaction(SIGINT, &sig_act, NULL);
  sigaction(SIGCHLD, &sig_act, NULL);


  // process used to handle node connection
  if((node_server = fork()) < 0)
  {
    perror("node_server fork()");
    exit(EXIT_FAILURE);
  }


  //main proc creating wallet proc
  if(node_server)
  {
    if((wallet_server = fork()) < 0)
    {
      perror("wallet_server fork()");
      exit(EXIT_FAILURE);
    }
  }

  if(node_server == 0)
    n_routine();

  if(wallet_server == 0)
    w_routine();


  //main process
  if(node_server && wallet_server)
  {
    printf("Waiting sons\n");
    wait(NULL);
    
    if(child_flag)
      sigchld_handl();

    if(exit_flag)
    {
      sigchld_handl();
      printf("Exiting\n");
      exit(EXIT_SUCCESS);
    }
  }
}

#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#include <getopt.h>

#include "utils.h"

int pnum = -1;
int* array;
pid_t* child_pid;

void kill_handle(int sig)
{
  for(int i = 0; i < pnum; i++)
  {
    kill(child_pid[i],SIGKILL);
  }
  free(array);
  free(child_pid);
  printf("Timeout.\n");
  exit(0);
}

int main(int argc, char **argv) 
{
  int seed = -1;
  int array_size = -1;
  int timeout = 0;
  bool with_files = false;

  while (true) 
  {
    int current_optind = optind ? optind : 1;

    static struct option options[] = {{"seed", required_argument, 0, 0},
                                      {"array_size", required_argument, 0, 0},
                                      {"pnum", required_argument, 0, 0},
                                      {"timeout", required_argument, 0, 1},  
                                      {"by_files", no_argument, 0, 'f'},
                                      {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "f", options, &option_index);

    if (c == -1) break;

    switch (c) 
    {
      case 0:
        switch (option_index) 
        {
          case 0:
            seed = atoi(optarg);
            if (seed <= 0) seed = -1;
            break;
          case 1:
            array_size = atoi(optarg);
            if (array_size <= 0) array_size = -1;
            break;
          case 2:
            pnum = atoi(optarg);
            if (pnum <= 0) pnum = -1;
            break;
          case 3:
            with_files = true;
            break;

          defalut:
            printf("Index %d is out of options\n", option_index);
        }
      case 1:
      {
          timeout = atoi(optarg);
          if (timeout <= 0) timeout = -1;
          break;
      } 
      case 'f':
        with_files = true;
        break;

      case '?':
        printf("Some error has been occurred. getopt_long returned ?\n");
        break;

      default:
        printf("getopt returned character code 0%o?\n", c);
    }
  }

  if (optind < argc) 
  {
    printf("Has at least one no option argument\n");
    return 1;
  }

  if (seed == -1 || array_size == -1 || pnum == -1 || timeout == -1) 
  {
    printf("Usage: %s --seed \"num\" --array_size \"num\" --pnum \"num\" \n",
           argv[0]);
    return 1;
  }

  array = malloc(sizeof(int) * array_size);
  GenerateArray(array, array_size, seed);
  
  int pipefd[2];
  if (pipe(pipefd) == -1) 
  {
    printf("Pipe failed!\n");
    free(array);
    return 1;
  }

  FILE* file = fopen("parallel_synch.txt","w");
  if (file == NULL)
  {
    printf("fopen error!\n");
    free(array);
    return 1;
  }
  
  struct timeval start_time;
  gettimeofday(&start_time, NULL);
  
  child_pid = malloc(sizeof(pid_t)*pnum);

  signal(SIGALRM, kill_handle);
  
  for (int i = 0; i < pnum; i++) 
  {
    child_pid[i] = fork();
    if (child_pid[i] >= 0) 
    {
      // successful fork
      if (child_pid[i] == 0) 
      {
        // child process
        struct MinMax min_max = GetMinMax(array,i*array_size/pnum,(i+1)*array_size/pnum);

        if (with_files) 
        {
          fprintf(file, "%d\n",min_max.min);
          fprintf(file, "%d\n",min_max.max);
        } 
        else 
        {
          write(pipefd[1],&min_max,8);
        }
        exit(0);
      }
    } 
    else 
    {
      printf("Fork failed!\n");
      free(child_pid);
      free(array);
      return 1;
    }
  }

  int status;
  alarm(timeout);
  waitpid(0, &status, WNOHANG);
  
  file = fopen("parallel_synch.txt","r");
  if (file == NULL)
  {
    printf("fopen error!\n");
    free(child_pid);
    free(array);
    return 1;
  }
    
  struct MinMax min_max;
  min_max.min = INT_MAX;
  min_max.max = INT_MIN;

  for (int i = 0; i < pnum; i++) 
  {
    int min = INT_MAX;
    int max = INT_MIN;
    char c;
    
    if (with_files) 
    {
      fscanf(file,"%d",&min);
      fscanf(file,"%d",&max);
    } 
    else 
    {
      read(pipefd[0],&min,4);
      read(pipefd[0],&max,4);
    }

    if (min < min_max.min) min_max.min = min;
    if (max > min_max.max) min_max.max = max;
  }

  struct timeval finish_time;
  gettimeofday(&finish_time, NULL);

  double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
  elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

  fclose(file);
  close(pipefd[0]);
  close(pipefd[1]);
  free(child_pid);
  free(array);      
  
  printf("Min: %d\n", min_max.min);
  printf("Max: %d\n", min_max.max);
  printf("Elapsed time: %fms\n", elapsed_time);
  fflush(NULL);
  return 0;
}

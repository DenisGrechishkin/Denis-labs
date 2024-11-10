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

#include <getopt.h>

#include "find_min_max.h"
#include "utils.h"

int main(int argc, char **argv) {
  int seed = -1;
  int array_size = -1;
  int pnum = -1;
  bool with_files = false;

  while (true) {
    int current_optind = optind ? optind : 1;

    static struct option options[] = {{"seed", required_argument, 0, 0},
                                      {"array_size", required_argument, 0, 0},
                                      {"pnum", required_argument, 0, 0},
                                      {"by_files", no_argument, 0, 'f'},
                                      {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "f", options, &option_index);

    if (c == -1) break;

    switch (c) {
      case 0:
        switch (option_index) {
          case 0:
            seed = atoi(optarg);
            // your code here
            // error handling
            if (seed<=0)
            {
              printf("Seed must be a positive number");
                return 1;
            }
            break;
          case 1:
            array_size = atoi(optarg);
            // your code here
            // error handling
            if (array_size<=0)
            {
              printf("Array size must be a positive number");
                return 1;
            }
            break;
          case 2:
            pnum = atoi(optarg);
            // your code here
            // error handling
            if (pnum<1)
            {
              printf("Process number must be > 1");
                return 1;
            }
            break;
          case 3:
            with_files = true;
            break;

          defalut:
            printf("Index %d is out of options\n", option_index);
        }
        break;
      case 'f':
        with_files = true;
        break;

      case '?':
        break;

      default:
        printf("getopt returned character code 0%o?\n", c);
    }
  }

  if (optind < argc) { // optind - количесвто комментов, которые обработал getopt_long
    printf("Has at least one no option argument\n");
    return 1;
  }

  if (seed == -1 || array_size == -1 || pnum == -1) {
    printf("Usage: %s --seed \"num\" --array_size \"num\" --pnum \"num\" \n",
           argv[0]);
    return 1;
  }

  int *array = malloc(sizeof(int) * array_size);
  GenerateArray(array, array_size, seed);
  int active_child_processes = 0;

  int fd[2];
  pipe(fd);
  
  FILE *f1;
  if(with_files)
  {
    f1 = fopen("task_2-3.txt", "w");
          if (!f1)
          {
            printf("Error txt");
            return 1;
          }
          fclose(f1);
  }
  struct timeval start_time;
  gettimeofday(&start_time, NULL);

  for (int i = 0; i < pnum; i++) {
    pid_t child_pid = fork();
    if (child_pid >= 0) {
      // successful fork
      active_child_processes += 1;
      if (child_pid == 0) {
        // child process
        // parallel somehow
        int begin = i*array_size/pnum;
        int end;
        if (i == pnum - 1)
          end = array_size;
        else
          end = (i + 1)*array_size/pnum;
        struct MinMax min_max = GetMinMax(array, begin, end);

        
        if (with_files) {
          // use files here
          FILE *f = fopen("task_2-3.txt", "a");
          if (!f)
          {
            printf("Error txt");
            return 1;
          }

          fprintf(f, "%d %d ", min_max.min, min_max.max);
          fclose(f);
        } else {
          // use pipe here
          write(fd[1], &min_max.min, sizeof(int));
          write(fd[1], &min_max.max, sizeof(int));
          close(fd[0]); // закрытие дискриптора на чтение
          close(fd[1]); // закрытие дискриптора на запись
        }
        return 0;
      }

    } else {
      printf("Fork failed!\n");
      return 1;
    }
  }

  close(fd[1]);
  while (active_child_processes > 0) {
    // your code here
    wait(NULL);
    active_child_processes -= 1;
  }

  struct MinMax min_max;
  min_max.min = INT_MAX;
  min_max.max = INT_MIN;

  FILE *f;
  if(with_files)
  {
     f = fopen("task_2-3.txt", "r");
          if (!f)
          {
            printf("Error txt");
            return 1;
          }
  }  

  for (int i = 0; i < pnum; i++) {
    int min = INT_MAX;
    int max = INT_MIN;

    
    if (with_files) {
      // read from files
      fscanf(f, "%d %d ", &min, &max);
    } else {
      // read from pipes
      read(fd[0], &min, sizeof(int));
      read(fd[0], &max, sizeof(int));
    }

    if (min < min_max.min) min_max.min = min;
    if (max > min_max.max) min_max.max = max;
  }
  if(with_files)
    fclose(f);
  close(fd[0]);
  
  struct timeval finish_time;
  gettimeofday(&finish_time, NULL);

  double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
  elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

  free(array);

  printf("Min: %d\n", min_max.min);
  printf("Max: %d\n", min_max.max);
  printf("Elapsed time: %fms\n", elapsed_time);
  fflush(NULL);
  return 0;
}

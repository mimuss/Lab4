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
#include <signal.h>   //Для signal

#include "find_min_max.h"
#include "utils.h"

void handler(int sig){
    printf("SIGALRM: pid=%d, sig=%d\n", getpid(), sig);
    fflush(NULL);
    int result = kill(0, SIGKILL);
    printf("kill return %d\n", result);
    _Exit (0); 
}

int main(int argc, char **argv) {
    signal (SIGALRM, handler);
  int seed = -1;
  int array_size = -1;
  int pnum = -1;
  int timeout = -1;
  bool with_files = false;

  while (true) {
    int current_optind = optind ? optind : 1;

    static struct option options[] = {{"seed", required_argument, 0, 0},
                                      {"array_size", required_argument, 0, 0},
                                      {"pnum", required_argument, 0, 0},
                                      {"by_files", no_argument, 0, 'f'},
                                      {"timeout", required_argument, 0, 0},
                                      {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "f", options, &option_index);

    if (c == -1) break;

    switch (c) {
      case 0:
        switch (option_index) {
          case 0:
            seed = atoi(optarg);
            if (seed <= 0) {
                printf("seed is a positive number\n");
                return 1;
            }
            break;
          case 1:
            array_size = atoi(optarg);
            if (array_size <= 0) {
               printf("array_size is a positive number\n");
               return 1;
            }
            break;
          case 2:
            pnum = atoi(optarg);
            if (array_size <= 0) {
               printf("pnum is a positive number\n");
               return 1;
            }
            break;
          case 3:
            with_files = true;
            break;
          case 4:
            timeout = atoi(optarg);
            if (timeout <= 0) {
               printf("timeout is a positive number\n");
               return 1;
            }
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

  if (optind < argc) {
    printf("Has at least one no option argument\n");
    return 1;
  }

  if (seed == -1 || array_size == -1 || pnum == -1) {
    printf("Usage: %s --seed \"num\" --array_size \"num\" --pnum \"num\" \n",
           argv[0]);
    return 1;
  }
  
  if (pnum > 1000){
      printf("pnum > 1000\n");
      return 1;
  }

  int *array = malloc(sizeof(int) * array_size);
  GenerateArray(array, array_size, seed);
  int active_child_processes = 0;

  struct timeval start_time;
  gettimeofday(&start_time, NULL);
  
  int fd[1000][2];
  if (with_files){
    FILE* file = fopen("result.txt", "w");
    fclose (file);
  }else{
      for (int i = 0; i < pnum; i++)
        if(pipe(fd[i]) < 0){
            printf("Can\'t create pipe\n");
            exit(-1); 
        } 
  }

  for (int i = 0; i < pnum; i++) {
    pid_t child_pid = fork();
    if (child_pid >= 0) {
      // successful fork
      active_child_processes += 1;
      if (child_pid == 0) {
        // child process
        // parallel somehow
        int start = i * array_size / pnum;
        int finish = (i + 1) * array_size / pnum;
        struct MinMax min_max = GetMinMax(array, start, finish);
        printf("Child: pid-%4d, start: %4d, finish: %4d", getpid(), start, finish);
        printf("min: %10d, max: %10d\n", min_max.min, min_max.max);
        if (with_files) {
          FILE* file = fopen("result.txt", "a");
          fprintf (file, "%d %d\n", min_max.min, min_max.max);
          fclose (file);
        } else {
          close(fd[i][0]);
          write(fd[i][1], &min_max, sizeof(min_max));
          close(fd[i][1]);
        }
        return 0;
      }

    } else {
      printf("Fork failed!\n");
      return 1;
    }
  }
 
  int rv;
  
  if (timeout > -1)
    alarm(timeout);
  
  printf("Коды возврата потомков: ");
  while (active_child_processes > 0) {
    printf("%d ", WEXITSTATUS(rv));

    active_child_processes -= 1;
  }
  printf("\n");

  struct MinMax min_max, min_max2;
  min_max.min = INT_MAX;
  min_max.max = INT_MIN;
  
  if (with_files){
      int min, max;
      FILE* file = fopen("result.txt", "r");
      while(!feof(file)){
        fscanf(file,"%d %d\n", &min, &max);
        
        if (min < min_max.min) min_max.min = min;
        if (max > min_max.max) min_max.max = max;
      }
      fclose(file);
  }else
      for (int i = 0; i < pnum; i++){
          close(fd[i][1]);
          read(fd[i][0], &min_max2, sizeof(min_max2));
          close(fd[i][0]);
          if (min_max2.min < min_max.min) min_max.min = min_max2.min;
          if (min_max2.max > min_max.max) min_max.max = min_max2.max;
          printf("%d: %d %d\n", i, min_max2.min, min_max2.max);
      }

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

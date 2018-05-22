#include <stdint.h>

#include <pthread.h>
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

#include "utils.h"
#include "mySum.h"

void *ThreadSum(void *args) {
  struct SumArgs *sum_args = (struct SumArgs *)args;
  return (void *)(size_t)Sum(sum_args);
}

int main(int argc, char **argv) {
  /*
   *  TODO:
   *  threads_num by command line arguments
   *  array_size by command line arguments
   *	seed by command line arguments
   */

  uint32_t threads_num = -1;
  uint32_t array_size = -1;
  uint32_t seed = -1;
  int temp;
  
  
  while (true) {
    int current_optind = optind ? optind : 1;

    static struct option options[] = {{"threads_num", required_argument, 0, 0},
                                      {"array_size", required_argument, 0, 0},
                                      {"seed", required_argument, 0, 0},
                                      {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "f", options, &option_index);

    if (c == -1) break;

    switch (c) {
      case 0:
        switch (option_index) {
          case 0:
            temp = atoi(optarg);
            if (temp <= 0) {
                printf("seed is a positive number\n");
                return 1;
            }
            threads_num = temp;
            break;
          case 1:
            temp = atoi(optarg);
            if (temp <= 0) {
               printf("array_size is a positive number\n");
               return 1;
            }
            array_size = temp;
            break;
          case 2:
            temp = atoi(optarg);
            if (temp <= 0) {
               printf("pnum is a positive number\n");
               return 1;
            }
            seed = temp;
            break;

          defalut:
            printf("Index %d is out of options\n", option_index);
        }
        break;

      default:
        printf("getopt returned character code 0%o?\n", c);
    }
  }

  if (optind < argc) {
    printf("Has at least one no option argument\n");
    return 1;
  }

  if (seed == -1 || array_size == -1 || threads_num == -1) {
    printf("Usage: %s --seed \"num\" --array_size \"num\" --threads_num \"num\" \n",
           argv[0]);
    return 1;
  }
  
  if (threads_num > 1000){
      printf("pnum > 1000\n");
      return 1;
  }

  /*
   * TODO:
   * your code here
   * Generate array here
   */

  int *array = malloc(sizeof(int) * array_size);
  
  GenerateArray(array, array_size, seed);
  /*printf("array: ");
  for (int i = 0; i < array_size; i++)
    printf("%d ", array[i]);
  printf("\n");*/
  
  //struct SumArgs args[threads_num];
  struct SumArgs *args = malloc(sizeof(struct SumArgs) * threads_num);
  //pthread_t threads[threads_num];
  pthread_t *threads = malloc(sizeof(pthread_t) * threads_num);

  //printf("parts: ");
  for (uint32_t i = 0; i < threads_num; i++) {
    args[i].begin = i * array_size / threads_num;
    args[i].end = (i + 1) * array_size / threads_num;
    //printf("%d - %d | ", args[i].begin, args[i].end);
    args[i].array = array;
  }
  //printf("\n");

  struct timeval start_time;
  gettimeofday(&start_time, NULL);

  for (uint32_t i = 0; i < threads_num; i++) {
    if (pthread_create(&threads[i], NULL, ThreadSum, (void *)&(args[i]))) {
      printf("Error: pthread_create failed!\n");
      return 1;
    }
  }

  int total_sum = 0;
  for (uint32_t i = 0; i < threads_num; i++) {
    int sum = 0;
    pthread_join(threads[i], (void **)&sum);
    total_sum += sum;
  }

  struct timeval finish_time;
  gettimeofday(&finish_time, NULL);

  double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
  elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

  free(array);
  free(args);
  free(threads);
  printf("Total: %d\n", total_sum);
  printf("Elapsed time: %fms\n", elapsed_time);
  return 0;
}

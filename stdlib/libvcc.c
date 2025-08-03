#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void print_line(char *s) {
  printf("%s\n", s);
  fflush(stdout);
}

void print_string(char *s) {
  printf("%s", s);
  fflush(stdout);
}

void print_integer(int a) {
  printf("%d", a);
  fflush(stdout);
}

void *vcc_malloc(size_t size) { return malloc(size); }

void vcc_free(void *at) { return free(at); }

void *get_nullptr() { return NULL; }

void init_rand() { srand(time(NULL)); }

int vcc_rand(int low, int high) { return low + (rand() % (high - low + 1)); }

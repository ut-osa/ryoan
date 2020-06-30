#include<unistd.h>
#include<stdio.h>
#include<fcntl.h>

#define N 20000

long cnt;
char buf[128];

int looper(int k) {
  int i;
  for (i = 0; i < k; i++) {
    if ((k * i) % 71 == 0) {
      cnt++;
    }
  }
}

void enable_flush() {
  read(-100, buf, 1);
}

void disable_flush() {
  read(-100, buf, 0);
}

void print_counts() {
  read(-100, buf, 100);
  printf("%s", buf);
}

int main(int argc, const char *argv[])
{
  int i;
  read(-100, buf, 64);
  cnt = 0;

  enable_flush();

  printf("abc\n");

  for (i = 0; i < N; i++) {
    looper(i);
  }
  printf("something meaningless: %ld\n", cnt);
  print_counts();
  disable_flush();
}

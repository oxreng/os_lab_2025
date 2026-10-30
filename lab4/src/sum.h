#ifndef SUM_H
#define SUM_H

struct SumArgs {
  const int *array;
  int begin;
  int end;
};

int Sum(const struct SumArgs *args);

#endif

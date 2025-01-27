#include <stdio.h>
#include <stdint.h>

int main() {

  int64_t i64 = 10;
  bool is_neg = i64 & INT64_MIN;
  printf("%llu, %d\n", (uint64_t) (i64 + INT64_MIN), is_neg);

  i64 = -10;
  is_neg = i64 & INT64_MIN;
  printf("%llu, %d\n", (uint64_t) (i64 + INT64_MIN), is_neg);

  return 1;

}


#include <stdint.h>

#include "vint.hpp"

void test_range(int64_t start, int64_t end) {
  uint8_t svint_str[10];
  size_t svint_len;
  for (int64_t i = start; i <= end; i++) {
    svint_len = gen::get_svint60_len(i);
    gen::copy_svint60(i, svint_str, svint_len);
    int64_t out64 = gen::read_svint60(svint_str);
    printf("Expected : %lld, Actual: %lld, len: %lu", i, out64, svint_len);
    if (i == out64)
      printf("\n");
    else {
      printf(" - NOT MATCHING\n");
      for (size_t i = 0; i < svint_len; i++)
        printf("%2x ", svint_str[i]);
      printf("\n");
    }
  }
}

int main() {

  // size_t err_count = 0;
  // for (int64_t i = SVINT60_MIN; i <= SVINT60_MAX; i++) {
  //   svint_len = gen::get_svint60_len(i);
  //   gen::copy_svint60(i, svint_str, svint_len);
  //   int64_t out64 = gen::read_svint60(svint_str);
  //   if (out64 != i) {
  //     printf("Expected : %lld, Actual: %lld, len: %lu\n", i, out64, svint_len);
  //     err_count++;
  //   }
  //   if ((abs(i) % 10000000) == 0) {
  //     //printf("%15lu\r", err_count);
  //     printf(err_count > 0 ? "x" : ".");
  //     fflush(stdout);
  //   }
  // }
  // printf("\nErrors: %lu\n", err_count);

  test_range(-1, -1);

  int bits = 4;
  while (bits <= 60) {
    int64_t start = (1LL << bits) - 10;
    int64_t end = (1LL << bits) + 10;
    if (bits == 60)
      end = (1LL << bits) - 1;
    test_range(start, end);
    start = (1LL << bits) + 10;
    end = (1LL << bits) - 10;
    if (bits == 60)
      start = (1LL << bits) - 1;
    test_range(start * -1, end * -1);
    bits += 8;
  }
  return 0;
}

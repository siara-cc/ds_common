#include <vector>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "bv.hpp"
#include "gen.hpp"

void print_bytes(const uint8_t *in, int len) {

  int l;
  uint8_t bit;
  for (l=0; l<len*8; l++) {
    bit = (in[l/8]>>(7-l%8))&0x01;
    printf("%d", bit);
    //if (l%8 == 7) printf(" ");
  }
  printf("\n");

}

double time_taken_in_secs(struct timespec t) {
  struct timespec t_end;
  clock_gettime(CLOCK_REALTIME, &t_end);
  return (t_end.tv_sec - t.tv_sec) + (t_end.tv_nsec - t.tv_nsec) / 1e9;
}

struct timespec print_time_taken(struct timespec t, const char *msg) {
  double time_taken = time_taken_in_secs(t); // in seconds
  printf("%s %lf\n", msg, time_taken);
  clock_gettime(CLOCK_REALTIME, &t);
  return t;
}

int last_byte_bits;
int append_ptr_bits(std::vector<uint8_t>& ptrs, uint32_t given_ptr, int bits_to_append) {
  uint64_t ptr = given_ptr;
  uint64_t *last_ptr = (uint64_t *) (ptrs.data() + ptrs.size() - 8);
  while (bits_to_append > 0) {
    if (bits_to_append < last_byte_bits) {
      last_byte_bits -= bits_to_append;
      *last_ptr |= (ptr << last_byte_bits);
      bits_to_append = 0;
    } else {
      bits_to_append -= last_byte_bits;
      *last_ptr |= (ptr >> bits_to_append);
      last_byte_bits = 64;
      gen::append_uint64(0, ptrs);
      last_ptr = (uint64_t *) (ptrs.data() + ptrs.size() - 8);
    }
  }
  return last_byte_bits;
}

int bit_count(int64_t num) {
  return num > 0 ? 64 - __builtin_clzll(num) : 1;
}

int bit_count(int32_t num) {
  return num > 0 ? 32 - __builtin_clz(num) : 1;
}

template <class T>
size_t load_ints(const char *filename, std::vector<T>& ints) {
  struct timespec t;
  clock_gettime(CLOCK_REALTIME, &t);
  FILE *fp = fopen(filename, "rb");
  char line[100];
  size_t i = 0;
  size_t line_count = 0;
  while (fgets(line, sizeof(line), fp)) {
    ints.push_back(atoll(line));
    line_count++;
  }
  fclose(fp);
  printf("\nLoad KPS: %lf, ", line_count / time_taken_in_secs(t) / 1000);
  print_time_taken(t, "Time taken: ");
  return line_count;
}

template <class T>
void process_ints(const char *filename) {
  std::vector<T> ints;
  size_t line_count = load_ints(filename, ints);

  print_bytes((const uint8_t *) ints.data(), 64);

  uint64_t log2_counts[128];
  memset(log2_counts, '\0', 128 * sizeof(uint64_t));

  std::vector<uint8_t> ptrs;
  std::vector<uint32_t> ptrs32;
  std::vector<uint16_t> ptrs16;
  std::vector<uint8_t> ptrs8;

  struct timespec t;
  clock_gettime(CLOCK_REALTIME, &t);
  last_byte_bits = 64;
  gen::append_uint64(0, ptrs);
  T prev_int = 0;
  for (size_t i = 0; i < line_count; i++) {
    if ((i % 64) == 0)
      printf("E\n\nS");
    // prev_int = ((i > 0 && i%64) ? ints.at(i - 1) : 0);
    T val = ints.at(i) - prev_int;
    uint8_t log2_i64 = 1;
    if (val < 0) {
      // log2_i64 = 0x40;
      log2_i64 = 1;
      val = std::abs(val);
    }
    log2_i64 += bit_count(val);
    log2_counts[log2_i64]++;
    // if (log2_i64 % 8)
    //   log2_i64 += (8 - (log2_i64 % 8));
    printf("%u\t", log2_i64);
    append_ptr_bits(ptrs, val, log2_i64);
  }
  printf("\nCount: %lu, compressed: %lu, KPS: %lf, ", line_count, ptrs.size() + ptrs32.size() * 4 + ptrs16.size() * 2 + ptrs8.size(), line_count / time_taken_in_secs(t) / 1000);
  print_time_taken(t, "Time taken: ");
  size_t total_bits = 0;
  size_t bits_count = 0;
  for (size_t i = 0; i < 128; i++) {
    if (log2_counts[i] > 0) {
      printf("%lu\t%llu\n", i, log2_counts[i]);
      total_bits += i;
      bits_count++;
    }
  }
  gen::int_bv_reader ibr;
  size_t bit_len = total_bits/bits_count + 3;
  printf("Bit len: %lu\n", bit_len);
  ibr.init(ptrs.data(), bit_len);
  clock_gettime(CLOCK_REALTIME, &t);
  for (size_t i = 0; i < line_count - 100; i++) {
    ints[i] = ibr[i];
  }
  printf("\nKPS: %lf, ", (line_count - 100) / time_taken_in_secs(t) / 1000);
  print_time_taken(t, "Time taken: ");
}

int main(int argc, char *argv[]) {

  if (argc < 2) {
    printf("Need filename\n");
    return 1;
  }

  process_ints<int64_t>(argv[1]);

  printf("Completed\n");

  return 0;
}

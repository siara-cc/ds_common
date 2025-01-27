#include <time.h>
#include <stdio.h>
#include <stdint.h>

#include "../src/vint.hpp"

#include <vector>
#include <cstdint>
#include <cstring>

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

void makeSortable(int64_t value, uint8_t *buffer) {
    uint64_t sortable = static_cast<uint64_t>(value) + static_cast<uint64_t>(INT64_MIN);
    for (int i = 7; i >= 0; --i) {
        buffer[i] = static_cast<uint8_t>(sortable & 0xFF);
        sortable >>= 8;
    }
}

int64_t read_sortable(uint8_t *buffer) {
  uint64_t u64 = 0;
  for (size_t i = 0; i < 8; i++) {
    u64 <<= 8;
    u64 += buffer[i];
  }
  int64_t i64 = (int64_t) u64;
  i64 -= INT64_MIN;
  return i64;
}

const static uint64_t mask64[] = { 0xFFUL, 0xFFFFUL, 0xFFFFFFUL, 0xFFFFFFFFUL , 0xFFFFFFFFFFUL , 0xFFFFFFFFFFFFUL , 0xFFFFFFFFFFFFFFUL , 0xFFFFFFFFFFFFFFFFUL };
int main(int argc, char *argv[]) {
  if (argc < 2) {
    printf("Please provide filename\n");
    return 0;
  }

  FILE *fp = fopen(argv[1], "rb");
  char buffer[50];
  size_t line_count = 0;
  std::vector<int64_t> mydata;
  while (fgets(buffer, sizeof(buffer), fp) != NULL) {
    mydata.push_back(atoll(buffer));
    line_count++;
  }
  fclose(fp);
  printf("Line count: %lu\n", line_count);

  std::vector<uint8_t> svint_buf(line_count * 8);

  struct timespec t;
  clock_gettime(CLOCK_REALTIME, &t);

  int64_t i64, i64_1;

  size_t svbuf_len = 0;
  for (size_t i = 0; i < line_count; i++) {
    i64 = mydata.at(i);
    size_t vlen = gen::get_svint60_len(i64);
    // printf("vlen: %lu\n", vlen);
    gen::copy_svint60(i64, svint_buf.data() + svbuf_len, vlen);
    svbuf_len += vlen;
  }
  printf("\nSortable svint encode ips: %lf, size: %lu\n", line_count / time_taken_in_secs(t) / 1000, svbuf_len);
  t = print_time_taken(t, "Time taken for svint encode: ");

  std::vector<uint8_t> vint_buf(line_count * 8);

  clock_gettime(CLOCK_REALTIME, &t);
  size_t vbuf_len = 0;
  uint8_t *vbuf_out = vint_buf.data();
  for (size_t i = 0; i < line_count; i++) {
    i64 = mydata.at(i);
    size_t vlen = 0;
    uint64_t u64 = (uint64_t) i64;
    vbuf_out++;
    do {
      vlen++;
      *vbuf_out++ = u64 & 0xFF;
      u64 >>= 8;
    } while (u64 > 0);
    *(vbuf_out - vlen - 1) = vlen;
  }
  vbuf_len = vbuf_out - vint_buf.data();
  printf("\nvint encode ips: %lf, size: %lu\n", line_count / time_taken_in_secs(t) / 1000, vbuf_len);
  t = print_time_taken(t, "Time taken for svint encode: ");

  size_t sbuf_len = line_count * 8;
  std::vector<uint8_t> sint_buf(sbuf_len);

  clock_gettime(CLOCK_REALTIME, &t);
  uint8_t *sint_src = sint_buf.data();
  for (size_t i = 0; i < line_count; i++) {
    i64 = mydata.at(i);
    makeSortable(i64, sint_src);
    sint_src += 8;
  }
  printf("\nSortable int encode ips: %lf, size: %lu\n", line_count / time_taken_in_secs(t) / 1000, sbuf_len);
  t = print_time_taken(t, "Time taken for vint encode: ");

  clock_gettime(CLOCK_REALTIME, &t);
  size_t err_count = 0;
  uint8_t *svint_src = svint_buf.data();
  for (size_t i = 0; i < line_count; i++) {
    i64 = mydata.at(i);
    i64_1 = gen::read_svint60(svint_src);
    size_t vlen = gen::get_svint60_len(i64_1);
    if (i64 != i64_1)
      err_count++;
    svint_src += vlen;
  }
  printf("\nSortable int decode ips: %lf, error count: %lu\n", line_count / time_taken_in_secs(t) / 1000, err_count);
  t = print_time_taken(t, "Time taken for vint encode: ");

  clock_gettime(CLOCK_REALTIME, &t);
  err_count = 0;
  uint8_t *vint_src = vint_buf.data();
  for (size_t i = 0; i < line_count; i++) {
    i64 = mydata.at(i);
    size_t vlen = *vint_src++;
    i64_1 = *((int64_t *) vint_src);
    i64_1 &= mask64[vlen - 1];
    if (i64 != i64_1)
      err_count++;
    vint_src += vlen;
  }
  printf("\nVInt decode ips: %lf, error count: %lu\n", line_count / time_taken_in_secs(t) / 1000, err_count);
  t = print_time_taken(t, "Time taken for vint decode: ");

  clock_gettime(CLOCK_REALTIME, &t);
  err_count = 0;
  sint_src = sint_buf.data();
  for (size_t i = 0; i < line_count; i++) {
    i64 = mydata.at(i);
    i64_1 = read_sortable(sint_src);
    if (i64 != i64_1)
      err_count++;
    sint_src += 8;
  }
  printf("\nSInt decode ips: %lf, error count: %lu\n", line_count / time_taken_in_secs(t) / 1000, err_count);
  t = print_time_taken(t, "Time taken for sint decode: ");

  return 1;

}

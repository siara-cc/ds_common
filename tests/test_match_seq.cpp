#include <stdio.h>
#include <stdint.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <iostream>

#include "../src/match_seq.hpp"

using namespace gen;
using namespace std;

void test_string() {
  seq_matcher sm(16);
  const char *s = "Rose is a rose is a rose is a rose\n" // 34
                  "Where is the Life we have lost in living?\n" // 41 76
                  "Where is the wisdom we have lost in knowledge?\n" // 46 123
                  "Where is the knowledge we have lost in information?"; // 51 175
  // const char *s = "Winfield 3 DLX Hardside Luggage with Spinners, 3-Piece Set (20/25/28), Blue/Navy\n"
  //                 "Oxford Expandable Spinner Luggage Suitcase with TSA Lock - 30.1 Inch, Black\n"
  //                 "Chatelet Hardside 2.0 Luggage with Spinner Wheels, Chocolate Brown, Checked-Medium 24 Inch\n"
  //                 "Anzio Softside Expandable Spinner Luggage, Teal, Checked-Large 30-Inch\n"
  //                 "Amsterdam Expandable Rolling Upright Luggage, Orange, Checked-Large 29-Inch\n"
  //                 "4 Kix Expandable Softside Luggage with Spinner Wheels, Black/Grey, Carry-On 21-Inch\n"
  //                 "4010 Softside Luggage with Spinner Wheels, Black, Checked-Medium 23-Inch\n";

  int s_len = strlen(s);
  sm.add_sequence((const uint8_t *) s, s_len, 0);
  for (int i = 4; i < s_len; i++) {
    sm_longest sl = sm.find_longest((const uint8_t *) s, i, s_len - i, (const uint8_t *) s, i + 1);
    if (sl.pos != -1) {
      printf("%d, %d, [%.*s], %d\n", i, i + 1 - sl.pos, sl.len, s + i + 1 - sl.pos, sl.len);
      i += sl.len;
      i--;
    }
  }
}

void test_file(const char *filename) {

  FILE *fp = fopen(filename, "rb");
  if (fp == NULL) {
    perror("Could not open file: ");
    return;
  }
  struct stat file_stat;
  memset(&file_stat, '\0', sizeof(file_stat));
  stat(filename, &file_stat);
  int file_len = file_stat.st_size;
  uint8_t *src_buf = (uint8_t *) malloc(file_len + 1);
  if (fread(src_buf, 1, file_len, fp) != file_len) {
    free(src_buf);
    perror("Error reading file: ");
    return;
  }
  fclose(fp);
  uint8_t *c_buf = (uint8_t *) malloc(file_len * 1.2);

  clock_t t = clock();

  seq_matcher sm(22);

  sm_longest sl;
  int savings = 0;
  int src_pos = 0;
  int c_len = 0;
  for (; src_pos < file_len; ) {
    sl = sm.find_longest(src_buf, src_pos, file_len - src_pos, c_buf, c_len);
    if (sl.pos == -1 || sl.sav < 6) {
      sm.add_sequence(src_buf + src_pos, 3, c_len);
      uint8_t b = src_buf[src_pos++];
      c_buf[c_len++] = b;
      if (b == '\0') {
        c_buf[c_len++] = '\0';
        savings--;
      }
    } else {
      //printf("%d, %d, [%.*s], %d\n", src_pos, c_len - sl.pos, sl.len, c_buf + c_len - sl.pos, sl.len);
      src_pos += sl.len;
      savings += sl.len;
      c_buf[c_len++] = '\0';
      savings--;
      int8_t vlen = gen::get_vlen_of_uint32(sl.pos);
      gen::copy_vint32(sl.pos, c_buf + c_len, vlen);
      c_len += vlen;
      savings -= vlen;
      vlen = gen::get_vlen_of_uint32(sl.len);
      gen::copy_vint32(sl.len, c_buf + c_len, vlen);
      c_len += vlen;
      savings -= vlen;
    }
  }
  c_buf[c_len++] = '\0';

  int w_file_len = strlen(filename);
  char w_filename[w_file_len + 5];
  strncpy(w_filename, filename, w_file_len);
  w_filename[w_file_len] = '\0';
  w_file_len += 4;
  strncat(w_filename, ".msz", w_file_len);
  fp = fopen(w_filename, "w+");
  fwrite(c_buf, c_len, 1, fp);
  fclose(fp);

  print_time_taken(t, "Time taken for matches: ");
  printf("Savings: %d, c_len: %d\n", savings, c_len);

  free(src_buf);
  free(c_buf);

}

void test_decode_file(const char *filename) {

  FILE *fp = fopen(filename, "rb");
  if (fp == NULL) {
    perror("Unable to open file: ");
    return;
  }
  struct stat file_stat;
  memset(&file_stat, '\0', sizeof(file_stat));
  stat(filename, &file_stat);
  int file_len = file_stat.st_size;
  uint8_t *src_buf = (uint8_t *) malloc(file_len + 1);
  if (fread(src_buf, 1, file_len, fp) != file_len) {
    free(src_buf);
    perror("Error reading file: ");
    return;
  }
  fclose(fp);

  int w_file_len = strlen(filename);
  printf("file: %d, [%.*s]\n", w_file_len, w_file_len, filename);
  char w_filename[w_file_len + 5];
  strncpy(w_filename, filename, w_file_len);
  w_filename[w_file_len] = '\0';
  w_file_len += 4;
  strncat(w_filename, ".msd", w_file_len);
  printf("filew: %d, [%.*s]\n", w_file_len, w_file_len, w_filename);
  fp = fopen(w_filename, "w+");

  uint8_t *t = src_buf;
  uint8_t *t_end = src_buf + file_len;
  while (t < t_end) {
    uint8_t b = *t++;
    while (b != '\0') {
      fputc(b, fp);
      b = *t++;
    }
    if (t == t_end)
      break;
    if (*t == 0) {
      fputc(0, fp);
      t++;
    } else {
      int offset = 1;
      int8_t vlen;
      int rel_pos = gen::read_vint32(t, &vlen);
      t += vlen;
      offset += vlen;
      int len = gen::read_vint32(t, &vlen);
      rel_pos += offset;
      fwrite(t - rel_pos, len, 1, fp);
      t += vlen;
    }
  }

  fclose(fp);

}

void test_file1(const char *filename) {

  FILE *fp = fopen(filename, "rb");
  if (fp == NULL) {
    perror("Could not open file: ");
    return;
  }
  struct stat file_stat;
  memset(&file_stat, '\0', sizeof(file_stat));
  stat(filename, &file_stat);
  int file_len = file_stat.st_size;
  uint8_t *src_buf = (uint8_t *) malloc(file_len + 1);
  if (fread(src_buf, 1, file_len, fp) != file_len) {
    free(src_buf);
    perror("Error reading file: ");
    return;
  }
  fclose(fp);
  uint8_t *c_buf = (uint8_t *) malloc(file_len * 1.2);

  clock_t t = clock();

  seq_matcher sm(22);

  sm_longest sl;
  int savings = 0;
  int src_pos = 0;
  int c_len = 0;
  for (; src_pos < file_len - 2; ) {
    sm.add_sequence(src_buf + src_pos, 3, src_pos);
    sl = sm.find_longest(src_buf, src_pos, file_len - src_pos, src_buf, src_pos);
    if (sl.pos == -1 || sl.sav < 6) {
      uint8_t b = src_buf[src_pos++];
      c_buf[c_len++] = b;
      if (b == '\0') {
        c_buf[c_len++] = '\0';
        savings--;
      }
    } else {
      //printf("%d, %d, [%.*s], %d\n", src_pos, c_len - sl.pos, sl.len, c_buf + c_len - sl.pos, sl.len);
      src_pos += sl.len;
      savings += sl.len;
      c_buf[c_len++] = '\0';
      savings--;
      int8_t vlen = gen::get_vlen_of_uint32(sl.pos);
      gen::copy_vint32(sl.pos, c_buf + c_len, vlen);
      c_len += vlen;
      savings -= vlen;
      vlen = gen::get_vlen_of_uint32(sl.len);
      gen::copy_vint32(sl.len, c_buf + c_len, vlen);
      c_len += vlen;
      savings -= vlen;
    }
  }
  uint8_t b = c_buf[c_len++] = src_buf[src_pos++];
  if (b == '\0') c_buf[c_len++] = '\0';
  b = c_buf[c_len++] = src_buf[src_pos++];
  if (b == '\0') c_buf[c_len++] = '\0';
  c_buf[c_len++] = '\0';
  c_buf[c_len++] = '\0';

  int w_file_len = strlen(filename);
  char w_filename[w_file_len + 5];
  strncpy(w_filename, filename, w_file_len);
  w_filename[w_file_len] = '\0';
  w_file_len += 4;
  strncat(w_filename, ".msz", w_file_len);
  fp = fopen(w_filename, "w+");
  fwrite(c_buf, c_len, 1, fp);
  fclose(fp);

  print_time_taken(t, "Time taken for matches: ");
  printf("Savings: %d, c_len: %d\n", savings, c_len);

  free(src_buf);
  free(c_buf);

}

void test_decode_file1(const char *filename) {

  FILE *fp = fopen(filename, "rb");
  if (fp == NULL) {
    perror("Unable to open file: ");
    return;
  }
  struct stat file_stat;
  memset(&file_stat, '\0', sizeof(file_stat));
  stat(filename, &file_stat);
  int file_len = file_stat.st_size;
  uint8_t *src_buf = (uint8_t *) malloc(file_len + 1);
  if (fread(src_buf, 1, file_len, fp) != file_len) {
    free(src_buf);
    perror("Error reading file: ");
    return;
  }
  fclose(fp);
  uint8_t *d_buf = (uint8_t *) malloc(file_len * 5);

  int w_file_len = strlen(filename);
  printf("file: %d, [%.*s]\n", w_file_len, w_file_len, filename);
  char w_filename[w_file_len + 5];
  strncpy(w_filename, filename, w_file_len);
  w_filename[w_file_len] = '\0';
  w_file_len += 4;
  strncat(w_filename, ".msd", w_file_len);
  printf("filew: %d, [%.*s]\n", w_file_len, w_file_len, w_filename);
  fp = fopen(w_filename, "w+");

  int d_len = 0;
  uint8_t *t = src_buf;
  uint8_t *t_end = src_buf + file_len;
  while (t < t_end) {
    uint8_t b = *t++;
    while (b != '\0') {
      d_buf[d_len++] = b;
      b = *t++;
    }
    if (t == t_end)
      break;
    if (*t == 0) {
      d_buf[d_len++] = '\0';
      t++;
    } else {
      int8_t vlen;
      int rel_pos = gen::read_vint32(t, &vlen);
      t += vlen;
      int len = gen::read_vint32(t, &vlen);
      memcpy(d_buf + d_len, d_buf + d_len - rel_pos, len);
      d_len += len;
      t += vlen;
    }
  }

  fwrite(d_buf, d_len, 1, fp);
  fclose(fp);

}

void test_file2(const char *filename) {

  FILE *fp = fopen(filename, "rb");
  if (fp == NULL) {
    perror("Could not open file: ");
    return;
  }
  struct stat file_stat;
  memset(&file_stat, '\0', sizeof(file_stat));
  stat(filename, &file_stat);
  int file_len = file_stat.st_size;
  uint8_t *src_buf = (uint8_t *) malloc(file_len + 1);
  if (fread(src_buf, 1, file_len, fp) != file_len) {
    free(src_buf);
    perror("Error reading file: ");
    return;
  }
  fclose(fp);
  uint8_t *c_buf = (uint8_t *) malloc(file_len * 1.2);

  clock_t t = clock();

  seq_matcher sm(22);

  sm_longest sl;
  int savings = 0;
  int src_pos = 0;
  int c_len = 0;
  for (; src_pos < file_len - 2; ) {
    sm.add_sequence(src_buf + src_pos, 3, src_pos);
    sl = sm.find_longest(src_buf, src_pos, file_len - src_pos, src_buf, src_pos);
    if (sl.pos == -1 || sl.sav < 6) {
      uint8_t b = src_buf[src_pos++];
      c_buf[c_len++] = b;
      if (b == '\0') {
        c_buf[c_len++] = '\0';
        savings--;
      }
    } else {
      //printf("%d, %d, [%.*s], %d\n", src_pos, c_len - sl.pos, sl.len, c_buf + c_len - sl.pos, sl.len);
      for (int j = 1; j < sl.len; j++)
        sm.add_sequence(src_buf + src_pos + j, 3, src_pos + j);
      src_pos += sl.len;
      savings += sl.len;
      c_buf[c_len++] = '\0';
      savings--;
      int8_t vlen = gen::get_vlen_of_uint32(sl.pos);
      gen::copy_vint32(sl.pos, c_buf + c_len, vlen);
      c_len += vlen;
      savings -= vlen;
      vlen = gen::get_vlen_of_uint32(sl.len);
      gen::copy_vint32(sl.len, c_buf + c_len, vlen);
      c_len += vlen;
      savings -= vlen;
    }
  }
  uint8_t b = c_buf[c_len++] = src_buf[src_pos++];
  if (b == '\0') c_buf[c_len++] = '\0';
  b = c_buf[c_len++] = src_buf[src_pos++];
  if (b == '\0') c_buf[c_len++] = '\0';
  c_buf[c_len++] = '\0';

  int w_file_len = strlen(filename);
  char w_filename[w_file_len + 5];
  strncpy(w_filename, filename, w_file_len);
  w_filename[w_file_len] = '\0';
  w_file_len += 4;
  strncat(w_filename, ".msz", w_file_len);
  fp = fopen(w_filename, "w+");
  fwrite(c_buf, c_len, 1, fp);
  fclose(fp);

  print_time_taken(t, "Time taken for matches: ");
  printf("Savings: %d, c_len: %d\n", savings, c_len);

  free(src_buf);
  free(c_buf);

}

void test_decode_file2(const char *filename) {

  FILE *fp = fopen(filename, "rb");
  if (fp == NULL) {
    perror("Unable to open file: ");
    return;
  }
  struct stat file_stat;
  memset(&file_stat, '\0', sizeof(file_stat));
  stat(filename, &file_stat);
  int file_len = file_stat.st_size;
  uint8_t *src_buf = (uint8_t *) malloc(file_len + 1);
  if (fread(src_buf, 1, file_len, fp) != file_len) {
    free(src_buf);
    perror("Error reading file: ");
    return;
  }
  fclose(fp);
  uint8_t *d_buf = (uint8_t *) malloc(file_len * 5);

  int w_file_len = strlen(filename);
  printf("file: %d, [%.*s]\n", w_file_len, w_file_len, filename);
  char w_filename[w_file_len + 5];
  strncpy(w_filename, filename, w_file_len);
  w_filename[w_file_len] = '\0';
  w_file_len += 4;
  strncat(w_filename, ".msd", w_file_len);
  printf("filew: %d, [%.*s]\n", w_file_len, w_file_len, w_filename);
  fp = fopen(w_filename, "w+");

  int d_len = 0;
  uint8_t *t = src_buf;
  uint8_t *t_end = src_buf + file_len;
  while (t < t_end) {
    uint8_t b = *t++;
    while (b != '\0') {
      d_buf[d_len++] = b;
      b = *t++;
    }
    if (t == t_end)
      break;
    if (*t == 0) {
      d_buf[d_len++] = '\0';
      t++;
    } else {
      int8_t vlen;
      int rel_pos = gen::read_vint32(t, &vlen);
      t += vlen;
      int len = gen::read_vint32(t, &vlen);
      memcpy(d_buf + d_len, d_buf + d_len - rel_pos, len);
      d_len += len;
      t += vlen;
    }
  }

  fwrite(d_buf, d_len, 1, fp);
  fclose(fp);

}

int main(int argc, const char *argv[]) {
  gen::is_gen_print_enabled = true;
  if (argc > 1) {
    if (argc > 2 && strncmp(argv[1], "-d", 2) == 0)
      test_decode_file1(argv[2]);
    else
      test_file1(argv[1]);
  } else
    test_string();
  return 1;
}

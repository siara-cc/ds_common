#include <stdio.h>
#include <stdint.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <iostream>

#include "../src/gen.hpp"
#include "../src/freq_grp.hpp"
#include "../src/match_words.hpp"

#include "../../madras-trie/src/madras_builder_dv1.hpp"

using namespace gen;

static int append_vint32_test(byte_vec& vec, uint32_t vint, int len, bool is_first) {
  for (int i = len - 1; i > 0; i--)
    vec.push_back(0x80 + ((vint >> (7 * i)) & 0x7F));
  vec.push_back((vint & 0x3F) | (is_first ? 0x40 : 0x00));
  return len;
}

int test_string() {
  gen::byte_blocks bb;
  word_matcher wm(bb);
  const char *s = "Hello World, How are you!!  Are you doing well?";
  for (int i = 0; i < strlen(s); i++)
    bb.push_back(s[i]);
  wm.add_word_combis(0, strlen(s), 1);
  std::vector<word_combi> *wcv = wm.get_combis();
  for (int i = 0; i < wcv->size(); i++) {
    word_combi wc = (*wcv)[i];
    printf("[%.*s], %d\n", wc.limit, bb[wc.pos], wc.limit);
  }
  return 1;
}

void test_file1(const char *filename) {

  FILE *fp = fopen(filename, "rb");
  if (fp == NULL) {
    perror("Could not open file");
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

  clock_t t = clock();
  clock_t tt = clock();

  gen::byte_blocks bb;
  word_matcher wm(bb);
  int line_len = 0;
  int line_no = 0;
  uint8_t *line = gen::extract_line(src_buf, line_len, file_len);
  do {
    uint32_t line_pos = bb.push_back(line, line_len);
    wm.add_words(line, line_len, line_no++);
    //printf("%.*s\n", line_len, line);
    line = gen::extract_line(line, line_len, file_len - (line - src_buf) - line_len);
    if ((line_no % 10000) == 0) {
      // std::cout << ".";
      // std::cout.flush();
      // gen::combi_freq_vec word_freq_vec;
      // gen::combis_vec *cv = wm.get_combis();
      // wm.make_uniq_words(word_freq_vec);
      // printf("Word count: %lu, Uniq: %lu\n", cv->size(), word_freq_vec.size());
      // wm.reset();
    }
  } while (line != NULL);
  // gen::combi_freq_vec word_freq_vec;
  // wm.make_uniq_words(word_freq_vec);
  // printf("Uniq word count: %lu\n", word_freq_vec.size());
  printf("Line count: %d\n", line_no);
  t = print_time_taken(t, "Time taken for adding words: ");
  // printf("Ctr: %d\n", wm.ctr);
  // free(src_buf);
  // return;

  std::vector<uint32_t> *word_positions = wm.get_word_positions();
  size_t total_words = word_positions->size();

  gen::freq_grp fg(total_words);

  gen::combi_freq_vec word_freq_vec;
  gen::combi_freq_ptr_vec word_freq_ptr_vec;
  wm.make_uniq_words(word_freq_vec, word_freq_ptr_vec);
  printf("No. of uniq words: %lu\n", word_freq_vec.size());

  free(src_buf);

}

int main(int argc, const char *argv[]) {
  gen::is_gen_print_enabled = true;
  if (argc > 1) {
    test_file1(argv[1]);
  } else
    test_string();
  return 1;
}

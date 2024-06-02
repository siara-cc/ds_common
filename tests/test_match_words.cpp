#include <stdio.h>
#include <stdint.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <iostream>

#include "../src/gen.hpp"
#include "../src/freq_grp.hpp"
#include "../src/match_words.hpp"

using namespace gen;

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

  gen::byte_blocks bb;
  word_matcher wm(bb);
  int line_len = 0;
  uint8_t *line = gen::extract_line(src_buf, line_len, file_len);
  do {
    uint32_t line_pos = bb.push_back(line, line_len);
    wm.add_words(line_pos, line_len, 0);
    //printf("%.*s\n", line_len, line);
    line = gen::extract_line(line, line_len, file_len - (line - src_buf) - line_len);
  } while (line != NULL);
  t = print_time_taken(t, "Time taken for adding words: ");

  gen::combis_vec *words = wm.get_combis();
  uint32_t total_words = words->size();
  printf("Total words: %u\n", total_words);

  gen::freq_grp fg(total_words);

  FILE *fps[5];
  memset(fps, '\0', sizeof(FILE *) * 5);
  gen::combi_freq_vec word_freq_vec;
  wm.make_uniq_words(word_freq_vec);
  for (int i = 0; i < word_freq_vec.size(); i++) {
    combi_freq *cf = &word_freq_vec[i];
    cf->byts = fg.add_freq(cf->freq, cf->len + 1);
    if (fps[cf->byts - 1] == nullptr) {
      char filename[10];
      strcpy(filename, "freq?.txt");
      filename[4] = '0' + cf->byts;
      fps[cf->byts - 1] = fopen(filename, "wb+");
    }
    // if (cf->freq > 1)
    //   fprintf(fp, "%u\t%u\t[%.*s]\n", cf->len, cf->freq, cf->len, (*bb)[cf->pos]);
    fprintf(fps[cf->byts - 1], "%.*s\n", cf->len, bb[cf->pos]);
  }
  for (int i = 0; i < 5; i++) {
    if (fps[i] != nullptr)
      fclose(fps[i]);
  }
  std::vector<uint32_t> *grps = fg.get_freq_grps();
  std::vector<uint32_t> *counts = fg.get_freq_counts();
  std::vector<uint32_t> *sizes = fg.get_freq_sizes();
  uint32_t tot_byts = 0;
  for (int i = 0; i < grps->size(); i++) {
    uint32_t byts = (*grps)[i] * (i + 1);
    tot_byts += byts;
    printf("%d\t%u\t%u\t%u\t%u\n", i + 1, (*grps)[i], (*counts)[i], (*sizes)[i], byts);
  }
  printf("Total ptr size: %u\n", tot_byts);

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

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

  gen::byte_blocks bb;
  word_matcher wm(bb);
  int line_len = 0;
  int line_no = 0;
  uint8_t *line = gen::extract_line(src_buf, line_len, file_len);
  do {
    uint32_t line_pos = bb.push_back(line, line_len);
    wm.add_words(line_pos, line_len, line_no++);
    //printf("%.*s\n", line_len, line);
    line = gen::extract_line(line, line_len, file_len - (line - src_buf) - line_len);
    if ((line_no % 10000) == 0) {
      // std::cout << ".";
      // std::cout.flush();
    }
  } while (line != NULL);
  printf("Line count: %d\n", line_no);
  t = print_time_taken(t, "Time taken for adding words: ");
  // printf("Ctr: %d\n", wm.ctr);
  // free(src_buf);
  // return;

  gen::combis_vec *words = wm.get_combis();
  uint32_t total_words = words->size();
  printf("Total words: %u\n", total_words);

  gen::freq_grp fg(total_words);

  gen::combi_freq_vec word_freq_vec;
  wm.make_uniq_words(word_freq_vec);
  gen::combi_freq_ptr_vec word_freq_ptr_vec;
  for (int i = 0; i < word_freq_vec.size(); i++) {
    combi_freq *cf = &word_freq_vec[i];
    word_freq_ptr_vec.push_back(cf);
  }
  std::sort(word_freq_ptr_vec.begin(), word_freq_ptr_vec.end(), [](const combi_freq *lhs, const combi_freq *rhs) -> bool {
    return lhs->freq > rhs->freq;
  });
  fp = fopen("wm_freq.txt", "wb+");
  FILE *fps = fopen("wm_freq_texts.txt", "wb+");
  uint32_t ptr = 0;
  uint8_t prev_byts = 0;
  for (int i = 0; i < word_freq_ptr_vec.size(); i++) {
    combi_freq *cf = word_freq_ptr_vec[i];
    cf->byts = fg.add_freq(cf->freq, cf->len + 1);
    if (prev_byts != cf->byts) {
      prev_byts = cf->byts;
      ptr = 0;
    }
    cf->ptr = ptr++;
    // if (cf->freq > 1)
    //   fprintf(fp, "%u\t%u\t[%.*s]\n", cf->len, cf->freq, cf->len, (*bb)[cf->pos]);
    fprintf(fps, "%c%.*s\n", '0' + cf->byts, cf->len, bb[cf->pos]);
    fprintf(fp, "%u\t%u\t%.*s\n", cf->freq, cf->len, cf->len, bb[cf->pos]);
  }
  fclose(fp);
  fclose(fps);
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

  fp = fopen("wm_ptrs.bin", "wb+");
  madras_dv1::builder bldr("ptr_trie.mdx", "kv_table,Key", 1, "*", "u");
  std::vector<uint8_t> ptr_vec;
  int line_word_count = 0;
  int total_ptr_size = 0;
  uint32_t prev_line_no = (*words)[0].ref_id;
  for (int i = 0; i < total_words; i++) {
    word_combi *wc = &(*words)[i];
    if (prev_line_no != wc->ref_id) {
      bool is_insert = bldr.insert(ptr_vec.data(), ptr_vec.size());
      // printf("\n%lu\t%d\t%d\n\n", ptr_vec.size(), line_word_count, is_insert);
      fwrite(ptr_vec.data(), ptr_vec.size(), 1, fp);
      ptr_vec.clear();
      prev_line_no = wc->ref_id;
      line_word_count = 0;
    }
    line_word_count++;
    combi_freq *cf = &word_freq_vec[wc->freq_id];
    // printf("{%u, %u} ", wc->freq_id, cf->ptr);
    total_ptr_size += append_vint32_test(ptr_vec, cf->ptr, cf->byts, line_word_count == 1);
  }
      fwrite(ptr_vec.data(), ptr_vec.size(), 1, fp);
      fclose(fp);
      bool is_insert = bldr.insert(ptr_vec.data(), ptr_vec.size());
  bldr.build();
  bldr.write_trie();
  printf("Total_ptr_size: %d\n", total_ptr_size);

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

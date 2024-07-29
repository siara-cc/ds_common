#ifndef _MATCH_WORDS_HPP_
#define _MATCH_WORDS_HPP_

#include <vector>
#include <string>
#include <cstring>
#include <unordered_map>
#include <stdio.h>

#include "byte_blocks.hpp"

namespace gen {

// const size_t freq_grp_caps[] = {(1ul << 6), (1ul << 6) + (1ul << 13),
//                (1ul << 6) + (1ul << 13) + (1ul << 20), (1ul << 6) + (1ul << 13) + (1ul << 20) + (1ul << 27),
//                (1ul << 6) + (1ul << 13) + (1ul << 20) + (1ul << 27) + (1ul << 34)};

const size_t freq_grp_caps[] = {(1ul << 7), (1ul << 7) + (1ul << 14),
               (1ul << 7) + (1ul << 14) + (1ul << 21), (1ul << 7) + (1ul << 14) + (1ul << 21) + (1ul << 28),
               (1ul << 7) + (1ul << 14) + (1ul << 21) + (1ul << 28) + (1ul << 35)};


typedef struct {
  uint32_t pos;
  uint32_t len;
  uint32_t freq;
  uint8_t grp;
  uint32_t ptr;
} combi_freq;
typedef std::vector<combi_freq> combi_freq_vec;
typedef std::vector<combi_freq *> combi_freq_ptr_vec;

typedef std::unordered_map<uint32_t, std::vector<uint8_t> > wm_hash;

class word_matcher {
  private:
    byte_blocks *bb;
    byte_blocks words;
    std::vector<uint32_t> word_positions;
    std::vector<uint32_t> words_sorted;
  public:
    int grp_count;
    word_matcher(byte_blocks& _bb) : bb (&_bb) {
      grp_count = 0;
    }
    void reset() {
      grp_count = 0;
      bb->reset();
      word_positions.clear();
      words_sorted.clear();
      words.reset();
    }
    void add_word(const uint8_t *word, uint32_t len, uint32_t ref_id) {
      int buf_size = len + 1 + 4 + 10;
      uint8_t buf[buf_size];
      memset(buf, '\0', buf_size);
      int buf_len = 5;
      int8_t vlen = gen::copy_fvint32(buf + buf_len, len);
      buf_len += vlen;
      memcpy(buf + buf_len, word, len);
      buf_len += len;
      //vlen = copy_fvint32(buf + buf_len, ref_id);
      size_t word_pos = words.push_back(buf, buf_len);
      word_positions.push_back(word_pos);
    }
    void set_last_word() {
      uint32_t word_pos = word_positions[word_positions.size() - 1];
      uint8_t *flag = words[word_pos + 4];
      *flag = '\1';
    }
    uint32_t add_words(const uint8_t *words, int len, uint32_t ref_id) {
      int last_word_len = 0;
      bool is_prev_non_word = false;
      uint32_t ret = word_positions.size();
      for (int i = 0; i < len; i++) {
        uint8_t c = words[i];
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c > 127) {
          if (last_word_len > 4 && is_prev_non_word) {
            if (len - i < 5) {
              last_word_len += (len - i);
              break;
            }
            add_word(words + i - last_word_len, last_word_len, ref_id);
            last_word_len = 0;
          }
          is_prev_non_word = false;
        } else {
          is_prev_non_word = true;
        }
        last_word_len++;
      }
      if (last_word_len > 0)
        add_word(words + len - last_word_len, last_word_len, ref_id);
      set_last_word();
      return ret;
    }
    byte_blocks *get_words() {
      return &words;
    }
    std::vector<uint32_t> *get_word_positions() {
      return &word_positions;
    }
    void make_uniq_words(combi_freq_vec& word_freq_vec, combi_freq_ptr_vec& word_freq_ptr_vec) {
      clock_t t = clock();
      size_t word_combis_sz = word_positions.size();
      printf("No. of words: %lu\n", word_combis_sz);
      words_sorted = word_positions;
      std::sort(words_sorted.begin(), words_sorted.end(), [this](const uint32_t& l, const uint32_t& r) -> bool {
        int8_t vlen;
        uint8_t *lhs = words[l] + 5;
        uint32_t lhs_len = gen::read_fvint32(lhs, vlen);
        lhs += vlen;
        uint8_t *rhs = words[r] + 5;
        uint32_t rhs_len = gen::read_fvint32(rhs, vlen);
        rhs += vlen;
        return gen::compare(lhs, lhs_len, rhs, rhs_len) < 0;
      });
      gen::print_time_taken(t, "Time taken to sort words: ");
      uint32_t freq_count = 0;
      if (word_combis_sz > 0) {
        uint32_t pwpos;
        uint32_t wpos = 0;
        uint8_t *w = nullptr;
        uint32_t w_len = 0;
        int8_t vlen = 0;
        for (size_t i = 1; i < word_combis_sz; i++) {
          wpos = words_sorted[i];
          pwpos = words_sorted[i - 1];
          w = words[wpos] + 5;
          w_len = gen::read_fvint32(w, vlen);
          w += vlen;
          uint8_t *pw = words[pwpos];
          gen::copy_uint32(word_freq_vec.size(), pw);
          pw += 5;
          int8_t pvlen;
          uint32_t pw_len = gen::read_fvint32(pw, pvlen);
          pw += pvlen;
          int cmp = gen::compare(w, w_len, pw, pw_len);
          if (cmp != 0) {
            word_freq_vec.push_back((combi_freq) {pwpos + 5 + pvlen, pw_len, freq_count + 1});
            freq_count = 0;
          } else {
            freq_count++;
          }
        }
        if (w != nullptr)
          gen::copy_uint32(word_freq_vec.size(), w - 5 - vlen);
        word_freq_vec.push_back((combi_freq) {wpos + 5 + vlen, w_len, freq_count + 1});
        for (size_t i = 0; i < word_freq_vec.size(); i++) {
          combi_freq *cf = &word_freq_vec[i];
          word_freq_ptr_vec.push_back(cf);
        }
        std::sort(word_freq_ptr_vec.begin(), word_freq_ptr_vec.end(), [](const combi_freq *lhs, const combi_freq *rhs) -> bool {
          return lhs->freq > rhs->freq;
        });
        int cur_grp = 0;
        for (size_t i = 0; i < word_freq_ptr_vec.size(); i++) {
          combi_freq *cf = word_freq_ptr_vec[i];
          if (i == freq_grp_caps[cur_grp])
            cur_grp++;
          // printf("%d, %u, [%.*s]\n", cur_grp, cf->len, cf->len, words[cf->pos]);
          cf->grp = cur_grp;
        }
        grp_count = cur_grp + 1;
      }
      t = gen::print_time_taken(t, "Time taken to make uniq: ");
    }
};

}

#endif

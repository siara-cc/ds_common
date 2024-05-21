#ifndef _MATCH_WORDS_HPP_
#define _MATCH_WORDS_HPP_

#include <vector>
#include <string>
#include <cstring>
#include <stdio.h>

#include "gen.hpp"
#include "../../leopard-trie/src/leopard.hpp"

namespace gen {

typedef struct {
  // const uint8_t *ptr;
  // int len;
  uint32_t pos;
  uint32_t len;
  uint32_t ref_id;
} word_combi;

typedef struct {
  uint32_t pos;
  uint32_t len;
  uint32_t freq;
} combi_freq;

class word_matcher {
  private:
    byte_vec& bv;
    std::vector<word_combi> word_combis;
    leopard::trie lt;
  public:
    word_matcher(byte_vec& _bv) : bv (_bv), lt(1, "*", "u", false, false) {
    }
    // void add_word_combi(const uint8_t *combi, int len, uint32_t ref_id) {
    //   word_combis.push_back((word_combi) {combi, len, ref_id}); // add entire string
    //   //lt.insert(combi, len);
    // }
    uint32_t make_hash(uint8_t *data, int len) {
      uint32_t hash = 0;
      for (int i = 0; i < len; i++) {
        hash += data[i];
      }
      return hash;
    }
    void add_word_combi(uint32_t combi_pos, uint32_t len, uint32_t ref_id) {
      word_combis.push_back((word_combi) {combi_pos, len, ref_id}); // add entire string
      //lt.insert(combi, len);
      // uint8_t pair[30];
      // uint32_t hash = make_hash(bv.data() + combi_pos, len);
      // int8_t vlen = gen::get_svint61_len(hash);
      // gen::copy_svint61(hash, pair, vlen);
      // int8_t vlen1 = gen::get_svint61_len(ref_id);
      // gen::copy_svint61(ref_id, pair + vlen, vlen1);
      // lt.insert(pair, vlen + vlen1);
    }
    void add_words(int pos, int len, uint32_t ref_id) {
      int last_word_len = 0;
      for (int i = 0; i < len; i++) {
        uint8_t c = *(bv.data() + pos + i);
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '\'' || c == '"') {
          last_word_len++;
          continue;
        } else {
          if (last_word_len > 3) {
            add_word_combi(pos + i - last_word_len, last_word_len, ref_id);
          }
          last_word_len = 0;
        }
      }
      if (last_word_len > 3) {
        add_word_combi(pos + len - last_word_len, last_word_len, ref_id);
      }
    }
    void add_all_combis(int pos, int len, uint32_t ref_id) {
      std::vector<uint8_t *> word_starts;
      std::vector<int> word_lens;
      int last_word_len = 0;
      for (int i = 0; i < len; i++) {
        uint8_t c = *(bv.data() + pos + i);
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) {
          last_word_len++;
          continue;
        } else {
          if (last_word_len > 0) {
            word_starts.push_back(bv.data() + pos + i - last_word_len);
            word_lens.push_back(last_word_len);
            last_word_len = 0;
          }
        }
      }
      if (last_word_len > 0) {
        word_starts.push_back(bv.data() + pos + len - last_word_len);
        word_lens.push_back(last_word_len);
      }
      add_word_combi(pos, len, ref_id); // add entire string
      if (word_lens.size() > 0 && word_lens[0] != len) {
        for (int i = 0; i < word_starts.size(); i++) { // add individual words
          if (word_lens[i] > 1)
            add_word_combi(word_starts[i] - bv.data(), word_lens[i], ref_id);
        }
      }
      if (word_lens.size() > 0) {
        int combi_len = 0; // add word combinations
        for (int i = 0; i < word_starts.size() - 1; i++) {
          for (int j = i + 1; j < word_starts.size(); j++) {
            combi_len = word_starts[j] - word_starts[i] + word_lens[j];
            add_word_combi(word_starts[i] - bv.data(), combi_len, ref_id);
          }
        }
      }
    }
    std::vector<word_combi> *get_combis() {
      return &word_combis;
    }
    void process_combis() {
      clock_t t = clock();
      std::sort(word_combis.begin(), word_combis.end(), [this](const word_combi& lhs, const word_combi& rhs) -> bool {
        return gen::compare(bv.data() + lhs.pos, lhs.len, bv.data() + rhs.pos, rhs.len) < 0;
      });
      t = gen::print_time_taken(t, "Time taken to sort words: ");
      std::vector<word_combi> word_combis1 = word_combis;
      std::sort(word_combis.begin(), word_combis.end(), [this](const word_combi& lhs, const word_combi& rhs) -> bool {
        return lhs.ref_id < rhs.ref_id;
      });
      t = gen::print_time_taken(t, "Time taken to assign and sort idx: ");
      uint32_t freq_count = 0;
      size_t combi_count = word_combis.size();
      std::vector<combi_freq> combi_freqs;
      if (combi_count > 0) {
        word_combi *wc = &word_combis[0];
        for (int i = 1; i < combi_count; i++) {
          wc = &word_combis[i];
          word_combi *pwc = &word_combis[i - 1];
          int cmp = gen::compare(bv.data() + pwc->pos, pwc->len, bv.data() + wc->pos, wc->len);
          if (cmp != 0) {
            combi_freqs.push_back((combi_freq) {pwc->pos, pwc->len, freq_count + 1});
            freq_count = 0;
          } else {
            freq_count++;
          }
        }
        combi_freqs.push_back((combi_freq) {wc->pos, wc->len, freq_count + 1});
        std::sort(combi_freqs.begin(), combi_freqs.end(), [this](const combi_freq& lhs, const combi_freq& rhs) -> bool {
          return lhs.freq > rhs.freq;
        });
      }
      FILE *fp = fopen("combis.txt", "w+");
      for (int i = 0; i < combi_freqs.size(); i++) {
        combi_freq *cf = &combi_freqs[i];
        fprintf(fp, "[%.*s]\t%u\t%u\n", cf->len, bv.data() + cf->pos, cf->freq, cf->len);
      }
      gen::print_time_taken(t, "Time taken to process words: ");
      fclose(fp);
    }
};

}

#endif

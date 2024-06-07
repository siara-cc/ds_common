#ifndef _MATCH_WORDS_HPP_
#define _MATCH_WORDS_HPP_

#include <vector>
#include <string>
#include <cstring>
#include <unordered_map>
#include <stdio.h>

#include "byte_blocks.hpp"

namespace gen {

typedef struct {
  uint32_t pos;
  uint32_t limit;
  union {
    uint32_t cmp;
    uint32_t freq_id;
  };
  uint32_t ref_id;
} word_combi;
typedef std::vector<word_combi> combis_vec;

typedef struct {
  uint32_t ref_id;
  uint32_t pos;
  uint32_t len;
} dict_match;
typedef std::vector<dict_match> dict_match_vec;

typedef struct {
  uint32_t pos;
  uint32_t len;
  uint32_t freq;
  uint8_t byts;
  uint32_t ptr;
} combi_freq;
typedef std::vector<combi_freq> combi_freq_vec;
typedef std::vector<combi_freq *> combi_freq_ptr_vec;

typedef std::unordered_map<uint32_t, std::vector<uint8_t> > wm_hash;

class word_matcher {
  private:
    byte_blocks *bb;
  public:
    std::vector<word_combi> word_combis;
    std::vector<uint32_t> ref_id_vec;
    word_matcher(byte_blocks& _bb) : bb (&_bb) {
    }
    void reset() {
      bb->reset();
      word_combis.clear();
    }
    void add_word_combi(uint32_t combi_pos, uint32_t len, uint32_t ref_id) {
      word_combis.push_back((word_combi) {combi_pos, len, 0, ref_id});
      //printf("[%.*s], %d\n", len, bv->data() + combi_pos, len);
    }
    void add_word_combis(uint32_t pos, uint32_t len, uint32_t ref_id) {
      if (len < 4)
        return;
      //printf("Val added: [%.*s]\n", len, bb == NULL ? bv->data() + pos : (*bb)[pos]);
      int last_word_len = 0;
      for (uint32_t i = 0; i < len; i++) {
        uint8_t c = *((*bb)[pos] + i);
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')
             || c == '\'' || c == '+' || c == '-' || c == '"' || c > 127) {
          last_word_len++;
          continue;
        } else {
          if (last_word_len > 0)
            add_word_combi(pos + i - last_word_len, len - i + last_word_len, ref_id);
          last_word_len = 0;
        }
      }
      if (last_word_len > 0)
        add_word_combi(pos + len - last_word_len, last_word_len, ref_id);
    }
    void add_words(uint32_t pos, uint32_t len, uint32_t ref_id) {
      //printf("Val added: [%.*s]\n", len, bb == NULL ? bv->data() + pos : (*bb)[pos]);
      int last_word_len = 0;
      bool is_prev_non_word = false;
      for (uint32_t i = 0; i < len; i++) {
        uint8_t c = *((*bb)[pos] + i);
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')
             || c == '\''  || c == '+' || c == '-' || c == '"' || c > 127) {
          if (last_word_len > 4 && is_prev_non_word) {
            if (len - i < 5) {
              last_word_len += (len - i);
              break;
            }
            add_word_combi(pos + i - last_word_len, last_word_len, ref_id);
            last_word_len = 0;
          }
          is_prev_non_word = false;
        } else {
          is_prev_non_word = true;
        }
        last_word_len++;
      }
      if (last_word_len > 0)
        add_word_combi(pos + len - last_word_len, last_word_len, ref_id);
    }
    std::vector<word_combi> *get_combis() {
      return &word_combis;
    }
    std::vector<uint32_t> *get_ref_id_vec() {
      return &ref_id_vec;
    }
    void process_combis() {
      clock_t t = clock();
      size_t word_combis_sz = word_combis.size();
      std::sort(word_combis.begin(), word_combis.end(), [this](const word_combi& lhs, const word_combi& rhs) -> bool {
        return gen::compare((*bb)[lhs.pos], lhs.limit, (*bb)[rhs.pos], rhs.limit) < 0;
      });
      t = gen::print_time_taken(t, "Time taken to sort words: ");
      for (int i = 1; i < word_combis_sz; i++) {
        word_combi *wc = &word_combis[i];
        word_combi *pwc = &word_combis[i - 1];
        int cmp = gen::compare((*bb)[wc->pos], wc->limit, (*bb)[pwc->pos], pwc->limit);
        cmp = (cmp == 0 ? pwc->limit : cmp - 1);
        cmp = (cmp > 4 ? cmp : 0);
        if (pwc->cmp < cmp)
          pwc->cmp = cmp;
        wc->cmp = cmp;
      }
      // for (int i = 0; i < word_combis_sz; i++) {
      //   word_combi *wc = &word_combis[i];
      //   printf("%u, [%.*s]\n", wc->cmp, wc->limit, bv->data() + wc->pos);
      // }
      for (int i = 0; i < word_combis_sz; i++) {
        ref_id_vec.push_back(i);
      }
      std::sort(ref_id_vec.begin(), ref_id_vec.end(), [this](const uint32_t& lhs, const uint32_t& rhs) -> bool {
        return (word_combis[lhs].ref_id == word_combis[rhs].ref_id ?
                (word_combis[lhs].cmp > word_combis[rhs].cmp) :
                (word_combis[lhs].ref_id < word_combis[rhs].ref_id));
      });
      t = gen::print_time_taken(t, "Time taken to assign and sort ref ids: ");
    }
    void make_uniq_words(combi_freq_vec& word_freq_vec) {
      clock_t t = clock();
      size_t word_combis_sz = word_combis.size();
      std::vector<word_combi *> word_combi_ptrs;
      for (int i = 0; i < word_combis_sz; i++) {
        word_combi *wc = &word_combis[i];
        word_combi_ptrs.push_back(wc);
      }
      std::sort(word_combi_ptrs.begin(), word_combi_ptrs.end(), [this](const word_combi *lhs, const word_combi *rhs) -> bool {
        return gen::compare((*bb)[lhs->pos], lhs->limit, (*bb)[rhs->pos], rhs->limit) < 0;
      });
      // t = gen::print_time_taken(t, "Time taken to sort words: ");
      uint32_t freq_count = 0;
      if (word_combis_sz > 0) {
        word_combi *pwc;
        word_combi *wc;
        for (int i = 1; i < word_combis_sz; i++) {
          wc = word_combi_ptrs[i];
          pwc = word_combi_ptrs[i - 1];
          pwc->freq_id = word_freq_vec.size();
          int cmp = gen::compare((*bb)[wc->pos], wc->limit, (*bb)[pwc->pos], pwc->limit);
          if (cmp != 0) {
            word_freq_vec.push_back((combi_freq) {pwc->pos, pwc->limit, freq_count + 1});
            freq_count = 0;
          } else {
            freq_count++;
          }
        }
        wc->freq_id = word_freq_vec.size();
        word_freq_vec.push_back((combi_freq) {wc->pos, wc->limit, freq_count + 1});
        // t = gen::print_time_taken(t, "Time taken to make uniq: ");
      }
    }
    void make_uniq_combis(dict_match_vec& dmv) {
      clock_t t = clock();
      std::sort(dmv.begin(), dmv.end(), [this](const dict_match& lhs, const dict_match& rhs) -> bool {
        return gen::compare((*bb)[lhs.pos], lhs.len, (*bb)[rhs.pos], rhs.len) < 0;
      });
      uint32_t freq_count = 0;
      size_t combi_count = dmv.size();
      std::vector<combi_freq> combi_freqs;
      if (combi_count > 0) {
        FILE *fp2 = fopen("combis_freq.txt", "w+");
        dict_match *dm = &dmv[0];
        for (int i = 1; i < combi_count; i++) {
          dm = &dmv[i];
          dict_match *pdm = &dmv[i - 1];
          fprintf(fp2, "[%.*s]\t%u\n", dm->len, (*bb)[dm->pos], dm->len);
          int cmp = gen::compare((*bb)[pdm->pos], pdm->len, (*bb)[dm->pos], dm->len);
          if (cmp != 0) {
            combi_freqs.push_back((combi_freq) {pdm->pos, pdm->len, freq_count + 1});
            freq_count = 0;
          } else {
            freq_count++;
          }
        }
        fclose(fp2);
        combi_freqs.push_back((combi_freq) {dm->pos, dm->len, freq_count + 1});
        std::sort(combi_freqs.begin(), combi_freqs.end(), [this](const combi_freq& lhs, const combi_freq& rhs) -> bool {
          //return lhs.freq * lhs.len > rhs.freq * rhs.len;
          return lhs.freq > rhs.freq;
        });
      }
      FILE *fp = fopen("combis1.txt", "w+");
      FILE *fp1 = fopen("combis.txt", "w+");
      uint32_t savings = 0;
      int l = 0;
      for (int i = 0; i < combi_freqs.size(); i++) {
        combi_freq *cf = &combi_freqs[i];
        if ((l < 1024 && cf->len > 3) || (l < (1024 + 131072) && cf->len > 4) || (l < (1024 + 131072 + 16777216) && cf->len > 5)) {
          l++;
          uint32_t saving = (cf->len - (l < 1024 ? 2 : (l < (1024 + 131072) ? 3 : 4))) * cf->freq;
          savings += saving;
          fprintf(fp, "%.*s\n", cf->len, (*bb)[cf->pos]);
          fprintf(fp1, "[%.*s]\t%u\t%u\t%u\n", cf->len, (*bb)[cf->pos], cf->freq, cf->len, saving);
        }
        // if (cf->freq < 2) {
        //   printf("Freq < 2: [%.*s]\t%u\t%u\n", cf->len, bv->data() + cf->pos, cf->freq, cf->len);
        // }
        if (l == 1024) {
          fclose(fp);
          fp = fopen("combis2.txt", "w+");
        }
        if (l == 131072 + 1024) {
          fclose(fp);
          fp = fopen("combis3.txt", "w+");
        }
      }
      fclose(fp);
      fclose(fp1);
      printf("Savings make_uniq_combis(): %d, count: %lu\n", savings, combi_freqs.size());
      gen::print_time_taken(t, "Time taken to make_uniq_combis(): ");
    }
};

}

#endif

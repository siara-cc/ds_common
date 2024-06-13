#ifndef _MATCH_WORDS_HPP_
#define _MATCH_WORDS_HPP_

#include <vector>
#include <string>
#include <cstring>
#include <unordered_map>
#include <stdio.h>

#include "byte_blocks.hpp"

namespace gen {

const size_t freq_grp_caps[] = {128, 128 + 16384, 128 + 16384 + 2097152, 128 + 16384 + 2097152 + 268435456,
                                128 + 16384 + 2097152 + 268435456 + 34359738368};

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
    std::vector<word_combi> word_combis;
    std::vector<uint32_t> ref_id_vec;
    word_matcher(byte_blocks& _bb) : bb (&_bb) {
      grp_count = 0;
    }
    void reset() {
      grp_count = 0;
      bb->reset();
      word_combis.clear();
      word_positions.clear();
      words_sorted.clear();
      words.reset();
    }
    void add_word_combi(const uint8_t *word, uint32_t len, uint32_t ref_id) {
      int buf_size = len + 1 + 4 + 10;
      uint8_t buf[buf_size];
      memset(buf, '\0', buf_size);
      int buf_len = 5;
      int8_t vlen = gen::copy_fvint32(buf + buf_len, len);
      buf_len += vlen;
      memcpy(buf + buf_len, word, len);
      buf_len += len;
      vlen = copy_fvint32(buf + buf_len, ref_id);
      size_t word_pos = words.push_back(buf, buf_len);
      word_positions.push_back(word_pos);
    }
    void set_last_word() {
      uint32_t word_pos = word_positions[word_positions.size() - 1];
      uint8_t *flag = words[word_pos + 4];
      *flag = '\1';
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
    uint32_t add_words(const uint8_t *words, int len, uint32_t ref_id) {
      int last_word_len = 0;
      bool is_prev_non_word = false;
      uint32_t ret = word_positions.size();
      for (uint32_t i = 0; i < len; i++) {
        uint8_t c = words[i];
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c > 127) {
          if (last_word_len > 4 && is_prev_non_word) {
            if (len - i < 5) {
              last_word_len += (len - i);
              break;
            }
            add_word_combi(words + i - last_word_len, last_word_len, ref_id);
            last_word_len = 0;
          }
          is_prev_non_word = false;
        } else {
          is_prev_non_word = true;
        }
        last_word_len++;
      }
      if (last_word_len > 0)
        add_word_combi(words + len - last_word_len, last_word_len, ref_id);
      set_last_word();
      return ret;
    }
    byte_blocks *get_words() {
      return &words;
    }
    std::vector<uint32_t> *get_word_positions() {
      return &word_positions;
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
        uint32_t wpos;
        uint8_t *w = nullptr;
        uint32_t w_len;
        int8_t vlen;
        for (int i = 1; i < word_combis_sz; i++) {
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
        for (int i = 0; i < word_freq_vec.size(); i++) {
          combi_freq *cf = &word_freq_vec[i];
          word_freq_ptr_vec.push_back(cf);
        }
        std::sort(word_freq_ptr_vec.begin(), word_freq_ptr_vec.end(), [](const combi_freq *lhs, const combi_freq *rhs) -> bool {
          return lhs->freq > rhs->freq;
        });
        int cur_grp = 0;
        for (int i = 0; i < word_freq_ptr_vec.size(); i++) {
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

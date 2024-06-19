#ifndef _MATCH_SEQ_HPP_
#define _MATCH_SEQ_HPP_

#include <stdint.h>
#include <vector>

#include "gen.hpp"

namespace gen {

typedef std::vector<uint32_t> pos_list;
typedef std::vector<pos_list> token_list;

typedef struct {
  int pos;
  int len;
  int sav;
} sm_longest;

class seq_matcher {
  private:
    token_list **token_ht;
    int ht_log;
    uint32_t calc_hash(const uint8_t *token, uint32_t& token_u32) {
      token_u32 = (token[0] << 16) + (token[1] << 8) + token[2];
      // token_u32 = token[0];
      // token_u32 = (token_u32 << 5) ^ token[1];
      // token_u32 = (token_u32 << 5) ^ token[2];
      return token_u32 & ((1UL << ht_log) - 1);
    }
    pos_list *add_new_token(token_list *tl, uint32_t hash, uint32_t token_u32, int pos) {
      token_ht[hash] = tl;
      pos_list pl;
      pl.push_back(token_u32);
      pl.push_back(2);
      pl.push_back(pos);
      tl->push_back(pl);
      return &tl->at(tl->size() - 1);
    }
    pos_list *match_token(uint32_t token_u32, token_list *tl) {
      for (int i = 0; i < tl->size(); i++) {
        pos_list *tp = &tl->at(i);
        if (tp->at(0) == token_u32)
          return tp;
      }
      return nullptr;
    }
    #define SMATCH_MAX_POS 16
    void append_cyclic_pos(pos_list *token_positions, int pos) {
      if (token_positions->size() < SMATCH_MAX_POS) {
        token_positions->push_back(pos);
        (*token_positions)[1] = token_positions->size() - 1;
      } else {
        uint32_t last_pos = token_positions->at(1) + 1;
        if (last_pos >= token_positions->size())
          last_pos = 2;
        (*token_positions)[1] = last_pos;
        (*token_positions)[last_pos] = pos;
      }
    }
    pos_list *find_token(const uint8_t *token) {
      uint32_t token_u32;
      uint32_t hash = calc_hash(token, token_u32);
      token_list *tl = token_ht[hash];
      if (tl != nullptr)
        return match_token(token_u32, tl);
      return nullptr;
    }
    void add_token(const uint8_t *token, int pos) {
      uint32_t token_u32;
      uint32_t hash = calc_hash(token, token_u32);
      pos_list *token_positions = nullptr;
      token_list *tl = token_ht[hash];
      if (tl == nullptr) {
        tl = new token_list();
        token_positions = add_new_token(tl, hash, token_u32, pos);
      } else {
        token_positions = match_token(token_u32, tl);
        if (token_positions == nullptr)
          token_positions = add_new_token(tl, hash, token_u32, pos);
        else {
          append_cyclic_pos(token_positions, pos);
        }
      }
    }
  public:
    seq_matcher(int _ht_log) : ht_log (_ht_log) {
      token_ht = new token_list*[1 << ht_log]();
    }
    ~seq_matcher() {
      for (int i = 0; i < ht_log; i++) {
        token_list *tl = token_ht[i];
        if (tl != nullptr)
          delete tl;
      }
      delete [] token_ht;
    }
    void add_sequence(const uint8_t *seq, int len, int pos) {
      for (int i = 0; i < len - 2; i++) {
        add_token(seq + i, pos + i);
      }
    }
    sm_longest find_longest(const uint8_t *buf, int pos, int remaining, const uint8_t *past_buf, int past_buf_cur_len) {
      sm_longest ret = {-1, 0, 0};
      if (remaining > 3) {
        pos_list *pl = find_token(buf + pos);
        if (pl != nullptr) {
          int savings = 0;
          uint32_t last_pos = pl->at(1);
          int i = last_pos;
          do {
            uint32_t token_pos = pl->at(i);
            if (token_pos < pos) {
              int cmp = gen::compare(buf + pos, remaining, past_buf + token_pos, pos - token_pos);
              cmp = (cmp == 0 ? gen::min(remaining, pos - token_pos) : abs(cmp) - 1);
              int rel_pos = past_buf_cur_len - token_pos;
              int saving = cmp - gen::get_vlen_of_uint32(rel_pos) - gen::get_vlen_of_uint32(cmp) - 1;
              if (savings < saving) {
                savings = saving;
                ret.sav = saving;
                ret.pos = rel_pos;
                ret.len = cmp;
              }
              if (ret.sav > 10)
                break;
              // if (past_buf_cur_len - token_pos > 65535)
              //   break;
            }
            i--;
            if (i == 1)
              i = pl->size() - 1;
          } while (i != last_pos);
        }
      }
      return ret;
    }
};

}

#endif

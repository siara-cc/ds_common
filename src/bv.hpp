#ifndef _BV_HPP_
#define _BV_HPP_

#include <vector>
#include <math.h>

namespace gen {

typedef std::vector<uint8_t> byte_vec;
typedef std::vector<uint8_t *> byte_ptr_vec;

static uint32_t get_lkup_tbl_size(uint32_t count, int block_size, int entry_size) {
  double d = count - 1;
  d /= block_size;
  uint32_t ret = floor(d) + 1;
  ret *= entry_size;
  return ret;
}

static uint32_t get_lkup_tbl_size2(uint32_t count, int block_size, int entry_size) {
  return get_lkup_tbl_size(count, block_size, entry_size) + entry_size;
}

class bit_vector {
  private:
    std::vector<uint8_t> bv;
    bool all_ones;
  public:
    bit_vector(bool _all_ones = false) {
      all_ones = _all_ones;
    }
    // bit_no starts from 0
    void set(size_t bit_no, bool val) {
      size_t byte_pos = bit_no / 8;
      if ((bit_no % 8) == 0)
        byte_pos++;
      while (bv.size() < byte_pos + 1)
        bv.push_back(all_ones ? 0xFF : 0x00);
      uint8_t mask = 0x80 >> (bit_no % 8);
      if (val)
        bv[byte_pos] |= mask;
      else
        bv[byte_pos] &= ~mask;
    }
    bool operator[](size_t bit_no) {
      size_t byte_pos = bit_no / 8;
      if ((bit_no % 8) == 0)
        byte_pos++;
      uint8_t mask = 0x80 >> (bit_no % 8);
      return (bv[byte_pos] & mask) > 0;
    }
    void reset() {
      bv.resize(0);
    }
};

class int_bit_vector {
  private:
    byte_vec *int_ptrs;
    int bit_len;
    int count;
    int last_idx;
    uint8_t last_byte_bits;
  public:
    int_bit_vector(byte_vec *_ptrs, int _bit_len, int _count)
        : int_ptrs (_ptrs), bit_len (_bit_len), count (_count) {
      int_ptrs->clear();
      int_ptrs->reserve(bit_len * count / 8 + 1);
      int_ptrs->push_back(0);
      last_byte_bits = 8;
      last_idx = 0;
    }
    void append(uint32_t ptr) {
      int bits_to_append = bit_len;
      while (bits_to_append > 0) {
        if (bits_to_append < last_byte_bits) {
          last_byte_bits -= bits_to_append;
          (*int_ptrs)[last_idx] |= (ptr << last_byte_bits);
          bits_to_append = 0;
        } else {
          bits_to_append -= last_byte_bits;
          (*int_ptrs)[last_idx] |= (ptr >> bits_to_append);
          int_ptrs->push_back(0);
          last_byte_bits = 8;
          last_idx++;
        }
      }
    }
};

class int_bv_reader {
  private:
    uint8_t *int_bv;
    int bit_len;
  public:
    int_bv_reader() {
    }
    void init(uint8_t *_int_bv, int _bit_len) {
      int_bv = _int_bv;
      bit_len = _bit_len;
    }
    uint32_t operator[](int pos) {
      uint32_t bit_pos = pos * bit_len;
      uint32_t byte_pos = bit_pos / 8;
      uint8_t bits_occu = (bit_pos % 8);
      const uint8_t *loc = int_bv + byte_pos;
      uint32_t ret = *loc++ & (0xFF >> bits_occu);
      int bits_left = bit_len + (bits_occu - 8);
      while (bits_left > 0) {
        ret = (ret << 8) | *loc++;
        bits_left -= 8;
      }
      ret >>= (bits_left * -1);
      return ret;
    }
};

}

#endif

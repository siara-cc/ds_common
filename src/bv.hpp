#ifndef _BV_HPP_
#define _BV_HPP_

#include <vector>
#include <math.h>

namespace gen {

typedef std::vector<uint8_t> byte_vec;
typedef std::vector<uint8_t *> byte_ptr_vec;

static uint32_t get_lkup_tbl_size(uint32_t count, int block_size, int entry_size) {
  uint32_t ret = (count / block_size) + ((count % block_size) == 0 ? 0 : 1);
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
      while (bv.size() <= byte_pos)
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
      bv.clear();
    }
};

class int_bit_vector {
  private:
    byte_vec *int_ptrs;
    int bit_len;
    int count;
    int last_idx;
    uint8_t last_byte_bits;
    static void append_uint32(uint32_t u32, byte_vec& v) {
      v.push_back(u32 & 0xFF);
      v.push_back((u32 >> 8) & 0xFF);
      v.push_back((u32 >> 16) & 0xFF);
      v.push_back(u32 >> 24);
    }
    static void append_uint64(uint64_t u64, byte_vec& v) {
      v.push_back(u64 & 0xFF);
      v.push_back((u64 >> 8) & 0xFF);
      v.push_back((u64 >> 16) & 0xFF);
      v.push_back((u64 >> 24) & 0xFF);
      v.push_back((u64 >> 32) & 0xFF);
      v.push_back((u64 >> 40) & 0xFF);
      v.push_back((u64 >> 48) & 0xFF);
      v.push_back(u64 >> 56);
    }
  public:
    int_bit_vector() {
    }
    int_bit_vector(byte_vec *_ptrs, int _bit_len, int _count) {
      init(_ptrs, _bit_len, _count);
    }
    void init(byte_vec *_ptrs, int _bit_len, int _count) {
      int_ptrs = _ptrs;
      bit_len = _bit_len;
      count = _count;
      int_ptrs->clear();
      int_ptrs->reserve(bit_len * count / 8 + 8);
      append_uint64(0, *int_ptrs);
      last_byte_bits = 64;
      last_idx = 0;
    }
    void append(uint32_t given_ptr) {
      uint64_t ptr = given_ptr;
      uint64_t *last_ptr = (uint64_t *) (int_ptrs->data() + int_ptrs->size() - 8);
      int bits_to_append = bit_len;
      while (bits_to_append > 0) {
        if (bits_to_append < last_byte_bits) {
          last_byte_bits -= bits_to_append;
          *last_ptr |= (ptr << last_byte_bits);
          bits_to_append = 0;
        } else {
          bits_to_append -= last_byte_bits;
          *last_ptr |= (ptr >> bits_to_append);
          last_byte_bits = 64;
          append_uint64(0, *int_ptrs);
          last_ptr = (uint64_t *) (int_ptrs->data() + int_ptrs->size() - 8);
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
      uint64_t bit_pos = pos * bit_len;
      uint64_t *ptr_loc = (uint64_t *) int_bv + bit_pos / 64;
      int bits_occu = (bit_pos % 64);
      uint64_t ret = (*ptr_loc++ << bits_occu);
      if (bits_occu + bit_len <= 64)
        return ret >> (64 - bit_len);
      return (ret | (*ptr_loc >> (64 - bits_occu))) >> (64 - bit_len);
    }
};

}

#endif

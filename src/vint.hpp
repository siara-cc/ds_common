#ifndef _VINT_HPP_
#define _VINT_HPP_

namespace gen {

static size_t append_rvint32(byte_vec& vec, uint32_t u32) {
  size_t len = 0;
  while (u32 > 31) {
    vec.push_back(u32 & 0xFF);
    u32 >>= 8;
    len++;
  }
  vec.push_back((u32 << 3) | len);
  return len + 1;
}
static uint32_t read_rvint32(const uint8_t *ptr) {
  uint32_t ret = *ptr >> 3;
  size_t len = *ptr-- & 0x07;
  while (len--) {
    ret <<= 7;
    ret += *ptr--;
  }
  return ret;
}
static size_t append_fvint64(byte_vec& vec, uint64_t u64) {
  size_t len = 0;
  do {
    uint8_t b = u64 & 0x7F;
    u64 >>= 7;
    if (u64 > 0)
      b |= 0x80;
    vec.push_back(b);
    len++;
  } while (u64 > 0);
  return len;
}
static size_t append_fvint64s(byte_vec& vec, int64_t i64) {
  bool is_neg = (i64 < 0);
  if (is_neg)
    i64 = -i64;
  i64 <<= 1;
  if (is_neg)
    i64 |= 1;
  return append_fvint64(vec, i64);
}
static size_t append_fvint32(byte_vec& vec, uint32_t u32) {
  return append_fvint64(vec, (uint64_t) u32);
}
static size_t copy_fvint32(uint8_t *ptr, uint32_t u32) {
  size_t len = 0;
  do {
    uint8_t b = u32 & 0x7F;
    u32 >>= 7;
    if (u32 > 0)
      b |= 0x80;
    *ptr++ = b;
    len++;
  } while (u32 > 0);
  return len;
}
static uint32_t read_fvint32(const uint8_t *ptr, size_t& len) {
  uint32_t ret = *ptr & 0x7F;
  len = 1;
  while (*ptr++ & 0x80) {
    uint32_t bval = *ptr & 0x7F;
    ret += (bval << (7 * len));
    len++;
  }
  return ret;
}
static size_t get_vlen_of_uint32(uint32_t vint) {
  return vint < (1 << 7) ? 1 : (vint < (1 << 14) ? 2 : (vint < (1 << 21) ? 3 :
          (vint < (1 << 28) ? 4 : 5)));
}
static size_t append_vint32(byte_vec& vec, uint32_t vint) {
  size_t len = get_vlen_of_uint32(vint);
  for (size_t i = len - 1; i > 0; i--)
    vec.push_back(0x80 + ((vint >> (7 * i)) & 0x7F));
  vec.push_back(vint & 0x7F);
  return len;
}
static size_t append_vint32(byte_vec& vec, uint32_t vint, size_t len) {
  for (size_t i = len - 1; i > 0; i--)
    vec.push_back(0x80 + ((vint >> (7 * i)) & 0x7F));
  vec.push_back(vint & 0x7F);
  return len;
}
static size_t append_ovint(byte_vec& vec, size_t vint, size_t offset_bits, uint8_t or_mask) {
  size_t mask_bits = 8 - offset_bits;
  uint8_t mask = (1 << mask_bits) - 1;
  size_t len = 0;
  uint8_t b = or_mask;
  do {
    b |= (vint & mask);
    vint >>= mask_bits;
    if (vint > 0)
      b |= (mask + 1);
    vec.push_back(b);
    b = 0;
    mask = '\x7F';
    mask_bits = 7;
    len++;
  } while (vint > 0);
  return len;
}
static size_t read_ovint(const uint8_t *ptr, size_t& len, size_t offset_bits) {
  size_t mask_bits = 7 - offset_bits;
  uint8_t mask = (1 << mask_bits) - 1;
  size_t ret = *ptr & mask;
  len = 1;
  while (*ptr++ & (mask + 1)) {
    uint32_t b = *ptr & '\x7F';
    ret += (b << mask_bits);
    mask = '\x7F';
    mask_bits += 7;
    len++;
  }
  return ret;
}
    static size_t get_svint60_len(int64_t vint) {
      vint = abs(vint);
      return vint < (1 << 4) ? 1 : (vint < (1 << 12) ? 2 : (vint < (1 << 20) ? 3 :
              (vint < (1 << 28) ? 4 : (vint < (1LL << 36) ? 5 : (vint < (1LL << 44) ? 6 :
              (vint < (1LL << 52) ? 7 : 8))))));
    }
    static size_t read_svint60_len(uint8_t *ptr) {
      return 1 + ((*ptr >> 4) & 0x07);
    }
    static void copy_svint60(int64_t input, uint8_t *out, size_t vlen) {
      vlen--;
      long lng = abs(input);
      *out++ = ((lng >> (vlen * 8)) & 0x0F) + (vlen << 4) + (input < 0 ? 0x00 : 0x80);
      while (vlen--)
        *out++ = ((lng >> (vlen * 8)) & 0xFF);
    }
    static void append_svint60(byte_vec& out, int64_t input) {
      size_t vlen = get_svint60_len(input);
      vlen--;
      long lng = abs(input);
      out.push_back(((lng >> (vlen * 8)) & 0x0F) + (vlen << 4) + (input < 0 ? 0x00 : 0x80));
      while (vlen--)
        out.push_back((lng >> (vlen * 8)) & 0xFF);
    }
    static int64_t read_svint60(uint8_t *ptr) {
      int64_t ret = *ptr & 0x0F;
      bool is_neg = true;
      if (*ptr & 0x80)
        is_neg = false;
      size_t len = (*ptr >> 4) & 0x07;
      while (len--) {
        ret <<= 8;
        ptr++;
        ret |= *ptr;
      }
      return is_neg ? -ret : ret;
    }
    static size_t get_svint61_len(uint64_t vint) {
      return vint < (1 << 5) ? 1 : (vint < (1 << 13) ? 2 : (vint < (1 << 21) ? 3 :
              (vint < (1 << 29) ? 4 : (vint < (1LL << 37) ? 5 : (vint < (1LL << 45) ? 6 :
              (vint < (1LL << 53) ? 7 : 8))))));
    }
    static size_t read_svint61_len(uint8_t *ptr) {
      return 1 + (*ptr >> 5);
    }
    static void copy_svint61(uint64_t input, uint8_t *out, size_t vlen) {
      vlen--;
      *out++ = ((input >> (vlen * 8)) & 0x1F) + (vlen << 5);
      while (vlen--)
        *out++ = ((input >> (vlen * 8)) & 0xFF);
    }
    static void append_svint61(byte_vec& out, uint64_t input) {
      size_t vlen = get_svint61_len(input);
      vlen--;
      out.push_back(((input >> (vlen * 8)) & 0x1F) + (vlen << 5));
      while (vlen--)
        out.push_back((input >> (vlen * 8)) & 0xFF);
    }
    static uint64_t read_svint61(uint8_t *ptr) {
      uint64_t ret = *ptr & 0x1F;
      size_t len = (*ptr >> 5);
      while (len--) {
        ret <<= 8;
        ptr++;
        ret |= *ptr;
      }
      return ret;
    }
    static size_t get_svint15_len(uint64_t vint) {
      return vint < (1 << 7) ? 1 : 2;
    }
    static size_t read_svint15_len(uint8_t *ptr) {
      return 1 + (*ptr >> 7);
    }
    static void copy_svint15(uint64_t input, uint8_t *out, size_t vlen) {
      vlen--;
      *out++ = ((input >> (vlen * 8)) & 0x7F) + (vlen << 7);
      while (vlen--)
        *out++ = ((input >> (vlen * 8)) & 0xFF);
    }
    static uint64_t read_svint15(uint8_t *ptr) {
      uint64_t ret = *ptr & 0x7F;
      size_t len = (*ptr >> 7);
      while (len--) {
        ret <<= 8;
        ptr++;
        ret |= *ptr;
      }
      return ret;
    }
    // void copy_tvint(uint32_t val, uint8_t *ptr) {
    //   if (val < 15) {
    //     *ptr++ = val;
    //     return;
    //   }
    //   val -= 15;
    //   uint32_t var_len = (val < 16 ? 2 : (val < 2048 ? 3 : (val < 262144 ? 4 : 5)));
    //   if (vec != NULL) {
    //     *vec++ = 15;
    //     int bit7s = var_len - 2;
    //     for (int i = bit7s - 1; i >= 0; i--)
    //       *vec++ = (0x80 + ((val >> (i * 7 + 4)) & 0x7F));
    //     *vec++ = (0x10 + (val & 0x0F));
    //   }
    // }
    // uint32_t read_tvint(uint8_t *ptr) {
    // }

}

#endif

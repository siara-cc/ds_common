#ifndef _DS_COMMON_HPP_
#define _DS_COMMON_HPP_

#include <stdarg.h>
#include <stdint.h>
#include <cstring>

#include "bv.hpp"
#include "vint.hpp"

namespace gen {

typedef std::vector<uint8_t> byte_vec;
typedef std::vector<uint8_t *> byte_ptr_vec;

static bool is_gen_print_enabled = false;
static void gen_printf(const char* format, ...) {
  if (!is_gen_print_enabled)
    return;
  va_list args;
  va_start(args, format);
  vprintf(format, args);
  va_end(args);
}

constexpr double dbl_div[] = {1.000000000000001, 10.000000000000001, 100.000000000000001, 1000.000000000000001, 10000.000000000000001, 100000.000000000000001, 1000000.000000000000001, 10000000.000000000000001, 100000000.000000000000001, 1000000000.000000000000001};
static int count_decimal_digits(double d) {
  int counter = 0;
  while (d != (int) d) {
    d *= 10;
    counter++;
    if (counter > 9)
      break;
  }
  return counter;
}
static double pow10(int p) {
  return dbl_div[p];
}
static uint32_t log4(uint32_t val) {
  return (val < 4 ? 1 : (val < 16 ? 2 : (val < 64 ? 3 : (val < 256 ? 4 : (val < 1024 ? 5 : (val < 4096 ? 6 : (val < 16384 ? 7 : (val < 65536 ? 8 : 9))))))));
}
static uint32_t log5(uint32_t val) {
  return (val < 5 ? 1 : (val < 25 ? 2 : (val < 125 ? 3 : (val < 625 ? 4 : (val < 3125 ? 5 : (val < 15625 ? 6 : (val < 78125 ? 7 : (val < 390625 ? 8 : 9))))))));
}
static uint32_t log8(uint32_t val) {
  return (val < 8 ? 1 : (val < 64 ? 2 : (val < 512 ? 3 : (val < 4096 ? 4 : (val < 32768 ? 5 : (val < 262144 ? 6 : (val < 2097152 ? 7 : 8)))))));
}
static uint32_t lg10(uint32_t val) {
  return (val < 10 ? 1 : (val < 100 ? 2 : (val < 1000 ? 3 : (val < 10000 ? 4 : (val < 100000 ? 5 : (val < 1000000 ? 6 : (val < 10000000 ? 7 : 8)))))));
}
static uint32_t log12(uint32_t val) {
  return (val < 12 ? 1 : (val < 96 ? 2 : (val < 768 ? 3 : (val < 6144 ? 4 : (val < 49152 ? 5 : (val < 393216 ? 6 : (val < 3145728 ? 7 : 8)))))));
}
static uint32_t log16(uint32_t val) {
  return (val < 16 ? 1 : (val < 256 ? 2 : (val < 4096 ? 3 : (val < 65536 ? 4 : (val < 1048576 ? 5 : (val < 16777216 ? 6 : (val < 268435456 ? 7 : 8)))))));
}
static uint32_t log256(uint32_t val) {
  return (val < 256 ? 1 : (val < 65536 ? 2 : (val < 16777216 ? 3 : 4)));
}
static size_t max(size_t v1, size_t v2) {
  return v1 > v2 ? v1 : v2;
}
static size_t min(size_t v1, size_t v2) {
  return v1 < v2 ? v1 : v2;
}
static clock_t print_time_taken(clock_t t, const char *msg) {
  t = clock() - t;
  double time_taken = ((double)t)/CLOCKS_PER_SEC; // in seconds
  gen_printf("%s%.5f\n", msg, time_taken);
  return clock();
}
static int compare(const uint8_t *v1, const uint8_t *v2) {
  int k = 0;
  uint8_t c1, c2;
  do {
    c1 = v1[k];
    c2 = v2[k];
    if (c1 == 0 && c2 == 0)
      return 0;
    k++;
  } while (c1 == c2);
  return (c1 < c2 ? -k : k);
}
static int compare(const uint8_t *v1, int len1, const uint8_t *v2) {
  if (len1 == 0 && v2[0] == 0)
    return 0;
  int k = 0;
  uint8_t c1, c2;
  c1 = c2 = 0;
  while (k < len1 && c1 == c2) {
    c1 = v1[k];
    c2 = v2[k];
    k++;
    if (k == len1 && c2 == 0)
      return 0;
  }
  return (c1 < c2 ? -k : k);
}
static int compare(const uint8_t *v1, const uint8_t *v2, int len2) {
  if (len2 == 0 && v1[0] == 0)
    return 0;
  int k = 0;
  uint8_t c1, c2;
  c1 = c2 = 0;
  while (k < len2 && c1 == c2) {
    c1 = v1[k];
    c2 = v2[k];
    k++;
    if (k == len2 && c1 == 0)
      return 0;
  }
  return (c1 < c2 ? -k : k);
}
static int compare(const uint8_t *v1, int len1, const uint8_t *v2, int len2) {
    int lim = (len2 < len1 ? len2 : len1);
    int k = 0;
    do {
        uint8_t c1 = v1[k];
        uint8_t c2 = v2[k];
        k++;
        if (c1 < c2)
            return -k;
        else if (c1 > c2)
            return k;
    } while (k < lim);
    if (len1 == len2)
        return 0;
    k++;
    return (len1 < len2 ? -k : k);
}
static int compare_rev(const uint8_t *v1, int len1, const uint8_t *v2,
        int len2) {
    int lim = (len2 < len1 ? len2 : len1);
    int k = 0;
      while (k++ < lim) {
        uint8_t c1 = v1[len1 - k];
        uint8_t c2 = v2[len2 - k];
        if (c1 < c2)
            return -k;
        else if (c1 > c2)
            return k;
    }
    if (len1 == len2)
        return 0;
    return (len1 < len2 ? -k : k);
}
static void copy_uint16(uint16_t input, uint8_t *out) {
  *out++ = input & 0xFF;
  *out = (input >> 8);
}
static void copy_uint24(uint32_t input, uint8_t *out) {
  *out++ = input & 0xFF;
  *out++ = (input >> 8) & 0xFF;
  *out = (input >> 16);
}
static void copy_uint32(uint32_t input, uint8_t *out) {
  *out++ = input & 0xFF;
  *out++ = (input >> 8) & 0xFF;
  *out++ = (input >> 16) & 0xFF;
  *out = (input >> 24);
}
static void copy_vint32(uint32_t input, uint8_t *out, int8_t vlen) {
  for (int i = vlen - 1; i > 0; i--)
    *out++ = 0x80 + ((input >> (7 * i)) & 0x7F);
  *out = input & 0x7F;
}
static uint32_t read_vint32(const uint8_t *ptr, int8_t *vlen = NULL) {
  uint32_t ret = 0;
  int8_t len = 5; // read max 5 bytes
  do {
    ret <<= 7;
    ret += *ptr & 0x7F;
    len--;
  } while ((*ptr++ >> 7) && len);
  if (vlen != NULL)
    *vlen = 5 - len;
  return ret;
}
static void write_uint16(uint32_t input, FILE *fp) {
  fputc(input & 0xFF, fp);
  fputc(input >> 8, fp);
}
static void write_uint24(uint32_t input, FILE *fp) {
  fputc(input & 0xFF, fp);
  fputc((input >> 8) & 0xFF, fp);
  fputc(input >> 16, fp);
}
static void write_uint32(uint32_t input, FILE *fp) {
  fputc(input & 0xFF, fp);
  fputc((input >> 8) & 0xFF, fp);
  fputc((input >> 16) & 0xFF, fp);
  fputc(input >> 24, fp);
}
static void append_uint16(uint16_t u16, byte_vec& v) {
  v.push_back(u16 & 0xFF);
  v.push_back(u16 >> 8);
}
static void append_uint24(uint32_t u24, byte_vec& v) {
  v.push_back(u24 & 0xFF);
  v.push_back((u24 >> 8) & 0xFF);
  v.push_back(u24 >> 16);
}
static void append_uint32(uint32_t u32, byte_vec& v) {
  v.push_back(u32 & 0xFF);
  v.push_back((u32 >> 8) & 0xFF);
  v.push_back((u32 >> 16) & 0xFF);
  v.push_back(u32 >> 24);
}
static void append_byte_vec(byte_vec& vec, uint8_t *val, int val_len) {
  for (int i = 0; i < val_len; i++)
    vec.push_back(val[i]);
}
static uint32_t read_uint16(byte_vec& v, int pos) {
  uint32_t ret = v[pos++];
  ret |= (v[pos] << 8);
  return ret;
}
static uint32_t read_uint16(uint8_t *ptr) {
  uint32_t ret = *ptr++;
  ret |= (*ptr << 8);
  return ret;
}
static uint32_t read_uint24(byte_vec& v, int pos) {
  uint32_t ret = v[pos++];
  ret |= (v[pos++] << 8);
  ret |= (v[pos] << 16);
  return ret;
}
static uint32_t read_uint24(uint8_t *ptr) {
  uint32_t ret = *ptr++;
  ret |= (*ptr++ << 8);
  ret |= (*ptr << 16);
  return ret;
}
static uint32_t read_uint32(uint8_t *ptr) {
  return *((uint32_t *) ptr);
}
static uint8_t *read_uint64(uint8_t *t, uint64_t& u64) {
  u64 = *((uint64_t *) t); // faster endian dependent
  return t + 8;
  // u64 = 0;
  // for (int v = 0; v < 8; v++) {
  //   u64 <<= 8;
  //   u64 |= *t++;
  // }
  // return t;
}
static uint32_t read_uintx(const uint8_t *ptr, uint32_t mask) {
  uint32_t ret = *((uint32_t *) ptr);
  return ret & mask; // faster endian dependent
}
uint8_t *extract_line(uint8_t *last_line, int& last_line_len, size_t remaining) {
  last_line += last_line_len;
  while (*last_line == '\r' || *last_line == '\n' || *last_line == '\0') {
    last_line++;
    remaining--;
    if (remaining == 0)
      return nullptr;
  }
  uint8_t *cr_pos = (uint8_t *) memchr(last_line, '\n', remaining);
  if (cr_pos != nullptr) {
    if (*(cr_pos - 1) == '\r')
      cr_pos--;
    *cr_pos = 0;
    last_line_len = cr_pos - last_line;
  } else
    last_line_len = remaining;
  return last_line;
}

class byte_str {
  int limit;
  int grow_bytes;
  int len;
  uint8_t *buf;
  bool is_buf_given;
  public:
    byte_str() {
      is_buf_given = false;
      buf = NULL;
    }
    ~byte_str() {
      if (!is_buf_given && buf != NULL)
        delete [] buf;
    }
    byte_str(uint8_t *_buf, int _limit, int _grow_bytes = 0) {
      buf = NULL;
      is_buf_given = false;
      set_buf_max_len(_buf, _limit, _grow_bytes);
    }
    void set_buf_max_len(uint8_t *_buf, int _limit, int _grow_bytes = 0) {
      if (_buf == NULL) {
        if (buf != NULL)
          delete [] buf;
      }
      len = 0;
      if (_buf == NULL)
        buf = new uint8_t[_limit];
      else
        buf = _buf;
      limit = _limit;
      grow_bytes = _grow_bytes;
      if (_buf != NULL)
        is_buf_given = true;
    }
    void expand(int addl_len = 1) {
      int grow_len = addl_len / grow_bytes + 1;
      grow_len *= grow_bytes;
      uint8_t *new_buf = new uint8_t[limit + grow_len];
      memcpy(new_buf, buf, len);
      limit += grow_len;
      delete [] buf;
      is_buf_given = false;
    }
    void truncate(int new_len) {
      // if (new_len < 0) {
      //   printf("New len < 0 : %d, %d\n", new_len, len);
      //   return;
      // }
      if (new_len > len && new_len >= limit) {
        if (grow_bytes > 0)
          expand(new_len - len);
      } else
        len = new_len;
    }
    void append(uint8_t b) {
      if (len >= limit) {
        if (grow_bytes == 0)
          return;
        expand();
      }
      buf[len++] = b;
    }
    void append(uint8_t *b, size_t blen) {
      if (len >= limit) {
        if (grow_bytes == 0)
          return;
        expand();
      }
      size_t start = 0;
      while (start < blen) {
        buf[len++] = *b++;
        start++;
      }
    }
    uint8_t *data() {
      return buf;
    }
    uint8_t operator[](uint32_t idx) const {
      return buf[idx];
    }
    size_t length() {
      return len;
    }
    void clear() {
      len = 0;
    }
};

}
#endif

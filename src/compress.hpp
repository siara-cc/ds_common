#ifndef _GEN_COMPRESS_HPP_
#define _GEN_COMPRESS_HPP_

#include <snappy.h>
#include <brotli/encode.h>
#include <brotli/decode.h>
#include <lz4.h>
#include <zlib.h>
#include <zstd.h>

namespace gen {

#define CMPR_TYPE_NONE 0
#define CMPR_TYPE_SNAPPY 1
#define CMPR_TYPE_LZ4 2
#define CMPR_TYPE_DEFLATE 3
#define CMPR_TYPE_BROTLI 4
#define CMPR_TYPE_LZMA 5
#define CMPR_TYPE_BZ2 6
#define CMPR_TYPE_ZSTD 7
#define CMPR_OPT_WLOG 18
static size_t compress_block(int8_t type, const uint8_t *input_block, size_t sz, uint8_t *out_buf) {
  size_t c_size = 65536;
  switch (type) {
    case CMPR_TYPE_SNAPPY:
      snappy::RawCompress((const char *) input_block, sz, (char *) out_buf, &c_size);
      break;
    case CMPR_TYPE_BROTLI:
      if (!BrotliEncoderCompress(BROTLI_DEFAULT_QUALITY, BROTLI_DEFAULT_WINDOW, BROTLI_DEFAULT_MODE,
                          sz, input_block, &c_size, out_buf)) {
        std::cout << "Brotli compress return 0" << std::endl;
      }
      break;
    case CMPR_TYPE_LZ4:
      c_size = LZ4_compress_default((const char *) input_block, (char *) out_buf, sz, c_size);
      break;
    case CMPR_TYPE_DEFLATE: {
      int result = compress(out_buf, &c_size, input_block, sz);
      if (result != Z_OK) {
        std::cout << "Uncompress failure: " << result << std::endl;
        return 0;
      }
    } break;
    case CMPR_TYPE_ZSTD:
      ZSTD_CCtx *cctx = ZSTD_createCCtx();
      if (cctx == nullptr) {
          std::cerr << "Failed to create compression context" << std::endl;
          return 0;
      }
      size_t result = ZSTD_CCtx_setParameter(cctx, ZSTD_c_windowLog, CMPR_OPT_WLOG);
      if (ZSTD_isError(result)) {
          std::cerr << "Failed to set window log parameter: " << ZSTD_getErrorName(result) << std::endl;
          ZSTD_freeCCtx(cctx);
          return 0;
      }
      c_size = ZSTD_compress2(cctx, out_buf, ZSTD_compressBound(sz), input_block, sz);
      if (ZSTD_isError(c_size)) {
          std::cerr << "Compression failed: " << ZSTD_getErrorName(c_size) << std::endl;
          c_size = 0;
      }
      ZSTD_freeCCtx(cctx);
      break;
  }
  return c_size;
}

static size_t decompress_block(int8_t type, const uint8_t *input_str, size_t sz, uint8_t *out_buf, size_t out_size) {
  size_t d_size = out_size;
  switch (type) {
    case CMPR_TYPE_SNAPPY:
      if (!snappy::GetUncompressedLength((const char *) input_str, sz, &d_size)) {
        std::cout << "Snappy GetUncompressedLength failure" << std::endl;
        return 0;
      }
      if (d_size != out_size) {
        std::cout << "Snappy GetUncompressedLength mismatch: " << d_size << ", " << out_size << std::endl;
        return 0;
      }
      if (!snappy::RawUncompress((const char *) input_str, sz, (char *) out_buf))
        std::cout << "Snappy uncompress failure" << std::endl;
      break;
    case CMPR_TYPE_BROTLI:
      if (!BrotliDecoderDecompress(sz, input_str, &d_size, out_buf)) {
        std::cout << "Brotli decompress return 0" << std::endl;
      }
      break;
    case CMPR_TYPE_LZ4:
      LZ4_decompress_safe((const char *) input_str, (char *) out_buf, sz, d_size);
      break;
    case CMPR_TYPE_DEFLATE: {
      int result = uncompress(out_buf, &d_size, input_str, sz);
      if (result != Z_OK) {
        std::cout << "Uncompress failure: " << result << std::endl;
        return 0;
      }
    } break;
    case CMPR_TYPE_ZSTD:
      ZSTD_DCtx *dctx = ZSTD_createDCtx();
      if (dctx == nullptr) {
          std::cerr << "Failed to create decompression context" << std::endl;
          return 0;
      }
      size_t result = ZSTD_DCtx_setParameter(dctx, ZSTD_d_windowLogMax, CMPR_OPT_WLOG);
      if (ZSTD_isError(result)) {
          std::cerr << "Failed to set window log parameter: " << ZSTD_getErrorName(result) << std::endl;
          ZSTD_freeDCtx(dctx);
          return 0;
      }
      size_t d_size = ZSTD_decompressDCtx(dctx, out_buf, out_size, input_str, sz);
      if (ZSTD_isError(d_size)) {
          std::cerr << "Decompression failed: " << ZSTD_getErrorName(d_size) << std::endl;
          d_size = 0;
      }
      ZSTD_freeDCtx(dctx);
  }
  return d_size;
}

}

#endif

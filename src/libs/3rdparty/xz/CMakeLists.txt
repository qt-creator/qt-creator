include (CheckTypeSize)
include (CheckIncludeFile)
CHECK_TYPE_SIZE("size_t" SIZEOF_SIZE_T)
CHECK_INCLUDE_FILE("stdbool.h" HAVE_STDBOOL_H)
CHECK_INCLUDE_FILE("stdint.h" HAVE_STDINT_H)

add_qtc_library(qtcXz STATIC
    SKIP_AUTOMOC
    SOURCES
    src/common/tuklib_cpucores.c
    src/liblzma/check/check.c
    src/liblzma/check/crc32_fast.c
    src/liblzma/check/crc32_table.c
    src/liblzma/check/crc64_fast.c
    src/liblzma/check/crc64_table.c
    src/liblzma/check/sha256.c
    src/liblzma/common/alone_decoder.c
    src/liblzma/common/alone_encoder.c
    src/liblzma/common/auto_decoder.c
    src/liblzma/common/block_decoder.c
    src/liblzma/common/block_encoder.c
    src/liblzma/common/block_header_decoder.c
    src/liblzma/common/block_header_encoder.c
    src/liblzma/common/block_util.c
    src/liblzma/common/common.c
    src/liblzma/common/easy_encoder.c
    src/liblzma/common/easy_preset.c
    src/liblzma/common/filter_common.c
    src/liblzma/common/filter_decoder.c
    src/liblzma/common/filter_encoder.c
    src/liblzma/common/filter_flags_decoder.c
    src/liblzma/common/filter_flags_encoder.c
    src/liblzma/common/hardware_cputhreads.c
    src/liblzma/common/index_decoder.c
    src/liblzma/common/index_encoder.c
    src/liblzma/common/index_hash.c
    src/liblzma/common/index.c
    src/liblzma/common/block_buffer_encoder.c
    src/liblzma/common/outqueue.c
    src/liblzma/common/stream_decoder.c
    src/liblzma/common/stream_encoder.c
    src/liblzma/common/stream_encoder_mt.c
    src/liblzma/common/stream_flags_common.c
    src/liblzma/common/stream_flags_decoder.c
    src/liblzma/common/stream_flags_encoder.c
    src/liblzma/common/vli_decoder.c
    src/liblzma/common/vli_encoder.c
    src/liblzma/common/vli_size.c
    src/liblzma/delta/delta_common.c
    src/liblzma/delta/delta_decoder.c
    src/liblzma/delta/delta_encoder.c
    src/liblzma/lz/lz_decoder.c
    src/liblzma/lz/lz_encoder_mf.c
    src/liblzma/lz/lz_encoder.c
    src/liblzma/lzma/fastpos_table.c
    src/liblzma/lzma/lzma_decoder.c
    src/liblzma/lzma/lzma_encoder_optimum_fast.c
    src/liblzma/lzma/lzma_encoder_optimum_normal.c
    src/liblzma/lzma/lzma_encoder_presets.c
    src/liblzma/lzma/lzma_encoder.c
    src/liblzma/lzma/lzma2_decoder.c
    src/liblzma/lzma/lzma2_encoder.c
    src/liblzma/rangecoder/price_table.c
    src/liblzma/simple/arm.c
    src/liblzma/simple/arm64.c
    src/liblzma/simple/armthumb.c
    src/liblzma/simple/ia64.c
    src/liblzma/simple/powerpc.c
    src/liblzma/simple/riscv.c
    src/liblzma/simple/simple_coder.c
    src/liblzma/simple/simple_decoder.c
    src/liblzma/simple/simple_encoder.c
    src/liblzma/simple/sparc.c
    src/liblzma/simple/x86.c
  DEFINES
    HAVE_XZ_SUPPORT
    SIZEOF_SIZE_T=${SIZEOF_SIZE_T}
    HAVE_STDBOOL_H=${HAVE_STDBOOL_H}
    HAVE_STDINT_H=${HAVE_STDINT_H}
    HAVE_CHECK_CRC32
    HAVE_CHECK_CRC64
    HAVE_CHECK_SHA256
    HAVE_ENCODER_LZMA1 HAVE_DECODER_LZMA1
    HAVE_ENCODER_LZMA2 HAVE_DECODER_LZMA2
    HAVE_ENCODER_X86 HAVE_DECODER_X86
    HAVE_ENCODER_POWERPC HAVE_DECODER_POWERPC
    HAVE_ENCODER_IA64 HAVE_DECODER_IA64
    HAVE_ENCODER_ARM HAVE_DECODER_ARM
    HAVE_ENCODER_ARMTHUMB HAVE_DECODER_ARMTHUMB
    HAVE_ENCODER_ARM64 HAVE_DECODER_ARM64
    HAVE_ENCODER_SPARC HAVE_DECODER_SPARC
    HAVE_ENCODER_RISCV HAVE_DECODER_RISCV
    HAVE_ENCODER_DELTA HAVE_DECODER_DELTA
    HAVE_MF_HC3 HAVE_MF_HC4 HAVE_MF_BT2 HAVE_MF_BT3 HAVE_MF_BT4
  PUBLIC_INCLUDES
    src/common
    src/liblzma/api
    src/liblzma/check
    src/liblzma/common
    src/liblzma/delta
    src/liblzma/lz
    src/liblzma/lzma
    src/liblzma/range_decoder
    src/liblzma/rangecoder
    src/liblzma/simple
)


if(WIN32)
  target_compile_definitions(qtcXz PRIVATE MYTHREAD_VISTA)
  if(MSVC)
    target_compile_options(qtcXz PUBLIC /wd4244 /wd4267)
  endif()
else()
  target_compile_definitions(qtcXz PRIVATE MYTHREAD_POSIX)
endif()

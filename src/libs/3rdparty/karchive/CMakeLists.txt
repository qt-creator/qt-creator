option(QTC_USE_SYSTEM_BZIP2 "Use system bzip2 instead of included bzip2" ON)
option(QTC_USE_SYSTEM_ZLIB "Use system zlib instead of included zlib" ON)
option(QTC_USE_SYSTEM_LZMA "Use system liblzma instead of included liblzma" ON)

if (QTC_USE_SYSTEM_BZIP2)
  find_package(BZip2)
  if (NOT TARGET BZip2::BZip2)
    set(BZIP2_FALLBACK " (No system bzip2 found)")
  endif()
endif()

if (QTC_USE_SYSTEM_ZLIB)
  find_package(ZLIB)
  if (NOT TARGET ZLIB::ZLIB)
    set(ZLIB_FALLBACK " (No system zlib found)")
  endif()
endif()

if (QTC_USE_SYSTEM_LZMA)
  find_package(LibLZMA)
  if (NOT TARGET LibLZMA::LibLZMA)
    set(LZMA_FALLBACK " (No system liblzma found)")
  endif()
endif()

add_qtc_library(
    karchive
    DEPENDS Qt::Core
    PUBLIC_INCLUDES src
    SOURCES
        src/kar.cpp
        src/kar.h
        src/karchive_export.h
        src/karchive_p.h
        src/karchive.cpp
        src/karchive.h
        src/karchivedirectory.h
        src/karchiveentry.h
        src/karchivefile.h
        src/kcompressiondevice_p.h
        src/kcompressiondevice.cpp
        src/kcompressiondevice.h
        src/kfilterbase.cpp
        src/kfilterbase.h
        src/klimitediodevice_p.h
        src/klimitediodevice.cpp
        src/knonefilter.cpp
        src/knonefilter.h
        src/krcc.cpp
        src/krcc.h
        src/ktar.cpp
        src/ktar.h
        src/kzipfileentry.h
        src/loggingcategory.cpp
        src/loggingcategory.h
)

# ZIP support
extend_qtc_library(
    karchive
    CONDITION QTC_USE_SYSTEM_ZLIB AND TARGET ZLIB::ZLIB
    FEATURE_INFO "KArchive (System) zlib support"
    DEPENDS ZLIB::ZLIB
    DEFINES
        HAVE_ZLIB_SUPPORT=1
    SOURCES
        src/kgzipfilter.cpp
        src/kgzipfilter.h
        src/kzip.cpp
        src/kzip.h
)

extend_qtc_library(
    karchive
    CONDITION NOT QTC_USE_SYSTEM_ZLIB OR NOT TARGET ZLIB::ZLIB
    SKIP_AUTOMOC
    DEFINES
        HAVE_ZLIB_SUPPORT=1
    PUBLIC_DEFINES
        KARCHIVE_HAS_ZLIB
    FEATURE_INFO "KArchive zlib support${ZLIB_FALLBACK}"
    INCLUDES
        ${CMAKE_CURRENT_LIST_DIR}/../zlib/src/
    SOURCES
        src/kgzipfilter.cpp
        src/kgzipfilter.h
        src/kzip.cpp
        src/kzip.h

        # Embed zlib sources
        ${CMAKE_CURRENT_LIST_DIR}/../zlib/src/adler32.c
        ${CMAKE_CURRENT_LIST_DIR}/../zlib/src/compress.c
        ${CMAKE_CURRENT_LIST_DIR}/../zlib/src/crc32.c
        ${CMAKE_CURRENT_LIST_DIR}/../zlib/src/crc32.h
        ${CMAKE_CURRENT_LIST_DIR}/../zlib/src/deflate.c
        ${CMAKE_CURRENT_LIST_DIR}/../zlib/src/deflate.h
        ${CMAKE_CURRENT_LIST_DIR}/../zlib/src/gzclose.c
        ${CMAKE_CURRENT_LIST_DIR}/../zlib/src/gzguts.h
        ${CMAKE_CURRENT_LIST_DIR}/../zlib/src/gzlib.c
        ${CMAKE_CURRENT_LIST_DIR}/../zlib/src/gzread.c
        ${CMAKE_CURRENT_LIST_DIR}/../zlib/src/gzwrite.c
        ${CMAKE_CURRENT_LIST_DIR}/../zlib/src/infback.c
        ${CMAKE_CURRENT_LIST_DIR}/../zlib/src/inffast.c
        ${CMAKE_CURRENT_LIST_DIR}/../zlib/src/inffast.h
        ${CMAKE_CURRENT_LIST_DIR}/../zlib/src/inffixed.h
        ${CMAKE_CURRENT_LIST_DIR}/../zlib/src/inflate.c
        ${CMAKE_CURRENT_LIST_DIR}/../zlib/src/inflate.h
        ${CMAKE_CURRENT_LIST_DIR}/../zlib/src/inftrees.c
        ${CMAKE_CURRENT_LIST_DIR}/../zlib/src/inftrees.h
        ${CMAKE_CURRENT_LIST_DIR}/../zlib/src/trees.c
        ${CMAKE_CURRENT_LIST_DIR}/../zlib/src/trees.h
        ${CMAKE_CURRENT_LIST_DIR}/../zlib/src/uncompr.c
        ${CMAKE_CURRENT_LIST_DIR}/../zlib/src/zconf.h
        ${CMAKE_CURRENT_LIST_DIR}/../zlib/src/zlib.h
        ${CMAKE_CURRENT_LIST_DIR}/../zlib/src/zutil.c
        ${CMAKE_CURRENT_LIST_DIR}/../zlib/src/zutil.h
)

# BZip2 support
extend_qtc_library(karchive
    CONDITION QTC_USE_SYSTEM_BZIP2 AND TARGET BZip2::BZip2
    FEATURE_INFO "KArchive (System) BZip2 support"
    DEPENDS BZip2::BZip2
    DEFINES
        HAVE_BZIP2_SUPPORT=1
        NEED_BZ2_PREFIX=1
    PUBLIC_DEFINES
        KARCHIVE_HAS_BZIP2

    SOURCES
        src/kbzip2filter.cpp
)

extend_qtc_library(
    karchive
    CONDITION NOT QTC_USE_SYSTEM_BZIP2 OR NOT TARGET BZip2::BZip2
    INCLUDES ./3rdparty/bzip2
    FEATURE_INFO "KArchive BZip2 support${BZIP2_FALLBACK}"
    DEFINES
        HAVE_BZIP2_SUPPORT=1
        NEED_BZ2_PREFIX=1
    PUBLIC_DEFINES
        KARCHIVE_HAS_BZIP2

    SOURCES
        src/kbzip2filter.cpp

        # Embed bzip2 sources
        ./3rdparty/bzip2/blocksort.c
        ./3rdparty/bzip2/bzlib.c
        ./3rdparty/bzip2/bzlib.h
        ./3rdparty/bzip2/compress.c
        ./3rdparty/bzip2/crctable.c
        ./3rdparty/bzip2/decompress.c
        ./3rdparty/bzip2/huffman.c
        ./3rdparty/bzip2/randtable.c
)

# libLZMA support
if (NOT QTC_USE_SYSTEM_LZMA)
    include (CheckTypeSize)
    include (CheckIncludeFile)
    CHECK_TYPE_SIZE("size_t"     SIZEOF_SIZE_T)
    CHECK_INCLUDE_FILE("stdbool.h" HAVE_STDBOOL_H)
endif()

extend_qtc_library(
    karchive
    CONDITION QTC_USE_SYSTEM_LZMA AND TARGET LibLZMA::LibLZMA
    FEATURE_INFO "KArchive (System) libLZMA support"
    DEPENDS LibLZMA::LibLZMA
    DEFINES
        HAVE_XZ_SUPPORT=1
    SOURCES
        src/kxzfilter.cpp
        src/kxzfilter.h
        src/k7zip.cpp
        src/k7zip.h
)

extend_qtc_library(
    karchive
    CONDITION NOT QTC_USE_SYSTEM_LZMA OR NOT TARGET LibLZMA::LibLZMA
    FEATURE_INFO "KArchive 7zip & xz support${LZMA_FALLBACK}"
    DEFINES
        HAVE_XZ_SUPPORT=1
        SIZEOF_SIZE_T=${SIZEOF_SIZE_T}
        HAVE_STDBOOL_H=${HAVE_STDBOOL_H}
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
    PUBLIC_DEFINES
        KARCHIVE_HAS_XZ
    INCLUDES
        ./3rdparty/xz/src/common
        ./3rdparty/xz/src/liblzma/api
        ./3rdparty/xz/src/liblzma/check
        ./3rdparty/xz/src/liblzma/common
        ./3rdparty/xz/src/liblzma/delta
        ./3rdparty/xz/src/liblzma/lz
        ./3rdparty/xz/src/liblzma/lzma
        ./3rdparty/xz/src/liblzma/range_decoder
        ./3rdparty/xz/src/liblzma/rangecoder
        ./3rdparty/xz/src/liblzma/simple
    SOURCES
        src/kxzfilter.cpp
        src/kxzfilter.h
        src/k7zip.cpp
        src/k7zip.h

        # Embed liblzma sources
        ./3rdparty/xz/src/liblzma/check/check.c
        ./3rdparty/xz/src/liblzma/check/crc32_fast.c
        ./3rdparty/xz/src/liblzma/check/crc32_table.c
        ./3rdparty/xz/src/liblzma/check/crc64_fast.c
        ./3rdparty/xz/src/liblzma/check/crc64_table.c
        ./3rdparty/xz/src/liblzma/check/sha256.c
        ./3rdparty/xz/src/liblzma/common/alone_decoder.c
        ./3rdparty/xz/src/liblzma/common/auto_decoder.c
        ./3rdparty/xz/src/liblzma/common/block_decoder.c
        ./3rdparty/xz/src/liblzma/common/block_encoder.c
        ./3rdparty/xz/src/liblzma/common/block_header_decoder.c
        ./3rdparty/xz/src/liblzma/common/block_header_encoder.c
        ./3rdparty/xz/src/liblzma/common/block_util.c
        ./3rdparty/xz/src/liblzma/common/common.c
        ./3rdparty/xz/src/liblzma/common/easy_encoder.c
        ./3rdparty/xz/src/liblzma/common/easy_preset.c
        ./3rdparty/xz/src/liblzma/common/filter_common.c
        ./3rdparty/xz/src/liblzma/common/filter_decoder.c
        ./3rdparty/xz/src/liblzma/common/filter_encoder.c
        ./3rdparty/xz/src/liblzma/common/filter_flags_decoder.c
        ./3rdparty/xz/src/liblzma/common/filter_flags_encoder.c
        ./3rdparty/xz/src/liblzma/common/index_decoder.c
        ./3rdparty/xz/src/liblzma/common/index_encoder.c
        ./3rdparty/xz/src/liblzma/common/index_hash.c
        ./3rdparty/xz/src/liblzma/common/index.c
        ./3rdparty/xz/src/liblzma/common/stream_decoder.c
        ./3rdparty/xz/src/liblzma/common/stream_encoder.c
        ./3rdparty/xz/src/liblzma/common/stream_flags_common.c
        ./3rdparty/xz/src/liblzma/common/stream_flags_decoder.c
        ./3rdparty/xz/src/liblzma/common/stream_flags_encoder.c
        ./3rdparty/xz/src/liblzma/common/vli_decoder.c
        ./3rdparty/xz/src/liblzma/common/vli_encoder.c
        ./3rdparty/xz/src/liblzma/common/vli_size.c
        ./3rdparty/xz/src/liblzma/delta/delta_common.c
        ./3rdparty/xz/src/liblzma/delta/delta_decoder.c
        ./3rdparty/xz/src/liblzma/delta/delta_encoder.c
        ./3rdparty/xz/src/liblzma/lz/lz_decoder.c
        ./3rdparty/xz/src/liblzma/lz/lz_encoder_mf.c
        ./3rdparty/xz/src/liblzma/lz/lz_encoder.c
        ./3rdparty/xz/src/liblzma/lzma/fastpos_table.c
        ./3rdparty/xz/src/liblzma/lzma/lzma_decoder.c
        ./3rdparty/xz/src/liblzma/lzma/lzma_encoder_optimum_fast.c
        ./3rdparty/xz/src/liblzma/lzma/lzma_encoder_optimum_normal.c
        ./3rdparty/xz/src/liblzma/lzma/lzma_encoder_presets.c
        ./3rdparty/xz/src/liblzma/lzma/lzma_encoder.c
        ./3rdparty/xz/src/liblzma/lzma/lzma2_decoder.c
        ./3rdparty/xz/src/liblzma/lzma/lzma2_encoder.c
        ./3rdparty/xz/src/liblzma/rangecoder/price_table.c
        ./3rdparty/xz/src/liblzma/simple/arm.c
        ./3rdparty/xz/src/liblzma/simple/arm64.c
        ./3rdparty/xz/src/liblzma/simple/armthumb.c
        ./3rdparty/xz/src/liblzma/simple/ia64.c
        ./3rdparty/xz/src/liblzma/simple/powerpc.c
        ./3rdparty/xz/src/liblzma/simple/riscv.c
        ./3rdparty/xz/src/liblzma/simple/simple_coder.c
        ./3rdparty/xz/src/liblzma/simple/simple_decoder.c
        ./3rdparty/xz/src/liblzma/simple/simple_encoder.c
        ./3rdparty/xz/src/liblzma/simple/sparc.c
        ./3rdparty/xz/src/liblzma/simple/x86.c
)

# Zstd support
# BSD License: https://github.com/facebook/zstd/blob/dev/LICENSE
# Disabled for now as we don't need it and the packages are suboptimal
#extend_qtc_library(
#    karchive
#    CONDITION TARGET PkgConfig::LibZstd
#    DEPENDS PkgConfig::LibZstd
#    FEATURE_INFO "KArchive Zstd support"
#    DEFINES
#        HAVE_ZSTD_SUPPORT=1
#    PUBLIC_DEFINES
#        KARCHIVE_HAS_ZSTD
#    SOURCES
#        src/kzstdfilter.cpp
#        src/kzstdfilter.h
#)

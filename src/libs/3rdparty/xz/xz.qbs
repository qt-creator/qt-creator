StaticLibrary {
    name: "qtcXz"
    builtByDefault: false

    Depends { name: "cpp" }

    Properties {
        cpp.defines: [
            "HAVE_XZ_SUPPORT",
            "SIZEOF_SIZE_T=${SIZEOF_SIZE_T}",
            "HAVE_STDBOOL_H=${HAVE_STDBOOL_H}",
            "HAVE_STDINT_H=${HAVE_STDINT_H}",
            "HAVE_CHECK_CRC32",
            "HAVE_CHECK_CRC64",
            "HAVE_CHECK_SHA256",
            "HAVE_ENCODER_LZMA1",
            "HAVE_DECODER_LZMA1",
            "HAVE_ENCODER_LZMA2",
            "HAVE_DECODER_LZMA2",
            "HAVE_ENCODER_X86",
            "HAVE_DECODER_X86",
            "HAVE_ENCODER_POWERPC",
            "HAVE_DECODER_POWERPC",
            "HAVE_ENCODER_IA64",
            "HAVE_DECODER_IA64",
            "HAVE_ENCODER_ARM",
            "HAVE_DECODER_ARM",
            "HAVE_ENCODER_ARMTHUMB",
            "HAVE_DECODER_ARMTHUMB",
            "HAVE_ENCODER_ARM64",
            "HAVE_DECODER_ARM64",
            "HAVE_ENCODER_SPARC",
            "HAVE_DECODER_SPARC",
            "HAVE_ENCODER_RISCV",
            "HAVE_DECODER_RISCV",
            "HAVE_ENCODER_DELTA",
            "HAVE_DECODER_DELTA",
            "HAVE_MF_HC3",
            "HAVE_MF_HC4",
            "HAVE_MF_BT2",
            "HAVE_MF_BT3",
            "HAVE_MF_BT4",
        ]
    }
    Properties {
        condition: qbs.targetOS.contains("windows")
        cpp.defines: "MYTHREAD_VISTA"
    }
    Properties {
        condition: qbs.toolchain.contains("msvc")
        cpp.cFlags: "/wd4244", "/wd4267"
    }
    Properties {
        condition: qbs.toolchain.contains("gcc")
        cpp.cFlags: "-Wno-type-limits"
    }

    Properties {
        condition: qbs.targetOS.contains("unix")
        cpp.defines: "MYTHREAD_POSIX"
    }

    property stringList commonIncludes: [
        sourceDirectory + "/src/common",
        sourceDirectory + "/src/liblzma/api",
        sourceDirectory + "/src/liblzma/check",
        sourceDirectory + "/src/liblzma/common",
        sourceDirectory + "/src/liblzma/delta",
        sourceDirectory + "/src/liblzma/lz",
        sourceDirectory + "/src/liblzma/lzma",
        sourceDirectory + "/src/liblzma/range_decoder",
        sourceDirectory + "/src/liblzma/rangecoder",
        sourceDirectory + "/src/liblzma/simple",
    ]
    cpp.includePaths: commonIncludes

    files: "src/common/tuklib_cpucores.c"
    Group {
        name: "lzma"
        prefix: "src/liblzma/"
        files: [
            "check/check.c",
            "check/crc32_fast.c",
            "check/crc32_table.c",
            "check/crc64_fast.c",
            "check/crc64_table.c",
            "check/sha256.c",
            "common/alone_decoder.c",
            "common/alone_encoder.c",
            "common/auto_decoder.c",
            "common/block_decoder.c",
            "common/block_encoder.c",
            "common/block_header_decoder.c",
            "common/block_header_encoder.c",
            "common/block_util.c",
            "common/common.c",
            "common/easy_encoder.c",
            "common/easy_preset.c",
            "common/filter_common.c",
            "common/filter_decoder.c",
            "common/filter_encoder.c",
            "common/filter_flags_decoder.c",
            "common/filter_flags_encoder.c",
            "common/hardware_cputhreads.c",
            "common/index_decoder.c",
            "common/index_encoder.c",
            "common/index_hash.c",
            "common/index.c",
            "common/block_buffer_encoder.c",
            "common/outqueue.c",
            "common/stream_decoder.c",
            "common/stream_encoder.c",
            "common/stream_encoder_mt.c",
            "common/stream_flags_common.c",
            "common/stream_flags_decoder.c",
            "common/stream_flags_encoder.c",
            "common/vli_decoder.c",
            "common/vli_encoder.c",
            "common/vli_size.c",
            "delta/delta_common.c",
            "delta/delta_decoder.c",
            "delta/delta_encoder.c",
            "lz/lz_decoder.c",
            "lz/lz_encoder_mf.c",
            "lz/lz_encoder.c",
            "lzma/fastpos_table.c",
            "lzma/lzma_decoder.c",
            "lzma/lzma_encoder_optimum_fast.c",
            "lzma/lzma_encoder_optimum_normal.c",
            "lzma/lzma_encoder_presets.c",
            "lzma/lzma_encoder.c",
            "lzma/lzma2_decoder.c",
            "lzma/lzma2_encoder.c",
            "rangecoder/price_table.c",
            "simple/arm.c",
            "simple/arm64.c",
            "simple/armthumb.c",
            "simple/ia64.c",
            "simple/powerpc.c",
            "simple/riscv.c",
            "simple/simple_coder.c",
            "simple/simple_decoder.c",
            "simple/simple_encoder.c",
            "simple/sparc.c",
            "simple/x86.c",
        ]
    }

    Export {
        Depends { name: "cpp" }
        cpp.includePaths: exportingProduct.commonIncludes
        Properties {
            condition: qbs.toolchain.contains("msvc")
            cpp.cFlags: "/wd4244", "/wd4267"
        }
    }
}

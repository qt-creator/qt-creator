import qbs.Probes

QtcLibrary {
    name: "karchive"

    property bool preferSystemZlib: true
    property bool preferSystemBZip2: true
    property bool preferSystemLzma: true

    readonly property bool useSystemZlib: preferSystemZlib && zlibHProbe.found && zlibLibProbe.found
    readonly property bool useSystemBZip2: preferSystemBZip2 && bzlibProbe.found && bzip2LibProbe.found
    readonly property bool useSystemLzma: preferSystemLzma && lzmaHProbe.found && lzmaLibProbe.found

    cpp.defines: base.concat([
                                 "KARCHIVE_LIBRARY", // general define
                                 "HAVE_ZLIB_SUPPORT=1",
                                 "HAVE_BZIP2_SUPPORT=1", "NEED_BZ2_PREFIX=1",
                                 "HAVE_XZ_SUPPORT=1"
                             ])

    cpp.includePaths: {
        var paths = base;
        paths.push("src"); // general include

        if (useSystemZlib)
            paths.push(zlibHProbe.path);
        else
            paths.push("../zlib/src");

        if (useSystemBZip2)
            paths.push(bzlibProbe.path);
        else
            paths.push("3rdparty/bzip2");

        if (useSystemLzma) {
            paths.push(lzmaHProbe.path);
        } else {
            paths.push(
                        "3rdparty/xz/src/common",
                        "3rdparty/xz/src/liblzma/api",
                        "3rdparty/xz/src/liblzma/check",
                        "3rdparty/xz/src/liblzma/common",
                        "3rdparty/xz/src/liblzma/delta",
                        "3rdparty/xz/src/liblzma/lz",
                        "3rdparty/xz/src/liblzma/lzma",
                        "3rdparty/xz/src/liblzma/range_decoder",
                        "3rdparty/xz/src/liblzma/rangecoder",
                        "3rdparty/xz/src/liblzma/simple"
                        );
        }
        return paths;
    }

    cpp.libraryPaths: {
        var paths = base;
        if (useSystemZlib)
            paths.push(zlibLibProbe.path);
        if (useSystemBZip2)
            paths.push(bzip2LibProbe.path);
        if (useSystemLzma)
            paths.push(lzmaLibProbe.path);
        return paths;
    }

    cpp.dynamicLibraries: {
        var libs = base;
        if (qbs.targetOS.contains("windows"))
            libs.push("advapi32");
        if (useSystemZlib)
            libs = libs.concat(zlibLibProbe.names);
        if (useSystemBZip2)
            libs = libs.concat(bzip2LibProbe.names);
        if (useSystemLzma)
            libs = libs.concat(lzmaLibProbe.names);
        return libs;
    }

    Group {
        name: "KArchive files"
        prefix: "src/"
        files: [
            "kar.cpp",
            "kar.h",
            "karchive_export.h",
            "karchive_p.h",
            "karchive.cpp",
            "karchive.h",
            "karchivedirectory.h",
            "karchiveentry.h",
            "karchivefile.h",
            "kcompressiondevice_p.h",
            "kcompressiondevice.cpp",
            "kcompressiondevice.h",
            "kfilterbase.cpp",
            "kfilterbase.h",
            "klimitediodevice_p.h",
            "klimitediodevice.cpp",
            "knonefilter.cpp",
            "knonefilter.h",
            "krcc.cpp",
            "krcc.h",
            "ktar.cpp",
            "ktar.h",
            "kzipfileentry.h",
            "loggingcategory.cpp",
            "loggingcategory.h",
        ]
    }

    Probes.IncludeProbe { id: zlibHProbe; names: "zlib.h" }
    Probes.LibraryProbe { id: zlibLibProbe; names: "zlib" }
    Group {
        name: "zlib support"
        prefix: "src/"
        files: [
            "kgzipfilter.cpp",
            "kgzipfilter.h",
            "kzip.cpp",
            "kzip.h",
        ]
    }
    Group {
        name: "embedded zlib"
        condition: !useSystemZlib
        prefix: "../zlib/src/"
        files: [
            "adler32.c",
            "compress.c",
            "crc32.c",
            "crc32.h",
            "deflate.c",
            "deflate.h",
            "gzclose.c",
            "gzguts.h",
            "gzlib.c",
            "gzread.c",
            "gzwrite.c",
            "infback.c",
            "inffast.c",
            "inffast.h",
            "inffixed.h",
            "inflate.c",
            "inflate.h",
            "inftrees.c",
            "inftrees.h",
            "trees.c",
            "trees.h",
            "uncompr.c",
            "zconf.h",
            "zlib.h",
            "zutil.c",
            "zutil.h",
        ]
    }

    Probes.LibraryProbe { id: bzip2LibProbe; names: "bzip2" }
    Probes.IncludeProbe { id: bzlibProbe; names: "bzlib.h" }
    Group {
        name: "bzip2 support"
        files: [ "src/kbzip2filter.cpp"]
    }
    Group {
        name: "embedded bzip2"
        condition: !useSystemBZip2
        cpp.cFlags: (qbs.toolchain.contains("msvc") ? ["/wd26819", "/wd4100"] : [])
            .concat(qbs.toolchain.contains("gcc") ? ["-Wno-implicit-fallthrough", "-Wno-unused-parameter"] : [])
        prefix: "3rdparty/bzip2/"
        files: [
            "blocksort.c",
            "bzlib.c",
            "bzlib.h",
            "compress.c",
            "crctable.c",
            "decompress.c",
            "huffman.c",
            "randtable.c",
        ]
    }

    Probes.IncludeProbe { id: lzmaHProbe; names: "lzma.h" }
    Probes.LibraryProbe { id: lzmaLibProbe; names: "lzma" }
    Group {
        name: "lzma support"
        prefix: "src/"
        files: [
            "kxzfilter.cpp",
            "kxzfilter.h",
            "k7zip.cpp",
            "k7zip.h",
        ]
    }

    Probes.IncludeProbe { id: stdBoolProbe; names: "stdbool.h" }
    Group {
        name: "embedded lzma"
        condition: !useSystemLzma
        cpp.defines: outer.concat([
                                      "SIZEOF_SIZE_T=" + "8", // FIXME!
                                      "HAVE_STDBOOL_H=" + "1", // FIXME! stdBoolProbe.found ? "1" : "0",
                                      "HAVE_CHECK_CRC32", "HAVE_CHECK_CRC64",
                                      "HAVE_CHECK_SHA256",
                                      "HAVE_ENCODER_LZMA1", "HAVE_DECODER_LZMA1",
                                      "HAVE_ENCODER_LZMA2", "HAVE_DECODER_LZMA2",
                                      "HAVE_ENCODER_X86", "HAVE_DECODER_X86",
                                      "HAVE_ENCODER_POWERPC", "HAVE_DECODER_POWERPC",
                                      "HAVE_ENCODER_IA64", "HAVE_DECODER_IA64",
                                      "HAVE_ENCODER_ARM", "HAVE_DECODER_ARM",
                                      "HAVE_ENCODER_ARMTHUMB", "HAVE_DECODER_ARMTHUMB",
                                      "HAVE_ENCODER_ARM64", "HAVE_DECODER_ARM64",
                                      "HAVE_ENCODER_SPARC", "HAVE_DECODER_SPARC",
                                      "HAVE_ENCODER_RISCV", "HAVE_DECODER_RISCV",
                                      "HAVE_ENCODER_DELTA", "HAVE_DECODER_DELTA",
                                      "HAVE_MF_HC3", "HAVE_MF_HC4", "HAVE_MF_BT2",
                                      "HAVE_MF_BT3", "HAVE_MF_BT4",
                                  ])
        prefix: "3rdparty/xz/src/liblzma/"
        files: [
            "check/check.c",
            "check/crc32_fast.c",
            "check/crc32_table.c",
            "check/crc64_fast.c",
            "check/crc64_table.c",
            "check/sha256.c",
            "common/alone_decoder.c",
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
            "common/index_decoder.c",
            "common/index_encoder.c",
            "common/index_hash.c",
            "common/index.c",
            "common/stream_decoder.c",
            "common/stream_encoder.c",
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
        cpp.defines: [ "KARCHIVE_HAS_ZLIB", "KARCHIVE_HAS_BZIP2", "KARCHIVE_HAS_XZ" ]
        cpp.includePaths: path + "/src"
    }
}
